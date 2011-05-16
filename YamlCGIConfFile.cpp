/*
 * YamlCGIConfFile.cpp
 *
 *  Created on: Apr 14, 2011
 *      Author: Elazar
 */

#include "YamlCGIConfFile.h"

#include <fstream>
#include <set>

#include <boost/algorithm/string.hpp>
#include <yaml-cpp/yaml.h>

#include "paths.h"

#include "ERBTemplate.h"
#include "Logger.h"
#include "runCGI.h"

using namespace std;
using boost::shared_ptr;
class RunCGI;

typedef std::map<std::string,boost::shared_ptr<YAML::Node> > nodemap;

template <bool condition, typename T>
struct enable_if_c {};        // by default do not declare inner type

template <typename T>
struct enable_if_c<true,T> {  // if true, typedef the argument as inner type
    typedef T type;
};

template <typename T>
class has_key_type {
    typedef char _a;
    struct _b { _a x[2]; };

    template <typename U>
    static _a foo( U const &, typename U::key_type* p = 0 );
    static _b foo( ... );
    static T& generate_ref();
public:
    static const bool value = sizeof(foo(generate_ref())) == sizeof(_a);
};

template<typename T> inline
typename enable_if_c< has_key_type<T>::value, bool >::type
has_item(const T& haystack,const typename T::key_type& needle) {
	return haystack.find(needle) != haystack.end();
}

template<typename T> inline
typename enable_if_c< !has_key_type<T>::value, bool >::type
has_item(const T& haystack,const typename T::value_type& needle) {
	return find(haystack.begin(),haystack.end(),needle) != haystack.end();
}

static const string dirname(const string& path) {
	string result(path);
	result.erase(std::find(result.rbegin(), result.rend(), pathsep[0]).base(), result.end());
	return result;
}

static const string basenamewithsep(const string& path) {
	string result(path);
	result.erase(result.begin(),std::find(result.rbegin(), result.rend(), pathsep[0]).base());
	return result;
}

const string EXEC_CONF = "_exec";
const string ARGS_CONF = "_args";
const string INHERIT_CONF = "_inherit";
const string WORKING_DIR_CONF = "_working_dir";
const string MANDATORY_TEMPLATE_CONF = "_need_params";

void YAML2map(const YAML::Node& root,smap*const valuem,nodemap*const nodem,smap*const conf=NULL) {
	for (YAML::Iterator it(root.begin()); it != root.end(); ++it) {
		const string& key(it.first().to<string>());
		const YAML::Node& val(it.second());
		if (val.Type() == YAML::NodeType::Scalar) {
			string string_val = val.to<string>();
			if (!key.empty() && key[0] == '_' && conf) (*conf)[key] = string_val;
			else if (valuem) (*valuem)[key] = string_val;
		}
		else if (nodem) {
			YAML::Node* node = val.Clone().release();
			(*nodem)[key] = shared_ptr<YAML::Node>(node);
		}
	}
}


void YamlCGIConfFile::readYamlFileWithTemplate(istream& f) {
//	if (template_map.size() == 0) _addYAMLtoCGIconf(cgi,f,template_map);
	if (templateParams.empty()) readYamlFile(f);

	stringstream yamlstream;
	ERBTemplate tmpl(templateParams);
	tmpl.renderTo(f,yamlstream);
	readYamlFile(yamlstream);
}

void YamlCGIConfFile::readFile() {
	if (isYamlFileRead) return;
	isYamlFileRead = true;

	if (fileName.empty() && NULL == fileStream) {
		throw std::runtime_error("YamlCGIConfFile: readFile called without input stream/filename");
	}
	if (fileStream) readYamlFileWithTemplate(*fileStream);
	else {
		ifstream f(fileName.c_str());
		readYamlFileWithTemplate(f);
	}
}

string type2str(YAML::NodeType::value t) {
	switch (t) {
	case YAML::NodeType::Null: return "Null";
	case YAML::NodeType::Scalar: return "Scalar";
	case YAML::NodeType::Sequence: return "Sequence";
	case YAML::NodeType::Map: return "Map";
	}
	return "Undefined NodeType";
}

