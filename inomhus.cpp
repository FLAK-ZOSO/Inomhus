#include "cross_platform.hpp"
#include "inomhus.hpp"
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>

#define DAY_DURATION 500
#define NIGHT_DURATION 200

#define WIDTH 70
#define HEIGHT 30

#define VERSION "BETA"
#define DATE "2024-10-12"

Player* Player::player;
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
std::bernoulli_distribution Chicken::eggDistribution(0.005); // 0.5%
std::bernoulli_distribution Chicken::movingDistribution(0.75); // 75%
std::bernoulli_distribution Archer::movingDistribution(0.25); // 25%
std::bernoulli_distribution Archer::shootDistribution(0.005); // 0.5%
std::bernoulli_distribution Walker::movingDistribution(0.30); // 30%

std::bernoulli_distribution walkerSpawnDistribution(0.004); // 0.4%
std::bernoulli_distribution archerSpawnDistribution(0.007); // 0.7%
std::bernoulli_distribution weaselSpawnDistribution(0.01); // 1%
std::bernoulli_distribution snakeSpawnDistribution(0.01); // 1%
// std::bernoulli_distribution chickenSpawnDistribution(0.05); // Chicken don't randomly spawn
std::bernoulli_distribution wallSpawnDistribution(0.01); // 1%

std::vector<char> gameControlKeys = {
    '+', '-', '.', 'p', 'P', 'Q'
};
std::vector<char> gameKeys = {
    'w', 'W', 'a', 'A', 's', 'S', 'd', 'D',
    'j', 'J', 'k', 'K', 'l', 'L', 'i', 'I',
    'c', 'C', 'b', 'B', 'e', 'E', 'q',
    '=', '0', '#', 'g', 'G', 't', 'T', 'm', 'M', '*',
    'h', 'H'
};
std::vector<char> acceptedKeys = {
    'w', 'W', 'a', 'A', 's', 'S', 'd', 'D',
    'j', 'J', 'k', 'K', 'l', 'L', 'i', 'I',
    'c', 'C', 'b', 'B', 'e', 'E', 'q', 'Q',
    '=', '0', '#', 'g', 'G', 't', 'T', 'm', 'M', '*',
    'h', 'H',
    '+', '-', '.', 'p', 'P', 'Q'
};

sista::SwappableField* field;
sista::Cursor cursor;
sista::Border border(
    '@', {
        ANSI::ForegroundColor::F_BLACK,
        ANSI::BackgroundColor::B_WHITE,
        ANSI::Attribute::BRIGHT
    }
);
bool speedup = false;
bool pause_ = false;
bool end = false;
bool day = true;


