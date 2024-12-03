#include "json5cpp.h"

#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
	Json5::ParseConfig conf;

	std::unique_ptr<std::ifstream> ifile;
	std::istream *input = &std::cin;

	for (int i = 1; i < argc; ++i) {
		char *opt = argv[i];
		if (strcmp(opt, "--newlines-as-commas") == 0) {
			conf.newlinesAsCommas = true;
		} else if (opt[0] == '-') {
			std::cerr << "Unknown option: '" << opt << "'\n";
			return 1;
		} else if (ifile) {
			std::cerr << "Too many file arguments\n";
			return 1;
		} else {
			ifile.reset(new std::ifstream(opt));
			if (ifile->bad()) {
				std::cerr << "Couldn't open " << opt << '\n';
				return 1;
			}
			input = ifile.get();
		}
	}

	Json::Value val;
	std::string err;
	if (!Json5::parse(*input, val, &err, conf)) {
		std::cerr << "Could not parse stdin: " << err << '\n';
		return 1;
	}

	std::cout << val << '\n';
}
