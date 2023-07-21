.PHONY: check
check: .prepared.stamp json5
	node run-tests.js

.prepared.stamp:
	rm -rf node_modules tests/json5-tests tests/JSONTestSuite
	npm install
	git clone https://github.com/json5/json5-tests.git
	json5-tests && git checkout c9af328e6d77286d78b77b520c4622d588b544c0
	git clone https://github.com/nst/JSONTestSuite.git
	JSONTestSuite && git checkout d64aefb55228d9584d3e5b2433f720ea8fd00c82
	touch $@

json5: ../cmd/json5.cc ../json5cpp.h
	$(CXX) -I.. $(CXXFLAGS) -fsanitize=address,undefined -o $@ $<

.PHONY: clean
clean:
	rm -f tests/.prepared.stamp
	rm -rf tests/node_modules tests/json5-tests tests/JSONTestSuite
	rm -rf tests/json5 tests/json5.dSYM
