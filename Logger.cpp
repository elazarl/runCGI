/*
 * Logger.cpp
 *
 *  Created on: Apr 7, 2011
 *      Author: Elazar
 */

#include "Logger.h"
#include <iostream>

using namespace std;

bool Logger::isOn = false;

void Logger::setVerbose(bool verbosity) {isOn = verbosity;}

void Logger::info(const string& msg) {
	if (isOn) cout << "INFO: " << msg << endl;
}

void Logger::warn(const string& msg) {
	cerr << "WARN: " << msg << endl;
}
