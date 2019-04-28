example_simple: example_simple.o fastcgi.o fastcgi_conn.o fastcgi_request.o buffer.o
	g++ -std=gnu++2a -o example_simple example_simple.o fastcgi.o fastcgi_conn.o fastcgi_request.o buffer.o -lgflags -lglog -lpthread

clean:
	rm --force exmaple_simple *.o

.cpp.o:
	g++ -std=gnu++2a -o $@ -c $<
