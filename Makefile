UNAME := $(shell uname)
STATIC_FLAG =
WINMM_FLAG =
ifeq ($(UNAME),Darwin)
    STATIC_FLAG =
else
    STATIC_FLAG = -static
endif

ifeq ($(OS),Windows_NT)
	PREFIX ?= C:\Program Files\Sista
    WINMM_FLAG = -lwinmm
	INCLUDE_PATH_DIRECTIVE = -I"$(PREFIX)\include"
	LD_LIBRARY_PATH_DIRECTIVE = -L"$(PREFIX)\lib"
else
	PREFIX ?= /usr/local
	INCLUDE_PATH_DIRECTIVE = -I$(PREFIX)/include
	LD_LIBRARY_PATH_DIRECTIVE = -L$(PREFIX)/lib
endif

all:
	g++ -std=c++17 -Wall -g $(STATIC_FLAG) -c inomhus.cpp $(INCLUDE_PATH_DIRECTIVE) -Wno-narrowing
	g++ -std=c++17 -Wall -g $(STATIC_FLAG) -o inomhus $(LD_LIBRARY_PATH_DIRECTIVE) $(WINMM_FLAG) inomhus.o -lpthread -lSista
	rm -f *.o
