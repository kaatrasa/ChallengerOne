#pragma once

#include <cctype>
#include <string>
#include <algorithm>
#include <vector>

namespace stringoperators {
	void ltrim(std::string &s);
	void rtrim(std::string &s);
	void trim(std::string &s);
	std::vector<std::string> split(std::string s, char delim);
}