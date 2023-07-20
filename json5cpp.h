#ifndef JSON5CPP_H
#define JSON5CPP_H

#include <json/json.h>
#include <istream>
#include <limits>

namespace Json5 {

bool parse(std::istream &is, Json::Value &v, int maxDepth = 100);

namespace detail {
// Skip to https://262.ecma-international.org/5.1/A#sec-7.3 LineTerminator
inline void skipPastLineTerminator(std::istream &is) {
	// I don't really want to parse UTF-8, so let's ignore U+2028 and U+2029
	while (true) {
		int ch = is.get();
		if (ch == EOF || ch == '\n' || ch == '\r') {
			return;
		}
	}
}

// Skip https://spec.json5.org/#white-space White Space
// and https://spec.json5.org/#comments Comments
inline void skipWhitespace(std::istream &is) {
	// Ignore the non-breaking space and BOM, because, again, no UTF-8 parsing,
	// and ignore Unicode "space separator" characters
	while (true) {
		int ch = is.peek();
		if (ch == EOF) {
			return;
		} else if (
				ch == '\t' || ch == '\n' || ch == '\v' ||
				ch == '\f' || ch == '\r' || ch == ' ') {
			is.get();
		} else if (ch == '/') {
			is.get();
			ch = is.peek();
			if (ch == '/') {
				is.get();
				skipPastLineTerminator(is);
			} else if (ch == '*') {
				is.get();
				int prev = is.get();
				ch = is.get();
				while (!(prev == '*' && ch == '/') && ch != EOF) {
					prev = ch;
					ch = is.get();
				}
			} else {
				is.putback('/');
				return;
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

inline bool readIdentifier(std::istream &is, std::string &str) {
	if (!isIdentStartChar(is.peek())) {
		return false;
	}

	str += is.get();
	while (isIdentPartChar(is.peek())) {
		str += is.get();
	}

	return true;
}

inline int hexChar(int ch) {
	if (ch == EOF) {
		return EOF;
	} else if (ch >= '0' && ch <= '9') {
		return ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'A' + 10;
	} else if (ch >= 'A' && ch <= 'Z') {
		return ch - 'A' + 10;
	} else {
		return EOF;
	}
}

// Write a code point (up to 16 bit) as UTF-8
inline void writeU16Utf8(unsigned int num, std::string &str) {
	if (num >= 0x0800u) {
		str += 0xe0 | ((num & 0xf000u) >> 12u);
		str += 0x80 | ((num & 0x0fc0u) >> 6u);
		str += 0x80 | ((num & 0x003fu) >> 0u);
	} else if (num >= 0x0080u) {
		str += 0xc0 | ((num & 0x07c0) >> 6);
		str += 0x80 | ((num & 0x003f) >> 0);
	} else {
		str += num;
	}
}

// https://spec.json5.org/#strings JSON5String
inline bool readStringLiteral(std::istream &is, std::string &str) {
	int startChar = is.get(); // '"' or "'"

	while (true) {
		int ch = is.get();
		if (ch == startChar) {
			return true;
		} else if (ch == '\\') {
			ch = is.get();
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
			} else if (ch == 'x') {
				int a = hexChar(is.get());
				int b = hexChar(is.get());
				if (a == EOF || b == EOF) {
					return false;
				}
				str += (a << 4) | b;
			} else if (ch == 'u') {
				// The IETF JSON spec explicitly says that Unicode code points bigger than 2^16
				// can be represented as two "\uXXXX" escapes which represent the code points
				// for a UTF-16 surrogate pair. However, the ECMA spec, which JSON5 references,
				// says nothing about that.
				// Dealing with surrogate pairs sounds terrible, so I'm not gonna do it.
				int a = hexChar(is.get());
				int b = hexChar(is.get());
				int c = hexChar(is.get());
				int d = hexChar(is.get());
				if (a == EOF || b == EOF || c == EOF || d == EOF) {
					return false;
				}

				unsigned int u =
					((unsigned int)a << 12) |
					((unsigned int)b << 8) |
					((unsigned int)c << 4) |
					((unsigned int)d << 0);
				writeU16Utf8(u, str);
			} else if (ch == '\n' || ch == '\r') {
				// Ignore line separator and paragraph separator, because again,
				// I don't wanna deal with parsing UTF-8
				str += ch;
				if (ch == '\r' && is.peek() == '\n') {
					str += is.get();
				}
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

inline bool parseNumber(std::istream &is, Json::Value &v) {
	// TODO
	v = 0;
	return true;
}

inline bool parseObject(std::istream &is, Json::Value &v, int maxDepth) {
	is.get(); // '{'

	while (true) {
		skipWhitespace(is);
		int ch = is.peek();
		if (ch == EOF) {
			return false;
		} else if (ch == ',') {
			is.get();
			skipWhitespace(is);
			ch = is.peek();
			if (ch == EOF) {
				return false;
			}
		}

		if (ch == '}') {
			is.get();
			return true;
		}

		std::string key;
		if (ch == '"' | ch == '\'') {
			if (!readStringLiteral(is, key)) {
				return false;
			}
		} else {
			if (!readIdentifier(is, key)) {
				return false;
			}
		}

		skipWhitespace(is);

		ch = is.peek();
		if (ch != ':') {
			return false;
		}
		is.get();

		parse(is, v[key], maxDepth);
	}
}

inline bool parseArray(std::istream &is, Json::Value &v, int maxDepth) {
	is.get(); // '['

	Json::ArrayIndex index = 0;
	while (true) {
		skipWhitespace(is);
		int ch = is.peek();
		if (ch == EOF) {
			return false;
		} else if (ch == ',') {
			is.get();
			skipWhitespace(is);
			ch = is.peek();
			if (ch == EOF) {
				return false;
			}
		}

		if (ch == ']') {
			is.get();
			return true;
		}

		parse(is, v[index++], maxDepth);
	}
}
}

inline bool parse(std::istream &is, Json::Value &v, int maxDepth) {
	detail::skipWhitespace(is);
	int ch = is.peek();
	if (ch == EOF) {
		return false;
	} else if (ch == '{') {
		return detail::parseObject(is, v, maxDepth - 1);
	} else if (ch == '[') {
		return detail::parseArray(is, v, maxDepth - 1);
	} else if (ch == '"' || ch == '\'') {
		std::string s;
		if (!detail::readStringLiteral(is, s)) {
			return false;
		}
		v = s;
	} else if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '+' || ch == '-') {
		if (!detail::parseNumber(is, v)) {
			return false;
		}
	} else {
		std::string ident;
		if (!detail::readIdentifier(is, ident)) {
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

	return false;
}

}

#endif
