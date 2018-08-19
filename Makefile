CXXFLAGS = -std=c++17 -Wall -O3
LINK.o = $(CXX)

all = varcalc username pal

.PHONY: all clean

all: $(all)

clean:
	rm $(all) *.o

pegparser.h: peg.h
	touch pegparser.h

varcalc.o: pegparser.h
username.o: peg.h
pal.o: peg.h

