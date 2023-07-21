CXXFLAGS += -g -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

DESTDIR ?= .

$(DESTDIR)/json5: cmd/json5.cc json5cpp.h
	$(CXX) -I. $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(DESTDIR)/json5 $(DESTDIR)/json5.dSYM

.PHONY: fuzz
fuzz:
	./fuzz.sh

.PHONY: clean-fuzz
clean-fuzz:
	rm -rf fuzz-data

.PHONY: check
check: tests/.prepared.stamp
	cd tests && node run-tests.js

tests/.prepared.stamp:
	rm -rf tests/node_modules tests/json5-tests tests/JSONTestSuite
	rm -rf tests/json5 tests/json5.dSYM
	cd tests && npm install
	$(MAKE) DESTDIR=tests CFLAGS=-fsanitize=address,undefined tests/json5
	cd tests && git clone https://github.com/json5/json5-tests.git
	cd tests/json5-tests && git checkout c9af328e6d77286d78b77b520c4622d588b544c0
	cd tests && git clone https://github.com/nst/JSONTestSuite.git
	cd tests/JSONTestSuite && git checkout d64aefb55228d9584d3e5b2433f720ea8fd00c82
	touch $@

.PHONY: clean-tests
clean-tests:
	rm -f tests/.prepared.stamp
	rm -rf tests/node_modules tests/json5-tests tests/JSONTestSuite
	rm -rf tests/json5 tests/json5.dSYM
