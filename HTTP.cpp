//
// Created by kwoodle on 11/1/17.
//

#include "NasdXML.h"
#include <fstream>

// cppnetlib
#include <boost/network/protocol/http/client.hpp>

namespace bn = boost::network;
using std::endl;

// libxml2 htmlReadFile doesn't work with https
// https://en.wikipedia.org/wiki/S%26P_100
/*
<table class="wikitable sortable">
<tr>
<th>Symbol</th>
<th>Name</th>
</tr>
<tr>
<td>AAPL</td>
<td><a href="/wiki/Apple_Inc." title="Apple Inc.">Apple Inc.</a></td>
</tr>
<tr>
<td>ABBV</td>
<td><a href="/wiki/AbbVie_Inc." title="AbbVie Inc.">AbbVie Inc.</a></td>
</tr>
</table>
*/
Rows getstocks(const Page& page)
{
    Rows out;
    bn::http::client me;
    bn::http::client::request req(page.loc);
    req << bn::header("Connection", "close");
    bn::http::client::response res = me.get(req);
    string fname{"outfile.html"};
    std::ofstream os(fname);
    auto before = os.tellp();
    if (!os) {
        std::cout << "Failed to open " << fname << endl;
    }
    if (os << body(res)) {
        std::cout << "Wrote " << (os.tellp()-before) << " bytes to " << fname << endl;
    }
    else {
        std::cout << "Failed to write to " << fname << endl;
        return out;
    }
    os.close();
    htmlDocPtr doc = htmlReadFile(fname.c_str(), page.enc.c_str(), HTML_PARSE_RECOVER |
            HTML_PARSE_NOERROR | HTML_PARSE_NOBLANKS);

    string table_class{"wikitable sortable"};

    // only the first such table
    //
    string qsub = "//table[@class='"+table_class+"'][1]/tr";
    string qs = qsub+"/td|"+qsub+"/td/a";

    auto* xpath = (xmlChar*) qs.c_str();
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result = getnodeset(doc, xpath);
    if (result) {
        xmlChar* sym;
        xmlChar* desc;
        nodeset = result->nodesetval;
        for (int i = 0; i<nodeset->nodeNr; i += 3) {
            sym = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
            string symb = (const char*) sym;
            desc = xmlNodeListGetString(doc, nodeset->nodeTab[i+2]->xmlChildrenNode, 1);
            string descrip = (const char*) desc;

            out.emplace_back(make_pair(symb, descrip));

            xmlFree(sym);
            xmlFree(desc);
        }
        xmlXPathFreeObject(result);
        xmlFreeDoc(doc);
    }
    return out;
}
