/* Json5Cpp, a JSON5 parser -- v1.1.1 -- https://github.com/mortie/json5cpp */

/*
 * Copyright (c) 2023 Martin Dørum
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef JSON5CPP_H
#define JSON5CPP_H

#include <json/json.h>
#include <string>

#ifndef JSON5CPP_FWD_ONLY
#include <istream>
#include <limits>
#include <memory>
#include <string.h>
#endif

namespace Json5 {

struct ParseConfig {
	// Whether or not to accept newlines instead of commas
	// between object/array elements.
	// Note: Setting this to 'true' enables invalid JSON5.
	bool newlinesAsCommas = false;

	// The maximum parse depth, to avoid unbounded recursion.
	int maxDepth = 100;
};

struct SerializeConfig {
	// Whether or not to add a trailing ',' after the last element
	// of an object/array.
	bool trailingCommas = true;

	// Whether or not to omit quotes in keys when possible.
	bool bareKeys = true;

	// The string used for each level of nesting.
	// Set to 'nullptr' to avoid whitespace completely.
	const char *indent = "\t";
};

bool parse(
		std::istream &is, Json::Value &v,
		std::string *err = nullptr, ParseConfig conf = {});

void serialize(
		std::ostream &os, const Json::Value &v,
		SerializeConfig conf = {}, int depth = 0);

#ifndef JSON5CPP_FWD_ONLY

namespace detail {

struct Location {
	int line = 1;
	int ch = 1;
};

class Reader {
public:
	Reader(std::istream &is, ParseConfig conf = {}): is_(is), conf_(conf) {
		fill();
	}

	int peek(int n = 0) {
		if (index_ + n >= size_) {
			fill();
		}

		if (index_ + n >= size_) {
			return EOF;
		}

		return buffer_[index_ + n];
	}

	int get() {
		int ch = peek();
		index_ += 1;
		loc_.ch += 1;
		if (ch == '\n') {
			loc_.ch = 1;
			loc_.line += 1;
		}
		return ch;
	}

	Location loc() {
		return loc_;
	}

	const ParseConfig &conf() {
		return conf_;
	}

	Json::CharReader &jsonCharReader() {
		if (!jsonCharReader_) {
			Json::CharReaderBuilder b;
			jsonCharReader_.reset(b.newCharReader());
		}

		return *jsonCharReader_;
	}

private:
	void fill() {
		if (index_ > size_) {
			return;
		}

		memmove(buffer_, buffer_ + index_, size_ - index_);
		size_ -= index_;
		index_ = 0;
		size_ += is_.read((char *)buffer_ + size_, sizeof(buffer_) - size_).gcount();
	}

	std::istream &is_;
	unsigned char buffer_[128];
	size_t index_ = 0;
	size_t size_ = 0;
	Location loc_;
	ParseConfig conf_;

	std::unique_ptr<Json::CharReader> jsonCharReader_;
};

bool parseValue(Reader &r, Json::Value &v, std::string *err, int depth);

void serializeValue(
		std::ostream &os, const Json::Value &v,
		const SerializeConfig &conf, int depth);
void serializeStringLiteral(std::ostream &os, const char *ident);

inline void error(Location loc, std::string *err, const char *what) {
	if (!err) {
		return;
	}

	*err = std::to_string(loc.line);
	*err += ':';
	*err += std::to_string(loc.ch);
	*err += ": ";
	*err += what;
}

inline bool isUtf8LineTerminator3B(int a, int b, int c) {
	// U+2028 Line separator and U+2029 Paragraph separator
	return a == 0xe2 && b == 0x80 && (c == 0xa8 || c == 0xa9);
}

inline bool isUtf8Whitespace1B(int ch) {
	return ch == '\t' || ch == '\n' || ch == '\v' ||
		ch == '\f' || ch == '\r' || ch == ' ';
}

inline bool isUtf8Whitespace2B(int a, int b) {
	return a == 0xc2 && b == 0xa0; // Non-breaking space
}

inline bool isUtf8Whitespace3B(int a, int b, int c) {
	if (a < 128) {
		return false;
	}

	return isUtf8LineTerminator3B(a, b, c) ||
		(a == 0xef && b == 0xbb && c == 0xbf) || // BOM
		(a == 0xe1 && b == 0x9a && c == 0x80) || // Ogham Space Mark
		(a == 0xe2 && b == 0x80 && (c >= 0x80 && c <= 0x8a)) || // En Quad - Hair Space
		(a == 0xe2 && b == 0x80 && (c == 0xa8 || c == 0xa9 || c == 0xaf)) || // LS, PS, Narrow NBSP
		(a == 0xe2 && b == 0x81 && c == 0x9f) || // Medium Mathematical Space (MMSP)
		(a == 0xe3 && b == 0x80 && c == 0x80); // Ideographic Space
}

// Skip past https://262.ecma-international.org/5.1/#sec-7.3 LineTerminator
inline void skipPastLineTerminator(Reader &r) {
	while (true) {
		int ch = r.get();
		if (ch == EOF || ch == '\n' || ch == '\r') {
			return;
		} else if (isUtf8LineTerminator3B(ch, r.peek(0), r.peek(1))) {
			// U+2028 Line separator and U+2029 Paragraph separator
			r.get();
			r.get();
			return;
		}
	}
}

// Skip https://spec.json5.org/#white-space White Space
// and https://spec.json5.org/#comments Comments
// Returns true if a newline was skipped.
inline bool skipWhitespace(Reader &r) {
	bool newline = false;
	// Ignore Unicode Space Separator characters,
	// Json5Cpp doesn't have a Unicode database
	while (true) {
		int ch = r.peek();
		if (ch == EOF) {
			return newline;
		} else if (ch == '\n') {
			newline = true;
			r.get();
		} else if (isUtf8Whitespace1B(ch)) {
			r.get();
		} else if (isUtf8Whitespace2B(ch, r.peek(1))) {
			r.get();
			r.get();
		} else if (isUtf8Whitespace3B(ch, r.peek(1), r.peek(2))) {
			r.get();
			r.get();
			r.get();
		} else if (ch == '/' && r.peek(1) == '/') {
			r.get();
			r.get();
			skipPastLineTerminator(r);
		} else if (ch == '/' && r.peek(1) == '*') {
			r.get();
			r.get();
			int prev = r.get();
			ch = r.get();
			while (!(prev == '*' && ch == '/') && ch != EOF) {
				prev = ch;
				ch = r.get();
			}
		} else {
			return newline;
		}
	}
}

// https://262.ecma-international.org/5.1/#sec-7.6 IdentifierStart
inline bool isIdentStartChar1B(int ch) {
	return
		(ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') ||
		ch == '$' || ch == '_' || ch == '\\';
}

// https://262.ecma-international.org/5.1/#sec-7.6 IdentifierPart
inline bool isIdentPartChar(int a, int b, int c) {
	// Json5Cpp doesn't have a Unicode database, so let's just assume non-whitespace
	// non-ASCII characters are valid identifier characters
	return isIdentStartChar1B(a) ||
		(a >= 128 && !isUtf8Whitespace2B(a, b) && !isUtf8Whitespace3B(a, b, c)) ||
		(a >= '0' && a <= '9');
}

// https://262.ecma-international.org/5.1/#sec-7.6 IdentifierName
inline bool readIdentifier(Reader &r, std::string &str, std::string *err) {
	// If we're here, we've already skipped whitespace,
	// so we can assume that characters >=128 are valid identifier characters
	int ch = r.peek();
	if (ch < 128 && !isIdentStartChar1B(ch)) {
		error(r.loc(), err, "Invalid start character in identifier");
		return false;
	}

	str += r.get();
	while (isIdentPartChar(r.peek(0), r.peek(1), r.peek(2))) {
		str += r.get();
	}

	return true;
}

inline void serializeIdentifier(
		std::ostream &os, const char *ident,
		const SerializeConfig &conf) {
	if (!conf.bareKeys || *ident == '\0') {
		return serializeStringLiteral(os, ident);
	}

	// Json5Cpp doesn't have a unicode database,
	// so let's conservatively only emit bare identifiers if
	// all the characters are alphanumeric ASCII
	const char *it = ident;
	for (char ch; (ch = *it); ++it) {
		// Legal characters anywhere in the identifier
		if (
				(ch >= 'a' && ch <= 'z') ||
				(ch >= 'A' && ch <= 'Z') ||
				ch == '$' || ch == '_') {
			continue;
		}

		// Legal characters after the first character
		if (it != ident && ch >= '0' && ch <= '9') {
			continue;
		}

		return serializeStringLiteral(os, ident);
	}

	// All good, we can just emit the string directly
	os << ident;
}

inline int hexChar(int ch) {
	if (ch == EOF) {
		return EOF;
	} else if (ch >= '0' && ch <= '9') {
		return ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	} else if (ch >= 'A' && ch <= 'Z') {
		return ch - 'A' + 10;
	} else {
		return EOF;
	}
}

// Write a code point (up to 16 bit) as UTF-8
inline void writeUtf8(unsigned int num, std::string &str) {
	if (num >= 0x10000u) {
		str += 0xf0u | ((num & 0x1c0000u) >> 18u);
		str += 0x80u | ((num & 0x03f000u) >> 12u);
		str += 0x80u | ((num & 0x000fc0u) >> 6u);
		str += 0x80u | ((num & 0x00003fu) >> 0u);
	} else if (num >= 0x0800u) {
		str += 0xe0u | ((num & 0x00f000u) >> 12u);
		str += 0x80u | ((num & 0x000fc0u) >> 6u);
		str += 0x80u | ((num & 0x00003fu) >> 0u);
	} else if (num >= 0x0080u) {
		str += 0xc0u | ((num & 0x0007c0u) >> 6);
		str += 0x80u | ((num & 0x00003fu) >> 0);
	} else {
		str += num;
	}
}

inline bool read4Hex(Reader &r, unsigned int &u, std::string *err) {
	Location loc = r.loc();
	int a = hexChar(r.get());
	int b = hexChar(r.get());
	int c = hexChar(r.get());
	int d = hexChar(r.get());
	if (a == EOF || b == EOF || c == EOF || d == EOF) {
		error(loc, err, "Invalid hex sequence");
		return false;
	}

	u =
		((unsigned int)a << 12) |
		((unsigned int)b << 8) |
		((unsigned int)c << 4) |
		((unsigned int)d << 0);
	return true;
}

inline bool readUnicodeEscape(Reader &r, std::string &str, std::string *err) {
	// Assume we have already read the first '\' and 'u'

	unsigned int u1;
	if (!read4Hex(r, u1, err)) {
		return false;
	}

	Location loc = r.loc();
	if (u1 >= 0xd800u && u1 <= 0xdbffu) {
		// First character was a high surrogate, read the low surrogate
		if (r.peek() != '\\') {
			error(loc, err, "Expected trailing surrogate");
			return false;
		}
		r.get();

		if (r.peek() != 'u') {
			error(loc, err, "Expected trailing surrogate");
			return false;
		}
		r.get();

		unsigned int u2;
		if (!read4Hex(r, u2, err)) {
			return false;
		}

		if (!(u2 >= 0xdc00u && u2 <= 0xdfffu)) {
			// Don't pair the high surrogate with a non-low-surrogate
			error(loc, err, "Expected trailing surrogate");
			return false;
		}

		writeUtf8((u1 - 0xd800u) * 0x400u + u2 - 0xdc00u + 0x10000u, str);
		return true;
	} else if (u1 >= 0xdc00u && u1 <= 0xdfffu) {
		// Don't allow unpaired surrogates
		error(loc, err, "Invalid trailing surrogate");
		return false;
	} else {
		writeUtf8(u1, str);
		return true;
	}
}

// https://spec.json5.org/#strings JSON5String
inline bool readStringLiteral(Reader &r, std::string &str, std::string *err) {
	int startChar = r.get(); // '"' or "'"

	while (true) {
		int ch = r.get();
		if (ch == EOF) {
			error(r.loc(), err, "Unexpected EOF");
			return false;
		} else if (ch == startChar) {
			return true;
		} else if (ch == '\\') {
			ch = r.get();
			if (ch == EOF) {
				error(r.loc(), err, "Unexpected EOF");
				return false;
			} else if (ch == 'b') {
				str += '\b';
			} else if (ch == 'f') {
				str += '\f';
			} else if (ch == 'n') {
				str += '\n';
			} else if (ch == 'r') {
				str += '\r';
			} else if (ch == 't') {
				str += '\t';
			} else if (ch == 'v') {
				str += '\v';
			} else if (ch == '0') {
				// I don't get why "\0" wouldn't always just be a nul character,
				// but hey, https://262.ecma-international.org/5.1/#sec-7.8.4 EscapeSequence
				// character says: 0 [lookahead != DecimalDigit].
				// If lookahead is a DecimalDigit, we fall back to NonEscapeCharacter,
				// which means we just add the character to the string
				int next = r.peek();
				if (next >= '0' && next <= '9') {
					str += '0';
				} else {
					str += '\0';
				}
			} else if (ch == 'x') {
				Location loc = r.loc();
				int a = hexChar(r.get());
				int b = hexChar(r.get());
				if (a == EOF || b == EOF) {
					error(loc, err, "Invalid hex sequence");
					return false;
				}
				str += (a << 4) | b;
			} else if (ch == 'u') {
				readUnicodeEscape(r, str, err);
			} else if (ch == '\n' || ch == '\r') {
				// Ignore line separator and paragraph separator, because again,
				// I don't wanna deal with parsing UTF-8
				str += ch;
				if (ch == '\r' && r.peek() == '\n') {
					str += r.get();
				}
			} else {
				// Yeah, all other sequences match the NonEscapeCharacter case,
				// so you may escape any character.
				// That means "\u1000" is a unicode escape while "\U1000" is a liteal "U1000".
				str += ch;
			}
		} else if (ch == '\n' || ch == '\r') {
			// This is *not* ignoring the spec, line separator and paragraph separator
			// are explicitly allowed in JSON5 strings!
			error(r.loc(), err, "Unescaped line terminator in string literal");
			return false;
		} else {
			str += ch;
		}
	}
}

inline void serializeStringLiteral(std::ostream &os, const char *ident) {
	os << '"';
	for (char ch; (ch = *ident); ++ident) {
		if (ch == '"') {
			os << "\\\"";
		} else if (ch == '\t') {
			os << "\\t";
		} else if (ch == '\r') {
			os << "\\r";
		} else if (ch == '\n') {
			os << "\\n";
		} else {
			os << ch;
		}
	}
	os << '"';
}

// https://spec.json5.org/#numbers JSON5Number
inline bool parseNumber(Reader &r, Json::Value &v, std::string *err) {
	// Parsing floats accurately is hard.
	// Instead, let's create a JSON string, then use jsoncpp to parse it.
	Location loc = r.loc();
	std::string str;

	bool negative = false;
	int ch = r.peek();
	if (ch == '+') {
		r.get();
		ch = r.peek();
	} else if (ch == '-') {
		negative = true;
		str += '-';
		r.get();
		ch = r.peek();
	}

	if (ch == 'I' || ch == 'N') {
		str.clear();
		readIdentifier(r, str, err);
		if (str == "Infinity") {
			if (negative) {
				v = -std::numeric_limits<double>::infinity();
			} else {
				v = std::numeric_limits<double>::infinity();
			}
			return true;
		} else if (str == "NaN") {
			// I assume negative NaN is just a normal NaN?
			v = std::numeric_limits<double>::quiet_NaN();
			return true;
		} else {
			error(loc, err, "Invalid number");
			return false;
		}
	} else if (ch == '.') {
		// JSON doesn't support leading dots
		str += '0';
	} else if (ch == '0') {
		str += '0';
		r.get();
		ch = r.peek();
		if (ch == 'x' || ch == 'X') {
			r.get();
			Json::UInt64 number = 0;
			int digit = hexChar(r.peek());
			if (digit == EOF) {
				error(r.loc(), err, "Unexpected EOF");
				return false;
			}
			number += digit;
			r.get();

			while (true) {
				ch = r.peek();
				digit = hexChar(ch);
				if (digit == EOF) {
					break;
				}
				number <<= 4;
				number += digit;
				r.get();
			}

			if (negative) {
				v = -(Json::Int64)number;
			} else {
				v = number;
			}

			return true;
		} else if (!((ch >= '1' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E')) {
			v = 0;
			return true;
		}
	}

	// Optional integer part
	while (ch >= '0' && ch <= '9') {
		str += ch;
		r.get();
		ch = r.peek();
	}

	// Potentially trailing dot
	if (ch == '.') {
		str += '.';
		r.get();
		ch = r.peek();
		// JSON doesn't support trailing dots
		if (!(ch >= '0' && ch <= '9')) {
			str += '0';
		}
	}

	// Optional decimal part
	while (ch >= '0' && ch <= '9') {
		str += ch;
		r.get();
		ch = r.peek();
	}

	// Optional exponent part
	if (ch == 'e' || ch == 'E') {
		str += 'e';
		r.get();
		ch = r.peek();
		if (ch == '+' || ch == '-') {
			str += ch;
			r.get();
			ch = r.peek();
		}

		while (ch >= '0' && ch <= '9') {
			str += ch;
			r.get();
			ch = r.peek();
		}
	}

	if (!r.jsonCharReader().parse(
			str.c_str(), str.c_str() + str.size(), &v, nullptr)) {
		error(loc, err, "Invalid number");
		return false;
	}

	return true;
}

inline void serializeNewLine(
		std::ostream &os, const SerializeConfig &conf, int depth) {
	if (conf.indent == nullptr) {
		return;
	}

	os << '\n';

	while (depth--) {
		os << conf.indent;
	}
}

// https://spec.json5.org/#prod-JSON5Object JSON5Object
inline bool parseObject(Reader &r, Json::Value &v, std::string *err, int depth) {
	r.get(); // '{'

	v = Json::objectValue;

	skipWhitespace(r);
	if (r.peek() == '}') {
		r.get();
		return true;
	}

	while (true) {
		std::string key;
		int ch = r.peek();
		if (ch == '"' || ch == '\'') {
			if (!readStringLiteral(r, key, err)) {
				return false;
			}
		} else {
			if (!readIdentifier(r, key, err)) {
				return false;
			}
		}

		skipWhitespace(r);
		ch = r.peek();
		if (ch != ':') {
			error(r.loc(), err, "Expected colon ':'");
			return false;
		}
		r.get();

		if (!parseValue(r, v[key], err, depth)) {
			return false;
		}

		bool newline = skipWhitespace(r);
		bool comma = false;

		ch = r.peek();
		if (ch == ',') {
			comma = true;
			r.get();
			skipWhitespace(r);
			ch = r.peek();
		}

		if (ch == EOF) {
			error(r.loc(), err, "Unexpected EOF");
		} else if (ch == '}') {
			r.get();
			return true;
		}

		if (r.conf().newlinesAsCommas) {
			if (!comma && !newline) {
				error(r.loc(), err, "Expected ',', newline or '}'");
				return false;
			}
		} else if (!comma) {
			error(r.loc(), err, "Expected ',' or '}'");
			return false;
		}
	}
}

inline void serializeObject(
		std::ostream &os, const Json::Value &v,
		const SerializeConfig &conf, int depth) {
	os << '{';
	bool first = true;
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (!first) {
			os << ',';
		}
		first = false;

		serializeNewLine(os, conf, depth + 1);

		serializeIdentifier(os, it.key().asCString(), conf);
		if (conf.indent == nullptr) {
			os << ":";
		} else {
			os << ": ";
		}

		serializeValue(os, *it, conf, depth + 1);
	}

	if (!first) {
		if (conf.trailingCommas) {
			os << ',';
		}

		serializeNewLine(os, conf, depth);
	}

	os << '}';
}

// https://spec.json5.org/#prod-JSON5Array JSON5Array
inline bool parseArray(Reader &r, Json::Value &v, std::string *err, int depth) {
	r.get(); // '['

	v = Json::arrayValue;

	skipWhitespace(r);
	if (r.peek() == ']') {
		r.get();
		return true;
	}

	Json::ArrayIndex index = 0;
	while (true) {
		if (!parseValue(r, v[index++], err, depth)) {
			return false;
		}

		bool newline = skipWhitespace(r);

		int ch = r.peek();
		bool comma = false;
		if (ch == ',') {
			comma = true;
			r.get();
			skipWhitespace(r);
			ch = r.peek();
		}

		if (ch == EOF) {
			error(r.loc(), err, "Unexpectef EOF");
		} else if (ch == ']') {
			r.get();
			return true;
		}

		if (r.conf().newlinesAsCommas) {
			if (!comma && !newline) {
				error(r.loc(), err, "Expected ',', newline or ']'");
				return false;
			}
		} else if (!comma) {
			error(r.loc(), err, "Expected ',' or ']'");
			return false;
		}
	}
}

inline void serializeArray(
		std::ostream &os, const Json::Value &v,
		const SerializeConfig &conf, int depth) {
	os << '[';
	bool first = true;
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (!first) {
			os << ',';
		}
		first = false;

		serializeNewLine(os, conf, depth + 1);
		serializeValue(os, *it, conf, depth + 1);
	}

	if (!first) {
		if (conf.trailingCommas) {
			os << ',';
		}

		serializeNewLine(os, conf, depth);
	}

	os << ']';
}

// https://spec.json5.org/#values JSON5Value
inline bool parseValue(Reader &r, Json::Value &v, std::string *err, int depth) {
	if (depth >= r.conf().maxDepth) {
		error(r.loc(), err, "Depth limit reached");
		return false;
	}

	detail::skipWhitespace(r);
	Location loc = r.loc();
	int ch = r.peek();
	if (ch == EOF) {
		error(loc, err, "Unexpected EOF");
		return false;
	} else if (ch == '{') {
		return detail::parseObject(r, v, err, depth + 1);
	} else if (ch == '[') {
		return detail::parseArray(r, v, err, depth + 1);
	} else if (ch == '"' || ch == '\'') {
		std::string s;
		if (!detail::readStringLiteral(r, s, err)) {
			return false;
		}
		v = s;
		return true;
	} else if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '+' || ch == '-') {
		return detail::parseNumber(r, v, err);
	} else {
		std::string ident;
		if (!detail::readIdentifier(r, ident, err)) {
			return false;
		}

		if (ident == "null") {
			v = Json::nullValue;
		} else if (ident == "true") {
			v = true;
		} else if (ident == "false") {
			v = false;
		} else if (ident == "Infinity") {
			v = std::numeric_limits<double>::infinity();
		} else if (ident == "NaN") {
			v = std::numeric_limits<double>::quiet_NaN();
		} else {
			error(loc, err, "Invalid keyword");
			return false;
		}
	}

	return true;
}

inline void serializeValue(
		std::ostream &os, const Json::Value &v,
		const SerializeConfig &conf, int depth) {
	if (v.isObject()) {
		serializeObject(os, v, conf, depth);
	} else if (v.isArray()) {
		serializeArray(os, v, conf, depth);
	} else if (v.isString()) {
		serializeStringLiteral(os, v.asCString());
	} else {
		os << v;
	}
}

}

#ifndef JSON5CPP_IMPL
inline
#endif
bool parse(
		std::istream &is, Json::Value &v,
		std::string *err, ParseConfig conf) {
	detail::Reader r(is, conf);

	if (!detail::parseValue(r, v, err, 0)) {
		return false;
	}

	detail::skipWhitespace(r);
	if (r.peek() != EOF) {
		detail::error(r.loc(), err, "Trailing garbage");
		return false;
	}

	return true;
}

#ifndef JSON5CPP_IMPL
inline
#endif
void serialize(
		std::ostream &os, const Json::Value &v,
		SerializeConfig conf, int depth) {
	detail::serializeValue(os, v, conf, depth);
}

#endif // JSON5CPP_FWD_ONLY

}

#endif // JSON5CPP_H
