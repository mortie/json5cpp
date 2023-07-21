json5: cmd/json5.cc json5cpp.h
	$(CXX) -I. -g -o $@ $< -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

test: cmd/test.cc json5cpp.h
	$(CXX) -I. -g -o $@ $< -std=c++17 $(shell pkg-config --libs --cflags jsoncpp)

.PHONY: check
check: test
	./test

clean:
	rm -f test
