UNAME := $(shell uname)
STATIC_FLAG =
ifeq ($(UNAME),Darwin)
    STATIC_FLAG =
else
    STATIC_FLAG = -static
endif

all:
	g++ -std=c++17 -Wall -g -c $(STATIC_FLAG) inomhus.cpp -Wno-narrowing
	g++ -std=c++17 -Wall -g $(STATIC_FLAG) -lpthread -lSista -o inomhus inomhus.o
	rm -f *.o
