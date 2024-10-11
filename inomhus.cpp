#include "cross_platform.hpp"
#include "inomhus.hpp"
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>


sista::SwappableField* field;
sista::Cursor cursor;
bool pause_ = false;
bool end = false;
Player* Player::player = new Player();
std::vector<Walker*> Walker::walkers;
std::vector<Archer*> Archer::archers;
std::vector<Bullet*> Bullet::bullets;
std::vector<EnemyBullet*> EnemyBullet::enemyBullets;
std::vector<Mine*> Mine::mines;
std::vector<Chest*> Chest::chests;
std::vector<Trap*> Trap::traps;
std::vector<Weasel*> Weasel::weasels;
std::vector<Snake*> Snake::snakes;
std::vector<Chicken*> Chicken::chickens;
std::vector<Egg*> Egg::eggs;
std::vector<Gate*> Gate::gates;
std::vector<Wall*> Wall::walls;

std::bernoulli_distribution Egg::hatchingDistribution(0.26); // 26%
std::bernoulli_distribution Chicken::eggDistribution(0.05); // 5%
std::bernoulli_distribution Chicken::movingDistribution(0.75); // 75%
std::bernoulli_distribution Archer::movingDistribution(0.25); // 25%
std::bernoulli_distribution Archer::shootDistribution(0.005); // 0.5%
std::bernoulli_distribution Walker::movingDistribution(0.5); // 50%


int main(int argc, char** argv) {
    #ifdef __APPLE__
        term_echooff();
    #endif
    std::ios_base::sync_with_stdio(false);
    ANSI::reset(); // Reset the settings
    srand(time(0)); // Seed the random number generator

    sista::SwappableField field_(50, 20);
    field = &field_;
    field->clear();
    sista::Border border(
        '@', {
            ANSI::ForegroundColor::F_BLACK,
            ANSI::BackgroundColor::B_WHITE,
            ANSI::Attribute::BRIGHT
        }
    );

    std::thread th = std::thread([&]() {
        char input = '_';
        while (input != 'Q' /*&& input != 'q'*/) {
            if (end) return;
            #if defined(_WIN32) or defined(__linux__)
                input = getch();
            #elif __APPLE__
                input = getchar();
            #endif
            if (end) return;
            switch (input) {
                case 'w': case 'W':
                    Player::player->move(Direction::UP);
                    break;
                case 'd': case 'D':
                    Player::player->move(Direction::RIGHT);
                    break;
                case 's': case 'S':
                    Player::player->move(Direction::DOWN);
                    break;
                case 'a': case 'A':
                    Player::player->move(Direction::LEFT);
                    break;

                case 'j': case 'J':
                    Player::player->shoot(Direction::LEFT);
                    break;
                case 'k': case 'K':
                    Player::player->shoot(Direction::DOWN);
                    break;
                case 'l': case 'L':
                    Player::player->shoot(Direction::RIGHT);
                    break;
                case 'i': case 'I':
                    Player::player->shoot(Direction::UP);
                    break;

                case 'c': case 'C':
                    Player::player->mode = Player::Mode::COLLECT;
                    break;
                case 'b': case 'B':
                    Player::player->mode = Player::Mode::BULLET;
                    break;
                case 'e': case 'E': case 'q':
                    Player::player->mode = Player::Mode::DUMPCHEST;
                    break;
                case '=': case '0': case '#':
                    Player::player->mode = Player::Mode::WALL;
                    break;
                case 'g': case 'G':
                    Player::player->mode = Player::Mode::GATE;
                    break;
                case 't': case 'T':
                    Player::player->mode = Player::Mode::TRAP;
                    break;
                case 'm': case 'M': case '*':
                    Player::player->mode = Player::Mode::MINE;
                    break;
                case 'h': case 'H':
                    Player::player->mode = Player::Mode::HATCH;
                    break;

                case '.': case 'p': case 'P':
                    pause_ = !pause_;
                    break;
                case 'Q': /* case 'q': */
                    end = true;
                    return;
                default:
                    break;
            }
        }
    });
}


