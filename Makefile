all: example_simple example_clock

objects = sse.o sse_stream.o fastcgi.o fastcgi_conn.o fastcgi_request.o fastcgi_parse.o stream_buffer.o buffer.o

example_simple: example_simple.o $(objects) Makefile
	clang++ -std=gnu++2a -o example_simple example_simple.o $(objects) -lgflags -lglog -lpthread

example_clock: example_clock.o $(objects) Makefile
	clang++ -std=gnu++2a -o example_clock example_clock.o $(objects) -lgflags -lglog -lpthread

%.o: %.cc *.h Makefile
	clang++ -std=gnu++2a -Wall -Werror -c -o $@ $<

clean:
	rm --force exmaple_simple example_clock *.o

