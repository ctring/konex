#include "GroupableTimeSeriesSet.hpp"
#include "GlobalGroupSpace.hpp"
#include "Exception.hpp"
#include "distance/Distance.hpp"
#include <iostream>

#include <fstream>

using std::ofstream;
using std::ifstream;
using std::cout;
using std::endl;

namespace konex {

GroupableTimeSeriesSet::~GroupableTimeSeriesSet()
{
  this->reset();
}

int GroupableTimeSeriesSet::groupAllLengths(const std::string& distance_name, data_t threshold, int numThreads)
{
  if (!this->isLoaded())
  {
    throw KOnexException("No data to group");
  }

  // clear old groups
  reset();

  this->groupsAllLengthSet = new GlobalGroupSpace(*this);
  int cntGroups;
  if (numThreads == 1) {
    cntGroups = this->groupsAllLengthSet->group(distance_name, threshold);
  }
  else if (numThreads > 1) {
    cntGroups = this->groupsAllLengthSet->groupMultiThreaded(distance_name, threshold, numThreads);    
  }
  else {
    throw KOnexException("Number of threads must be positive");    
  }

  this->threshold = threshold;
  return cntGroups;
}

bool GroupableTimeSeriesSet::isGrouped() const
{
  return this->groupsAllLengthSet != nullptr;
}

void GroupableTimeSeriesSet::reset()
{
  delete this->groupsAllLengthSet;
  this->groupsAllLengthSet = nullptr;
}

void GroupableTimeSeriesSet::saveGroups(const string& path, bool groupSizeOnly) const
{
  if (!this->isGrouped()) {
    throw KOnexException("No group found");
  }

  ofstream fout(path);
  if (fout)
  {
    // Version of the file format, the threshold and the required dataset dimensions
    fout << GROUP_FILE_VERSION << " " 
         << this->threshold << " "
         << this->getItemCount() << " "
         << this->getItemLength() << endl;
    this->groupsAllLengthSet->saveGroups(fout, groupSizeOnly);
  }
  else
  {
    throw KOnexException("Cannot open file");
  }
}

int GroupableTimeSeriesSet::loadGroups(const string& path)
{
  int numberOfGroups = 0;
  ifstream fin(path);
  if (fin)
  {
    int version, grpItemCount, grpItemLength;
    data_t threshold;
    fin >> version >> threshold >> grpItemCount >> grpItemLength;
    if (version != GROUP_FILE_VERSION)
    {
      throw KOnexException("Incompatible file version");
    }
    if (grpItemCount != this->getItemCount())
    {
      throw KOnexException("Incompatible item count");
    }
    if (grpItemLength != this->getItemLength())
    {
      throw KOnexException("Incompatible item length");
    }
    cout << "Saved groups are compatible with the dataset" << endl;
    reset();
    this->threshold = threshold;
    this->groupsAllLengthSet = new GlobalGroupSpace(*this);
    numberOfGroups = this->groupsAllLengthSet->loadGroups(fin);
  }
  else
  {
    throw KOnexException("Cannot open file");
  }
  return numberOfGroups;
}

candidate_time_series_t GroupableTimeSeriesSet::getBestMatch(const TimeSeries& query) const
{
  if (this->groupsAllLengthSet) //not nullptr
  {
    return this->groupsAllLengthSet->getBestMatch(query);
  }
  throw KOnexException("Dataset is not grouped");
}

std::vector<candidate_time_series_t> GroupableTimeSeriesSet::kSim(const TimeSeries& query, int k, int h)
{
  if (this->groupsAllLengthSet) //not nullptr
  {
    if (h < k) {
      throw KOnexException("Number of examined time series must be larger than "
                           "or equal to the number of time series to look for");
    }
    std::vector<candidate_time_series_t> results = this->groupsAllLengthSet->kSim(query, h);
    std::sort(results.begin(), results.end());
    results.resize(k);
    return results;
  }
  throw KOnexException("Dataset is not grouped");
}


} // namespace konex