std::unordered_map<Direction, sista::Coordinates> directionMap = {
    {Direction::UP, {(unsigned short)-1, 0}},
    {Direction::RIGHT, {0, 1}},
    {Direction::DOWN, {1, 0}},
    {Direction::LEFT, {0, (unsigned short)-1}}
};
std::unordered_map<Direction, char> directionSymbol = {
    {Direction::UP, '^'},
    {Direction::RIGHT, '>'},
    {Direction::DOWN, 'v'},
    {Direction::LEFT, '<'}
};
std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
bool day = true;


void Inventory::operator+=(const Inventory& other) {
    walls += other.walls;
    eggs += other.eggs;
    meat += other.meat;
}

Entity::Entity(char symbol, sista::Coordinates coordinates, ANSI::Settings& settings, Type type) : sista::Pawn(symbol, coordinates, settings), type(type) {}
Entity::Entity() : sista::Pawn(' ', sista::Coordinates(0, 0), Player::playerStyle), type(Type::PLAYER) {}

ANSI::Settings Player::playerStyle = {
    ANSI::ForegroundColor::F_RED,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
Player::Player(sista::Coordinates coordinates) : Entity('$', coordinates, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory({0, 0}) {}
Player::Player() : Entity('$', {0, 0}, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory({0, 0}) {}
void Player::move(Direction direction) {
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction];
    if (field->isOutOfBounds(nextCoordinates)) {
        return;
    } else if (field->isOccupied(nextCoordinates)) {
        Entity* entity = (Entity*)field->getPawn(nextCoordinates);
        if (entity->type == Type::WALL) {
            return;
        } else if (entity->type == Type::CHEST) {
            inventory += ((Chest*)entity)->inventory;
            Chest::removeChest((Chest*)entity);
        } // TODO
    }
    field->movePawn(this, nextCoordinates);
    coordinates = nextCoordinates;
}
void Player::shoot(Direction direction) {
    Mode mode = Player::player->mode;
    sista::Coordinates targetCoordinates = coordinates + directionMap[direction];
    if (field->isOutOfBounds(targetCoordinates)) {
        return;
    } else if (field->isOccupied(targetCoordinates)) {
        Entity* entity = (Entity*)field->getPawn(targetCoordinates);
        if (entity->type == Type::WALL) {
            Wall* wall = (Wall*)entity;
            if (mode == Mode::BULLET) {
                wall->strength--;
                if (wall->strength == 0) {
                    wall->setSymbol('@'); // Change the symbol to '@' to indicate that the wall was destroyed
                    field->rePrintPawn(wall); // It will be reprinted in the next frame and then removed because of (strength == 0)
                }
            } else if (mode == Mode::GATE) {
                // Replace the wall with a gate
                Wall::removeWall((Wall*)entity);
                Gate::gates.push_back(new Gate(targetCoordinates));
                field->addPrintPawn(Gate::gates.back());
            } else if (mode == Mode::COLLECT) {
                // Collect the wall
                inventory.walls += wall->strength;
                Wall::removeWall((Wall*)entity);
            }
            return;
        } else if (entity->type == Type::CHEST) {
            inventory += ((Chest*)entity)->inventory;
            Chest::removeChest((Chest*)entity);
        } else if (entity->type == Type::MINE) {
            Mine* mine = (Mine*)entity;
            mine->triggered = true;
        } else if (entity->type == Type::TRAP) {
            if (mode == Mode::BULLET) {
                inventory.eggs--;
                Trap::removeTrap((Trap*)entity);
            }
        } else if (entity->type == Type::WEASEL) {
            if (mode == Mode::BULLET) {
                inventory.eggs--;
                inventory.meat++;
                Weasel::removeWeasel((Weasel*)entity);
            } else if (mode == Mode::COLLECT) {
                inventory.meat++;
                Weasel::removeWeasel((Weasel*)entity);
            }
        } else if (entity->type == Type::SNAKE) {
            if (mode == Mode::BULLET) {
                inventory.eggs--;
                inventory.meat++;
                Snake::removeSnake((Snake*)entity);
            } else if (mode == Mode::COLLECT) {
                inventory.meat++;
                Snake::removeSnake((Snake*)entity);
            }
        } else if (entity->type == Type::GATE) {
            if (mode == Mode::BULLET) {
                Gate::removeGate((Gate*)entity);
            }
        } else if (entity->type == Type::CHICKEN) {
            if (mode == Mode::BULLET) {
                inventory.eggs--;
                inventory.meat++;
                Chicken::removeChicken((Chicken*)entity);
            } else if (mode == Mode::COLLECT) {
                inventory.meat++;
                Chicken::removeChicken((Chicken*)entity);
            }
        } else if (entity->type == Type::EGG) {
            if (mode == Mode::BULLET) {
                inventory.eggs--;
                Egg::removeEgg((Egg*)entity);
            } else if (mode == Mode::COLLECT) {
                inventory.eggs++;
                Egg::removeEgg((Egg*)entity);
            }
        }
    } else if (field->isFree(targetCoordinates)) {
        if (mode == Mode::BULLET) {
            inventory.eggs--;
            Bullet::bullets.push_back(new Bullet(targetCoordinates, direction));
            field->addPrintPawn(Bullet::bullets.back());
        } else if (mode == Mode::DUMPCHEST) {
            Chest::chests.push_back(new Chest(targetCoordinates, inventory));
            field->addPrintPawn(Chest::chests.back());
            inventory = {0, 0};
        } else if (mode == Mode::WALL) {
            if (inventory.walls > 0) {
                Wall::walls.push_back(new Wall(targetCoordinates, 3));
                field->addPrintPawn(Wall::walls.back());
                inventory.walls--;
            }
        } else if (mode == Mode::GATE) {
            inventory.walls -= 2;
            inventory.eggs--;
            Gate::gates.push_back(new Gate(targetCoordinates));
            field->addPrintPawn(Gate::gates.back());
        } else if (mode == Mode::TRAP) {
            inventory.walls--;
            inventory.eggs--;
            Trap::traps.push_back(new Trap(targetCoordinates));
            field->addPrintPawn(Trap::traps.back());
        } else if (mode == Mode::MINE) {
            inventory.walls--;
            inventory.eggs -= 3;
            Mine::mines.push_back(new Mine(targetCoordinates));
            field->addPrintPawn(Mine::mines.back());
        } else if (mode == Mode::HATCH) {
            if (inventory.eggs > 0) {
                if (Egg::hatchingDistribution(rng)) {
                    Chicken::chickens.push_back(new Chicken(targetCoordinates));
                    field->addPrintPawn(Chicken::chickens.back());
                }
                inventory.eggs--;
            }
        }
    }
}


ANSI::Settings Bullet::bulletStyle = {
    ANSI::ForegroundColor::F_MAGENTA,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Bullet::removeBullet(Bullet* bullet) {
    Bullet::bullets.erase(std::find(Bullet::bullets.begin(), Bullet::bullets.end(), bullet));
    field->erasePawn(bullet);
    delete bullet;
}
Bullet::Bullet() : Entity(' ', {0, 0}, bulletStyle, Type::BULLET), direction(Direction::RIGHT), speed(1) {}
Bullet::Bullet(sista::Coordinates coordinates, Direction direction) : Entity(directionSymbol[direction], coordinates, bulletStyle, Type::BULLET), direction(direction), speed(1) {}
Bullet::Bullet(sista::Coordinates coordinates, Direction direction, unsigned short speed) : Entity(directionSymbol[direction], coordinates, bulletStyle, Type::BULLET), direction(direction), speed(speed) {}
void Bullet::move() {
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction]*speed;
    if (field->isOutOfBounds(nextCoordinates)) {
        Bullet::removeBullet(this);
        return;
    } else if (field->isFree(nextCoordinates)) {
        field->movePawn(this, nextCoordinates);
        coordinates = nextCoordinates;
        return;
    } else { // Something was hitten
        Entity* hitten = (Entity*)field->getPawn(nextCoordinates);
        if (hitten->type == Type::WALL) {
            Wall* wall = (Wall*)hitten;
            wall->strength--;
            if (wall->strength == 0) {
                wall->setSymbol('@'); // Change the symbol to '@' to indicate that the wall was destroyed
                field->rePrintPawn(wall); // It will be reprinted in the next frame and then removed because of (strength == 0)
            }
        } else if (hitten->type == Type::ARCHER) {
            // debug << "\tZombie" << std::endl;
            Archer::removeArcher((Archer*)hitten);
            // debug << "\tZombie removed" << std::endl;
        } else if (hitten->type == Type::WALKER) {
            // debug << "\tWalker" << std::endl;
            Walker::removeWalker((Walker*)hitten);
            // debug << "\tWalker removed" << std::endl;
        } else if (hitten->type == Type::BULLET) {
            // debug << "\tBullet" << std::endl;
            ((Bullet*)hitten)->collided = true;
            // debug << "\tBullet collided" << std::endl;
            // Bullet::removeBullet((Bullet*)hitten);
            return;
        } else if (hitten->type == Type::ENEMYBULLET) {
            // When two bullets collide, their "collided" attribute is set to true
            // debug << "\tEnemy bullet" << std::endl;
            ((EnemyBullet*)hitten)->collided = true;
            // debug << "\tEnemy bullet collided" << std::endl;
            collided = true;
            // debug << "\tBullet collided" << std::endl;
            return;
        } else if (hitten->type == Type::MINE) {
            // debug << "\tMine" << std::endl;
            Mine* mine = (Mine*)hitten;
            mine->triggered = true;
            // debug << "\tMine triggered" << std::endl;
        } else if (hitten->type == Type::CHEST) {
            // debug << "\tChest" << std::endl;
            Chest::removeChest((Chest*)hitten);
            // debug << "\tChest removed" << std::endl;
        } else if (hitten->type == Type::TRAP) {
            // debug << "\tTrap" << std::endl;
            Trap::removeTrap((Trap*)hitten);
            // debug << "\tTrap removed" << std::endl;
        } else if (hitten->type == Type::WEASEL) {
            // debug << "\tWeasel" << std::endl;
            Weasel::removeWeasel((Weasel*)hitten);
            // debug << "\tWeasel removed" << std::endl;
        } else if (hitten->type == Type::SNAKE) {
            // debug << "\tSnake" << std::endl;
            Snake::removeSnake((Snake*)hitten);
            // debug << "\tSnake removed" << std::endl;
        } else if (hitten->type == Type::GATE) {
            // debug << "\tGate" << std::endl;
            Gate::removeGate((Gate*)hitten);
            // debug << "\tGate removed" << std::endl;
        } else if (hitten->type == Type::CHICKEN) {
            // debug << "\tChicken" << std::endl;
            Chicken::removeChicken((Chicken*)hitten);
            // debug << "\tChicken removed" << std::endl;
        } else if (hitten->type == Type::EGG) {
            // debug << "\tEgg" << std::endl;
            Egg::removeEgg((Egg*)hitten);
            // debug << "\tEgg removed" << std::endl;
        }
        // debug << "\tAfter collision" << std::endl;
        Bullet::removeBullet(this);
        // debug << "\tAfter remove" << std::endl;
    }
}


ANSI::Settings EnemyBullet::enemyBulletStyle = {
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
EnemyBullet::EnemyBullet(sista::Coordinates coordinates, Direction direction, unsigned short speed) : Entity(directionSymbol[direction], coordinates, enemyBulletStyle, Type::BULLET), direction(direction), speed(speed) {}
EnemyBullet::EnemyBullet(sista::Coordinates coordinates, Direction direction) : Entity(directionSymbol[direction], coordinates, enemyBulletStyle, Type::BULLET), direction(direction), speed(1) {}
EnemyBullet::EnemyBullet() : Entity(' ', {0, 0}, enemyBulletStyle, Type::ENEMYBULLET), direction(Direction::UP), speed(1) {}
void EnemyBullet::removeEnemyBullet(EnemyBullet* enemyBullet) {
    EnemyBullet::enemyBullets.erase(std::find(EnemyBullet::enemyBullets.begin(), EnemyBullet::enemyBullets.end(), enemyBullet));
    field->erasePawn(enemyBullet);
    delete enemyBullet;
}
void EnemyBullet::move() { // Pretty sure there's a segfault here
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction]*speed;
    if (field->isOutOfBounds(nextCoordinates)) {
        EnemyBullet::removeEnemyBullet(this);
        return;
    } else if (field->isFree(nextCoordinates)) {
        field->movePawn(this, nextCoordinates);
        coordinates = nextCoordinates;
        return;
    } else { // Something was hitten
        Entity* hitten = (Entity*)field->getPawn(nextCoordinates);
        if (hitten->type == Type::PLAYER) {
            // lose();
            end = true;
        } else if (hitten->type == Type::WALL) {
            Wall* wall = (Wall*)hitten;
            wall->strength--;
            if (wall->strength == 0) {
                wall->setSymbol('@'); // Change the symbol to '@' to indicate that the wall was destroyed
                field->rePrintPawn(wall); // It will be reprinted in the next frame and then removed because of (strength == 0)
            }
        } else if (hitten->type == Type::BULLET) {
            ((Bullet*)hitten)->collided = true;
            collided = true;
            return;
        } else if (hitten->type == Type::ARCHER || hitten->type == Type::WALKER) {
            // No friendly fire
        } else if (hitten->type == Type::ENEMYBULLET) {
            collided = true;
            return;
        } else if (hitten->type == Type::MINE) {
            Mine* mine = (Mine*)hitten;
            mine->triggered = true;
        } else if (hitten->type == Type::CHEST) {
            Chest::removeChest((Chest*)hitten);
        } else if (hitten->type == Type::TRAP) {
            Trap::removeTrap((Trap*)hitten);
        } else if (hitten->type == Type::WEASEL) {
            Weasel::removeWeasel((Weasel*)hitten);
        } else if (hitten->type == Type::SNAKE) {
            Snake::removeSnake((Snake*)hitten);
        } else if (hitten->type == Type::GATE) {
            Gate::removeGate((Gate*)hitten);
        } else if (hitten->type == Type::CHICKEN) {
            Chicken::removeChicken((Chicken*)hitten);
        } else if (hitten->type == Type::EGG) {
            Egg::removeEgg((Egg*)hitten);
        }
        EnemyBullet::removeEnemyBullet(this);
    }
}

ANSI::Settings Mine::mineStyle = {
    ANSI::ForegroundColor::F_MAGENTA,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BLINK   
};
void Mine::removeMine(Mine* mine) {
    Mine::mines.erase(std::find(Mine::mines.begin(), Mine::mines.end(), mine));
    field->erasePawn(mine);
    delete mine;
}
Mine::Mine(sista::Coordinates coordinates) : Entity('*', coordinates, mineStyle, Type::MINE), triggered(false) {}
Mine::Mine() : Entity('*', {0, 0}, mineStyle, Type::MINE), triggered(false) {}
bool Mine::checkTrigger() {
    for (int j=-1; j<=1; j++) {
        for (int i=-1; i<=1; i++) {
            if (i == 0 && j == 0) continue;
            sista::Coordinates nextCoordinates = coordinates + sista::Coordinates(j, i);
            if (field->isOutOfBounds(nextCoordinates)) continue;
            Entity* neighbor = (Entity*)field->getPawn(nextCoordinates);
            if (neighbor == nullptr) {
                continue;
            } else if (neighbor->type == Type::ARCHER || neighbor->type == Type::WALKER) {
                trigger();
                return true;
            }
        }
    }
    return false;
}
void Mine::trigger() {
    triggered = true;
    symbol = '%';
    settings.foregroundColor = ANSI::ForegroundColor::F_WHITE;
}
void Mine::explode() {
    for (int j=-2; j<=2; j++) {
        for (int i=-2; i<=2; i++) {
            if (i == 0 && j == 0) continue;
            sista::Coordinates nextCoordinates = coordinates + sista::Coordinates(j, i);
            if (field->isOutOfBounds(nextCoordinates)) continue;
            Entity* neighbor = (Entity*)field->getPawn(nextCoordinates);
            if (neighbor == nullptr) {
                continue;
            } else if (neighbor->type == Type::ARCHER) {
                Archer::removeArcher((Archer*)neighbor);
            } else if (neighbor->type == Type::WALKER) {
                Walker::removeWalker((Walker*)neighbor);
            } else if (neighbor->type == Type::ENEMYBULLET) {
                EnemyBullet::removeEnemyBullet((EnemyBullet*)neighbor);
            } else if (neighbor->type == Type::MINE) {
                Mine* mine = (Mine*)neighbor;
                mine->triggered = true;
            } else if (neighbor->type == Type::CHEST) {
                Chest::removeChest((Chest*)neighbor);
            } else if (neighbor->type == Type::TRAP) {
                Trap::removeTrap((Trap*)neighbor);
            } else if (neighbor->type == Type::WEASEL) {
                Weasel::removeWeasel((Weasel*)neighbor);
            } else if (neighbor->type == Type::SNAKE) {
                Snake::removeSnake((Snake*)neighbor);
            } else if (neighbor->type == Type::GATE) {
                Gate::removeGate((Gate*)neighbor);
            } else if (neighbor->type == Type::CHICKEN) {
                Chicken::removeChicken((Chicken*)neighbor);
            } else if (neighbor->type == Type::EGG) {
                Egg::removeEgg((Egg*)neighbor);
            } else if (neighbor->type == Type::WALL) {
                Wall* wall = (Wall*)neighbor;
                int damage = rand() % 3 + 1;
                if (wall->strength <= damage) {
                    wall->strength = 0;
                    wall->setSymbol('@'); // Change the symbol to '@' to indicate that the wall was destroyed
                    field->rePrintPawn(wall); // It will be reprinted in the next frame and then removed because of (strength == 0)
                } else {
                    wall->strength -= damage;
                }
            }
        }
    }
}

ANSI::Settings Chest::chestStyle = {
    ANSI::ForegroundColor::F_YELLOW,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Chest::removeChest(Chest* chest) {
    Chest::chests.erase(std::find(Chest::chests.begin(), Chest::chests.end(), chest));
    field->erasePawn(chest);
    delete chest;
}
Chest::Chest(sista::Coordinates coordinates, Inventory& inventory) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {}
Chest::Chest() : Entity('C', {0, 0}, chestStyle, Type::CHEST), inventory({0, 0}) {}

ANSI::Settings Trap::trapStyle = {
    ANSI::ForegroundColor::F_CYAN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Trap::removeTrap(Trap* trap) {
    Trap::traps.erase(std::find(Trap::traps.begin(), Trap::traps.end(), trap));
    field->erasePawn(trap);
    delete trap;
}
Trap::Trap(sista::Coordinates coordinates) : Entity('T', coordinates, trapStyle, Type::TRAP) {}
Trap::Trap() : Entity('T', {0, 0}, trapStyle, Type::TRAP) {}

ANSI::Settings Weasel::weaselStyle = {
    ANSI::ForegroundColor::F_WHITE,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Weasel::removeWeasel(Weasel* weasel) {
    Weasel::weasels.erase(std::find(Weasel::weasels.begin(), Weasel::weasels.end(), weasel));
    field->erasePawn(weasel);
    delete weasel;
}
Weasel::Weasel(sista::Coordinates coordinates, Direction direction) : Entity('}', coordinates, weaselStyle, Type::WEASEL), direction(direction) {
    symbol = (direction == Direction::RIGHT) ? '}' : '{';
}
Weasel::Weasel() : Entity('}', {0, 0}, weaselStyle, Type::WEASEL), direction(Direction::RIGHT) {
    symbol = (direction == Direction::RIGHT) ? '}' : '{';
}
void Weasel::move() {
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction];
    if (field->isOutOfBounds(nextCoordinates)) {
        crossed = true;
        return;
    } else if (field->isOccupied(nextCoordinates)) {
        Entity* entity = (Entity*)field->getPawn(nextCoordinates);
        if (entity->type == Type::WALL) {
            direction = (direction == Direction::RIGHT) ? Direction::LEFT : Direction::RIGHT;
            symbol = (direction == Direction::RIGHT) ? '}' : '{';
            return;
        } else if (entity->type == Type::CHEST) {
            // Steal some meat from the chest
        } else if (entity->type == Type::MINE) {
            Mine* mine = (Mine*)entity;
            mine->triggered = true;
        } else if (entity->type == Type::TRAP) {
            caught = true; // The weasel will be removed
            return;
        } else if (entity->type == Type::WEASEL) {
            // They will both be scared and run away
        } else if (entity->type == Type::SNAKE) {
            crossed = true; // The snake kills the weasel
            return;
        } else if (entity->type == Type::GATE) {
            if (day) {
                // passing through the gate
                this->move();
                return;
            }
        } else if (entity->type == Type::CHICKEN) {
            // Eat the chicken
            Chicken::removeChicken((Chicken*)entity);
        } else if (entity->type == Type::EGG) {
            // Jump over the egg or the gate
            this->move();
            return;
        }
        direction = (direction == Direction::RIGHT) ? Direction::LEFT : Direction::RIGHT;
        symbol = (direction == Direction::RIGHT) ? '}' : '{';
        return;
    }
    field->movePawn(this, nextCoordinates);
    coordinates = nextCoordinates;
}

ANSI::Settings Snake::snakeStyle = {
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Snake::removeSnake(Snake* snake) {
    Snake::snakes.erase(std::find(Snake::snakes.begin(), Snake::snakes.end(), snake));
    field->erasePawn(snake);
    delete snake;
}
Snake::Snake(sista::Coordinates coordinates, Direction direction) : Entity('~', coordinates, snakeStyle, Type::SNAKE), direction(direction) {}
Snake::Snake() : Entity('~', {0, 0}, snakeStyle, Type::SNAKE), direction(Direction::RIGHT) {}
void Snake::move() {
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction];
    if (field->isOutOfBounds(nextCoordinates)) {
        crossed = true;
        return;
    } else if (field->isOccupied(nextCoordinates)) {
        Entity* entity = (Entity*)field->getPawn(nextCoordinates);
        if (entity->type == Type::WALL) {
            direction = (direction == Direction::RIGHT) ? Direction::LEFT : Direction::RIGHT;
            return;
        } else if (entity->type == Type::CHEST) {
            Chest::removeChest((Chest*)entity);
        } else if (entity->type == Type::MINE) {
            Mine* mine = (Mine*)entity;
            mine->triggered = true;
        } else if (entity->type == Type::TRAP) {
            Trap::removeTrap((Trap*)entity);
        } else if (entity->type == Type::WEASEL) {
            Weasel::removeWeasel((Weasel*)entity);
        } else if (entity->type == Type::SNAKE) {
            Snake::removeSnake((Snake*)entity);
        } else if (entity->type == Type::GATE) {
            Gate::removeGate((Gate*)entity);
        } else if (entity->type == Type::CHICKEN) {
            Chicken::removeChicken((Chicken*)entity);
        } else if (entity->type == Type::EGG) {
            Egg::removeEgg((Egg*)entity);
        }
        direction = (direction == Direction::RIGHT) ? Direction::LEFT : Direction::RIGHT;
        return;
    }
    field->movePawn(this, nextCoordinates);
    coordinates = nextCoordinates;
}

ANSI::Settings Chicken::chickenStyle = {
    ANSI::ForegroundColor::F_YELLOW,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Chicken::removeChicken(Chicken* chicken) {
    Chicken::chickens.erase(std::find(Chicken::chickens.begin(), Chicken::chickens.end(), chicken));
    field->erasePawn(chicken);
    delete chicken;
}
Chicken::Chicken(sista::Coordinates coordinates) : Entity('%', coordinates, chickenStyle, Type::CHICKEN) {}
Chicken::Chicken() : Entity('%', {0, 0}, chickenStyle, Type::CHICKEN) {}
void Chicken::move() {
    sista::Coordinates nextCoordinates = coordinates + directionMap[Direction::DOWN];
    if (field->isFree(nextCoordinates)) {
        field->movePawn(this, nextCoordinates);
        if (eggDistribution(rng)) {
            Egg::eggs.push_back(new Egg(nextCoordinates));
            field->addPrintPawn(Egg::eggs.back());
        }
        coordinates = nextCoordinates;
    }
}

ANSI::Settings Egg::eggStyle = {
    ANSI::ForegroundColor::F_WHITE,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Egg::removeEgg(Egg* egg) {
    Egg::eggs.erase(std::find(Egg::eggs.begin(), Egg::eggs.end(), egg));
    field->erasePawn(egg);
    delete egg;
}
Egg::Egg(sista::Coordinates coordinates) : Entity('0', coordinates, eggStyle, Type::EGG) {}
Egg::Egg() : Entity('0', {0, 0}, eggStyle, Type::EGG) {}

ANSI::Settings Gate::gateStyle = {
    ANSI::ForegroundColor::F_BLUE,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Gate::removeGate(Gate* gate) {
    Gate::gates.erase(std::find(Gate::gates.begin(), Gate::gates.end(), gate));
    field->erasePawn(gate);
    delete gate;
}
Gate::Gate(sista::Coordinates coordinates) : Entity('=', coordinates, gateStyle, Type::GATE) {}
Gate::Gate() : Entity('=', {0, 0}, gateStyle, Type::GATE) {}

ANSI::Settings Wall::wallStyle = {
    ANSI::ForegroundColor::F_BLACK,
    ANSI::BackgroundColor::B_WHITE,
    ANSI::Attribute::BRIGHT
};
void Wall::removeWall(Wall* wall) {
    Wall::walls.erase(std::find(Wall::walls.begin(), Wall::walls.end(), wall));
    field->erasePawn(wall);
    delete wall;
}
Wall::Wall(sista::Coordinates coordinates, short int strength) : Entity('#', coordinates, wallStyle, Type::WALL), strength(strength) {}
Wall::Wall() : Entity('#', {0, 0}, wallStyle, Type::WALL), strength(1) {}

ANSI::Settings Walker::walkerStyle = {
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::FAINT
};
void Walker::removeWalker(Walker* walker) {
    Walker::walkers.erase(std::find(Walker::walkers.begin(), Walker::walkers.end(), walker));
    field->erasePawn(walker);
    delete walker;
}
Walker::Walker(sista::Coordinates coordinates) : Entity('W', coordinates, walkerStyle, Type::WALKER) {}
Walker::Walker() : Entity('W', {0, 0}, walkerStyle, Type::WALKER) {}

ANSI::Settings Archer::archerStyle = {
    ANSI::ForegroundColor::F_RED,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};
void Archer::removeArcher(Archer* archer) {
    Archer::archers.erase(std::find(Archer::archers.begin(), Archer::archers.end(), archer));
    field->erasePawn(archer);
    delete archer;
}
Archer::Archer(sista::Coordinates coordinates) : Entity('A', coordinates, archerStyle, Type::ARCHER) {}
Archer::Archer() : Entity('A', {0, 0}, archerStyle, Type::ARCHER) {}

void removeNullptrs(std::vector<Entity*>& entities) {
    for (unsigned i=0; i<entities.size(); i++) {
        if (entities[i] == nullptr) {
            entities.erase(entities.begin() + i);
            i--;
        }
    }
}