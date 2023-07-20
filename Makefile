test: test.cc json5cpp.h
	$(CXX) -g -o $@ $< -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

clean:
	rm -f test
