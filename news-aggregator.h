/**
 * File: news-aggregator.h
 * -----------------------
 * Defines the NewsAggregator class, which understands how to 
 * build an index of all HTML articles reachable from a 
 * command-line-provided RSS feed list URI and interact
 * with a user interested in searching that index.
 *
 * Note that NewsAggregator is an abstract base class, and it
 * exists to be subclassed and unify state and logic common to
 * all subclasses into one location.
 */

#pragma once
#include <string>
#include <algorithm>
#include <cassert>
#include <thread>
#include <mutex>
#include <set>

#include "log.h"
#include "rss-index.h"
#include "html-document.h"
#include "rss-feed.h"
#include "rss-feed-list.h"
#include "utils.h"

class NewsAggregator {
 public: // define those entries that everyone can see and touch
  static NewsAggregator *createNewsAggregator(int argc, char *argv[]);
  virtual ~NewsAggregator() {}
  
  void buildIndex();
  void queryIndex() const;
  
 protected: // defines those entries that only subclasses can see and touch
  std::string rssFeedListURI;
  NewsAggregatorLog log;
  RSSIndex index;
  bool built;


  void updateIndex(Article article, std::vector<std::string> tokens);
  bool containsUrl(const std::string& Url);
  std::map<std::string, std::string> getFeedMap();
  std::vector<std::string> getTokenVector(const std::string& Url);
  bool processFeed(RSSFeed* feed);


  std::map<std::string, std::pair<Article, std::vector<std::string> > > titleMap;
  std::set<std::string> articleUrls;

  std::mutex urls;
  std::mutex titleMapLock;
  std::mutex indexLock;
  
  NewsAggregator(const std::string& rssFeedListURI, bool verbose);
  virtual void processAllFeeds() = 0;
  
 private: // defines those entries that only NewsAggregator methods (not even subclasses) can see and touch
  NewsAggregator(const NewsAggregator& original) = delete;
  NewsAggregator& operator=(const NewsAggregator& rhs) = delete;
};
