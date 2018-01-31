//
// Created by kwoodle on 10/31/17.
//

#ifndef NASDXML_NASDXML_H
#define NASDXML_NASDXML_H

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <vector>
#include <string>
#include <map>
#include <curl/curl.h>

#include <boost/date_time/posix_time/posix_time.hpp> //include all types plus i/o

namespace boop = boost::posix_time;


using std::vector;
using std::string;
using std::pair;

using Rows = vector<pair<string, string>>;
struct Page {
    string loc;
    string enc;
};

// Get vector of stocks
// e.g. from S&P 500
Rows getstocks(const Page& page);

using Trades = std::multimap<boop::ptime, pair<string, string>>;

xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar* xpath);

void TestXml2(const string& encod, const Rows& rows, CURL* curl);


#endif //NASDXML_NASDXML_H