#include "json5cpp.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <stdlib.h>
#include <nlohmann/json.hpp>

template<typename T>
__attribute__((noinline))
static void doNotOptimize(T &v) {}

__attribute__((noinline))
static void parseJson5Cpp(std::istream &is) {
	Json::Value v;
	Json5::parse(is, v);
	doNotOptimize(v);
}

__attribute__((noinline))
static void parseJsonCpp(std::istream &is) {
	Json::Value v;
	is >> v;
	doNotOptimize(v);
}

__attribute__((noinline))
static void parseNlohmann(std::istream &is) {
	nlohmann::json v = nlohmann::json::parse(is);
	doNotOptimize(v);
}

static double benchOne(std::stringstream &is, void (*parse)(std::istream &is)) {
	for (int i = 0; i < 2; ++i) {
		is.clear();
		is.seekg(0);
		parse(is);
	}

	std::chrono::duration<double> delta;
	auto start = std::chrono::steady_clock::now();
	int numRuns = 0;
	while (true) {
		numRuns += 1;
		is.clear();
		is.seekg(0);
		parse(is);

		delta = std::chrono::steady_clock::now() - start;
		if (delta.count() > 0.2 && numRuns >= 5) {
			break;
		}
	}

	return delta.count() / numRuns;
}

static std::string timeToString(double secs) {
	if (secs > 10) {
		return std::to_string(int(secs)) + "s";
	} else if (secs > 0.01) {
		return std::to_string(int(secs * 1000)) + "ms";
	} else if (secs > 0.00001) {
		return std::to_string(int(secs * 1000000)) + "μs";
	} else {
		return std::to_string(int(secs * 1000000000)) + "ns";
	}
}

static void benchAll(std::string name, std::string &str) {
	std::stringstream is{std::move(str)};
	str.clear();

	std::cout << "Benchmark '" << name << "':\n";
	std::cout << "Json5Cpp: " << timeToString(benchOne(is, parseJson5Cpp)) << '\n';
	std::cout << "JsonCpp:  " << timeToString(benchOne(is, parseJsonCpp)) << '\n';
	std::cout << "Nlohmann: " << timeToString(benchOne(is, parseNlohmann)) << '\n';
	std::cout << '\n';
}

int main() {
	std::string json = "[]";
	benchAll("Tiny", json);

	json = '[';
	for (int i = 0; i < 50; ++i) {
		json += '[';
		for (int j = 0; j < 50; ++j) {
			json += '[';
			for (int k = 0; k < 50; ++k) {
				json += "[\"Hello\", \"World\"]";
				if (k != 49) json += ',';
			}
			json += ']';
			if (j != 49) json += ',';
		}
		json += ']';
		if (i != 49) json += ',';
	}
	json += ']';
	benchAll("Big Nested String Array", json);

	srand(0);
	json = "{\n";
	for (int i = 0; i < 10000; ++i) {
		json += "    \"";
		int l = rand() % 1000 + 2;
		for (int i = 0; i < l; ++i) {
			json += 'A' + (rand() % 26);
		}
		json += "\": " + std::to_string(rand());
		if (i != 9999) json += ',';
		json += '\n';
	}
	json += "}";
	benchAll("Big Object Of Numbers", json);

	json = R"({"oid":"6e1ef259b54c3639440c970bb0c438e0064bb4f4","url":"/mortie/json5cpp/commit/6e1ef259b54c3639440c970bb0c438e0064bb4f4","date":"2023-07-21T21:58:51.000+02:00","shortMessageHtmlLink":"<a data-pjax=\"true\" title=\"improve and flesh out tests\" class=\"Link--secondary\" href=\"/mortie/json5cpp/commit/6e1ef259b54c3639440c970bb0c438e0064bb4f4\">improve and flesh out tests</a>","bodyMessageHtml":"","author":{"displayName":"Martin Dørum","login":"mortie","path":"/mortie","avatarUrl":"https://avatars.githubusercontent.com/u/3728194?s=40&v=4"},"status":null,"isSpoofed":false})";
	benchAll("GitHub REST Response", json);
}
