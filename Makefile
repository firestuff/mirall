FCGI_CXX ?= clang++
FCGI_CXXFLAGS ?= -O3 -std=gnu++2a -Wall -Werror
FCGI_LDLIBS ?= -lgflags -lglog -lpthread

all: example_simple example_clock

objects = sse.o sse_stream.o fastcgi.o fastcgi_conn.o fastcgi_request.o fastcgi_parse.o stream_buffer.o buffer.o

example_simple: example_simple.o $(objects)
	$(FCGI_CXX) $(FCGI_CXXFLAGS) -o $@ $+ $(FCGI_LDLIBS)

example_clock: example_clock.o $(objects)
	$(FCGI_CXX) $(FCGI_CXXFLAGS) -o $@ $+ $(FCGI_LDLIBS)

%.o: %.cc *.h Makefile
	$(FCGI_CXX) $(FCGI_CXXFLAGS) -c -o $@ $<

clean:
	rm --force example_simple example_clock fastcgi_conn_afl *.o

afl:
	$(MAKE) clean
	FCGI_CXX=afl-g++ $(MAKE) afl_int

afl_int: fastcgi_conn_afl

fastcgi_conn_afl: fastcgi_conn_afl.o $(objects)
	$(FCGI_CXX) $(FCGI_CXXFLAGS) -o $@ $+ $(FCGI_LDLIBS)

test: fastcgi_conn_afl
	for FILE in afl_state/testcases/*; do ./fastcgi_conn_afl < $$FILE; done
	@printf '\033[0;32mALL TESTS PASSED\033[0m\n'
