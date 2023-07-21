#include "json5cpp.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

int main() {
	int numTests = 0;
	int numSuccesses = 0;

	for (auto &entry: std::filesystem::directory_iterator{"JSONTestSuite/test_parsing"}) {
		auto &path = entry.path();
		auto filename = path.filename();
		if (filename.c_str()[0] == 'y') {
			numTests += 1;
			std::ifstream is{path};
			Json::Value jsonValue;
			is >> jsonValue;

			is = std::ifstream{path};
			Json::Value json5Value;
			if (!Json5::parse(is, json5Value)) {
				std::cout << "Parse error: " << path << '\n';
				continue;
			}

			if (jsonValue != json5Value) {
	  			std::cout << "Parse difference: " << path << ":\nJSON: "
					<< jsonValue << "\nJSON5: " << json5Value << "\n\n";
				continue;
			}

			numSuccesses += 1;
		}
	}

	std::cout << numSuccesses << '/' << numTests << " tests succeeded.";
	if (numTests == numSuccesses) {
		return 0;
	} else {
		return 1;
	}
}
