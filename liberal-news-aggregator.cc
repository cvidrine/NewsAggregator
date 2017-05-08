/**
 * File: liberal-news-aggregator.cc
 * --------------------------------
 * Presents the implementation of the LiberalNewsAggregator class.
 */

#include "liberal-news-aggregator.h"
using namespace std;
static const int kNumFeedThreads = 6;
static const int kNumThreadsPerServer = 6;
static const int kMaxThreads = 24;
static semaphore feedThreads(kNumFeedThreads);
static semaphore totalThreads(kMaxThreads);
//Server Locks


LiberalNewsAggregator::LiberalNewsAggregator(const string& rssFeedListURI, bool verbose) : 
  NewsAggregator(rssFeedListURI, verbose) {}



void LiberalNewsAggregator::processArticle(Article article){
    totalThreads.signal(on_thread_exit);
    mapLock.lock();
    serverLocks[getURLServer(article.url)]->signal(on_thread_exit);
    mapLock.unlock();
    vector<string> tokens = NewsAggregator::getTokenVector(article.url);
    if(tokens.empty()) return;
    NewsAggregator::updateIndex(article, tokens);
}



void LiberalNewsAggregator::processFeed(const string& Url){
    feedThreads.signal(on_thread_exit);
    totalThreads.signal(on_thread_exit);
     RSSFeed feed(Url);
     if(!NewsAggregator::processFeed(&feed)) return;
     vector<Article>& articles = (vector<Article>&)feed.getArticles();
     sort(articles.begin(), articles.end());
     if(articles.empty()) return;
     vector<thread> v;
     for(Article article : articles){
         if(NewsAggregator::containsUrl(article.url)) continue;
         string server = getURLServer(article.url);
         mapLock.lock();
         unique_ptr<semaphore>& up = serverLocks[server];
         if(up == nullptr) up.reset(new semaphore(kNumThreadsPerServer));
         mapLock.unlock();
         totalThreads.wait();
         up->wait();
         v.push_back(thread([this](Article article){
                 LiberalNewsAggregator::processArticle(article);
                 }, article));
     }
     for(thread& child : v)
         child.join();
}


void LiberalNewsAggregator::processAllFeeds() {
    map<string, string> feeds = NewsAggregator::getFeedMap();
    if(feeds.empty()) return;
    vector<thread> v;
    for(auto& feed : feeds){
       feedThreads.wait();
       totalThreads.wait();
       v.push_back(thread([this](const string& Url){
                   LiberalNewsAggregator::processFeed(Url);         
       }, feed.first));
    }
    for(thread& child : v)
        child.join();
}
