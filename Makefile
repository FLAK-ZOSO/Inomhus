# Makefile for Inomhus
IMPLEMENTATIONS=Sista/include/sista/ANSI-Settings.cpp Sista/include/sista/border.cpp Sista/include/sista/coordinates.cpp Sista/include/sista/cursor.cpp Sista/include/sista/field.cpp Sista/include/sista/pawn.cpp

all:
	g++ -std=c++17 -Wall -g -c $(IMPLEMENTATIONS)
	g++ -std=c++17 -Wall -g -c inomhus.cpp -Wno-narrowing
	g++ -std=c++17 -Wall -g -o inomhus inomhus.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o
	rm -f *.o