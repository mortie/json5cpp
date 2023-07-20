#include <json/json.h>
#include <istream>

namespace Json5 {

namespace detail {
inline void skipWhitespace(std::istream &is) {
	while (true) {
		int ch = is.peek();
		if (ch == EOF) {
			return;
		} else if (ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ') {
			is.get();
		} else if (ch == '/') {
			is.get();
			ch = is.peek();
			if (ch == '/') {
				is.get();
				do {
					ch = is.get();
				} while (ch != '\n' && ch != EOF);
			} else if (ch == '*') {
				is.get();
				int prev = is.get();
				ch = is.get();
				while (!(prev == '*' && ch == '/') && ch != EOF) {
					prev = ch;
					ch = is.get();
				}
			} else if (ch == EOF) {
				is.putback('/');
				return;
			}
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
}

		inline bool readStringLiteral(std::istream &is, std::string &str) {
}

inline bool parseObject(std::istream &is, Json::Value &v) {
	is.get(); // '{'
	skipWhitespace(is);

	while (true) {
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
			return true;
		}

		std::string key;
		if (ch == '"' | ch == '\'') {
			if (!readStringLiteral(is, key)) {
				return false;
			}
		} else if (
	}
}
}

inline bool parse(std::istream &is, Json::Value &v) {
	detail::skipWhitespace(is);
	int ch = is.peek();
	if (ch == EOF) {
		return false;
	} else if (ch == '{') {
		return detail::parseObject(is, v);
	}

	return false;
}

}
