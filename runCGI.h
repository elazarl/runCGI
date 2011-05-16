/*
 * runCGI.h
 *
 *  Created on: Apr 4, 2011
 *      Author: elazar
 */

#ifndef RUNCGI_H_
#define RUNCGI_H_
#include <string>
#include <vector>
#include <map>

class RunCGI {
public:
  RunCGI();
  // does not return, cgi is running
  void run();
  void run(const std::string& method);
  RunCGI& setMethod(const std::string& method);
  std::string getMethod();
  void setQueryString(const std::map<std::string,std::string>&);
  void addQueryString(const std::map<std::string,std::string>&);
  void addEnv(const std::map<std::string,std::string>&);
  void addEnvIfUnexist(const std::map<std::string,std::string>& newenv);
  RunCGI& setExecutable(const std::string& path);
  RunCGI& addArgv(const std::string& arg);
  RunCGI& resetArgv();
  std::string getExecutable();
  std::string getWorkingDirectory() {return working_directory;}
  void setWorkingDirectory(std::string _wd) {working_directory = _wd;}
private:
  void runGet();
  void runPost();
  std::string working_directory;
  std::string method;
  std::string queryStringForGet() const;
  std::string executable;
  std::vector<std::string> argv;
  std::string executable_basename;
  std::string executable_path;
  std::vector<std::string> uploaded_files;
  void setServerEnv();
    void executeProcess();
  std::map<std::string,std::string> env;
  std::map<std::string,std::string> querystring;
};

#endif /* RUNCGI_H_ */
