# Json5Cpp

Json5Cpp is a small, mostly correct header-only library to parse [JSON5](https://json5.org/),
built on top of [JsonCpp](https://github.com/open-source-parsers/jsoncpp).
Like JsonCpp, it requires C++11.

The API is simple: just one `bool Json5::parse(std::istream &, Json::Value &)` function.
It returns `true` on success, `false` on error.

## Examples

This repo contains a `json5` command, which reads JOSN5 and outputs JSON:
[cmd/json5.cc](./cmd/json5.cc), and a `test` command, which checks Json5Cpp against
the [JSONTestSuite](https://github.com/nst/JSONTestSuite): [cmd/test.cc](./cmd/test.cc).

## JSON5 Incompatibilities

Json5Cpp isn't 100% compatible with the JSON5 spec, and never will be.
However, more or less all actual JSON5 documents will work.

The JSON5 spec treats any character in the Space Separator Unicode category as whitespace.
That means that correctly parsing a valid JSON5 document requires a Unicode database,
which Json5Cpp will never include.
Instead, Json5Cpp treats all the normal ASCII whitespace characters as whitespace.

The JSON5 spec also treats the non-breaking space and the byte-order mark as whitespace,
which Json5Cpp doesn't at the moment because that would require parsing UTF-8.
This is something I might fix in the future, since writing a UTF-8 parser
is way less problematic than adding a dependency on a Unicode database.

There are also some invalid JSON5 documents which Json5Cpp will accept,
also due to the lack of a Unicode database.
For example, the JSON5 spec restricts which Unicode character classes which an identifier can
contain, but Json5Cpp will treat all non-ASCII characters as legal in identifiers.
