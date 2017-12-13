#define BOOST_TEST_MODULE "Test TimeSeriesSet class"

#include <boost/test/unit_test.hpp>


#include <iostream>     // std::cout
#include <algorithm>    // std::make_heap, std::pop_heap, std::push_heap, std::sort_heap
#include <vector>       // std::vector

#include "TimeSeries.hpp"
#include "KOnexAPI.hpp"
#include "Exception.hpp"

#include "Group.hpp"

using namespace konex;

struct MockDataset
{
  std::string test_10_20_space = "datasets/test/test_10_20_space.txt";
  std::string test_15_20_comma = "datasets/test/test_15_20_comma.csv";
  std::string not_exist = "unicorn_santa_magic_halting_problem_solution";
  std::string uneven_rows = "datasets/test/uneven_rows.txt";
} data;

const bool timeSeriesEqual(const TimeSeries& a, const TimeSeries& b)
{
  return a.getLength() == b.getLength() 
      && a.getStart()  == b.getStart()
      && a.getIndex()  == b.getIndex();
}

const bool containsTimeSeries(const std::vector<TimeSeries> a, const TimeSeries& b)
{
  return std::any_of(a.begin(), a.end(), [&b](TimeSeries i){return timeSeriesEqual(i,b);});
}

const bool containsTimeSeries(const std::vector<candidate_time_series_t> a, const TimeSeries& b)
{
  return std::any_of(a.begin(), a.end(), [&b](candidate_time_series_t i){return timeSeriesEqual(i.data,b);});
}

BOOST_AUTO_TEST_CASE( api_load_dataset )
{
  KOnexAPI api;
  int id0 = api.loadDataset(data.test_10_20_space, 0, 0, " ").id;
  int id1 = api.loadDataset(data.test_15_20_comma, 10, 0, ",").id;
  BOOST_CHECK_EQUAL( id0, 0 );
  BOOST_CHECK_EQUAL( id1, 1 );
  BOOST_CHECK_EQUAL( api.getDatasetCount(), 2 );
  BOOST_CHECK_THROW( api.loadDataset(data.not_exist, 10, 0, " "), KOnexException );
  BOOST_CHECK_THROW( api.loadDataset(data.uneven_rows, 10, 0, " "), KOnexException );
}

BOOST_AUTO_TEST_CASE( api_unload_dataset )
{
  KOnexAPI api;
  int id0 = api.loadDataset(data.test_10_20_space, 5, 0, " ").id;
  int id1 = api.loadDataset(data.test_15_20_comma, 10, 0, ",").id;
  int id2 = api.loadDataset(data.test_10_20_space, 6, 0, " ").id;
  BOOST_CHECK_EQUAL( id0, 0 );
  BOOST_CHECK_EQUAL( id1, 1 );
  BOOST_CHECK_EQUAL( id2, 2 );
  BOOST_CHECK_EQUAL( api.getDatasetCount(), 3 );

  api.unloadDataset(0);
  api.unloadDataset(2);

  BOOST_CHECK_THROW( api.unloadDataset(0), KOnexException );
  BOOST_CHECK_THROW( api.unloadDataset(2), KOnexException );

  BOOST_CHECK_EQUAL( api.getDatasetCount(), 1 );

  id0 = api.loadDataset(data.test_15_20_comma, 14, 4, ",").id;
  id2 = api.loadDataset(data.test_15_20_comma, 9, 0, ",").id;
  BOOST_CHECK_EQUAL( id0, 0 );
  BOOST_CHECK_EQUAL( id2, 2 );
  BOOST_CHECK_EQUAL( api.getDatasetCount(), 3 );
}

BOOST_AUTO_TEST_CASE( api_unload_all_dataset )
{
  KOnexAPI api;
  int id0 = api.loadDataset(data.test_10_20_space, 5, 0, " ").id;
  int id1 = api.loadDataset(data.test_15_20_comma, 10, 0, ",").id;
  int id2 = api.loadDataset(data.test_10_20_space, 6, 0, " ").id;
  BOOST_CHECK_EQUAL( api.getDatasetCount(), 3 );

  api.unloadAllDataset();
  BOOST_CHECK_THROW( api.unloadDataset(0), KOnexException );
  BOOST_CHECK_EQUAL( api.getDatasetCount(), 0 );
}

BOOST_AUTO_TEST_CASE( api_get_dataset_info )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  api.loadDataset(data.test_15_20_comma, 10, 4, ",");

  dataset_info_t info = api.getDatasetInfo(1);
  BOOST_CHECK_EQUAL( info.name, data.test_15_20_comma );
  BOOST_CHECK_EQUAL( info.id, 1 );
  BOOST_CHECK_EQUAL( info.itemCount, 10 );
  BOOST_CHECK_EQUAL( info.itemLength, 16 );

  api.loadDataset(data.test_15_20_comma, 10, 0, ",");

  api.unloadDataset(1);
  BOOST_CHECK_THROW( api.getDatasetInfo(1), KOnexException );
}

