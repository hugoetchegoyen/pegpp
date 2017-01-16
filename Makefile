CXXFLAGS = -std=c++11 -Wall
LINK.o = $(CXX)

varcalc: varcalc.o
varcalc.o: varcalc.cc peg.h

