g++ -std=c++17 -Wall -g -c -static include/sista/ANSI-Settings.cpp include/sista/border.cpp include/sista/coordinates.cpp include/sista/cursor.cpp include/sista/field.cpp include/sista/pawn.cpp
g++ -std=c++17 -Wall -g -c -static inomhus.cpp -Wno-narrowing
g++ -std=c++17 -Wall -g -static -o inomhus inomhus.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o
rm -f *.o
