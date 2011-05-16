/*
 * runCGI.cpp
 *
 *  Created on: Apr 4, 2011
 *      Author: elazar
 */

#include "runCGI.h"
#include "Logger.h"
#include "paths.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "urlencode.h"
#include <stdlib.h>
#include <libgen.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace boost;


RunCGI::RunCGI() {
  // TODO Auto-generated constructor stub
}

void RunCGI::run(const string& _method) {
	setMethod(_method);
	run();
}

void RunCGI::setServerEnv() {
  Logger::info("Setting basic CGI environment variables");
  // maybe I need to put it in an inlined YAML file, but meh.
  setenv("SERVER_SOFTWARE","RunCGI runner",1);
  setenv("SERVER_NAME","localhost",1);
  setenv("GATEWAY_INTERFACE","CGI/1.1",1);

  setenv("SERVER_PROTOCOL","HTTP/1.1",1);
  setenv("SERVER_PORT","80",1);
  setenv("REQUEST_METHOD",method.c_str(),1);
  //setenv("PATH_INFO","",1); default no path info
  //setenv("PATH_TRANSLATED","",1); since no pathinfo, no translation
  setenv("SCRIPT_NAME",executable_basename.c_str(),1);
  // setenv("REMOTE_HOST","",1); not mandatory
  setenv("REMOTE_ADDR","127.0.0.1",1); // running it on loopback device
  // setenv("AUTH_TYPE","",1); not mandatory
  // setenv("REMOTE_USER","",1); ditto, related to AUTH_TYPE
  // setenv("REMOTE_IDENT","",1); not mandatory

  Logger::info("Setting environment variables from configuration file ("+boost::lexical_cast<string>(env.size())+" variables)");
  for (map<string,string>::const_iterator it(env.begin());
		  it != env.end();
		  ++it) {
	  Logger::info("var "+it->first+" -> "+it->second);
	  setenv(it->first.c_str(),it->second.c_str(),1);
  }
}

void RunCGI::run() {
	if (method == "GET") runGet();
	else if (method == "POST") runPost();
	else if (method.empty()) {
		Logger::warn("No method set, defaults to GET");
		method = "GET";
		runGet();
	}
	else throw runtime_error("method unsupported: '"+method+"'");
}

static inline void writeToFile(const string& filename,const string& data) {
	ofstream f(filename.c_str());
	f << data;
}

struct String2cstrdup {
	char* operator() (const string &s) const {return strdup(s.c_str());}
};

/**
 * User of this function should expect this function never to return
 */
void RunCGI::executeProcess()
{
	Logger::info("changing directory to "+working_directory);
	chdir(working_directory.c_str());
    vector<char*> _argv;
    // I'm leaking some memory here, but it doesn't matter, since we'll never return here anyhow.
    _argv.push_back(strdup(executable.c_str()));
    transform(argv.begin(), argv.end(), back_inserter(_argv), String2cstrdup());
    _argv.push_back(0);
    Logger::info("executing '"+executable+"'");
    for (unsigned int i=0;i<argv.size();i++) Logger::info("- Arg"+boost::lexical_cast<string>(i)+": "+argv[i]);
    if(execv(executable.c_str(), &(_argv.front())) == -1) {
        perror(("Can't exeute: " + executable).c_str());
    }
}

void RunCGI::runGet() {
  Logger::info("executing CGI in GET mode");
  clearenv();
  setServerEnv();
  Logger::info("setting QUERY_STRING="+queryStringForGet());
  setenv("QUERY_STRING",queryStringForGet().c_str(),1);
  executeProcess();
}

static string gen_random(const int len) {
	using namespace std;
	string s(len,'*');
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ_";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
	return s;
}