BOOST_AUTO_TEST_CASE( api_group )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
 
  int count_1 = api.groupDataset(0, 0.5, "euclidean");
  BOOST_TEST( count_1 > 20 );

  BOOST_CHECK_THROW( api.groupDataset(1, 0.5, "euclidean"), KOnexException ); // no dataset 1 loaded
}

BOOST_AUTO_TEST_CASE( api_match )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  api.loadDataset(data.test_10_20_space, 5, 0, " ");

  int count_1 = api.groupDataset(0, 0.5, "euclidean");

  candidate_time_series_t best_1 = api.getBestMatch(0, 0, 0);
  candidate_time_series_t best_2 = api.getBestMatch(0, 1, 0);
  candidate_time_series_t best_3 = api.getBestMatch(0, 1, 1);
  candidate_time_series_t best_4 = api.getBestMatch(0, 1, 0, 5, 10);

  BOOST_TEST(best_1.dist == 0.0);
  BOOST_TEST(best_2.dist == 0.0);
  BOOST_TEST(best_3.dist == 0.0);
  BOOST_TEST(best_4.dist == 0.0);

  BOOST_CHECK_THROW( api.getBestMatch(1, 0, 0), KOnexException ); // dataset not grouped
  BOOST_CHECK_THROW( api.getBestMatch(1, 0, 35), KOnexException ); // not that many ts in dataset
  BOOST_CHECK_THROW( api.getBestMatch(1, 0, 1, 100, 125), KOnexException ); // not that big ts in dataset
}


BOOST_AUTO_TEST_CASE( api_knn_k_1 )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  api.loadDataset(data.test_10_20_space, 5, 0, " ");

  int count_1 = api.groupDataset(0, 0.5, "euclidean");
  api.setWarpingBandRatio(1.0);  
  
  std::vector<candidate_time_series_t> best_1 = api.kSim(1, 1, 0, 0, 0);
  std::vector<candidate_time_series_t> best_2 = api.kSim(1, 1, 0, 1, 0);
  std::vector<candidate_time_series_t> best_3 = api.kSim(1, 1, 0, 1, 1);
  std::vector<candidate_time_series_t> best_4 = api.kSim(1, 1, 0, 1, 0, 5, 10);
  
  candidate_time_series_t expected_1 = api.getBestMatch(0, 0, 0);
  candidate_time_series_t expected_2 = api.getBestMatch(0, 1, 0);
  candidate_time_series_t expected_3 = api.getBestMatch(0, 1, 1);
  candidate_time_series_t expected_4 = api.getBestMatch(0, 1, 0, 5, 10);
  BOOST_CHECK_EQUAL( best_1.size(), 1 );
  BOOST_CHECK_EQUAL( best_2.size(), 1 );
  BOOST_CHECK_EQUAL( best_3.size(), 1 );
  BOOST_CHECK_EQUAL( best_4.size(), 1 );
  
  BOOST_TEST(timeSeriesEqual(best_1[0].data, expected_1.data));
  BOOST_TEST(timeSeriesEqual(best_2[0].data, expected_2.data));
  BOOST_TEST(timeSeriesEqual(best_3[0].data, expected_3.data));
  BOOST_TEST(timeSeriesEqual(best_4[0].data, expected_4.data));
  
  BOOST_CHECK_THROW( api.kSim(1, 1, 1, 0, 0), KOnexException ); // dataset not grouped
  BOOST_CHECK_THROW( api.kSim(1, 1, 1, 0, 35), KOnexException ); // not that many ts in dataset
  BOOST_CHECK_THROW( api.kSim(1, 1, 1, 0, 1, 100, 125), KOnexException ); // not that big ts in dataset
}

BOOST_AUTO_TEST_CASE( api_knn_k_2 )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  api.loadDataset(data.test_10_20_space, 5, 0, " ");

  int count_1 = api.groupDataset(0, 0.5, "euclidean");
  api.setWarpingBandRatio(1.0);  
  std::vector<candidate_time_series_t> best_1 = api.kSim(2, 2, 0, 0, 0);
  std::vector<candidate_time_series_t> best_2 = api.kSim(2, 2, 0, 1, 0);
  std::vector<candidate_time_series_t> best_3 = api.kSim(2, 2, 0, 1, 1);
  std::vector<candidate_time_series_t> best_4 = api.kSim(2, 2, 0, 1, 0, 5, 10);

  candidate_time_series_t expected_1 = api.getBestMatch(0, 0, 0);
  candidate_time_series_t expected_2 = api.getBestMatch(0, 1, 0);
  candidate_time_series_t expected_3 = api.getBestMatch(0, 1, 1);
  candidate_time_series_t expected_4 = api.getBestMatch(0, 1, 0, 5, 10);

  BOOST_CHECK_EQUAL( best_1.size(), 2 );
  BOOST_CHECK_EQUAL( best_2.size(), 2 );
  BOOST_CHECK_EQUAL( best_3.size(), 2 );
  BOOST_CHECK_EQUAL( best_4.size(), 2 );
  
  BOOST_TEST(containsTimeSeries(best_1, expected_1.data));
  BOOST_TEST(containsTimeSeries(best_2, expected_2.data));
  BOOST_TEST(containsTimeSeries(best_3, expected_3.data));
  BOOST_TEST(containsTimeSeries(best_4, expected_4.data));
}