int main(int argc, char** argv) {
    #ifdef __APPLE__
        term_echooff();
        system("stty raw -echo");
    #endif
    std::ios_base::sync_with_stdio(false);
    ANSI::reset(); // Reset the settings
    srand(time(0)); // Seed the random number generator


    sista::SwappableField field_(WIDTH, HEIGHT);
    field = &field_;
    field->clear();
    printIntro();
    field->addPrintPawn(Player::player = new Player({0, 0}));
    populate(field);
    sista::clearScreen();
    field->print(border);

    std::thread th(input);
    int dayCountdown = NIGHT_DURATION;
    int nightCountdown = DAY_DURATION;
    for (int i=0; !end; i++) {
        if (pause_) {
            while (pause_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        if (speedup) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (day) {
            nightCountdown--;
        } else {
            dayCountdown--;
            // Implement lycanthropy for the user, randomly picking a game key
            char key = gameKeys[rand() % gameKeys.size()];
            act(key);
        } // This could be golfed lol, but it's clearer this way
        if (dayCountdown <= 0 || nightCountdown <= 0) {
            day = !day;
            dayCountdown = NIGHT_DURATION;
            nightCountdown = DAY_DURATION;
            if (day) {
                // The day is back, but the out-of-control player destroys the inventory
                Player::player->inventory = (Inventory){0, 0, 0};
                Player::player->setSettings(Player::playerStyle);
                border = sista::Border(
                    '@', {
                        ANSI::ForegroundColor::F_BLACK,
                        ANSI::BackgroundColor::B_WHITE,
                        ANSI::Attribute::BRIGHT
                    }
                );
            } else {
                // The player is now out of control
                Player::player->setSettings(nightPlayerStyle);
                border = sista::Border(
                    '@', {
                        ANSI::ForegroundColor::F_WHITE,
                        ANSI::BackgroundColor::B_BLACK,
                        ANSI::Attribute::BRIGHT
                    }
                );
            }
            sista::clearScreen();
            field->print(border);
        }
        std::vector<sista::Coordinates> coordinates;
        for (unsigned short j=0; j<HEIGHT; j++) {
            for (unsigned short i=0; i<WIDTH; i++) {
                Entity* pawn = (Entity*)field->getPawn(j, i);
                if (pawn == nullptr) continue;
                if (std::find(Bullet::bullets.begin(), Bullet::bullets.end(), pawn) == Bullet::bullets.end() &&
                    std::find(EnemyBullet::enemyBullets.begin(), EnemyBullet::enemyBullets.end(), pawn) == EnemyBullet::enemyBullets.end() &&
                    std::find(Archer::archers.begin(), Archer::archers.end(), pawn) == Archer::archers.end() &&
                    std::find(Walker::walkers.begin(), Walker::walkers.end(), pawn) == Walker::walkers.end() &&
                    std::find(Wall::walls.begin(), Wall::walls.end(), pawn) == Wall::walls.end() &&
                    std::find(Mine::mines.begin(), Mine::mines.end(), pawn) == Mine::mines.end() &&
                    std::find(Gate::gates.begin(), Gate::gates.end(), pawn) == Gate::gates.end() &&
                    std::find(Weasel::weasels.begin(), Weasel::weasels.end(), pawn) == Weasel::weasels.end() &&
                    std::find(Snake::snakes.begin(), Snake::snakes.end(), pawn) == Snake::snakes.end() &&
                    std::find(Chicken::chickens.begin(), Chicken::chickens.end(), pawn) == Chicken::chickens.end() &&
                    std::find(Egg::eggs.begin(), Egg::eggs.end(), pawn) == Egg::eggs.end() &&
                    std::find(Chest::chests.begin(), Chest::chests.end(), pawn) == Chest::chests.end() &&
                    std::find(Trap::traps.begin(), Trap::traps.end(), pawn) == Trap::traps.end() &&
                    pawn != Player::player) {
                    coordinates.push_back(pawn->getCoordinates());
                    #if DEBUG
                    debug << "Erasing " << pawn << " at " << pawn->getCoordinates() << std::endl;
                    debug << "\t" << typeid(*pawn).name() << std::endl;
                    #endif
                }
            }
        }
        /*
        PLAYER, BULLET, WALL,
        GATE, CHEST, TRAP,
        MINE, WALKER, ARCHER,
        ENEMYBULLET, WEASEL,
        SNAKE, CHICKEN, EGG
        */
        for (unsigned j=0; j<Bullet::bullets.size(); j++) {
            if (j >= Bullet::bullets.size()) break;
            Bullet* bullet = Bullet::bullets[j];
            if (bullet == nullptr) continue;
            if (bullet->collided) continue;
            bullet->move();
        }
        for (auto enemyBullet : EnemyBullet::enemyBullets) {
            if (enemyBullet->collided) {
                EnemyBullet::removeEnemyBullet(enemyBullet);
            }
        }
        for (auto bullet : Bullet::bullets) {
            if (bullet->collided) {
                Bullet::removeBullet(bullet);
            }
        }
        for (unsigned j=0; j<EnemyBullet::enemyBullets.size(); j++) {
            if (j >= EnemyBullet::enemyBullets.size()) break;
            EnemyBullet* enemyBullet = EnemyBullet::enemyBullets[j];
            if (enemyBullet == nullptr) continue;
            if (enemyBullet->collided) continue;
            enemyBullet->move();
        }
        for (auto enemyBullet : EnemyBullet::enemyBullets) {
            if (enemyBullet->collided) {
                EnemyBullet::removeEnemyBullet(enemyBullet);
            }
        }
        for (auto bullet : Bullet::bullets) {
            if (bullet->collided) {
                Bullet::removeBullet(bullet);
            }
        }
        for (auto mine : Mine::mines) {
            if (mine->triggered) {
                mine->explode();
            }
        }
        for (auto mine : Mine::mines) {
            if (!mine->alive) {
                Mine::removeMine(mine);
            } else {
                mine->checkTrigger();
            }
        }
        for (unsigned c=0; c<Chest::chests.size(); c++) {
            if (c >= Chest::chests.size()) break;
            Chest* chest = Chest::chests[c];
            if (chest == nullptr) continue;
            if (chest->inventory.walls == 0 && chest->inventory.eggs == 0 && chest->inventory.meat == 0) {
                Chest::removeChest(chest);
            }
        }
        for (unsigned c=0; c<Chicken::chickens.size(); c++) {
            if (c >= Chicken::chickens.size()) break;
            Chicken* chicken = Chicken::chickens[c];
            if (chicken == nullptr) continue;
            if (Chicken::movingDistribution(rng)) {
                chicken->move();
            }
        }
        for (unsigned w=0; w<Walker::walkers.size(); w++) {
            if (w >= Walker::walkers.size()) break;
            Walker* walker = Walker::walkers[w];
            if (walker == nullptr) continue;
            if (Walker::movingDistribution(rng)) {
                walker->move();
            }
        }
        for (unsigned a=0; a<Archer::archers.size(); a++) {
            if (a >= Archer::archers.size()) break;
            Archer* archer = Archer::archers[a];
            if (archer == nullptr) continue;
            if (Archer::movingDistribution(rng)) {
                archer->move();
            }
            if (Archer::shootDistribution(rng)) {
                archer->shoot();
            }
        }
        for (unsigned w=0; w<Weasel::weasels.size(); w++) {
            if (w >= Weasel::weasels.size()) break;
            Weasel* weasel = Weasel::weasels[w];
            if (weasel == nullptr) continue;
            weasel->move();
        }
        for (unsigned s=0; s<Snake::snakes.size(); s++) {
            if (s >= Snake::snakes.size()) break;
            Snake* snake = Snake::snakes[s];
            if (snake == nullptr) continue;
            snake->move();
        }
        for (unsigned w=0; w<Wall::walls.size(); w++) {
            if (w >= Wall::walls.size()) break;
            Wall* wall = Wall::walls[w];
            if (wall == nullptr) continue;
            if (wall->strength == 0) {
                Wall::removeWall(wall);
            }
        }
        // Iterate over wild animals to see if they have reached the other side of the field or they have been caught
        for (unsigned w=0; w<Weasel::weasels.size(); w++) {
            if (w >= Weasel::weasels.size()) break;
            Weasel* weasel = Weasel::weasels[w];
            if (weasel == nullptr) continue;
            if (weasel->crossed) {
                Weasel::removeWeasel(weasel);
            } else if (weasel->caught) {
                Weasel::removeWeasel(weasel);
                Player::player->inventory.meat++;
            }
        }
        for (unsigned s=0; s<Snake::snakes.size(); s++) {
            if (s >= Snake::snakes.size()) break;
            Snake* snake = Snake::snakes[s];
            if (snake == nullptr) continue;
            if (snake->crossed) {
                Snake::removeSnake(snake);
            }
        }

        if (i % 128 == 127) {
            // repopulate the field from scratch for preventing nullptr pawns from laying around
            repopulate(field);
        }
        #if __linux__
        if (i % 10 == 9) {
        #elif __APPLE__ or _WIN32
        if (i % 100 == 99) {
        #endif
            // reprint the field every 10 frames
            sista::clearScreen();
            field->print(border);
            printSideInstructions();
        }
        // Spawn new entities
        spawnNew(field);

        // Print the inventory
        ANSI::reset();
        cursor.set(10, 80);
        ANSI::setAttribute(ANSI::Attribute::BRIGHT);
        std::cout << "Inventory\n";
        ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
        cursor.set(11, 80);
        std::cout << "Walls: " << Player::player->inventory.walls << "   \n";
        cursor.set(12, 80);
        std::cout << "Eggs: " << Player::player->inventory.eggs << "   \n";
        cursor.set(13, 80);
        std::cout << "Meat: " << Player::player->inventory.meat << "   \n";
        cursor.set(14, 80);
        std::cout << "Mode: ";
        switch (Player::player->mode) {
            case Player::Mode::COLLECT:
                std::cout << "Collect";
                break;
            case Player::Mode::BULLET:
                std::cout << "Bullet";
                break;
            case Player::Mode::DUMPCHEST:
                std::cout << "Dump chest";
                break;
            case Player::Mode::WALL:
                std::cout << "Wall";
                break;
            case Player::Mode::GATE:
                std::cout << "Gate";
                break;
            case Player::Mode::TRAP:
                std::cout << "Trap";
                break;
            case Player::Mode::MINE:
                std::cout << "Mine";
                break;
            case Player::Mode::HATCH:
                std::cout << "Hatch";
                break;
        }
        std::cout << "      ";
        cursor.set(18, 80);
        ANSI::setAttribute(ANSI::Attribute::BRIGHT);
        std::cout << "Time survived: " << i << "    \n";
        ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
        cursor.set(20, 80);
        ANSI::setAttribute(ANSI::Attribute::BRIGHT);
        std::cout << "Time before ";
        if (day) {
            std::cout << "night: " << nightCountdown << "    \n";
        } else {
            std::cout << "day: " << dayCountdown << "    \n";
        }
        ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
        std::flush(std::cout);
    }

    th.join();
    cursor.set(72, 0); // Move the cursor to the bottom of the screen, so the terminal is not left in a weird state
    #if __linux__
        getch();
    #elif __APPLE__
        getchar();
    #elif _WIN32
        getch();
    #endif
    #ifdef __APPLE__
        system("stty -raw echo");
        tcsetattr(0, TCSANOW, &orig_termios);
    #endif
}

void input() {
    char input = '_';
    while (input != 'Q' /*&& input != 'q'*/) {
        if (end) return;
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
        if (end) return;
        if (day) {
            act(input);
        } else {
            // At night only game control keys are processed
            switch (input) {
                case '+': case '-':
                    speedup = !speedup;
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
    }
}

void act(char input) {
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

        case '+': case '-':
            speedup = !speedup;
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

void printIntro() {
    std::cout << CLS; // Clear screen
    std::cout << SSB; // Clear scrollback buffer
    field->print(border);
    cursor.set(5, 0);
    // if linux
    #if __linux__ or __APPLE__
        std::cout << "\x1b]2;Bättre att stanna Inomhus\x07"; // Set window title
        std::cout << "\t\t\t  \x1b[3mInomhus\x1b[0m\n\t\t\x1b[3mYou better be at home by night\x1b[0m\n\n";
        std::cout << "\t  \x1b[31;1mInomhus v" << VERSION << "\t\tFLAK-ZOSO " << DATE << "\x1b[0m\n\n";
    #elif defined(_WIN32)
        std::cout << "\x1b]2;de dodas angrepp\x07"; // Set window title
        std::cout << "\t\t\t  \x1b[3mInomhus\x1b[0m\n\t\t\x1b[3mYou better be at home by night\x1b[0m\n\n";
        std::cout << "\t  \x1b[31;1mInomhus v" << VERSION << "\t\tFLAK-ZOSO " << DATE << "\x1b[0m\n\n";
    #endif

    std::cout << "\t  https://github.com/FLAK-ZOSO/Inomhus\n\n";
    std::cout << "\t- '\x1b[35m.\x1b[0m' or '\x1b[35mp\x1b[0m' or '\x1b[35mP\x1b[0m' to pause or resume\n";
    std::cout << "\t- '\x1b[35mw\x1b[0m | \x1b[35ma\x1b[0m | \x1b[35ms\x1b[0m | \x1b[35md\x1b[0m' to step\n";
    std::cout << "\t- '\x1b[35mi\x1b[0m | \x1b[35mj\x1b[0m | \x1b[35mk\x1b[0m | \x1b[35ml\x1b[0m' to shoot, deposit, collect\n";
    std::cout << "\t- '\x1b[35mb\x1b[0m' to select bullets (well, to throw eggs)\n";
    std::cout << "\t- '\x1b[35mm\x1b[0m' to select mines\n";
    std::cout << "\t- '\x1b[35m+\x1b[0m' or '\x1b[35m-\x1b[0m' to enter or exit speedup mode\n";
    std::cout << "\t- '\x1b[35m=\x1b[0m' or '\x1b[35m0\x1b[0m' or '\x1b[35m#\x1b[0m' to select walls\n";
    std::cout << "\t- '\x1b[35mg\x1b[0m' or '\x1b[35mG\x1b[0m' to select gates\n";
    std::cout << "\t- '\x1b[35mt\x1b[0m' or '\x1b[35mT\x1b[0m' to select traps\n";
    std::cout << "\t- '\x1b[35mh\x1b[0m' or '\x1b[35mH\x1b[0m' to enter egg-hatching mode\n";
    std::cout << "\t- '\x1b[35mQ\x1b[0m' to quit\n\n";

    std::cout << "\tMake sure to have the terminal full-screen\n\n";

    ANSI::setAttribute(ANSI::Attribute::BLINK);
    std::cout << "\t\t\x1b[3mPress any key to start\x1b[0m";
    std::flush(std::cout);
    #if defined(_WIN32) or defined(__linux__)
        getch();
    #elif __APPLE__
        getchar();
    #endif
    sista::clearScreen();
}

void printSideInstructions() {
    // Be aware not to overwrite the inventory and the time survived which use {10, 80} to ~{18, 80}
    cursor.set(20, 80);
}

void populate(sista::SwappableField* field) {
    // Walls, some randomly around the field and some in a row
    sista::Coordinates coordinates;
    for (int j=0; j<5; j++) {
        int length = rand() % (HEIGHT - 10) + 1;
        int start_column = rand() % (WIDTH - length);
        int row = rand() % (HEIGHT - 10);
        for (int i=0; i<length; i++) {
            coordinates = {row, start_column + i};
            if (field->isFree(coordinates)) {
                Wall::walls.push_back(new Wall(coordinates, rand() % 2 + 1));
                field->addPrintPawn(Wall::walls.back());
            }
        }
    }
    for (int i=0; i<HEIGHT; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Wall::walls.push_back(new Wall(coordinates, rand() % 2 + 1));
            field->addPrintPawn(Wall::walls.back());
        }
    }
    // Chests, a couple of them
    for (int i=0; i<3; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Chest::chests.push_back(new Chest(coordinates, (Inventory){rand() % 5, rand() % 5}, true));
            field->addPrintPawn(Chest::chests.back());
        }
    }
    // Walkers, some randomly around the field, but none of them in a 5x5 square around the player, which starts in {0, 0}
    for (int i=0; i<5; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates) && coordinates.y > 5 && coordinates.x > 5) {
            Walker::walkers.push_back(new Walker(coordinates));
            field->addPrintPawn(Walker::walkers.back());
        }
    }
    // Archers, some randomly around the field, but none in the same row or column as the player
    for (int i=0; i<5; i++) {
        coordinates = {rand() % (HEIGHT - 5) + 5, rand() % (WIDTH - 5) + 5};
        if (field->isFree(coordinates)) {
            Archer::archers.push_back(new Archer(coordinates));
            field->addPrintPawn(Archer::archers.back());
        }
    }
    // Only one Weasel, to be generated from the left side of the field
    coordinates = {rand() % HEIGHT, 0};
    if (field->isFree(coordinates)) {
        Weasel::weasels.push_back(new Weasel(coordinates, Direction::RIGHT));
        field->addPrintPawn(Weasel::weasels.back());
    }
    // Only one Snake, to be generated from the right side of the field
    coordinates = {rand() % (HEIGHT - 10), WIDTH - 1};
    if (field->isFree(coordinates)) {
        Snake::snakes.push_back(new Snake(coordinates, Direction::LEFT));
        field->addPrintPawn(Snake::snakes.back());
    }
    // Some Chickens, randomly around the field
    for (int i=0; i<5; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Chicken::chickens.push_back(new Chicken(coordinates));
            field->addPrintPawn(Chicken::chickens.back());
        }
    }
    // Some Eggs, randomly around the field
    for (int i=0; i<15; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Egg::eggs.push_back(new Egg(coordinates));
            field->addPrintPawn(Egg::eggs.back());
        }
    }
}

