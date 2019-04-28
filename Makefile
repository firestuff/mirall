example_simple: example_simple.o fastcgi.o fastcgi_conn.o fastcgi_request.o stream_buffer.o buffer.o
	clang++ -std=gnu++2a -o example_simple example_simple.o fastcgi.o fastcgi_conn.o fastcgi_request.o stream_buffer.o buffer.o -lgflags -lglog -lpthread

clean:
	rm --force exmaple_simple *.o

.cpp.o:
	clang++ -std=gnu++2a -Wall -Werror -o $@ -c $<
