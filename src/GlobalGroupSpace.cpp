#include "GlobalGroupSpace.hpp"
#include "LocalLengthGroupSpace.hpp"
#include "TimeSeries.hpp"
#include "TimeSeriesSet.hpp"
#include "Group.hpp"
#include "distance/Distance.hpp"
#include "lib/ThreadPool.hpp"

#include <cmath>
#include <sstream>
#include <functional>
#include <queue>
#include <vector>
#include <fstream>
#include <future>
#include <iostream>
#include <boost/algorithm/string.hpp>


using std::vector;
using std::max;
using std::min;
using std::cout;
using std::endl;
using std::ofstream;
using std::ifstream;
using std::string;

namespace konex {

void GlobalGroupSpace::reset(void)
{
  for (auto i = 0; i < this->localLengthGroupSpace.size(); i++) {
    delete this->localLengthGroupSpace[i];
    this->localLengthGroupSpace[i] = nullptr;
  }
  this->localLengthGroupSpace.clear();
}

void GlobalGroupSpace::_loadDistance(const string& distance_name)
{
  this->distanceName = distance_name;
  this->pairwiseDistance = getDistance(distance_name);
  this->warpedDistance = cascadeDistance;
}

int GlobalGroupSpace::_group(int i)
{
  this->localLengthGroupSpace[i] = new LocalLengthGroupSpace(this->dataset, i);
  int noOfGenerated = this->localLengthGroupSpace[i]->generateGroups(this->pairwiseDistance, this->threshold);
  return noOfGenerated;
}

int GlobalGroupSpace::group(const string& distance_name, data_t threshold)
{
  reset();
  this->_loadDistance(distance_name);
  this->localLengthGroupSpace.resize(dataset.getItemLength() + 1, nullptr);
  this->threshold = threshold;
  int numberOfGroups = 0;

  for (auto i = 2; i < this->localLengthGroupSpace.size(); i++)
  {
    numberOfGroups += this->_group(i);
  }
  return numberOfGroups;
}

int GlobalGroupSpace::groupMultiThreaded(const std::string& distance_name, data_t threshold, int num_thread)
{
  reset();
  this->_loadDistance(distance_name);
  this->localLengthGroupSpace.resize(dataset.getItemLength() + 1, nullptr);
  this->threshold = threshold;
  int numberOfGroups = 0;

  ThreadPool pool(num_thread);
  vector< std::future<int> > groupCounts;
  for (auto i = 2; i < this->localLengthGroupSpace.size(); i++)
  {
    groupCounts.emplace_back(
      pool.enqueue([this, i] {
        return this->_group(i);
      })
    );
  }
  for (auto i = 0; i < groupCounts.size(); i++)
  {
    numberOfGroups += groupCounts[i].get();
  }
  return numberOfGroups;
}


candidate_time_series_t GlobalGroupSpace::getBestMatch(const TimeSeries& query)
{
  if (query.getLength() <= 1) {
    throw KOnexException("Length of query must be larger than 1");
  }
  data_t bestSoFarDist = INF;
  const Group* bestSoFarGroup = nullptr;

  vector<int> order (generateTraverseOrder(query.getLength(), this->localLengthGroupSpace.size() - 1));
  for (auto io = 0; io < order.size(); io++) {
    int i = order[io];
    // this looks through each group of a certain length finding the best of those groups
    candidate_group_t candidate = this->localLengthGroupSpace[i]->getBestGroup(query, this->warpedDistance, bestSoFarDist);
    if (candidate.second < bestSoFarDist)
    {
      bestSoFarGroup = candidate.first;
      bestSoFarDist = candidate.second;
    }
  }
  return bestSoFarGroup->getBestMatch(query, this->warpedDistance);
}

bool GlobalGroupSpace::grouped(void) const
{
  return localLengthGroupSpace.size() > 0;
}

std::vector<candidate_time_series_t> GlobalGroupSpace::kSim(const TimeSeries& query, int k)
{
  std::vector<candidate_time_series_t> best;
  std::vector<group_index_t> bestSoFar;
  int kPrime = k;
  
  // process each group of a certain length keeping top sum-k groups
  vector<int> order (generateTraverseOrder(query.getLength(), this->localLengthGroupSpace.size() - 1));
  for (auto io = 0; io < order.size(); io++) 
  {
    int i = order[io];
    kPrime = this->localLengthGroupSpace[i]->
        interLevelKSim(query, this->warpedDistance, bestSoFar, kPrime);
  }
  
  // process top group directly
  if (!bestSoFar.empty()) 
  {
    group_index_t g = bestSoFar.front();
    bestSoFar.erase(bestSoFar.begin());
    vector<candidate_time_series_t> intraResults = 
        this->localLengthGroupSpace[g.length]->
            getGroup(g.index)->intraGroupKSim(query, kPrime+g.members, this->warpedDistance);
    // add all of the worst's best to answer
    for (int i = 0; i < intraResults.size(); ++i) 
    {
      best.push_back(intraResults[i]);
    }
  }
  // add all timeseries in the *better* groups 
  for (auto i = 0; i < bestSoFar.size(); i++)
  {
    group_index_t g = bestSoFar[i];  
    vector<TimeSeries> members = 
        this->localLengthGroupSpace[g.length]->getGroup(g.index)->getMembers();
    std::vector<candidate_time_series_t> withBounds;
    for (auto i = 0; i < members.size(); i++) {
      withBounds.push_back(candidate_time_series_t(members[i], g.dist + this->threshold));
    }
    best.insert(std::end(best), std::begin(withBounds), std::end(withBounds));  
  }

  for (auto i = 0; i < best.size(); i++) {
    best[i].dist = this->warpedDistance(query, best[i].data, INF);
  }

  // clean up
  return best;
}

void GlobalGroupSpace::saveGroups(ofstream &fout, bool groupSizeOnly) const
{
  // Range of lengths and distance name
  fout << 2 << " " << this->localLengthGroupSpace.size() << endl;
  fout << this->distanceName << endl;
  for (auto i = 2; i < this->localLengthGroupSpace.size(); i++) {
    this->localLengthGroupSpace[i]->saveGroups(fout, groupSizeOnly);
  }
}

int GlobalGroupSpace::loadGroups(ifstream &fin)
{
  reset();

  int lenFrom, lenTo;
  int numberOfGroups = 0;
  string distance;
  fin >> lenFrom >> lenTo >> distance;
  boost::trim_right(distance);
  this->_loadDistance(distance);
  this->localLengthGroupSpace.resize(dataset.getItemLength() + 1, nullptr);  
  for (auto i = lenFrom; i < lenTo; i++) {
    auto gel = new LocalLengthGroupSpace(dataset, i);
    numberOfGroups += gel->loadGroups(fin);
    this->localLengthGroupSpace[i] = gel;
  }
  return numberOfGroups;
}

vector<int> generateTraverseOrder(int queryLength, int totalLength)
{
  vector<int> order;
  int low = queryLength - 1;
  int high = queryLength + 1;
  bool lowStop = false, highStop = false;

  order.push_back(queryLength);
  while (!(lowStop && highStop)) {
    if (low < 2) lowStop = true;
    if (high > totalLength) highStop = true;

    if (!lowStop) {
      // queryLength is always larger than low
      int r = calculateWarpingBandSize(queryLength);
      if (low + r >= queryLength) {
        order.push_back(low);
        low--;
      }
      else {
        lowStop = true;
      }
    }

    if (!highStop) {
      // queryLength is always smaller than high
      int r = calculateWarpingBandSize(high);
      if (queryLength + r >= high) {
        order.push_back(high);
        high++;
      }
      else {
        highStop = true;
      }
    }
  }
  return order;
}

} // namespace konex
