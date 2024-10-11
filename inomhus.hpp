#include "Sista/include/sista/sista.hpp"
#include <unordered_map>
#include <vector>
#include <random>

enum Type {
    PLAYER,
    BULLET,
    WALL, // #
    GATE, // G, only open during the day
    MINE, // *, will be triggered when passing by

    WALKER,
    ARCHER,
    // WORMHEAD,
    // WORMBODY,
    ENEMYBULLET,
    WEASEL, // } or {, depending on the direction, will eat chickens
    SNAKE, // ~, in any of the two directions, will eat eggs

    CHICKEN, // %, will lay eggs
    EGG, // 0, will hatch into a chicken
};


enum Direction {UP, RIGHT, DOWN, LEFT};
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::unordered_map<Direction, char> directionSymbol;
extern std::mt19937 rng;


class Entity : public sista::Pawn {
public:
    Type type;

    Entity();
    Entity(char, sista::Coordinates, ANSI::Settings&, Type);
};