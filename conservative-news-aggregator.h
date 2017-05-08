/**
 * File: conservative-news-aggregator.h
 * ------------------------------------
 * Defines the subclass of NewsAggregator that is more conservative with threads
 * than its more liberal peer.  It relies on a ThreadPool to limit the
 * number of threads ever created, and manages to recycle threads to execute many
 * functions instead of just one.
 */

#pragma once
#include "news-aggregator.h"
#include "thread-pool.h"
#include "article.h"
#include <string>
#include <utility>

class ConservativeNewsAggregator: public NewsAggregator {
public:
  ConservativeNewsAggregator(const std::string& rssFeedListURI, bool verbose);
  
protected:
  // implement the one abstract method required of all concrete subclasses
  void processAllFeeds();
  void processFeed(const std::string& Url);
  void processArticle(Article article);
};

