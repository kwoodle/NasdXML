//
// Created by kwoodle on 10/25/17.
//
//
#include <iostream>
#include "NasdXML.h"

//    string enc{"ISO-8859-1"};
// %b. %d, %Y
int main()
{

    // Get list of 100 stocks
    //
    string enc{"UTF-8"};
    Page SP100{"https://en.wikipedia.org/wiki/S%26P_100", enc};
    xmlInitParser();
    Rows sandp = getstocks(SP100);
    for (auto& r:sandp) {
//        std::cout << r.first << "  " << r.second << "\n";
    }
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "curl_easy_int failed!" << std::endl;
        return 1;
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    TestXml2(enc, sandp, curl);
    curl_easy_cleanup(curl);

    return 0;
}
