#include "json5cpp.h"
#include <iostream>

int main() {
	Json::Value val;
	Json5::parse(std::cin, val);
	Json5::serialize(std::cout, val);
}
