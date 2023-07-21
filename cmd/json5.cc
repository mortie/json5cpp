#include "json5cpp.h"

#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
	for (int i = 1; i < argc; ++i) {
		Json::Value val;
		std::ifstream is{argv[i]};
		if (!is) {
			std::cerr << "Could not open " << argv[i] << '\n';
			continue;
		}

		if (!Json5::parse(is, val)) {
			std::cerr << "Could not parse " << argv[i] << '\n';
			continue;
		}

		std::cout << val << '\n';
	}
}
