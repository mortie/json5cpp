CXXFLAGS += \
	-g -std=c++11 -fsanitize=address,undefined \
	$(shell pkg-config --libs --cflags jsoncpp)

.PHONY: check
check: .prepared.stamp json5-to-json
	node run-tests.js

.prepared.stamp:
	rm -rf node_modules json5-tests JSONTestSuite
	npm install
	git clone https://github.com/json5/json5-tests.git
	cd json5-tests && git checkout c9af328e6d77286d78b77b520c4622d588b544c0
	git clone https://github.com/nst/JSONTestSuite.git
	cd JSONTestSuite && git checkout d64aefb55228d9584d3e5b2433f720ea8fd00c82
	touch $@

json5-to-json: ../examples/json5-to-json.cc ../json5cpp.h
	$(CXX) -I.. -o $@ $< $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f .prepared.stamp
	rm -rf node_modules json5-tests JSONTestSuite
	rm -rf json5-to-json json5-to-json.dSYM
