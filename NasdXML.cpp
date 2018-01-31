//
// Created by kwoodle on 10/31/17.
//

#include <cstdio>
#include <libxml/HTMLtree.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>
#include "NasdXML.h"
//#include <chrono>

//using namespace std::chrono;
using std::cout;
using std::cerr;
using std::endl;

xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar* xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context==nullptr) {
        cerr << "Error in xmlXPathNewContext" << endl;
        return nullptr;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result==nullptr) {
        cerr << "Error in xmlXPathEvalExpression" << endl;
        return nullptr;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        xmlXPathFreeObject(result);
        cerr << "No result" << endl;
        return nullptr;
    }
    return result;
}


// *Delayed - data as of <span id="qwidget_markettime">Nov. 8, 2017 </span>
/* 		            <table id="AfterHoursPagingContents_Table">

                                <thead>
				                    <tr>
					                    <th>NLS Time (ET)</th>
					                    <th>NLS Price</th>
					                    <th>NLS Share Volume</th>
				                    </tr>
				                </thead>

				                <tr>
					                <td>09:59:57</td>
					                <td>$&nbsp;49.81&nbsp; </td>
					                <td>100</td>
				                </tr>

				                <tr>
					                <td>09:59:57 </td>
					                <td>$&nbsp;49.81 &nbsp;</td>
					                <td>100</td>
				                </tr>
				                        .
		                                .
                                        .
				                <tr>
					                <td>09:59:25 </td>
					                <td>$&nbsp;49.805 &nbsp;</td>
					                <td>100</td>
				                </tr>

			        </table>
*/

/* Symbol | TradeTime           | TradePrice | TradeSize
 *   MS   | YYYY-MM-DD HH:MM:SS | 55.43      | 100
 *   */

