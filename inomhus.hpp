#include "Sista/include/sista/sista.hpp"
#include <unordered_map>
#include <vector>
#include <random>


enum Type {
    PLAYER,
    BULLET,
    WALL, // #
    GATE, // G, only open during the day
    CHEST, // C, can be collected by the player
    TRAP, // T, will act when stepped on
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


struct Inventory {
    short walls = 0;
    short eggs = 0;
    short meat = 0;

    void operator+=(const Inventory&);
}; // The idea is that the inventory can be dropped (as CHEST) and picked up by the player


class Entity : public sista::Pawn {
public:
    Type type;

    Entity();
    Entity(char, sista::Coordinates, ANSI::Settings&, Type);
};


class Player : public Entity {
public:
    static ANSI::Settings playerStyle;
    static Player* player;
    enum Mode {
        COLLECT, BULLET, DUMPCHEST,
        WALL, GATE, TRAP, MINE, HATCH
    } mode;
    Inventory inventory;

    Player();
    Player(sista::Coordinates);

    void move(Direction);
    void shoot(Direction);
};


class Bullet : public Entity {
public:
    static ANSI::Settings bulletStyle;
    static std::vector<Bullet*> bullets;
    Direction direction;
    unsigned short speed = 1; // The bullet moves speed cells per frame
    bool collided = false; // If the bullet was destroyed in a collision with an opposite bullet

    Bullet();
    Bullet(sista::Coordinates, Direction);
    Bullet(sista::Coordinates, Direction, unsigned short);

    void move();

    static void removeBullet(Bullet*);
};


class EnemyBullet : public Entity {
public:
    static ANSI::Settings enemyBulletStyle;
    static std::vector<EnemyBullet*> enemyBullets;
    Direction direction;
    unsigned short speed = 1; // The bullet moves speed cells per frame
    bool collided = false; // If the bullet was destroyed in a collision with an opposite bullet

    EnemyBullet();
    EnemyBullet(sista::Coordinates, Direction);
    EnemyBullet(sista::Coordinates, Direction, unsigned short);

    void move();

    static void removeEnemyBullet(EnemyBullet*);
};


class Wall : public Entity {
public:
    static ANSI::Settings wallStyle;
    static std::vector<Wall*> walls;
    short int strength; // The wall has a certain strength (when it reaches 0, the wall is destroyed)

    Wall();
    Wall(sista::Coordinates, short int);

    static void removeWall(Wall*);
};


class Mine : public Entity {
public:
    static ANSI::Settings mineStyle;
    static std::vector<Mine*> mines;
    bool triggered = false;
    bool alive = true;

    Mine();
    Mine(sista::Coordinates);

    bool checkTrigger();
    void trigger();
    void explode();

    static void removeMine(Mine*);
};


class Chest : public Entity {
public:
    static ANSI::Settings chestStyle;
    static std::vector<Chest*> chests;
    Inventory inventory;

    Chest();
    Chest(sista::Coordinates, Inventory, bool);
    Chest(sista::Coordinates, Inventory&);

    static void removeChest(Chest*);
};


class Trap : public Entity {
public:
    static ANSI::Settings trapStyle;
    static std::vector<Trap*> traps;

    Trap();
    Trap(sista::Coordinates);

    static void removeTrap(Trap*);
}; // The trap will act when stepped on by wild hostile animals, and this will be implemented in the other Entity classes


class Walker : public Entity {
public:
    static ANSI::Settings walkerStyle;
    static std::vector<Walker*> walkers;
    static std::bernoulli_distribution movingDistribution;

    Walker();
    Walker(sista::Coordinates);

    void move();

    static void removeWalker(Walker*);
};


class Archer : public Entity {
public:
    static ANSI::Settings archerStyle;
    static std::vector<Archer*> archers;
    static std::bernoulli_distribution movingDistribution;
    static std::bernoulli_distribution shootDistribution;

    Archer();
    Archer(sista::Coordinates);

    void move();
    void shoot();

    static void removeArcher(Archer*);
};


class Chicken : public Entity {
public:
    static ANSI::Settings chickenStyle;
    static std::vector<Chicken*> chickens;
    static std::bernoulli_distribution movingDistribution;
    static std::bernoulli_distribution eggDistribution;

    Chicken();
    Chicken(sista::Coordinates);

    void move(); // The laying of eggs will be implemented here

    static void removeChicken(Chicken*);
};


class Egg : public Entity {
public:
    static ANSI::Settings eggStyle;
    static std::vector<Egg*> eggs;
    static std::bernoulli_distribution hatchingDistribution;

    Egg();
    Egg(sista::Coordinates);

    static void removeEgg(Egg*);
};


class Weasel : public Entity {
public:
    static ANSI::Settings weaselStyle;
    static std::vector<Weasel*> weasels;
    bool crossed = false; // If the weasel has reached the other side of the field and will be removed
    bool caught = false; // If the weasel was caught in a trap
    Direction direction;

    Weasel();
    Weasel(sista::Coordinates, Direction);

    void move();

    static void removeWeasel(Weasel*);
}; // The weasel will eat chickens and jump over eggs, while changing the direction on obstacles


class Snake : public Entity {
public:
    static ANSI::Settings snakeStyle;
    static std::vector<Snake*> snakes;
    bool crossed = false; // If the snake has reached the other side of the field and will be removed
    Direction direction;

    Snake();
    Snake(sista::Coordinates, Direction);

    void move();

    static void removeSnake(Snake*);
}; // The snake will eat eggs and jump over chickens


class Gate : public Entity {
public:
    static ANSI::Settings gateStyle;
    static std::vector<Gate*> gates;
    // bool open = false; // Redundant, open at day, closed at night

    Gate();
    Gate(sista::Coordinates);

    static void removeGate(Gate*);
};

void input();
void act(char);
void populate(sista::SwappableField*);
void repopulate(sista::SwappableField*);
void spawnNew(sista::SwappableField*);
void removeNullptrs(std::vector<Entity*>&);