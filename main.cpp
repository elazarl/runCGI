//============================================================================
// Name        : runCGI.cpp
// Author      : Elazar Leibovich
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <string>
#include <iostream>
#include <map>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "runCGI.h"
#include "Logger.h"
#include "YamlCGIConfFile.h"

#define NULL_FILE "/dev/null"

using namespace std;

void setExecutableToCGI(string & executableFile, RunCGI* cgi, vector<string> & args)
{
    if(!executableFile.empty())
        cgi->setExecutable(executableFile);

    for(unsigned int i(0);i < args.size();++i)
        cgi->addArgv(args[i]);

}

void addTemplateParam(smap* m,const string& keyval) {
	string::size_type eq_ix = keyval.find('=');
	if (eq_ix == string::npos) {
		Logger::info("Empty parameter: "+keyval);
		(*m)[keyval] = "";
		return;
	}
	string key = keyval.substr(0,eq_ix);
	if (key.empty()) {
		Logger::warn("Bad key-value template parameter given: "+keyval);
		return;
	}
	string val;
	if (eq_ix+1 >= keyval.size()) val = "";
	else val = keyval.substr(eq_ix+1,keyval.size()-(eq_ix+1));
	Logger::info("Parameter '"+key+"' mapped to '"+val+"'");
	(*m)[key] = val;
}

int main(int argc,char** argv) {
	RunCGI cgi;

	string executableFile;
	vector<string> args;
	vector<string> template_params_vector;
	bool verbose;
	vector<string> yamlFiles;

	po::positional_options_description p;
	p.add("input-file", -1);

	po::options_description desc("Allowed options");
	desc.add_options()
	            		("help", "produce help message")
	            		("exec,x", po::value<string>(&executableFile), "cgi file to execute (override YAML file _exec)")
	            		("args,a",po::value<vector<string> >(&args),"arguments to pass to file to execute (override _args)")
	            		("verbose,v",po::value<bool>(&verbose)->default_value(false)->zero_tokens(),"Verbose log messages")
	            		("params,p",po::value<vector<string> >(&template_params_vector),"given --params x=y, we will replace any "
	            		                                                "occurrence of <%=x%> in the YAML file with 'y'"
	            				                                        " multiple parameters can be separated by commas"
	            				                                        " for example --params x=a,y=b will replace"
	            				                                        " <%=x%> with a, and <%=y%> with b."
	            														" In case you want to include commas after the equal"
	            														" sign, prepend : to the whole --params value, and it will"
	            														" be treated literaly, so --params :x=y,z=w will replace x with"
	            				                                        " 'y,z=w'.")
	            		("input-file", po::value<vector<string> >(&yamlFiles), "alternative way to specify a yaml file")
	            		;

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << "usage: " << argv[0] << " [ options ] <yaml-files,...>" << endl;
		cout << "  Specifying more than one yaml file, will run them simultaneously." << endl;
		cout << "  yaml files can be alternatively specified by the --input-file option." << endl;
		cout << "Typical Usage: " << argv[0] << " file.yaml" << endl;
		cout << desc;
		return 1;
	}
	Logger::setVerbose(verbose);

	if (yamlFiles.empty()) {
		cerr << "No YAML file, or empty YAML file given" << endl;
		exit(-1);
	}

	smap template_params;
	if (template_params_vector.size() > 0) Logger::info("Applying template substitution to input YAML file");
	for (unsigned int i = 0; i < template_params_vector.size(); ++i) {
		if (template_params_vector[i].empty()) continue;
		if (template_params_vector[i][0] == ':') {
			addTemplateParam(&template_params,template_params_vector[i].substr(1));
			continue;
		}
		std::vector<std::string> strs;
		boost::split(strs, template_params_vector[i], boost::is_any_of(","));
		for (unsigned int j=0;j<strs.size();j++) addTemplateParam(&template_params,strs[j]);
	}

	vector<pid_t> children_pids;
	YamlCGIConfFile yamlconf;
	yamlconf.setTemplateParams(template_params);
	if (yamlFiles.size() == 1) {
		if (yamlFiles[0] == "-") {
			Logger::info("Received file name '-', reading input from stdandard input");
			stringstream yamlstream;
			yamlconf.setFileStream(cin);
		} else {
			Logger::info("Processing YAML file "+yamlFiles[0]);
			yamlconf.setFileName(yamlFiles[0]);
		}
		Logger::info("Applying command line options to CGI");
		yamlconf.updateCGI(&cgi);
		setExecutableToCGI(executableFile, &cgi, args);
		Logger::info("executing a single cgi");
		cgi.run();
	}
	else for (unsigned int i = 0; i < yamlFiles.size(); ++i) {
		YamlCGIConfFile yamlconf;
		RunCGI cgi_copy(cgi);
		Logger::info("Processing "+boost::lexical_cast<string>(i)+"th YAML file "+yamlFiles[i]);
		yamlconf.setTemplateParams(template_params);
		yamlconf.setFileName(yamlFiles[i]);
		yamlconf.updateCGI(&cgi_copy);
		Logger::info("Applying command line options to yaml file");
		setExecutableToCGI(executableFile, &cgi_copy, args);

		Logger::info("Forking a new process to execute CGI silently");
		pid_t pid = fork();
		if (pid < 0) cerr << "can't execute " << cgi_copy.getExecutable() << " from " << yamlFiles[i] << endl;
		if (pid == 0) {
			int nullfile = open(NULL_FILE,O_WRONLY);
			dup2(nullfile,STDOUT_FILENO);
			cgi_copy.run();
		}
		else children_pids.push_back(pid);
	}

	for (unsigned int i = 0; i < children_pids.size(); ++i) {
		int stat;
		Logger::info("waiting for child "+boost::lexical_cast<string>(children_pids[i]));
		waitpid(children_pids[i],&stat,0); // we don't want zombies eating our brains
	}

	return EXIT_SUCCESS;
}
