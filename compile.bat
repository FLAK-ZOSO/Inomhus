g++ -std=c++17 -Wall -g -c -static inomhus.cpp -Wno-narrowing
g++ -std=c++17 -Wall -g -static -lpthread -o inomhus inomhus.o -lSista
rm -f *.o
