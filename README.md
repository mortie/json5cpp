# Json5Cpp

Json5Cpp is a small, [mostly correct](#json5-incompatibilities)
header-only library to parse [JSON5](https://json5.org/),
built on top of [JsonCpp](https://github.com/open-source-parsers/jsoncpp).
Like JsonCpp, it requires C++11.

The API is simple, just one function with this signature:

```c++
bool Json5::parse(
        std::istream &, Json::Value &,
        std::string *err = nullptr, int maxDepth = 100);
```

It returns `true` on success, `false` on error.
If an error occurs, the string pointed to by `err` will be filled with an error message,
if it's not null.
The `maxDepth` argument sets the recursion limit.

## Examples

Here's a minimal program which reads JSON5 from stdin and writes JSON to stdout:

```c++
#include <json5cpp/json5cpp.h>
#include <iostream>

int main() {
    Json::Value value;
    std::string err;
    if (Json5::parse(std::cin, value, &err)) {
        std::cout << value << '\n';
    } else {
        std::cerr << "Invalid JSON5: " << err << '\n';
    }
}
```

For a slightly bigger example, this repo contains a `json5-to-json` example program
which reads JOSN5 files and outputs JSON:
[examples/json5-to-json.cc](./examples/json5-to-json.cc).

Here's an example JSON5 document which showcases some of its features
(stolen from [json5.org](https://json5.org/)):

```json5
{
  // comments
  unquoted: 'and you can quote me on that',
  singleQuotes: 'I can use "double quotes" here',
  lineBreaks: "Look, Mom! \
No \\n's!",
  hexadecimal: 0xdecaf,
  leadingDecimalPoint: .8675309, andTrailing: 8675309.,
  positiveSign: +1,
  trailingComma: 'in objects', andIn: ['arrays',],
  "backwardsCompatible": "with JSON",
}
```

## Tests

Run tests with: `make check`. This depends on git, npm and node.

The test suite consists of the
[JSON Parsing Test Suite](https://github.com/nst/JSONTestSuite) and the
[JSON5 test suite](https://github.com/json5/json5-tests).
The test script validates Json5Cpp's output against JavaScript's `JSON.parse`
for all JSON files in the test set, and against the JSON5 reference implementation
for all JSON5 files in the set.

There are currently 175 test JSON and JSON5 files.
All tests pass.

## Benchmarks

Run benchmarks with: `make bench`.

The benchmarking suite compares Json5Cpp against
[JsonCpp](https://github.com/open-source-parsers/jsoncpp) and
[nlohmann/json](https://github.com/nlohmann/json).
In my testing, Json5Cpp turns out a bit faster than JsonCpp across the board,
and trades blows with nlohmann/json.

Here's a typical run on my AMD R9 5950X:

```
Benchmark 'Tiny':
Json5Cpp: 88ns
JsonCpp:  1924ns
Nlohmann: 124ns

Benchmark 'Big Nested String Array':
Json5Cpp: 55ms
JsonCpp:  60ms
Nlohmann: 37ms

Benchmark 'Big Object Of Numbers':
Json5Cpp: 15ms
JsonCpp:  22ms
Nlohmann: 31ms

Benchmark 'GitHub REST Response':
Json5Cpp: 2906ns
JsonCpp:  5811ns
Nlohmann: 5007ns
```

And a typical run on my Apple M1 Pro:

```
Benchmark 'Tiny':
Json5Cpp: 82ns
JsonCpp:  1599ns
Nlohmann: 139ns

Benchmark 'Big Nested String Array':
Json5Cpp: 43ms
JsonCpp:  53ms
Nlohmann: 30ms

Benchmark 'Big Object Of Numbers':
Json5Cpp: 21ms
JsonCpp:  38ms
Nlohmann: 22ms

Benchmark 'GitHub REST Response':
Json5Cpp: 3268ns
JsonCpp:  6960ns
Nlohmann: 3920ns
```

Feel free to contribute more benchmarks.

## Fuzzing

Install [AFLplusplus](https://aflplus.plus/) (`sudo apt install afl++` on Ubuntu),
then run `make fuzz` to start fuzzing.

By default, 20 fuzzer processes will start.
You can change this in the [fuzz.sh](./fuzz.sh) script.

I've ran the fuzzer against `Json5::parse` for quite a while on beefy machines,
and no issues have been found.

## JSON5 Incompatibilities

Json5Cpp isn't 100% compatible with the JSON5 spec, and never will be.
However, more or less all actual JSON5 documents will work
(and it passes the JSON5 test suite).

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
