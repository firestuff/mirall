mirall: Makefile mirall.o fastcgi.o fastcgi_conn.o buffer.o
	g++ -std=gnu++2a -o mirall mirall.o fastcgi.o fastcgi_conn.o buffer.o -lgflags -lglog -lpthread

clean:
	rm --force mirall *.o

.cpp.o: Makefile *.h
	g++ -std=gnu++2a -o $@ -c $<
