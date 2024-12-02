OUT ?= build

CXXFLAGS += -g -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

.PHONY: all
all: $(OUT)/json5-to-json $(OUT)/json-to-json5 $(OUT)/separate-compilation

$(OUT)/json5-to-json: examples/json5-to-json.cc json5cpp.h
	@mkdir -p $(@D)
	$(CXX) -I. -o $@ $< $(CXXFLAGS)

$(OUT)/json-to-json5: examples/json-to-json5.cc json5cpp.h
	@mkdir -p $(@D)
	$(CXX) -I. -o $@ $< $(CXXFLAGS)

$(OUT)/separate-compilation: \
	examples/separate-compilation/json5cpp.cc \
	examples/separate-compilation/main.cc \
	examples/separate-compilation/empty.cc
	@mkdir -p $(@D)
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean
clean:
	rm -rf $(OUT)

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
