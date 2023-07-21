DESTDIR ?= .

$(DESTDIR)/json5: cmd/json5.cc json5cpp.h
	$(CXX) -I. -g -o $@ $< -std=c++11 $(shell pkg-config --libs --cflags jsoncpp)

$(DESTDIR)/test: cmd/test.cc json5cpp.h
	$(CXX) -I. -g -o $@ $< -std=c++17 $(shell pkg-config --libs --cflags jsoncpp)

.PHONY: check
check: $(DESTDIR)/test
	$(DESTDIR)/test

clean:
	rm -f $(DESTDIR)/test $(DESTDIR)/json5
