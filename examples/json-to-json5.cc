#include "json5cpp.h"

#include <memory>
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
	Json5::SerializeConfig conf;

	std::unique_ptr<std::ifstream> ifile;
	std::istream *input = &std::cin;

	for (int i = 1; i < argc; ++i) {
		char *opt = argv[i];
		if (strcmp(opt, "--no-trailing-commas") == 0) {
			conf.trailingCommas = false;
		} else if (strcmp(opt, "--no-bare-keys") == 0) {
			conf.bareKeys = false;
		} else if (strcmp(opt, "--compact") == 0) {
			conf.indent = nullptr;
			conf.trailingCommas = false;
		} else if (strcmp(opt, "--no-indent") == 0) {
			conf.indent = nullptr;
		} else if (strcmp(opt, "--indent") == 0) {
			if (i >= argc - 1) {
				std::cerr << "'--indent' requires an argument\n";
				return 1;
			}

			conf.indent = argv[i + 1];
			i += 1;
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
	*input >> val;

	Json5::serialize(std::cout, val, conf);
	std::cout << '\n';
}