BOOST_AUTO_TEST_CASE( api_knn_k_4 )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  
  int count_1 = api.groupDataset(0, 0.5, "euclidean");
 
  api.setWarpingBandRatio(1.0); 
  std::vector<candidate_time_series_t> best_1 = api.kSim(4, 4, 0, 0, 0);
  std::vector<candidate_time_series_t> best_2 = api.kSim(4, 4, 0, 1, 0);
  std::vector<candidate_time_series_t> best_3 = api.kSim(4, 4, 0, 1, 1);
  std::vector<candidate_time_series_t> best_4 = api.kSim(4, 4, 0, 1, 0, 4, 10);

  candidate_time_series_t expected_1 = api.getBestMatch(0, 0, 0);
  candidate_time_series_t expected_2 = api.getBestMatch(0, 1, 0);
  candidate_time_series_t expected_3 = api.getBestMatch(0, 1, 1);
  candidate_time_series_t expected_4 = api.getBestMatch(0, 1, 0, 5, 10);

  BOOST_CHECK_EQUAL( best_1.size(), 4 );
  BOOST_CHECK_EQUAL( best_2.size(), 4 );
  BOOST_CHECK_EQUAL( best_3.size(), 4 );
  BOOST_CHECK_EQUAL( best_4.size(), 4 );
  
  BOOST_TEST(containsTimeSeries(best_1, expected_1.data));
  BOOST_TEST(containsTimeSeries(best_2, expected_2.data));
  BOOST_TEST(containsTimeSeries(best_3, expected_3.data));
  BOOST_TEST(containsTimeSeries(best_4, expected_4.data));
 }


BOOST_AUTO_TEST_CASE( api_kx_k_1 )
{
  KOnexAPI api;
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  api.loadDataset(data.test_10_20_space, 5, 0, " ");
  
  int count_1 = api.groupDataset(0, 0.5, "euclidean");

  std::vector<candidate_time_series_t> best_1 = api.kSimRaw(1, 0, 0, 0);
  std::vector<candidate_time_series_t> best_2 = api.kSimRaw(1, 0, 1, 0);
  std::vector<candidate_time_series_t> best_3 = api.kSimRaw(1, 0, 1, 1);
  std::vector<candidate_time_series_t> best_4 = api.kSimRaw(1, 0, 1, 0, 5, 10);

  candidate_time_series_t expected_1 = api.getBestMatch(0, 0, 0);
  candidate_time_series_t expected_2 = api.getBestMatch(0, 1, 0);
  candidate_time_series_t expected_3 = api.getBestMatch(0, 1, 1);
  candidate_time_series_t expected_4 = api.getBestMatch(0, 1, 0, 5, 10);

  BOOST_CHECK_EQUAL( best_1.size(), 1 );
  BOOST_CHECK_EQUAL( best_2.size(), 1 );
  BOOST_CHECK_EQUAL( best_3.size(), 1 );
  BOOST_CHECK_EQUAL( best_4.size(), 1 );
  
  BOOST_TEST(timeSeriesEqual(best_1[0].data, expected_1.data));
  BOOST_TEST(timeSeriesEqual(best_2[0].data, expected_2.data));
  BOOST_TEST(timeSeriesEqual(best_3[0].data, expected_3.data));
  BOOST_TEST(timeSeriesEqual(best_4[0].data, expected_4.data));
 }

 BOOST_AUTO_TEST_CASE( api_kx_k_4 )
 {
   KOnexAPI api;
   api.loadDataset(data.test_10_20_space, 5, 0, " ");
   api.loadDataset(data.test_10_20_space, 5, 0, " ");
   
   int count_1 = api.groupDataset(0, 0.5, "euclidean");
 
   std::vector<candidate_time_series_t> best_1 = api.kSimRaw(4, 0, 0, 0);
   std::vector<candidate_time_series_t> best_2 = api.kSimRaw(4, 0, 1, 0);
   std::vector<candidate_time_series_t> best_3 = api.kSimRaw(4, 0, 1, 1);
   std::vector<candidate_time_series_t> best_4 = api.kSimRaw(4, 0, 1, 0, 5, 10);
 
   candidate_time_series_t expected_1 = api.getBestMatch(0, 0, 0);
   candidate_time_series_t expected_2 = api.getBestMatch(0, 1, 0);
   candidate_time_series_t expected_3 = api.getBestMatch(0, 1, 1);
   candidate_time_series_t expected_4 = api.getBestMatch(0, 1, 0, 5, 10);
 
   BOOST_CHECK_EQUAL( best_1.size(), 4 );
   BOOST_CHECK_EQUAL( best_2.size(), 4 );
   BOOST_CHECK_EQUAL( best_3.size(), 4 );
   BOOST_CHECK_EQUAL( best_4.size(), 4 );
   
   BOOST_TEST(containsTimeSeries(best_1, expected_1.data));
   BOOST_TEST(containsTimeSeries(best_2, expected_2.data));
   BOOST_TEST(containsTimeSeries(best_3, expected_3.data));
   BOOST_TEST(containsTimeSeries(best_4, expected_4.data));
  }