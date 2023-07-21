CXXFLAGS += -g -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

json5: cmd/json5.cc json5cpp.h
	$(CXX) -I. $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf json5 json5.dSYM

.PHONY: fuzz
fuzz:
	./fuzz.sh

.PHONY: clean-fuzz
clean-fuzz:
	rm -rf fuzz-data

.PHONY: check
check:
	$(MAKE) -C tests check
