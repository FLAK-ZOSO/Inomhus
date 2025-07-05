# Makefile for Inomhus
IMPLEMENTATIONS = include/sista/ANSI-Settings.cpp include/sista/border.cpp include/sista/coordinates.cpp include/sista/cursor.cpp include/sista/field.cpp include/sista/pawn.cpp

UNAME := $(shell uname)
STATIC_FLAG =
ifeq ($(UNAME),Darwin)
    STATIC_FLAG =
else
    STATIC_FLAG = -static
endif

all:
	g++ -std=c++17 -Wall -g -c $(IMPLEMENTATIONS)
	g++ -std=c++17 -Wall -g -c $(STATIC_FLAG) inomhus.cpp -Wno-narrowing
	g++ -std=c++17 -Wall -g $(STATIC_FLAG) -lpthread -o inomhus inomhus.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o
	rm -f *.o
