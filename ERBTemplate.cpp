/*
 * ERBTemplate.cpp
 *
 *  Created on: Apr 11, 2011
 *      Author: Elazar
 */

#include "ERBTemplate.h"

#include <iterator>
#include <algorithm>
#include <vector>
#include <sstream>
#include <boost/regex.hpp>

using namespace std;
using boost::shared_ptr;
using boost::regex;

/**
 * Intialized with dictionary of key,val, replaces every occurrence of <%=key%> with val
 */
struct TemplateTransformer {
	TemplateTransformer(const map<string,string>& dictionary) {
		for (map<string,string>::const_iterator it(dictionary.begin());
				it!=dictionary.end();
				++it) {
			string pattern = "<%=\\s*"+it->first+"\\s*%>";
			regexps.push_back(regex(pattern));
			replace_regexp_with.push_back(it->second);
		}
	}
	string transform (const string& line) {
		string newline = line;
		for (unsigned int i=0;i<regexps.size();++i) newline = boost::regex_replace(newline,
				regexps[i],replace_regexp_with[i],
				boost::match_default|boost::format_sed);
		return newline;
	}
	string operator() (const string& line) {return transform(line);}
	vector<regex> regexps;
	vector<string> replace_regexp_with;
};

ERBTemplate::ERBTemplate(const std::map<std::string,std::string>& _dictionary):
dictionary(_dictionary),transformer(new TemplateTransformer(dictionary)){}

void ERBTemplate::renderTo(std::istream& in,std::ostream& out) {
	string line;
	while (getline(in,line)) {
		// TODO: this sounds like a bad idea, it would strip the native EOL
		// and replace it with UNIX.
		// I think I don't care enough to solve it though.
		out << transformer->transform(line) << "\n";
	}
}

void ERBTemplate::renderTo(const string& tmpl,ostream& out) {
	istringstream in(tmpl);
	renderTo(in,out);
}
std::string ERBTemplate::render(std::istream& in) {
	ostringstream out;
	renderTo(in,out);
	return out.str();
}
std::string ERBTemplate::render(const std::string& tmpl) {
	istringstream in(tmpl);
	return render(in);
}
