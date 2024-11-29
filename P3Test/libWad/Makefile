output: Wad.o libWad.a

wad.o: Wad.cpp Wad.h
	g++ -c Wad.cpp -o Wad.o

libWad.a: Wad.o
	ar cr libWad.a Wad.o
