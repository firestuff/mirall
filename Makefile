mirall: Makefile mirall.cpp
	g++ -std=gnu++2a -o mirall mirall.cpp -lgflags -lglog

clean:
	rm --force mirall