void repopulate(sista::SwappableField* field) {
    field->clear();
    field->addPrintPawn(Player::player);
    for (auto wall : Wall::walls) {
        field->addPrintPawn(wall);
    }
    for (auto chest : Chest::chests) {
        field->addPrintPawn(chest);
    }
    for (auto mine : Mine::mines) {
        field->addPrintPawn(mine);
    }
    for (auto trap : Trap::traps) {
        field->addPrintPawn(trap);
    }
    for (auto gate : Gate::gates) {
        field->addPrintPawn(gate);
    }
    for (auto weasel : Weasel::weasels) {
        field->addPrintPawn(weasel);
    }
    for (auto snake : Snake::snakes) {
        field->addPrintPawn(snake);
    }
    for (auto chicken : Chicken::chickens) {
        field->addPrintPawn(chicken);
    }
    for (auto egg : Egg::eggs) {
        field->addPrintPawn(egg);
    }
    for (auto bullet : Bullet::bullets) {
        field->addPrintPawn(bullet);
    }
    for (auto enemyBullet : EnemyBullet::enemyBullets) {
        field->addPrintPawn(enemyBullet);
    }
    for (auto walker : Walker::walkers) {
        field->addPrintPawn(walker);
    }
    for (auto archer : Archer::archers) {
        field->addPrintPawn(archer);
    }
}

