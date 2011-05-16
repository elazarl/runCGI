/*
 * ERBTemplate.h
 *
 *  Created on: Apr 11, 2011
 *      Author: Elazar
 */

#ifndef ERBTEMPLATE_H_
#define ERBTEMPLATE_H_

/**
 * A very simple RE based template engine which is capable
 * of parsing a subset of the ERB template engine.
 * The constructor takes a string to string map, and for
 * each KEY,VAL in the map it replaces the RE <=%\s*KEY\s*%>
 * with VAL.
 */
#include <map>
#include <string>
#include <iosfwd>
#include <boost/shared_ptr.hpp>

class TemplateTransformer;

class ERBTemplate {
public:
	ERBTemplate(const std::map<std::string,std::string>&);
	void renderTo(std::istream&,std::ostream&);
	void renderTo(const std::string&,std::ostream&);
	std::string render(std::istream&);
	std::string render(const std::string&);
private:
	std::map<std::string,std::string> dictionary;
	boost::shared_ptr<TemplateTransformer> transformer;
};

#endif /* ERBTEMPLATE_H_ */
