CXXFLAGS = -std=c++11 -Wall -O3
LINK.o = $(CXX)

varcalc: varcalc.o
varcalc.o: varcalc.cc peg.h