void spawnNew(sista::SwappableField* field) {
    if (walkerSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Walker::walkers.push_back(new Walker(coordinates));
            field->addPrintPawn(Walker::walkers.back());
        }
    }
    if (archerSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Archer::archers.push_back(new Archer(coordinates));
            field->addPrintPawn(Archer::archers.back());
        }
    }
    if (weaselSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, 0};
        if (field->isFree(coordinates)) {
            Weasel::weasels.push_back(new Weasel(coordinates, Direction::RIGHT));
            field->addPrintPawn(Weasel::weasels.back());
        }
    }
    if (snakeSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % (HEIGHT - 10), WIDTH - 1};
        if (field->isFree(coordinates)) {
            Snake::snakes.push_back(new Snake(coordinates, Direction::LEFT));
            field->addPrintPawn(Snake::snakes.back());
        }
    }
    if (wallSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Wall::walls.push_back(new Wall(coordinates, 3));
            field->addPrintPawn(Wall::walls.back());
        }
    }
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
ANSI::Settings nightPlayerStyle = {
    ANSI::ForegroundColor::F_BLUE,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::REVERSE
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
        } else if (entity->type == Type::MINE) {
            Mine* mine = (Mine*)entity;
            mine->triggered = true;
            return;
        } else if (entity->type == Type::EGG) {
            Egg::removeEgg((Egg*)entity);
        } else if (entity->type == Type::CHICKEN) {
            inventory.meat++;
            Chicken::removeChicken((Chicken*)entity);
        } else if (entity->type == Type::WEASEL) {
            inventory.meat++;
            Weasel::removeWeasel((Weasel*)entity);
        } else if (entity->type == Type::SNAKE) {
            Snake::removeSnake((Snake*)entity);
        } else if (entity->type == Type::GATE) {
            if (day) {
                // Pass through the gate
                nextCoordinates = coordinates + directionMap[direction]*2;
                if (field->isOutOfBounds(nextCoordinates)) {
                    return;
                } else if (field->isOccupied(nextCoordinates)) {
                    return;
                } else if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            } // else, the gate is closed
        } else if (entity->type == Type::TRAP || entity->type == Type::WALL) {
            return;
        } else if (entity->type == Type::BULLET || entity->type == Type::ENEMYBULLET || entity->type == Type::WALKER || entity->type == Type::ARCHER) {
            end = true;
            return;
        }
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
                if (inventory.eggs <= 0) {
                    return;
                }
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
                if (inventory.eggs > 0) {
                    inventory.eggs--;
                    Trap::removeTrap((Trap*)entity);
                }
            }
        } else if (entity->type == Type::WEASEL) {
            if (mode == Mode::BULLET) {
                if (inventory.eggs > 0) {
                    inventory.eggs--;
                    inventory.meat++;
                    Weasel::removeWeasel((Weasel*)entity);
                }
            } else if (mode == Mode::COLLECT) {
                inventory.meat++;
                Weasel::removeWeasel((Weasel*)entity);
            }
        } else if (entity->type == Type::SNAKE) {
            if (mode == Mode::BULLET) {
                if (inventory.eggs > 0) {
                    inventory.eggs--;
                    inventory.meat++;
                    Snake::removeSnake((Snake*)entity);
                }
            } else if (mode == Mode::COLLECT) {
                inventory.meat++;
                Snake::removeSnake((Snake*)entity);
            }
        } else if (entity->type == Type::GATE) {
            if (mode == Mode::BULLET) {
                if (inventory.eggs > 0) {
                    inventory.eggs--;
                    Gate::removeGate((Gate*)entity);
                }
            }
        } else if (entity->type == Type::CHICKEN) {
            if (mode == Mode::BULLET) {
                if (inventory.eggs > 0) {
                    inventory.eggs--;
                    inventory.meat++;
                    Chicken::removeChicken((Chicken*)entity);
                }
            } else if (mode == Mode::COLLECT) {
                inventory.meat++;
                Chicken::removeChicken((Chicken*)entity);
            }
        } else if (entity->type == Type::EGG) {
            if (mode == Mode::BULLET) {
                if (inventory.eggs > 0) {
                    inventory.eggs--;
                    Egg::removeEgg((Egg*)entity);
                }
            } else if (mode == Mode::COLLECT) {
                inventory.eggs++;
                Egg::removeEgg((Egg*)entity);
            }
        }
    } else if (field->isFree(targetCoordinates)) {
        if (mode == Mode::BULLET) {
            if (inventory.eggs <= 0) {
                return;
            }
            inventory.eggs--;
            Bullet::bullets.push_back(new Bullet(targetCoordinates, direction));
            field->addPrintPawn(Bullet::bullets.back());
        } else if (mode == Mode::DUMPCHEST) {
            if (inventory.walls > 0 || inventory.eggs > 0 || inventory.meat > 0) {
                Chest::chests.push_back(new Chest(targetCoordinates, inventory));
                field->addPrintPawn(Chest::chests.back());
                inventory = {0, 0, 0};
            }
        } else if (mode == Mode::WALL) {
            if (inventory.walls > 0) {
                Wall::walls.push_back(new Wall(targetCoordinates, 3));
                field->addPrintPawn(Wall::walls.back());
                inventory.walls--;
            }
        } else if (mode == Mode::GATE) {
            if (inventory.walls >= 2 && inventory.eggs > 0) {
                inventory.walls -= 2;
                inventory.eggs--;
                Gate::gates.push_back(new Gate(targetCoordinates));
                field->addPrintPawn(Gate::gates.back());
            }
        } else if (mode == Mode::TRAP) {
            if (inventory.walls > 0 && inventory.eggs > 0) {
                inventory.walls--;
                inventory.eggs--;
                Trap::traps.push_back(new Trap(targetCoordinates));
                field->addPrintPawn(Trap::traps.back());
            }
        } else if (mode == Mode::MINE) {
            if (inventory.walls > 0 && inventory.eggs >= 3) {
                inventory.walls--;
                inventory.eggs -= 3;
                Mine::mines.push_back(new Mine(targetCoordinates));
                field->addPrintPawn(Mine::mines.back());
            }
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
Chest::Chest(sista::Coordinates coordinates, Inventory inventory, bool _) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {}
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
                nextCoordinates = coordinates + directionMap[direction]*2;
                // if it is free then pass, otherwise change direction
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
        } else if (entity->type == Type::CHICKEN) {
            // Eat the chicken
            Chicken::removeChicken((Chicken*)entity);
        } else if (entity->type == Type::EGG) {
            // Jump over the egg
            if (direction == Direction::RIGHT) {
                nextCoordinates = coordinates + directionMap[Direction::UP];
            } else {
                nextCoordinates = coordinates + directionMap[Direction::DOWN];
            }
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
                return;
            } // Otherwise change direction
        } else if (entity->type == Type::GATE) {
            if (day) {
                // passing through the gate
                nextCoordinates = coordinates + directionMap[direction]*2;
                // if it is free then pass, otherwise change direction
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
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
            if (day) { // Or maybe the snake should be able to sneak through the closed gate
                // passing through the gate
                nextCoordinates = coordinates + directionMap[direction]*2;
                // if it is free then pass, otherwise change direction
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
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
    sista::Coordinates nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
    sista::Coordinates oldCoordinates = coordinates;
    if (field->isFree(nextCoordinates)) {
        field->movePawn(this, nextCoordinates);
        coordinates = nextCoordinates;
        if (field->isFree(oldCoordinates) && eggDistribution(rng)) {
            Egg::eggs.push_back(new Egg(oldCoordinates));
            field->addPrintPawn(Egg::eggs.back());
        }
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
    ANSI::ForegroundColor::F_RED,
    ANSI::BackgroundColor::B_BLUE,
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
Walker::Walker(sista::Coordinates coordinates) : Entity('Z', coordinates, walkerStyle, Type::WALKER) {}
Walker::Walker() : Entity('Z', {0, 0}, walkerStyle, Type::WALKER) {}
void Walker::move() {
    sista::Coordinates nextCoordinates = coordinates;
    sista::Coordinates playerCoordinates = Player::player->getCoordinates();
    if (playerCoordinates.x == coordinates.x) {
        if (playerCoordinates.y < coordinates.y) {
            nextCoordinates.y--;
        } else {
            nextCoordinates.y++;
        }
    } else if (playerCoordinates.y == coordinates.y) {
        if (playerCoordinates.x < coordinates.x) {
            nextCoordinates.x--;
        } else {
            nextCoordinates.x++;
        }
    } else {
        // The same applies for x = p.x +- 2 and y = p.y +- 2, the walker will move towards the player
        if (playerCoordinates.x < coordinates.x) {
            if (coordinates.x - playerCoordinates.x <= 2) {
                if (playerCoordinates.y < coordinates.y) {
                    nextCoordinates.y--;
                } else {
                    nextCoordinates.y++;
                }
            } else {
                // Randomly choose the direction
                nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
            }
        } else {
            if (playerCoordinates.x - coordinates.x <= 2) {
                if (playerCoordinates.y < coordinates.y) {
                    nextCoordinates.y--;
                } else {
                    nextCoordinates.y++;
                }
            } else {
                // Randomly choose the direction
                nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
            }
        }
        if (playerCoordinates.y < coordinates.y) {
            if (coordinates.y - playerCoordinates.y <= 2) {
                if (playerCoordinates.x < coordinates.x) {
                    nextCoordinates.x--;
                } else {
                    nextCoordinates.x++;
                }
            } else {
                // Randomly choose the direction
                nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
            }
        } else {
            if (playerCoordinates.y - coordinates.y <= 2) {
                if (playerCoordinates.x < coordinates.x) {
                    nextCoordinates.x--;
                } else {
                    nextCoordinates.x++;
                }
            } else {
                // Randomly choose the direction
                nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
            }
        }
    }
    if (field->isOutOfBounds(nextCoordinates)) {
        return;
    } else if (field->isFree(nextCoordinates)) {
        field->movePawn(this, nextCoordinates);
        coordinates = nextCoordinates;
    } else if (field->isOccupied(nextCoordinates)) {
        Entity* entity = (Entity*)field->getPawn(nextCoordinates);
        if (entity->type == Type::WALL) {
            // Walkers break walls
            Wall* wall = (Wall*)entity;
            wall->strength--;
            if (wall->strength == 0) {
                wall->setSymbol('@'); // Change the symbol to '@' to indicate that the wall was destroyed
                field->rePrintPawn(wall); // It will be reprinted in the next frame and then removed because of (strength == 0)
            }
        } else if (entity->type == Type::CHEST) {
            Chest *chest = (Chest*)entity;
            if (chest->inventory.meat == 0) {
                // if there is no meat in the chest, the walker is angry and destroys the chest
                Chest::removeChest(chest);
            } else {
                chest->inventory.meat--; // The walker eats the meat
            }
        } else if (entity->type == Type::MINE) {
            Mine* mine = (Mine*)entity;
            mine->triggered = true;
        } else if (entity->type == Type::WEASEL) {
            // The weasel is scared and runs away
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
            }
        } else if (entity->type == Type::SNAKE) {
            // The two don't care about each other and just swap places
            field->swapTwoPawns(this, (Walker*)entity); // WARNING: really hope this works
        } else if (entity->type == Type::GATE || entity->type == Type::TRAP) {
            // The walker can't pass through the gate and the trap is too small to be triggered
        } else if (entity->type == Type::CHICKEN) {
            // The chicken is scared and moves randomly
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
            // So if the chicken moved out of the way, the walker can move
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
            }
        } else if (entity->type == Type::EGG) {
            // The egg is broken when the walker steps on it
            Egg::removeEgg((Egg*)entity);
            // So the walker can move
            field->movePawn(this, nextCoordinates);
            coordinates = nextCoordinates;
        }
    }
}

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
void Archer::move() {
    Direction direction = (Direction)(rand() % 4);
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction];
    if (field->isOutOfBounds(nextCoordinates)) {
        return;
    } else if (field->isOccupied(nextCoordinates)) {
        Entity* entity = (Entity*)field->getPawn(nextCoordinates);
        if (entity->type == Type::WALL) {
            Wall* wall = (Wall*)entity;
            wall->strength--;
            if (wall->strength == 0) {
                wall->setSymbol('@'); // Change the symbol to '@' to indicate that the wall was destroyed
                field->rePrintPawn(wall); // It will be reprinted in the next frame and then removed because of (strength == 0)
            }
        } else if (entity->type == Type::CHEST) {
            Chest *chest = (Chest*)entity;
            if (chest->inventory.eggs == 0) {
                // if there is no egg in the chest, the archer is angry and destroys the chest
                Chest::removeChest(chest);
            } else {
                chest->inventory.eggs--; // The archer eats the egg
            }
        } else if (entity->type == Type::MINE) {
            Mine* mine = (Mine*)entity;
            mine->triggered = true;
        } else if (entity->type == Type::TRAP || entity->type == Type::GATE) {
            // The archer can't pass through the gate and the trap is too small to be triggered
        } else if (entity->type == Type::WEASEL) {
            // The weasel is scared and runs away
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
            }
        } else if (entity->type == Type::SNAKE) {
            // The snake is scared and moves randomly
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
            }
        } else if (entity->type == Type::CHICKEN) {
            // The chicken is scared and moves randomly
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates = coordinates + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates)) {
                    field->movePawn(this, nextCoordinates);
                    coordinates = nextCoordinates;
                    return;
                }
            }
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
            }
        } else if (entity->type == Type::EGG) {
            // The egg is broken when the archer accidentally steps on it
            Egg::removeEgg((Egg*)entity);
            if (field->isFree(nextCoordinates)) {
                field->movePawn(this, nextCoordinates);
                coordinates = nextCoordinates;
            }
        }
    } else if (field->isFree(nextCoordinates)) {
        field->movePawn(this, nextCoordinates);
        coordinates = nextCoordinates;
    }
}
void Archer::shoot() {
    Direction direction;;
    sista::Coordinates playerCoordinates = Player::player->getCoordinates();
    if (playerCoordinates.x == coordinates.x) {
        // The arrow will be shot in the y direction towards the player
        if (playerCoordinates.y < coordinates.y)
            direction = Direction::UP;
        else
            direction = Direction::DOWN;
    } else if (playerCoordinates.y == coordinates.y) {
        if (playerCoordinates.x < coordinates.x)
            direction = Direction::LEFT;
        else
            direction = Direction::RIGHT;
    } else {
        direction = (Direction)(rand() % 4);
    }
    sista::Coordinates nextCoordinates = coordinates + directionMap[direction];
    if (field->isFree(nextCoordinates)) {
        EnemyBullet::enemyBullets.push_back(new EnemyBullet(nextCoordinates, direction));
        field->addPrintPawn(EnemyBullet::enemyBullets.back());
    } else {
        // For the moment I would just give up this option, because the player doesn't know what's going on
    }
}


void removeNullptrs(std::vector<Entity*>& entities) {
    for (unsigned i=0; i<entities.size(); i++) {
        if (entities[i] == nullptr) {
            entities.erase(entities.begin() + i);
            i--;
        }
    }
}