void YamlCGIConfFile::readYamlFile(istream& f) {
	YAML::Parser parser(f);
	YAML::Node doc;
	parser.GetNextDocument(doc);

	smap configuration;
	nodemap data;
	YAML2map(doc,&environment,&data,&configuration);

	if (has_item(configuration,INHERIT_CONF)) {
		Logger::info("Inheriting from "+toCWDPath(configuration[INHERIT_CONF]));
		inheritFrom.push_back(toCWDPath(configuration[INHERIT_CONF]));
	}
	if (has_item(data,INHERIT_CONF)) {
		for (YAML::Iterator it(data[INHERIT_CONF]->begin());
				it != data[INHERIT_CONF]->end();
				++it) {
			if (it->Type() == YAML::NodeType::Scalar) inheritFrom.push_back(toCWDPath(it->to<string>()));
		}
	}

	if (has_item(configuration,MANDATORY_TEMPLATE_CONF) &&
		boost::to_lower_copy(configuration[MANDATORY_TEMPLATE_CONF]) == "true" &&
		templateParams.size() == 0) {
		throw runtime_error("YAML file with mandatory template parameters, "
				"but no parameters given");
	}

	if (has_item(data,"query_string")) {
		YAML2map(*data["query_string"],&queryString,NULL);
	}

	if (has_item(environment,"method")) {
		method = boost::to_upper_copy(environment["method"]);
		environment.erase("method");
	} else if (has_item(environment,"METHOD")) {
		method = boost::to_upper_copy(environment["METHOD"]);
		environment.erase("METHOD");
	}

	if (has_item(configuration,EXEC_CONF)) executable = configuration[EXEC_CONF];
	if (has_item(configuration,WORKING_DIR_CONF)) working_directory = configuration[WORKING_DIR_CONF];

	if (has_item(configuration,ARGS_CONF)) args.push_back(configuration[ARGS_CONF]);
	if (has_item(data,ARGS_CONF)) {
		shared_ptr<YAML::Node> args_node =  data[ARGS_CONF];
		if (args_node->Type() != YAML::NodeType::Sequence) {
			cout << "Illegal YAML configuration " << type2str(args_node->Type()) << " for" << ARGS_CONF << ", use list instead of map" << endl;
			exit(-1);
		}
		for (YAML::Iterator it(args_node->begin()); it != args_node->end(); ++it) {
			args.push_back(it->to<string>());
		}
	}
}
string YamlCGIConfFile::toCWDPath(const string& path) {
	if (!path.empty() && path[0] == pathsep[0]) return path;
	return fileBaseDir+path;
}

void YamlCGIConfFile::setInheritance(deque<shared_ptr<YamlCGIConfFile> >* processedInheritedFiles) {
	deque<string> unprocessedInheritedFiles;
	unprocessedInheritedFiles.insert(unprocessedInheritedFiles.end(),
			inheritFrom.begin(),inheritFrom.end());

	set<string> allInheritanceSet;
	while (!unprocessedInheritedFiles.empty()) {
		string file = unprocessedInheritedFiles.back();
		unprocessedInheritedFiles.pop_back();

		if (has_item(allInheritanceSet,file)) throw std::runtime_error("cyclic inheritance in YAML file");
		else allInheritanceSet.insert(file);

		shared_ptr<YamlCGIConfFile> f(new YamlCGIConfFile(file,templateParams));
		f->readFile();
		processedInheritedFiles->push_front(f);
		unprocessedInheritedFiles.insert(unprocessedInheritedFiles.end(),
				f->inheritFrom.begin(),f->inheritFrom.end());
	}
}
void YamlCGIConfFile::updateSingleCGI(RunCGI* cgi) {
	readFile();
	if (!executable.empty()) cgi->setExecutable(executable);
	if (!method.empty()) cgi->setMethod(method);
	for (int i=0;i<args.size();i++) {cgi->addArgv(args[i]);}
	cgi->addEnv(environment);
	cgi->addQueryString(queryString);
	if (!working_directory.empty()) cgi->setWorkingDirectory(working_directory);
}
void YamlCGIConfFile::updateCGI(RunCGI* cgi) {
	readFile();
	deque<shared_ptr<YamlCGIConfFile> > processedInheritedFiles;
	setInheritance(&processedInheritedFiles);
	for (int i=0;i<processedInheritedFiles.size();i++) {
		Logger::info("Processing "+processedInheritedFiles[i]->getFileName());
		processedInheritedFiles[i]->updateSingleCGI(cgi);
	}

	updateSingleCGI(cgi);
	if (cgi->getWorkingDirectory().empty()) cgi->setWorkingDirectory(fileBaseDir);
}

void YamlCGIConfFile::seal() {
	if (isSealed) throw std::runtime_error("Cannot change sealed YamlCGIConfFile");
	isSealed = true;
}

void YamlCGIConfFile::setTemplateParams(const smap& _templateParams) {
	templateParams = _templateParams;
}

void YamlCGIConfFile::setFileStream(std::istream& stream) {
	seal();
	fileStream = &stream;
}

void YamlCGIConfFile::setFileName(const string& _fileName) {
	seal();
	fileName = _fileName;
	fileBaseDir = dirname(fileName);
}

const string YamlCGIConfFile::getFileName() const {
	return fileName;
}

const smap defaultEmptySMap;

YamlCGIConfFile::YamlCGIConfFile(const string& _fileName,const smap& map):
		isSealed(false),
		isYamlFileRead(false),
		fileName(_fileName),
		fileStream(NULL),
		templateParams(map)
		{}

