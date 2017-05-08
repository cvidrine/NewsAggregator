/**
 * File: conservative-news-aggregator.cc
 * -------------------------------------
 * Presents the implementation of the ConservativeNewsAggregator class.
 */

#include "conservative-news-aggregator.h"
using namespace std;
static const size_t kNumRssThreads = 6;
static const size_t kNumArticleThreads = 24;
ThreadPool articlePool(kNumArticleThreads);

ConservativeNewsAggregator::ConservativeNewsAggregator(const string& rssFeedListURI, bool verbose) : 
  NewsAggregator(rssFeedListURI, verbose) {}

void ConservativeNewsAggregator::processArticle(Article article){
    vector<string> tokens = NewsAggregator::getTokenVector(article.url);
    if(tokens.empty()) return;
    NewsAggregator::updateIndex(article, tokens);
}


void ConservativeNewsAggregator::processFeed(const string& Url){
     RSSFeed feed(Url);
     if(!NewsAggregator::processFeed(&feed)) return;
     vector<Article>& articles = (vector<Article>&)feed.getArticles();
     sort(articles.begin(), articles.end());
     if(articles.empty()) return;
     for(Article article : articles){
         if(NewsAggregator::containsUrl(article.url)) continue;
         articlePool.schedule([this, article]{ConservativeNewsAggregator::processArticle(article);});
     }
}
void ConservativeNewsAggregator::processAllFeeds() {
    map<string, string> feeds = NewsAggregator::getFeedMap();
    if(feeds.empty()) return;
    ThreadPool rssPool(kNumRssThreads);
    for(auto& feed: feeds)
        rssPool.schedule([this, feed]{ConservativeNewsAggregator::processFeed(feed.first);});
}
