/**
 * File: liberal-news-aggregator.h
 * -------------------------------
 * Defines the subclass of NewsAggregator that is openly liberal about its
 * use of threads.  While it is smart enough to limit the number of threads
 * that can exist at any one time, it does not try to converve threads
 * by pooling or reusing them.  Instead, it creates a new thread
 * every time something needs to be downloaded.  It's easy, but wasteful.
 */

#pragma once
#include "news-aggregator.h"
#include <map>
#include <mutex>
#include <memory>

#include "semaphore.h"
#include "article.h"
#include "html-document.h"
#include "rss-feed.h"
#include "rss-feed-list.h"
#include "rss-index.h"


class LiberalNewsAggregator : public NewsAggregator {
public:
  LiberalNewsAggregator(const std::string& rssFeedListURI, bool verbose);

protected:  
  // implement the one abstract method required of all concrete subclasses
  void processFeed(const std::string& Url);
  void processArticle(Article article);
  void processAllFeeds();

  std::map<std::string, std::unique_ptr<semaphore>> serverLocks;
  std::mutex mapLock;

};
