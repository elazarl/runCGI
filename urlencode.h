/*
 * urlencode.h
 *
 *  Created on: Apr 11, 2011
 *      Author: Elazar
 */

#ifndef URLENCODE_H_
#define URLENCODE_H_

#include <string>

std::string form_urlencode(const std::string& src);

std::string form_urldecode(const std::string& src);

#endif /* URLENCODE_H_ */
