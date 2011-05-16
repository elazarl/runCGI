/*
 * YamlCGIConfFile.h
 *
 *  Created on: Apr 14, 2011
 *      Author: Elazar
 */

#ifndef YAMLCGICONFFILE_H_
#define YAMLCGICONFFILE_H_

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <iosfwd>

#include <boost/shared_ptr.hpp>

typedef std::map<std::string,std::string> smap;

extern const smap defaultEmptySMap;
class RunCGI;

class YamlCGIConfFile {
public:
	YamlCGIConfFile(const std::string& fileName="",const smap& _templateParams=defaultEmptySMap);
	void setTemplateParams(const std::map<std::string,std::string>&);
	void setFileName(const std::string&);
	void setFileStream(std::istream&);
	const std::string getFileName() const;
	void setInheritance(std::deque<boost::shared_ptr<YamlCGIConfFile> >* cgi);
	void readFile();
	void updateCGI(RunCGI* cgi);
private:
	// normalized a path relative to CGI's base dir, to
	// the current CWD.
	// So if you're in /home/bob, and you're running
	// $ runCGI x/a.yaml, which contains "_exec: a.exe", it'll
	// change the path "a.exe" to "x/a.exe"
	std::string toCWDPath(const std::string& path);
	void seal();
	void readYamlFileWithTemplate(std::istream& f);
	void readYamlFile(std::istream& f);
	void updateSingleCGI(RunCGI* cgi);
	bool isSealed; // poor man's immutability
	bool isYamlFileRead;
	std::string working_directory;
	std::string fileName;
	std::string fileBaseDir;
	std::istream* fileStream;
	std::string method;
	std::string executable;
	std::vector<std::string> args;
	std::vector<std::string> inheritFrom;
	smap queryString;
	smap configuration;
	smap environment;
	smap templateParams;
};

#endif /* YAMLCGICONFFILE_H_ */
