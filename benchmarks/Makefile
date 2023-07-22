CXXFLAGS += \
	-O3 -std=c++11 \
	$(shell pkg-config --libs --cflags jsoncpp)

.PHONY: bench
bench: run-bench
	./run-bench

run-bench: run-bench.cc ../json5cpp.h
	$(CXX) -I.. -o $@ $< $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f run-bench
