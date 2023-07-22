/* Json5Cpp, a JSON5 parser -- https://github.com/mortie/json5cpp */

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

#include "json/writer.h"
#include <json/json.h>
#include <istream>
#include <limits>
#include <memory>
#include <string.h>

namespace Json5 {

namespace detail {

class Reader {
public:
	Reader(std::istream &is): is_(is) {
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
		return ch;
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
		size_ += is_.rdbuf()->sgetn(buffer_ + size_, sizeof(buffer_) - size_);
	}

	std::istream &is_;
	char buffer_[128];
	size_t index_ = 0;
	size_t size_ = 0;

	std::unique_ptr<Json::CharReader> jsonCharReader_;
};

bool parseValue(Reader &r, Json::Value &v, int maxDepth);

// Skip to https://262.ecma-international.org/5.1/A#sec-7.3 LineTerminator
inline void skipPastLineTerminator(Reader &r) {
	// I don't really want to parse UTF-8, so let's ignore U+2028 and U+2029
	while (true) {
		int ch = r.get();
		if (ch == EOF || ch == '\n' || ch == '\r') {
			return;
		}
	}
}

// Skip https://spec.json5.org/#white-space White Space
// and https://spec.json5.org/#comments Comments
inline void skipWhitespace(Reader &r) {
	// Ignore the non-breaking space and BOM, because, again, no UTF-8 parsing,
	// and ignore Unicode "space separator" characters
	while (true) {
		int ch = r.peek();
		if (ch == EOF) {
			return;
		} else if (
				ch == '\t' || ch == '\n' || ch == '\v' ||
				ch == '\f' || ch == '\r' || ch == ' ') {
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
			return;
		}
	}
}

// https://262.ecma-international.org/5.1/#sec-7.6 IdentifierStart
inline bool isIdentStartChar(int ch) {
	// I really don't want to depend on a unicode database.
	// Let's just assume everything >=128 is part of a unicode sequence
	// which makes a valid unicode letter character.
	return ch != EOF && (
		ch >= 128 ||
		(ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') ||
		ch == '$' || ch == '_' || ch == '\\');
}

// https://262.ecma-international.org/5.1/#sec-7.6 IdentifierPart
inline bool isIdentPartChar(int ch) {
	// isIdentStartChar already returns true for any unicode characters,
	// so we don't have to care about UnicodeDigit and the like
	return isIdentStartChar(ch) || (ch >= '0' && ch <= '9');
}

// https://262.ecma-international.org/5.1/#sec-7.6 IdentifierName
inline bool readIdentifier(Reader &r, std::string &str) {
	if (!isIdentStartChar(r.peek())) {
		return false;
	}

	str += r.get();
	while (isIdentPartChar(r.peek())) {
		str += r.get();
	}

	return true;
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

inline bool read4Hex(Reader &r, unsigned int &u) {
	int a = hexChar(r.get());
	int b = hexChar(r.get());
	int c = hexChar(r.get());
	int d = hexChar(r.get());
	if (a == EOF || b == EOF || c == EOF || d == EOF) {
		return false;
	}

	u =
		((unsigned int)a << 12) |
		((unsigned int)b << 8) |
		((unsigned int)c << 4) |
		((unsigned int)d << 0);
	return true;
}

inline bool readUnicodeEscape(Reader &r, std::string &str) {
	// Assume we have already read the first '\' and 'u'

	unsigned int u1;
	if (!read4Hex(r, u1)) {
		return false;
	}

	if (u1 >= 0xd800u && u1 <= 0xdbffu) {
		// First character was a high surrogate, read the low surrogate
		if (r.peek() != '\\') {
			return false;
		}
		r.get();

		if (r.peek() != 'u') {
			return false;
		}
		r.get();

		unsigned int u2;
		if (!read4Hex(r, u2)) {
			return false;
		}

		if (!(u2 >= 0xdc00u && u2 <= 0xdfffu)) {
			// Don't pair the high surrogate with a non-low-surrogate
			return false;
		}

		writeUtf8((u1 - 0xd800u) * 0x400u + u2 - 0xdc00u + 0x10000u, str);
		return true;
	} else if (u1 >= 0xdc00u && u1 <= 0xdfffu) {
		// Don't allow unpaired surrogates
		return false;
	} else {
		writeUtf8(u1, str);
		return true;
	}
}

// https://spec.json5.org/#strings JSON5String
inline bool readStringLiteral(Reader &r, std::string &str) {
	int startChar = r.get(); // '"' or "'"

	while (true) {
		int ch = r.get();
		if (ch == EOF) {
			return false;
		} else if (ch == startChar) {
			return true;
		} else if (ch == '\\') {
			ch = r.get();
			if (ch == EOF) {
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
				int a = hexChar(r.get());
				int b = hexChar(r.get());
				if (a == EOF || b == EOF) {
					return false;
				}
				str += (a << 4) | b;
			} else if (ch == 'u') {
				readUnicodeEscape(r, str);
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
			return false;
		} else {
			str += ch;
		}
	}
}

// https://spec.json5.org/#numbers JSON5Number
inline bool parseNumber(Reader &r, Json::Value &v) {
	// Parsing floats accurately is hard.
	// Instead, let's create a JSON string, then use jsoncpp to parse it.
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
		readIdentifier(r, str);
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

	return r.jsonCharReader().parse(
		str.c_str(), str.c_str() + str.size(), &v, nullptr);
}

// https://spec.json5.org/#prod-JSON5Object JSON5Object
inline bool parseObject(Reader &r, Json::Value &v, int maxDepth) {
	r.get(); // '{'

	v = Json::objectValue;
	while (true) {
		skipWhitespace(r);
		int ch = r.peek();
		if (ch == EOF) {
			return false;
		} else if (ch == ',') {
			r.get();
			skipWhitespace(r);
			ch = r.peek();
			if (ch == EOF) {
				return false;
			}
		}

		if (ch == '}') {
			r.get();
			return true;
		}

		std::string key;
		if (ch == '"' | ch == '\'') {
			if (!readStringLiteral(r, key)) {
				return false;
			}
		} else {
			if (!readIdentifier(r, key)) {
				return false;
			}
		}

		skipWhitespace(r);

		ch = r.peek();
		if (ch != ':') {
			return false;
		}
		r.get();

		if (!parseValue(r, v[key], maxDepth)) {
			return false;
		}
	}
}

// https://spec.json5.org/#prod-JSON5Array JSON5Array
inline bool parseArray(Reader &r, Json::Value &v, int maxDepth) {
	r.get(); // '['

	v = Json::arrayValue;
	Json::ArrayIndex index = 0;
	while (true) {
		skipWhitespace(r);
		int ch = r.peek();
		if (ch == EOF) {
			return false;
		} else if (ch == ',') {
			r.get();
			skipWhitespace(r);
			ch = r.peek();
			if (ch == EOF) {
				return false;
			}
		}

		if (ch == ']') {
			r.get();
			return true;
		}

		if (!parseValue(r, v[index++], maxDepth)) {
			return false;
		}
	}
}

// https://spec.json5.org/#values JSON5Value
inline bool parseValue(Reader &r, Json::Value &v, int maxDepth) {
	if (maxDepth < 0) {
		return false;
	}

	detail::skipWhitespace(r);
	int ch = r.peek();
	if (ch == EOF) {
		return false;
	} else if (ch == '{') {
		return detail::parseObject(r, v, maxDepth - 1);
	} else if (ch == '[') {
		return detail::parseArray(r, v, maxDepth - 1);
	} else if (ch == '"' || ch == '\'') {
		std::string s;
		if (!detail::readStringLiteral(r, s)) {
			return false;
		}
		v = s;
		return true;
	} else if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '+' || ch == '-') {
		return detail::parseNumber(r, v);
	} else {
		std::string ident;
		if (!detail::readIdentifier(r, ident)) {
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
			return false;
		}
	}

	return true;
}

}

inline bool parse(std::istream &is, Json::Value &v, int maxDepth = 100) {
	detail::Reader r(is);

	if (!detail::parseValue(r, v, maxDepth)) {
		return false;
	}

	detail::skipWhitespace(r);
	if (r.peek() != EOF) {
		return false;
	}

	return true;
}

}

#endif
