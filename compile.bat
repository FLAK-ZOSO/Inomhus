g++ -std=c++17 -Wall -g -c -static Sista/include/sista/ANSI-Settings.cpp Sista/include/sista/border.cpp Sista/include/sista/coordinates.cpp Sista/include/sista/cursor.cpp Sista/include/sista/field.cpp Sista/include/sista/pawn.cpp
g++ -std=c++17 -Wall -g -c -static inomhus.cpp
g++ -std=c++17 -Wall -g -static -o inomhus inomhus.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o
rm -f *.o
