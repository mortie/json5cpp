OUT ?= .

CXXFLAGS += -g -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

.PHONY: all
all: $(OUT)/json5-to-json $(OUT)/json-to-json5

$(OUT)/json5-to-json: examples/json5-to-json.cc json5cpp.h
	$(CXX) -I. -o $@ $< $(CXXFLAGS)

$(OUT)/json-to-json5: examples/json-to-json5.cc json5cpp.h
	$(CXX) -I. -o $@ $< $(CXXFLAGS)

.PHONY: clean
clean:
	rm -rf json5-to-json json5-to-json.dSYM
	rm -rf json-to-json5 json-to-json5.dSYM

.PHONY: fuzz
fuzz:
	./fuzz.sh

.PHONY: clean-fuzz
clean-fuzz:
	rm -rf fuzz-data

.PHONY: check
check:
	$(MAKE) -C tests check

.PHONY: bench
bench:
	$(MAKE) -C benchmarks bench
