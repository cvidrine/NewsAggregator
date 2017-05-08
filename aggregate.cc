/**
 * File: aggregate.cc
 * ------------------
 * Defines the entry point to the aggregate executable.
 * Note that the main function essentially passes the buck
 * to whatever instance is returned by the createNewsAggregator
 * factory method.  That instance is asked to build the 
 * index and then allow the user to search it.
 */

#include "news-aggregator.h"
#include <memory> // for unique_ptr
using std::unique_ptr;

int main(int argc, char *argv[]) {
  unique_ptr<NewsAggregator> aggregator(NewsAggregator::createNewsAggregator(argc, argv));
  aggregator->buildIndex();
  aggregator->queryIndex();
  return 0;
}
