CXXFLAGS = -std=c++11 -Wall -O3
LINK.o = $(CXX)

all: varcalc username

varcalc: varcalc.o
varcalc.o: varcalc.cc peg.h

username: username.o
username.o: username.cc peg.h