void RunCGI::runPost() {
  Logger::info("running CGI in POST mode");
  clearenv();
  setServerEnv();
  setenv("QUERY_STRING","",1);
  // This is really bad, since I'm leaving open files in the wild
  // what I should do is fork another process that would feed the
  // data to his father, aka, me. yum yum.
  // I don't want to fork, since I want to easily enable debugging
  // of the CGI script.
  // I also should use freopen instead of dup2, to be portable on
  // windows.
  unsigned int now = time(NULL);
  unsigned int pid = static_cast<int>(getpid());
  const string boundary = "-----runCGIBoundary"+gen_random(10);
  srand((pid << 24) | (now && 0xFFFFFF));
  ostringstream tmpfile_stream;
  tmpfile_stream << "runCGI" << ".pid" << pid << ".time" << std::hex << now << ".tmp";
  const string tmpfile = tmpfile_stream.str();
  if (uploaded_files.size() == 0) {
    setenv("CONTENT_TYPE","application/x-www-form-urlencoded",1);
    const string qstring = queryStringForGet();
    Logger::info("writing POST data to "+tmpfile);
    writeToFile(tmpfile,qstring);
    setenv("CONTENT_LENGTH",lexical_cast<string>(qstring.size()).c_str(),1);
  } else {
	const string contentType = "multipart/form-data; boundary="+boundary;
	setenv("CONTENT_TYPE",contentType.c_str(),1);
	throw runtime_error("File upload Not implemented yet");
  }

  Logger::info("Redirecting file "+tmpfile+" to stdin");
  int fid = open(tmpfile.c_str(),O_RDONLY);
  unlink(tmpfile.c_str());
  close(0);
  dup2(fid,0);

  executeProcess();
}

bool FileExists(const string& filename) {
  struct stat info;

  return stat(filename.c_str(),&info) == 0;
}

struct CharStar {
	CharStar(char *_p):p(_p){}
	~CharStar() {free(p);}
	char *p;
};

RunCGI& RunCGI::setExecutable(const string& path) {
  executable = path;
  CharStar tmp = strdup(executable.c_str());
  executable_basename = basename(tmp.p);
  executable_path = dirname(tmp.p);
  if ((executable_path.empty() || executable_path == ".") && getenv("PATH") != NULL) {
	  // search for it in path
	  Logger::info("Looking for "+executable_basename+" in PATH");
	  vector<string> paths;
	  string pathenv = getenv("PATH");
	  boost::algorithm::split(paths,pathenv,boost::is_any_of(pathenvsep));
	  int i=0;
	  for (;i<paths.size();i++) {
		  if (paths[i][paths[i].size()-1] != pathsep[0]) paths[i] += pathsep;
		  if (FileExists(paths[i]+pathsep+executable_basename)) {
			  executable_path = paths[i];
			  executable = executable_path+executable_basename;
			  break;
		  }
	  }
	  if (i == paths.size()) {
		  Logger::info(executable_basename+" not found in PATH, delegating to CWD");
		  executable_path = "."+pathsep;
		  executable = executable_path+executable_basename;
	  }
  }
  return *this;
}
string RunCGI::getExecutable() {return executable;}

RunCGI& RunCGI::addArgv(const string& arg) {
  argv.push_back(arg);
  return *this;
}

RunCGI& RunCGI::resetArgv() {
  argv.erase(argv.begin(),argv.end());
  return *this;
}

string RunCGI::getMethod() {return method;}
RunCGI& RunCGI::setMethod(const string& _method) {
	method.resize(_method.size(),'@'); // setting the string to NULL is a bad idea, it looks weird when printed
	// I should probably use a function object as it might be inlined
	// but the terseness of the current line captured my heart.
	std::transform(_method.begin(), _method.end(),method.begin(), ::toupper);
	return *this;
}

void RunCGI::setQueryString(const map<string,string>& m) {
	querystring = m;
}

void RunCGI::addQueryString(const map<string,string>& m) {
	querystring.insert(m.begin(),m.end());
}

string RunCGI::queryStringForGet() const {
  std::ostringstream os;

  // querystring.map(lambda x,y: x+"="+y).join("&")
  int proc_items(querystring.size());
  for(map<string,string>::const_iterator it(querystring.begin());
      it != querystring.end();
      ++it) {
    os << form_urlencode(it->first) << "=" <<
        form_urlencode(it->second);
    proc_items--;
    if (proc_items > 0) os << "&";
  }
  return os.str();
}

void RunCGI::addEnv(const std::map<std::string,std::string>& newenv) {
	env.insert(newenv.begin(),newenv.end());
}

void RunCGI::addEnvIfUnexist(const std::map<std::string,std::string>& newenv) {
	for(map<string,string>::const_iterator it(newenv.begin());
			it != newenv.end();
			++it) {
		if (env.find(it->first) != env.end()) env[it->first] = it->second;
	}
}
