/*
 * Logger.h
 *
 *  Created on: Apr 7, 2011
 *      Author: Elazar
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>

// static classes are evil in principle, but in this case
// I think it's OK.
struct Logger {
	static bool isOn;
	static void setVerbose(bool verbosity);
	static void info(const std::string& msg);
	static void warn(const std::string& msg);
private:
	Logger();
	Logger(const Logger&);
};

#endif /* LOGGER_H_ */