void TestXml2(const string& enc, const Rows& rows, CURL* curl)
{

    // http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=3
    static const string root{"http://www.nasdaq.com/symbol/"};
    static const string tsls{"/time-sales?time="};
    static const string pgnos{"&pageno="};

    // step through stocks in the index
    //
    static const int ntest{4};
    int itest{0};
    for (auto& r: rows) {
        if (itest++==ntest) return;
        Trades trds;
        string eloc = root+r.first += tsls;
        //
        // drop-down list of times of day
        // eleven by end of day
        for (int it = 1; it<=11; ++it) {
            string loc = eloc+std::to_string(it);
            //
            // get the last page
            int mxpno;
            {   // scope htmlDocPtr;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
                string fname{"nasd.html"};
                FILE* filptr = fopen(fname.c_str(), "w");
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, filptr);
                curl_easy_setopt(curl, CURLOPT_URL, loc.c_str());
                CURLcode res = curl_easy_perform(curl);
                if (!(CURLE_OK==res)) {
                    long error;
                    res = curl_easy_getinfo(curl, CURLINFO_OS_ERRNO, &error);
                    if (res && error) {
                        std::cerr << "CURLErrorno: " << error << endl;
                    }
                    else
                        std::cerr << "CURL error" << endl;
                    fclose(filptr);
                    return;
                }
                double total;
                res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
                if (CURLE_OK==res) {
                    std::cout << "Total CURL time = " << total << endl;
                }
                fclose(filptr);
                htmlDocPtr doc = htmlReadFile(fname.c_str(), enc.c_str(), HTML_PARSE_RECOVER |
                        HTML_PARSE_NOERROR | HTML_PARSE_NOBLANKS);

// <a href="http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=32" id="quotes_content_left_lb_LastPage" class="pagerlink">last &gt;&gt;</a>
                string anch{"quotes_content_left_lb_LastPage"};
                string ancid = "//a[@id='"+anch+"']";
                auto* axp = (xmlChar*) ancid.c_str();
                xmlNodeSetPtr anod;
                xmlXPathObjectPtr ares = getnodeset(doc, axp);
                string lstpage;
                if (ares) {
                    xmlChar* xlstpg;
                    anod = ares->nodesetval;
                    xmlNodePtr nd = anod->nodeTab[0];
                    xlstpg = xmlGetProp(nd, reinterpret_cast<const xmlChar*>("href"));
                    if (xlstpg) {
                        lstpage = (const char*) xlstpg;
                    }
                    else {
                        std::cerr << "Failed to find last page." << endl;
                        xmlFree(xlstpg);
                        xmlXPathFreeObject(ares);
                        xmlFreeDoc(doc);
                        return;
                    }
                    xmlXPathFreeObject(ares);
                    xmlFree(xlstpg);
                }
                std::regex pat{R"(pageno=(\d+))"};
                std::smatch matches;
                bool foun = regex_search(lstpage, matches, pat);
                if (!foun) {
                    std::cerr << "Couldn't find last page number" << endl;
                    xmlFreeDoc(doc);
                    return;
                }
                mxpno = std::stoi(matches[1]);
                std::cout << "Last page for " << r.first << " is " << mxpno << endl;
                xmlFreeDoc(doc);
            }
            for (int ip = mxpno; ip>=1; --ip) {
//                if (itest++ == ntest) return;

                // URL for curl
                string ploc = loc+pgnos+std::to_string(ip);
                double total;
                curl_easy_setopt(curl, CURLOPT_URL, ploc.c_str());
                string fname{"nasd.html"};
                FILE* filptr = fopen(fname.c_str(), "w");
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, filptr);
                CURLcode curl_res = curl_easy_perform(curl);
                if (CURLE_OK!=curl_res) {
                    long error;
                    curl_res = curl_easy_getinfo(curl, CURLINFO_OS_ERRNO, &error);
                    if (curl_res && error) {
                        std::cerr << "CURLErrorno: " << error << endl;
                    }
                    else
                        std::cerr << "CURL error" << endl;
                    fclose(filptr);
                    return;
                }
                curl_res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
                if (CURLE_OK==curl_res) {
                    std::cout << "Total CURL time = " << total << endl;
                }
                fclose(filptr);
                htmlDocPtr doc = htmlReadFile(fname.c_str(), enc.c_str(), HTML_PARSE_RECOVER |
                        HTML_PARSE_NOERROR | HTML_PARSE_NOBLANKS);
                // get page date
                //
                string span{"qwidget_markettime"};
                string spid = "//span[@id='"+span+"']";
                auto* xp = (xmlChar*) spid.c_str();
                xmlNodeSetPtr nod;
                xmlXPathObjectPtr res = getnodeset(doc, xp);
                string pdate;
                if (res) {
                    xmlChar* pgdate;
                    nod = res->nodesetval;
                    pgdate = xmlNodeListGetString(doc, nod->nodeTab[0]->xmlChildrenNode, 1);
                    pdate = (const char*) pgdate;
                    xmlFree(pgdate);
                }
                else {
                    std::cerr << "Failed to get page date" << endl;
                    xmlXPathFreeObject(res);
                    return;
                }
                xmlXPathFreeObject(res);
                //
                // get Time and Sales
                string table_class = "AfterHoursPagingContents_Table";
                string qs = "//table[@id='"+table_class+"']/tr/td";
                auto* xpath = (xmlChar*) qs.c_str();
                xmlNodeSetPtr nodeset;
                xmlXPathObjectPtr result = getnodeset(doc, xpath);
                if (result) {
                    xmlChar* time, * price, * shares;
                    nodeset = result->nodesetval;
                    int i{0};
                    for (; i<nodeset->nodeNr; i += 3) {
                        time = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
                        string ti = (const char*) time;
                        ti.erase(std::remove_if(ti.begin(), ti.end(),
                                [](unsigned char x) { return std::isspace(x); }), ti.end());

                        // construct timestamp
                        //
                        struct tm tm;
                        memset(&tm, 0, sizeof(struct tm));
                        string dattim = pdate+" " += ti;
                        // e.g. Nov. 7, 2017
                        //
                        strptime(dattim.c_str(), "%b. %d, %Y %H:%M:%S", &tm);
                        //
                        // use boost posix time in multimap to provide sorting
                        //
                        boop::ptime pt(boop::ptime_from_tm(tm));
                        string dtim(boop::to_iso_extended_string(pt));
                        //
                        // Replace T in 2017-11-07T09:38:00
                        std::replace(dtim.begin(), dtim.end(), 'T', ' ');

                        // MySQL TIMESTAMP 2038-01-19 03:14:07 %Y-%m-%d

                        price = xmlNodeListGetString(doc, nodeset->nodeTab[i+1]->xmlChildrenNode, 1);
                        string pri = (const char*) price;
                        size_t pos = pri.find('$');
                        if (pos!=string::npos) pri.erase(pos, 1);
                        pri.erase(std::remove_if(pri.begin(), pri.end(),
                                [](unsigned char x) { return std::isspace(x); }), pri.end());
                        pri.erase(std::remove_if(pri.begin(), pri.end(),
                                [](unsigned char x) { return !std::isprint(x); }), pri.end());
                        shares = xmlNodeListGetString(doc, nodeset->nodeTab[i+2]->xmlChildrenNode, 1);
                        string sha = (const char*) shares;
                        pos = sha.find(',');
                        if (pos!=string::npos) sha.erase(pos, 1);
                        trds.emplace(pt, std::make_pair(pri, sha));
                        xmlFree(time);
                        xmlFree(price);
                        xmlFree(shares);
                    }
                    auto t = *trds.rbegin();
                    std::cout << (i/3) << " trades for " << r.first << " on page no " << ip << " and it = " << it
                              << endl;
                    std::cout << "Last Trade " << t.first << " " << t.second.first << " " << t.second.second << "\n";
                }
                std::cout << trds.size() << " trades for " << r.first << "\n" << endl;
//                for (auto &t:trds) {
//                    std::cout << r.first <<"|"<< boop::to_iso_extended_string(t.first) << "|" << t.second.first << "|"
//                              << t.second.second << "\n";
//                }
                xmlXPathFreeObject(result);
                xmlFreeDoc(doc);
            }
        }
    }
}