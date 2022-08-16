CXXFLAGS = -std=c++17 -Wall -O3
LINK.o = $(CXX)

all = intcalc varcalc username pal numsum intcalcerr mpal

.PHONY: all clean

all: $(all)

clean:
	rm $(all) *.o

intcalc.o: peg.h
intcalcerr.o: peg.h
varcalc.o: peg.h
username.o: peg.h
pal.o: peg.h
numsum.o: peg.h
mpal.o: peg.h

