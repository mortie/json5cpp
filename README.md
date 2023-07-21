# Json5Cpp

Json5Cpp is a small, mostly correct header-only library to parse [JSON5](https://json5.org/),
built on top of [JsonCpp](https://github.com/open-source-parsers/jsoncpp).

The API is simple: just one `bool Json5::parse(std::istream &, Json::Value &)` function.
It returns `true` on success, `false` on error.
