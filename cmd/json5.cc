#include "json5cpp.h"

#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
	if (argc <= 1) {
		Json::Value val;
		if (!Json5::parse(std::cin, val)) {
			std::cerr << "Could not parse stdin\n";
			return 1;
		}

		std::cout << val << '\n';
		return 0;
	}

	for (int i = 1; i < argc; ++i) {
		Json::Value val;
		std::ifstream is{argv[i]};
		if (!is) {
			std::cerr << "Could not open " << argv[i] << '\n';
			return 1;
		}

		if (!Json5::parse(is, val)) {
			std::cerr << "Could not parse " << argv[i] << '\n';
			return 1;
		}

		std::cout << val << '\n';
	}

	return 0;
}
