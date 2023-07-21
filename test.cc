#include "json5cpp.h"

#include <sstream>
#include <iostream>

int main() {
	std::stringstream ss{R"(
		{
			foo: "Hello World",
			// lol
			"bar mitsva": "This is \
			actually kinda cool",
			somearr: [NaN, "true", false, 100, 88.6, 4E-9],
			/* meh */
		}
	)"};

	std::cout << "JSON5 str: " << ss.str() << '\n';

	Json::Value v;
	if (!Json5::parse(ss, v)) {
		std::cout << "parse error\n";
		return 1;
	}

	std::cout << "JSON: " << v << '\n';

	return 0;
}
