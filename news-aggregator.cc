/**
 * File: news-aggregator.cc
 * ------------------------
 * Presents the implementation of the NewsAggregator class.
 */

#include "news-aggregator.h"
#include "log.h"
#include "liberal-news-aggregator.h"
#include "conservative-news-aggregator.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include <algorithm>
#include <thread>

#include <getopt.h>
#include <libxml/parser.h>
#include <libxml/catalog.h>
#include "rss-feed-list.h"
#include "rss-feed.h"
#include "utils.h"
#include "string-utils.h"

using namespace std;

static const string kDefaultRSSFeedListURL = "small-feed.xml";

void NewsAggregator::updateIndex(Article article, vector<string> tokens){
    titleMapLock.lock();
    if(titleMap.count(article.title) == 1 && getURLServer(titleMap[article.title].first.url) == getURLServer(article.url)){
        auto& mapEntry = titleMap[article.title];
        vector<string> intersection;
        set_intersection(tokens.begin(), tokens.end(), mapEntry.second.begin(), mapEntry.second.end(), back_inserter(intersection));
        sort(intersection.begin(), intersection.end());
        indexLock.lock();
        index.remove(mapEntry.first, mapEntry.second);
        Article a = mapEntry.first.url < article.url ? mapEntry.first : article;
        titleMap[a.title] = pair<Article, vector<string> >(a, intersection);
        index.add(a, intersection);
        indexLock.unlock();
        titleMapLock.unlock();
    }else{
        titleMap[article.title] =  pair<Article, vector<string> >(article, tokens);
        titleMapLock.unlock();
        indexLock.lock();
        index.add(article, tokens);
        indexLock.unlock();
    }
}


vector<string> NewsAggregator::getTokenVector(const string& Url){
    HTMLDocument document(Url);
    try{
        document.parse();
    } catch(const HTMLDocumentException& ex){
        vector<string> emptyResult;
        return emptyResult;
    }
    vector<string> tokens = document.getTokens();
    sort(tokens.begin(), tokens.end());
    return tokens;
}

bool NewsAggregator::containsUrl(const string& Url){
    urls.lock();
    if(articleUrls.count(Url) == 1){
        urls.unlock();
        return true;
    }
    articleUrls.insert(Url);
    urls.unlock();
    return false;
}


bool NewsAggregator::processFeed(RSSFeed *feed){
    try{
     feed->parse();
    } catch(const RSSFeedException& ex){
        return false;
     }
     return true;
}

map<string, string> NewsAggregator::getFeedMap(){
    RSSFeedList feedList(rssFeedListURI);
    try {
        feedList.parse();
    } catch(const RSSFeedListException& ex){
        map<string, string> emptyResult;
        return emptyResult;
    }
    return feedList.getFeeds();
}



NewsAggregator *NewsAggregator::createNewsAggregator(int argc, char *argv[]) {
  struct option options[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"url", required_argument, NULL, 'u'},
    {"conserve-threads", no_argument, NULL, 'c'},
    {NULL, 0, NULL, 0},
  };

  string rssFeedListURI = kDefaultRSSFeedListURL;
  bool verbose = false;
  bool conserve = false;
  while (true) {
    int ch = getopt_long(argc, argv, "vqcu:", options, NULL);
    if (ch == -1) break;
    switch (ch) {
    case 'v':
      verbose = true;
      break;
    case 'q':
      verbose = false;
      break;
    case 'u':
      rssFeedListURI = optarg;
      break;
    case 'c':
      conserve = true;
      break;
    default:
      NewsAggregatorLog::printUsage("Unrecognized flag.", argv[0]);
    }
  }
  
  argc -= optind;
  if (argc > 0) NewsAggregatorLog::printUsage("Too many arguments.", argv[0]);
  
  if (conserve)
    return new ConservativeNewsAggregator(rssFeedListURI, verbose);
  else
    return new LiberalNewsAggregator(rssFeedListURI, verbose);
}

NewsAggregator::NewsAggregator(const string& rssFeedListURI, bool verbose):
  rssFeedListURI(rssFeedListURI), log(verbose), built(false) {}

void NewsAggregator::buildIndex() {
  if (built) return;
  built = false;
  xmlInitParser();
  xmlInitializeCatalog();
  processAllFeeds();
  xmlCatalogCleanup();
  xmlCleanupParser();
}

static const size_t kMaxMatchesToShow = 15;
void NewsAggregator::queryIndex() const {
  while (true) {
    cout << "Enter a search term [or just hit <enter> to quit]: ";
    string response;
    getline(cin, response);
    response = trim(response);
    if (response.empty()) break;
    const vector<pair<Article, int> >& matches = index.getMatchingArticles(response);
    if (matches.empty()) {
      cout << "Ah, we didn't find the term \"" << response << "\". Try again." << endl;
    } else {
      cout << "That term appears in " << matches.size() << " article" 
           << (matches.size() == 1 ? "" : "s") << ".  ";
      if (matches.size() > kMaxMatchesToShow) 
        cout << "Here are the top " << kMaxMatchesToShow << " of them:" << endl;
      else if (matches.size() > 1)
        cout << "Here they are:" << endl;
      else
        cout << "Here it is:" << endl;
      size_t count = 0;
      for (const pair<Article, int>& match: matches) {
        if (count == kMaxMatchesToShow) break;
        count++;
        string title = match.first.title;
        if (shouldTruncate(title)) title = truncate(title);
        string url = match.first.url;
        if (shouldTruncate(url)) url = truncate(url);
        string times = match.second == 1 ? "time" : "times";
        cout << "  " << setw(2) << setfill(' ') << count << ".) "
             << "\"" << title << "\" [appears " << match.second << " " << times << "]." << endl;
        cout << "       \"" << url << "\"" << endl;
      }
    }
  }
}

