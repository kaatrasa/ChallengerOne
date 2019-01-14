#include <cctype>
#include <string>
#include <algorithm>
#include <functional>
#include <sstream>
#include <vector>

#include "stringoperators.h"

namespace stringoperators {

	void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(),
				std::not1(std::ptr_fun<int, int>(std::isspace))));
	}

	void rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(),
				std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	}

	void trim(std::string &s) {
		ltrim(s);
		rtrim(s);
	}

	std::vector<std::string> split(std::string s, char delim) {
	  std::stringstream ss(s);
	  std::string item;
	  std::vector<std::string> elems;

	  while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	  }
	  return elems;
	}
}

