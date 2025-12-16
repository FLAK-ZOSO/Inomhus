#include "cross_platform.hpp"
#include "inomhus.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>

#define DAY_DURATION 700
#define NIGHT_DURATION 200

#define WIDTH 70
#define HEIGHT 30

#define VERSION "0.3.0"
#define DATE "2025-07-06"

#define REPOPULATE 0
#define DEBUG 0
#if DEBUG
    std::ofstream debug("debug.log");
#endif

std::shared_ptr<Player> Player::player;
std::vector<std::shared_ptr<Walker>> Walker::walkers;
std::vector<std::shared_ptr<Archer>> Archer::archers;
std::vector<std::shared_ptr<Bullet>> Bullet::bullets;
std::vector<std::shared_ptr<EnemyBullet>> EnemyBullet::enemyBullets;
std::vector<std::shared_ptr<Mine>> Mine::mines;
std::vector<std::shared_ptr<Chest>> Chest::chests;
std::vector<std::shared_ptr<Trap>> Trap::traps;
std::vector<std::shared_ptr<Weasel>> Weasel::weasels;
std::vector<std::shared_ptr<Snake>> Snake::snakes;
std::vector<std::shared_ptr<Chicken>> Chicken::chickens;
std::vector<std::shared_ptr<Egg>> Egg::eggs;
std::vector<std::shared_ptr<Gate>> Gate::gates;
std::vector<std::shared_ptr<Wall>> Wall::walls;

std::bernoulli_distribution Egg::hatchingDistribution(0.26); // 26%
std::bernoulli_distribution eggSelfHatchingDistribution(0.001); // 0.1%
std::bernoulli_distribution Chicken::eggDistribution(0.0075); // 0.75%
std::bernoulli_distribution Chicken::movingDistribution(0.75); // 75%
std::bernoulli_distribution Archer::movingDistribution(0.25); // 25%
std::bernoulli_distribution Archer::shootDistribution(0.005); // 0.5%
std::bernoulli_distribution Walker::movingDistribution(0.30); // 30%

std::bernoulli_distribution walkerSpawnDistribution(0.003); // 0.3%
std::bernoulli_distribution archerSpawnDistribution(0.004); // 0.4%
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
        sista::ForegroundColor::BLACK,
        sista::BackgroundColor::WHITE,
        sista::Attribute::BRIGHT
    }
);
std::mutex streamMutex;
bool tutorial_ = true;
bool speedup = false;
bool pause_ = false;
bool end = false;
bool day = true;


int main(int argc, char** argv) {
    #ifdef __APPLE__
        term_echooff();
        // system("stty raw -echo");
    #endif
    std::ios_base::sync_with_stdio(false);
    sista::resetAnsi(); // Reset the settings
    srand(time(0)); // Seed the random number generator

    if (argc > 1) {
        for (int i=1; i<argc; i++) {
            if (strcmp(argv[i], "--no-tutorial") == 0 || strcmp(argv[i], "-n") == 0) {
                tutorial_ = false;
            }
        }
    }

    sista::SwappableField field_(WIDTH, HEIGHT);
    field = &field_;
    field->clear();
    printIntro();
    #if defined(_WIN32) or defined(__linux__)
        char c = getch();
    #elif __APPLE__
        char c = getchar();
    #endif
    if (c == 'n' || c == 'N') {
        tutorial_ = false;
    }
    sista::clearScreen();

    if (tutorial_) {
        tutorial();
    } else {
        field->addPrintPawn(Player::player = std::make_shared<Player>(sista::Coordinates{0, 0}));
    }

    populate(field);
    sista::clearScreen();
    field->print(border);

    std::thread th(input);
    int dayCountdown = NIGHT_DURATION;
    int nightCountdown = DAY_DURATION;
    for (int i=0; !end; i++) {
        while (pause_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (end) break;
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
                Player::player->inventory = Inventory{0, 0, 0};
                Player::player->setSettings(Player::playerStyle);
                border = sista::Border(
                    '@', {
                        sista::ForegroundColor::BLACK,
                        sista::BackgroundColor::WHITE,
                        sista::Attribute::BRIGHT
                    }
                );
            } else {
                // The player is now out of control
                Player::player->setSettings(nightPlayerStyle);
                border = sista::Border(
                    '@', {
                        sista::ForegroundColor::WHITE,
                        sista::BackgroundColor::BLACK,
                        sista::Attribute::BRIGHT
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
                auto not_in = [&](const auto &vec) {
                    return std::find_if(vec.begin(), vec.end(),
                        [pawn](const std::shared_ptr<Entity> &e){ return e.get() == pawn; }
                    ) == vec.end();
                };
                if (not_in(Bullet::bullets) &&
                    not_in(EnemyBullet::enemyBullets) &&
                    not_in(Archer::archers) &&
                    not_in(Walker::walkers) &&
                    not_in(Wall::walls) &&
                    not_in(Mine::mines) &&
                    not_in(Gate::gates) &&
                    not_in(Weasel::weasels) &&
                    not_in(Snake::snakes) &&
                    not_in(Chicken::chickens) &&
                    not_in(Egg::eggs) &&
                    not_in(Chest::chests) &&
                    not_in(Trap::traps) &&
                    pawn != Player::player.get()) {
                    coordinates.push_back(pawn->getCoordinates());
                    #if DEBUG
                    debug << "Erasing " << pawn << " at {" << j << ", " << i << "}" << std::endl;
                    debug << "\t" << typeid(*pawn).name() << std::endl;
                    #endif
                }
            }
        }
        // for (auto coord : coordinates) {
        //     field->erasePawn(field->getPawn(coord));
        // }
        std::lock_guard<std::mutex> lock(streamMutex);
        for (unsigned j=0; j<Bullet::bullets.size(); j++) {
            if (j >= Bullet::bullets.size()) break;
            Bullet* bullet = Bullet::bullets[j].get();
            if (bullet == nullptr) continue;
            if (bullet->collided) continue;
            bullet->move();
        }
        EnemyBullet::enemyBullets.erase(
            std::remove_if(
                EnemyBullet::enemyBullets.begin(),
                EnemyBullet::enemyBullets.end(),
                [](const std::shared_ptr<EnemyBullet>& enemyBullet) {
                    if (!enemyBullet) return true;
                    if (enemyBullet->collided) {
                        // Remove pawn from field before erasing
                        field->erasePawn(enemyBullet.get());
                        return true;
                    }
                    return false;
                }
            ),
            EnemyBullet::enemyBullets.end()
        );
        Bullet::bullets.erase(
            std::remove_if(
                Bullet::bullets.begin(),
                Bullet::bullets.end(),
                [](const std::shared_ptr<Bullet>& bullet) {
                    if (!bullet) return true;
                    if (bullet->collided) {
                        // Remove pawn from field before erasing
                        field->erasePawn(bullet.get());
                        return true;
                    }
                    return false;
                }
            ),
            Bullet::bullets.end()
        );
        for (unsigned j=0; j<EnemyBullet::enemyBullets.size(); j++) {
            if (j >= EnemyBullet::enemyBullets.size()) break;
            EnemyBullet* enemyBullet = EnemyBullet::enemyBullets[j].get();
            if (enemyBullet == nullptr) continue;
            if (enemyBullet->collided) continue;
            enemyBullet->move();
        }
        EnemyBullet::enemyBullets.erase(
            std::remove_if(
                EnemyBullet::enemyBullets.begin(),
                EnemyBullet::enemyBullets.end(),
                [](const std::shared_ptr<EnemyBullet>& enemyBullet) {
                    if (!enemyBullet) return true;
                    if (enemyBullet->collided) {
                        // Remove pawn from field before erasing
                        field->erasePawn(enemyBullet.get());
                        return true;
                    }
                    return false;
                }
            ),
            EnemyBullet::enemyBullets.end()
        );
        Bullet::bullets.erase(
            std::remove_if(
                Bullet::bullets.begin(),
                Bullet::bullets.end(),
                [](const std::shared_ptr<Bullet>& bullet) {
                    if (!bullet) return true;
                    if (bullet->collided) {
                        // Remove pawn from field before erasing
                        field->erasePawn(bullet.get());
                        return true;
                    }
                    return false;
                }
            ),
            Bullet::bullets.end()
        );
        for (auto mine : Mine::mines) {
            if (mine->triggered) {
                mine->explode();
                mine->alive = false;
            }
        }
        for (auto mine : Mine::mines) {
            if (!mine->alive) {
                Mine::removeMine(mine.get());
            } else {
                mine->checkTrigger();
            }
        }
        for (unsigned c=0; c<Chest::chests.size(); c++) {
            if (c >= Chest::chests.size()) break;
            Chest* chest = Chest::chests[c].get();
            if (chest == nullptr) continue;
            if (chest->inventory.walls == 0 && chest->inventory.eggs == 0 && chest->inventory.meat == 0) {
                Chest::removeChest(chest);
            }
        }
        for (unsigned c=0; c<Chicken::chickens.size(); c++) {
            if (c >= Chicken::chickens.size()) break;
            Chicken* chicken = Chicken::chickens[c].get();
            if (chicken == nullptr) continue;
            if (Chicken::movingDistribution(rng)) {
                chicken->move();
            }
        }
        // Eggs self-hatching
        for (unsigned e=0; e<Egg::eggs.size(); e++) {
            if (e >= Egg::eggs.size()) break;
            Egg* egg = Egg::eggs[e].get();
            if (egg == nullptr) continue;
            if (eggSelfHatchingDistribution(rng)) {
                if (Egg::hatchingDistribution(rng)) {
                    sista::Coordinates coords = egg->getCoordinates();
                    Egg::removeEgg(egg);
                    Chicken::chickens.push_back(std::make_shared<Chicken>(coords));
                    field->addPrintPawn(Chicken::chickens.back());
                } else {
                    Egg::removeEgg(egg);
                }
            }
        }
        for (unsigned w=0; w<Walker::walkers.size(); w++) {
            if (w >= Walker::walkers.size()) break;
            Walker* walker = Walker::walkers[w].get();
            if (walker == nullptr) continue;
            if (Walker::movingDistribution(rng)) {
                walker->move();
            }
        }
        for (unsigned a=0; a<Archer::archers.size(); a++) {
            if (a >= Archer::archers.size()) break;
            Archer* archer = Archer::archers[a].get();
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
            Weasel* weasel = Weasel::weasels[w].get();
            if (weasel == nullptr) continue;
            weasel->move();
        }
        for (unsigned s=0; s<Snake::snakes.size(); s++) {
            if (s >= Snake::snakes.size()) break;
            Snake* snake = Snake::snakes[s].get();
            if (snake == nullptr) continue;
            snake->move();
        }
        for (unsigned w=0; w<Wall::walls.size(); w++) {
            if (w >= Wall::walls.size()) break;
            Wall* wall = Wall::walls[w].get();
            if (wall == nullptr) continue;
            if (wall->strength == 0) {
                Wall::removeWall(wall);
            }
        }
        // Iterate over wild animals to see if they have reached the other side of the field or they have been caught
        for (unsigned w=0; w<Weasel::weasels.size(); w++) {
            if (w >= Weasel::weasels.size()) break;
            Weasel* weasel = Weasel::weasels[w].get();
            if (weasel == nullptr) continue;
            if (weasel->crossed) {
                Weasel::removeWeasel(weasel);
            } else if (weasel->caught) {
                Weasel::removeWeasel(weasel);
                Player::player->inventory.meat += 2;
            }
        }
        for (unsigned s=0; s<Snake::snakes.size(); s++) {
            if (s >= Snake::snakes.size()) break;
            Snake* snake = Snake::snakes[s].get();
            if (snake == nullptr) continue;
            if (snake->crossed) {
                Snake::removeSnake(snake);
            }
        }

        #if REPOPULATE
        if (i % 128 == 127) {
            // repopulate the field from scratch for preventing nullptr pawns from laying around
            repopulate(field);
        }
        #endif
        #if __linux__
        if (i % 10 == 9) {
        #elif __APPLE__ or _WIN32
        if (i % 100 == 99) {
        #endif
            // reprint the field every 10 frames
            sista::clearScreen();
            field->print(border);
        }
        // Spawn new entities
        spawnNew(field);

        // Print inventory, time and instructions
        printSideInstructions(i, dayCountdown, nightCountdown);
        std::flush(std::cout);
    }

    th.join();
    field->clear();
    cursor.goTo(72, 0); // Move the cursor to the bottom of the screen, so the terminal is not left in a weird state
    std::this_thread::sleep_for(std::chrono::seconds(2));
    flushInput();
    #if __linux__
        getch();
    #elif __APPLE__
        getchar();
    #elif _WIN32
        getch();
    #endif
    #ifdef __APPLE__
        // system("stty -raw echo");
        tcsetattr(0, TCSANOW, &orig_termios);
    #endif
}

void tutorial() {
    /*
    Your character is represented by the `$` symbol and is happy to live in a 2D grid world; there are chickens (`%`) laying eggs (`0`) that the you can eat, chests (`C`) that may contain bricks and food, and comfortable walls (`#`) that can be used to build a house.
    However the world is not as friendly as it may seem at daylight, there are snakes (`~`) that eat the precious eggs, weasels (`}`) that hunt the chickens, threatening blind zombies (`Z`) that will follow you around and that you should better not bump, archers (`A`) that will shoot you from a distance.
    But, as it often happens in life, the worst enemy is yourself: when the night comes the character goes **out of control** and as a player you cannot control your character anymore, you can only watch it move around, destroy its own inventory and put itself in real danger.
    Will you be able to build a house before the night comes? When it happens all the gates (`=`) will be closed and your fury will be unleashed.
    */

    sista::clearScreen();
    field->print();
    field->addPrintPawn(Player::player = std::make_shared<Player>(sista::Coordinates{0, 0}));
    sista::resetAnsi();
    cursor.goTo(3, 10);
    std::cout << "Tutorial\n";
    cursor.goTo(4, 10);
    std::cout << "Your character is represented by the '";
    Player::playerStyle.apply();
    std::cout << "$";
    sista::resetAnsi();
    std::cout << "' symbol" << std::endl;

    #if __linux__ or _WIN32
        getch();
    #elif __APPLE__
        getchar();
    #endif

    Chicken::chickens.push_back(std::make_shared<Chicken>(sista::Coordinates{3, 5}));
    field->addPrintPawn(Chicken::chickens.back());
    sista::resetAnsi();
    cursor.goTo(7, 10);
    std::cout << "Chickens are represented by the '";
    Chicken::chickenStyle.apply();
    std::cout << "%";
    sista::resetAnsi();
    std::cout << "' symbol and lay eggs;";
    cursor.goTo(8, 10);
    std::cout << "Eggs are represented by the '";
    Egg::eggStyle.apply();
    std::cout << "0";
    sista::resetAnsi();
    std::cout << "' symbol" << std::endl;

    #if __linux__ or _WIN32
        getch();
    #elif __APPLE__
        getchar();
    #endif

    Egg::eggs.push_back(std::make_shared<Egg>(sista::Coordinates{4, 5}));
    field->addPrintPawn(Egg::eggs.back());
    sista::resetAnsi();
    cursor.goTo(10, 10);
    std::cout << "You can collect items by being in a neighboring cell" << std::endl;
    sista::setAttribute(sista::Attribute::BRIGHT);
    cursor.goTo(11, 10);
    std::cout << "Navigate to the egg";
    sista::resetAnsi();
    std::cout << " with the 'w', 'a', 's', 'd' keys" << std::endl;

    while (true) {
        // Check if the player is in a neighboring cell to the egg
        int distance_y = abs(Player::player->getCoordinates().y - Egg::eggs.back()->getCoordinates().y);
        int distance_x = abs(Player::player->getCoordinates().x - Egg::eggs.back()->getCoordinates().x);
        if (distance_y + distance_x == 1) {
            break;
        }
        // Take input and move the player
        char input = '_';
        while (input != 'w' && input != 'a' && input != 's' && input != 'd') {
            #if defined(_WIN32) or defined(__linux__)
                input = getch();
            #elif __APPLE__
                input = getchar();
            #endif
        }
        act(input);
        std::flush(std::cout);
    }

    sista::resetAnsi();
    cursor.goTo(12, 10);
    std::cout << "You are next to the egg, now press 'c' to ";
    sista::setAttribute(sista::Attribute::BRIGHT);
    std::cout << "enter collect mode" << std::endl;
    sista::resetAnsi();
    char input = '_';
    while (input != 'c' && input != 'C') {
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
    }
    Player::player->mode = Player::Mode::COLLECT;

    sista::resetAnsi();
    cursor.goTo(13, 10);
    std::cout << "Choose among 'i', 'j', 'k', 'l' the right direction to collect the egg" << std::endl;

    while (true) {
        // Take input and move the player
        char input = '_';
        while (input != 'i' && input != 'j' && input != 'k' && input != 'l') {
            #if defined(_WIN32) or defined(__linux__)
                input = getch();
            #elif __APPLE__
                input = getchar();
            #endif
        }
        sista::Coordinates target = Player::player->getCoordinates();
        switch (input) {
            case 'i':
                target.y--;
                break;
            case 'j':
                target.x--;
                break;
            case 'k':
                target.y++;
                break;
            case 'l':
                target.x++;
                break;
        }
        if (target == Egg::eggs.back()->getCoordinates()) {
            Egg::removeEgg(Egg::eggs.back().get());
            Player::player->inventory.eggs++;
            break;
        }
        std::flush(std::cout);
    }

    sista::resetAnsi();
    cursor.goTo(15, 10);
    std::cout << "You have collected the egg, now press 'b' to enter bullet mode" << std::endl;
    input = '_';
    while (input != 'b' && input != 'B') {
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
    }
    Player::player->mode = Player::Mode::BULLET;
    std::flush(std::cout);

    // Add an archer 5 blocks away from the player
    sista::Coordinates archerCoords = Player::player->getCoordinates();
    archerCoords.y += 5;
    Archer::archers.push_back(std::make_shared<Archer>(archerCoords));
    field->addPrintPawn(Archer::archers.back());

    sista::resetAnsi();
    cursor.goTo(16, 10);
    std::cout << "Under you there is an archer!";
    cursor.goTo(17, 10);
    std::cout << "Choose among 'i', 'j', 'k', 'l' the right direction to shoot the egg" << std::endl;

    while (input != 'i' && input != 'j' && input != 'k' && input != 'l') {
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
    }
    act(input);
    std::flush(std::cout);

    // We have to move the bullet until it hits the archer
    while (Bullet::bullets.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::shared_ptr<Bullet> bullet = Bullet::bullets[0];
        bullet->move();
        std::flush(std::cout);
    }

    sista::resetAnsi();
    cursor.goTo(19, 10);
    std::cout << "You have thrown an egg, now ";
    sista::setAttribute(sista::Attribute::BRIGHT);
    std::cout << "Collect the wall";
    sista::resetAnsi();
    std::cout << " '";
    Wall::wallStyle.apply();
    std::cout << "#";
    sista::resetAnsi();
    std::cout << "' as explained" << std::endl;

    // Add a wall 5 blocks away from the player
    sista::Coordinates wallCoords = Player::player->getCoordinates();
    wallCoords.x -= 3;
    Wall::walls.push_back(std::make_shared<Wall>(wallCoords, 3));
    field->addPrintPawn(Wall::walls.back());
    std::flush(std::cout);

    input = '_';
    while (true) {
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
        act(input);
        printSideInstructions(0, DAY_DURATION, NIGHT_DURATION);
        std::flush(std::cout);
        if (Player::player->inventory.walls > 0) {
            break;
        }
    }
    std::flush(std::cout);

    sista::resetAnsi();
    cursor.goTo(21, 10);
    std::cout << "Wall collected, now press '#' or '=' to enter wall mode" << std::endl;
    input = '_';
    while (input != '#' && input != '=' && input != '0') {
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
    }
    Player::player->mode = Player::Mode::WALL;
    printSideInstructions(0, DAY_DURATION, NIGHT_DURATION);
    std::flush(std::cout);

    sista::resetAnsi();
    cursor.goTo(22, 10);
    std::cout << "Choose among 'i', 'j', 'k', 'l' a random direction to build a wall" << std::endl;

    input = '_';
    while (input != 'i' && input != 'j' && input != 'k' && input != 'l') {
        #if defined(_WIN32) or defined(__linux__)
            input = getch();
        #elif __APPLE__
            input = getchar();
        #endif
    }
    act(input);
    std::flush(std::cout);

    sista::resetAnsi();
    cursor.goTo(24, 10);
    std::cout << "Now you have the basic knowledge to survive in the world of Inomhus" << std::endl;
    cursor.goTo(25, 10);
    std::cout << "For more tricks and tips, visit the GitHub repository" << std::endl;
    cursor.goTo(26, 10);
    std::cout << "https://github.com/FLAK-ZOSO/Inomhus" << std::endl;
    cursor.goTo(28, 10);
    std::cout << "Press any key to start the game" << std::endl;

    #if defined(_WIN32) or defined(__linux__)
        getch();
    #elif __APPLE__
        getchar();
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
        case 'w': case 'W': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::UP);
            break;
        }
        case 'a': case 'A': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::LEFT);
            break;
        }
        case 's': case 'S': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::DOWN);
            break;
        }
        case 'd': case 'D': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::RIGHT);
            break;
        }

        case 'j': case 'J': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::LEFT);
            break;
        }
        case 'k': case 'K': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::DOWN);
            break;
        }
        case 'l': case 'L': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::RIGHT);
            break;
        }
        case 'i': case 'I': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::UP);
            break;
        }

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
    cursor.goTo(5, 0);
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

    std::cout << "\tMake sure to have the terminal full-screen\n";
    std::cout << "\t\t'\x1b[35mn\x1b[0m' to skip the tutorial\n\n";

    sista::setAttribute(sista::Attribute::BLINK);
    std::cout << "\t\t\x1b[3mPress any key to start\x1b[0m";
    std::flush(std::cout);
}

void printSideInstructions(int i, int dayCountdown, int nightCountdown) {
    // Print the inventory
    sista::resetAnsi();
    cursor.goTo(3, WIDTH+10);
    sista::setAttribute(sista::Attribute::BRIGHT);
    std::cout << "Inventory\n";
    sista::resetAttribute(sista::Attribute::BRIGHT);
    cursor.goTo(4, WIDTH+10);
    std::cout << "Walls: " << Player::player->inventory.walls << "   \n";
    cursor.goTo(5, WIDTH+10);
    std::cout << "Eggs: " << Player::player->inventory.eggs << "   \n";
    cursor.goTo(6, WIDTH+10);
    std::cout << "Meat: " << Player::player->inventory.meat << "   \n";
    cursor.goTo(7, WIDTH+10);
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
    cursor.goTo(10, WIDTH+10);
    sista::setAttribute(sista::Attribute::BRIGHT);
    std::cout << "Time survived: " << i << "    \n";
    sista::resetAttribute(sista::Attribute::BRIGHT);
    cursor.goTo(11, WIDTH+10);
    sista::setAttribute(sista::Attribute::BRIGHT);
    std::cout << "Time before ";
    if (day) {
        std::cout << "night: " << nightCountdown << "    \n";
    } else {
        std::cout << "day: " << dayCountdown << "    \n";
    }
    sista::resetAttribute(sista::Attribute::BRIGHT);
    // Be aware not to overwrite the inventory and the time survived which use {3, WIDTH+10} to ~{11, WIDTH+10}
    #if __linux__
    if (i % 10 == 9) {
    #elif __APPLE__ or _WIN32
    if (i % 100 == 99 || i == 0) {
    #endif
        cursor.goTo(14, WIDTH+10);
        sista::setAttribute(sista::Attribute::BRIGHT);
        std::cout << "Instructions\n";
        sista::resetAttribute(sista::Attribute::BRIGHT);
        cursor.goTo(15, WIDTH+10);
        std::cout << "Move: \x1b[35mw\x1b[37m | \x1b[35ma\x1b[37m | \x1b[35ms\x1b[37m | \x1b[35md\x1b[37m\n";
        cursor.goTo(16, WIDTH+10);
        std::cout << "Act: \x1b[35mi\x1b[37m | \x1b[35mj\x1b[37m | \x1b[35mk\x1b[37m | \x1b[35ml\x1b[37m\n";
        cursor.goTo(18, WIDTH+10);
        std::cout << "Collect mode: \x1b[35mc\x1b[37m\n";
        cursor.goTo(19, WIDTH+10);
        std::cout << "Bullet mode: \x1b[35mb\x1b[37m\n";
        cursor.goTo(20, WIDTH+10);
        std::cout << "Dump Chest mode: \x1b[35me\x1b[37m\n";
        cursor.goTo(21, WIDTH+10);
        std::cout << "Build Wall mode: \x1b[35m=\x1b[37m | \x1b[35m0\x1b[37m | \x1b[35m#\x1b[37m\n";
        cursor.goTo(22, WIDTH+10);
        std::cout << "Build Gate mode: \x1b[35mg\x1b[37m\n";
        cursor.goTo(23, WIDTH+10);
        std::cout << "Place Trap mode: \x1b[35mt\x1b[37m\n";
        cursor.goTo(24, WIDTH+10);
        std::cout << "Place Mine mode: \x1b[35mm\x1b[37m | \x1b[35m*\x1b[37m\n";
        cursor.goTo(25, WIDTH+10);
        std::cout << "Egg-hatching mode: \x1b[35mh\x1b[37m\n";
        cursor.goTo(27, WIDTH+10);
        std::cout << "Speedup mode: \x1b[35m+\x1b[37m | \x1b[35m-\x1b[37m\n";
        cursor.goTo(28, WIDTH+10);
        std::cout << "Pause or resume: \x1b[35m.\x1b[37m | \x1b[35mp\x1b[37m\n";
        cursor.goTo(29, WIDTH+10);
        std::cout << "Quit: \x1b[35mQ\x1b[37m\n";
    }
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
                Wall::walls.push_back(std::make_shared<Wall>(coordinates, rand() % 2 + 1));
                field->addPrintPawn(Wall::walls.back());
            }
        }
    }
    for (int i=0; i<HEIGHT; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Wall::walls.push_back(std::make_shared<Wall>(coordinates, rand() % 2 + 1));
            field->addPrintPawn(Wall::walls.back());
        }
    }
    // Chests, a couple of them
    for (int i=0; i<3; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Chest::chests.push_back(std::make_shared<Chest>(coordinates, Inventory{(short)(rand() % 5), (short)(rand() % 5)}, true));
            field->addPrintPawn(Chest::chests.back());
        }
    }
    // Walkers, some randomly around the field, but none of them in a 5x5 square around the player, which starts in {0, 0}
    for (int i=0; i<5; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates) && coordinates.y > 5 && coordinates.x > 5) {
            Walker::walkers.push_back(std::make_shared<Walker>(coordinates));
            field->addPrintPawn(Walker::walkers.back());
        }
    }
    // Archers, some randomly around the field, but none in the same row or column as the player
    for (int i=0; i<5; i++) {
        coordinates = {rand() % (HEIGHT - 5) + 5, rand() % (WIDTH - 5) + 5};
        if (field->isFree(coordinates)) {
            Archer::archers.push_back(std::make_shared<Archer>(coordinates));
            field->addPrintPawn(Archer::archers.back());
        }
    }
    // Only one Weasel, to be generated from the left side of the field
    coordinates = {rand() % HEIGHT, 0};
    if (field->isFree(coordinates)) {
        Weasel::weasels.push_back(std::make_shared<Weasel>(coordinates, Direction::RIGHT));
        field->addPrintPawn(Weasel::weasels.back());
    }
    // Only one Snake, to be generated from the right side of the field
    coordinates = {rand() % (HEIGHT - 10), WIDTH - 1};
    if (field->isFree(coordinates)) {
        Snake::snakes.push_back(std::make_shared<Snake>(coordinates, Direction::LEFT));
        field->addPrintPawn(Snake::snakes.back());
    }
    // Some Chickens, randomly around the field
    for (int i=0; i<5; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Chicken::chickens.push_back(std::make_shared<Chicken>(coordinates));
            field->addPrintPawn(Chicken::chickens.back());
        }
    }
    // Some Eggs, randomly around the field
    for (int i=0; i<15; i++) {
        coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Egg::eggs.push_back(std::make_shared<Egg>(coordinates));
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
            Walker::walkers.push_back(std::make_shared<Walker>(coordinates));
            field->addPrintPawn(Walker::walkers.back());
        }
    }
    if (archerSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Archer::archers.push_back(std::make_shared<Archer>(coordinates));
            field->addPrintPawn(Archer::archers.back());
        }
    }
    if (weaselSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, 0};
        if (field->isFree(coordinates)) {
            Weasel::weasels.push_back(std::make_shared<Weasel>(coordinates, Direction::RIGHT));
            field->addPrintPawn(Weasel::weasels.back());
        }
    }
    if (snakeSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % (HEIGHT - 10), WIDTH - 1};
        if (field->isFree(coordinates)) {
            Snake::snakes.push_back(std::make_shared<Snake>(coordinates, Direction::LEFT));
            field->addPrintPawn(Snake::snakes.back());
        }
    }
    if (wallSpawnDistribution(rng)) {
        sista::Coordinates coordinates = {rand() % HEIGHT, rand() % WIDTH};
        if (field->isFree(coordinates)) {
            Wall::walls.push_back(std::make_shared<Wall>(coordinates, 3));
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

Entity::Entity(char symbol, sista::Coordinates coordinates, sista::ANSISettings& settings, Type type) : sista::Pawn(symbol, coordinates, settings), type(type) {}
Entity::Entity() : sista::Pawn(' ', sista::Coordinates(0, 0), Player::playerStyle), type(Type::PLAYER) {}

sista::ANSISettings Player::playerStyle = {
    sista::ForegroundColor::RED,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
sista::ANSISettings nightPlayerStyle = {
    sista::ForegroundColor::BLUE,
    sista::BackgroundColor::BLACK,
    sista::Attribute::REVERSE
};
Player::Player(sista::Coordinates coordinates) : Entity('$', coordinates, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory({0, 0, 0}) {}
Player::Player() : Entity('$', {0, 0}, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory({0, 0, 0}) {}
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
            inventory.meat += 2;
            Chicken::removeChicken((Chicken*)entity);
        } else if (entity->type == Type::WEASEL) {
            inventory.meat += 2;
            Weasel::removeWeasel((Weasel*)entity);
        } else if (entity->type == Type::SNAKE) {
            inventory.meat++;
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
            return;
        } else if (entity->type == Type::TRAP || entity->type == Type::WALL || entity->type == Type::BULLET) {
            return;
        } else if (entity->type == Type::ENEMYBULLET || entity->type == Type::WALKER || entity->type == Type::ARCHER) {
            end = true;
            sista::resetAnsi();
            cursor.goTo(HEIGHT, 80);
            sista::setAttribute(sista::Attribute::BLINK);
            sista::setBackgroundColor(sista::BackgroundColor::RED);
            sista::setForegroundColor(sista::ForegroundColor::BLACK);
            std::cout << "You ran into an enemy entity!";
            sista::resetAnsi();
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
                Gate::gates.push_back(std::make_shared<Gate>(targetCoordinates));
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
            Bullet::bullets.push_back(std::make_shared<Bullet>(targetCoordinates, direction));
            field->addPrintPawn(Bullet::bullets.back());
        } else if (mode == Mode::DUMPCHEST) {
            if (inventory.walls > 0 || inventory.eggs > 0 || inventory.meat > 0) {
                Chest::chests.push_back(std::make_shared<Chest>(targetCoordinates, inventory));
                field->addPrintPawn(Chest::chests.back());
                inventory = {0, 0, 0};
            }
        } else if (mode == Mode::WALL) {
            if (inventory.walls > 0) {
                Wall::walls.push_back(std::make_shared<Wall>(targetCoordinates, 3));
                field->addPrintPawn(Wall::walls.back());
                inventory.walls--;
            }
        } else if (mode == Mode::GATE) {
            if (inventory.walls >= 2 && inventory.eggs > 0) {
                inventory.walls -= 2;
                inventory.eggs--;
                Gate::gates.push_back(std::make_shared<Gate>(targetCoordinates));
                field->addPrintPawn(Gate::gates.back());
            }
        } else if (mode == Mode::TRAP) {
            if (inventory.walls > 0 && inventory.meat > 0) {
                inventory.walls--;
                inventory.meat--;
                Trap::traps.push_back(std::make_shared<Trap>(targetCoordinates));
                field->addPrintPawn(Trap::traps.back());
            }
        } else if (mode == Mode::MINE) {
            if (inventory.walls > 0 && inventory.eggs >= 3) {
                inventory.walls--;
                inventory.eggs -= 3;
                Mine::mines.push_back(std::make_shared<Mine>(targetCoordinates));
                field->addPrintPawn(Mine::mines.back());
            }
        } else if (mode == Mode::HATCH) {
            if (inventory.eggs > 0) {
                if (Egg::hatchingDistribution(rng)) {
                    Chicken::chickens.push_back(std::make_shared<Chicken>(targetCoordinates));
                    field->addPrintPawn(Chicken::chickens.back());
                }
                inventory.eggs--;
            }
        }
    }
}


sista::ANSISettings Bullet::bulletStyle = {
    sista::ForegroundColor::MAGENTA,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Bullet::removeBullet(Bullet* bullet) {
    auto it = std::find_if(Bullet::bullets.begin(), Bullet::bullets.end(),
        [bullet](const std::shared_ptr<Bullet>& b) { return b.get() == bullet; });
    if (it != Bullet::bullets.end()) {
        field->erasePawn(bullet);
        Bullet::bullets.erase(it);
    }
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


sista::ANSISettings EnemyBullet::enemyBulletStyle = {
    sista::ForegroundColor::GREEN,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
EnemyBullet::EnemyBullet(sista::Coordinates coordinates, Direction direction, unsigned short speed) : Entity(directionSymbol[direction], coordinates, enemyBulletStyle, Type::BULLET), direction(direction), speed(speed) {}
EnemyBullet::EnemyBullet(sista::Coordinates coordinates, Direction direction) : Entity(directionSymbol[direction], coordinates, enemyBulletStyle, Type::BULLET), direction(direction), speed(1) {}
EnemyBullet::EnemyBullet() : Entity(' ', {0, 0}, enemyBulletStyle, Type::ENEMYBULLET), direction(Direction::UP), speed(1) {}
void EnemyBullet::removeEnemyBullet(EnemyBullet* enemyBullet) {
    auto it = std::find_if(EnemyBullet::enemyBullets.begin(), EnemyBullet::enemyBullets.end(),
        [enemyBullet](const std::shared_ptr<EnemyBullet>& b) { return b.get() == enemyBullet; });
    if (it != EnemyBullet::enemyBullets.end()) {
        field->erasePawn(enemyBullet);
        EnemyBullet::enemyBullets.erase(it);
    }
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
            sista::resetAnsi();
            cursor.goTo(HEIGHT, 80);
            sista::setAttribute(sista::Attribute::BLINK);
            sista::setBackgroundColor(sista::BackgroundColor::RED);
            sista::setForegroundColor(sista::ForegroundColor::BLACK);
            std::cout << "You were hit by an enemy bullet!";
            sista::resetAnsi();
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

sista::ANSISettings Mine::mineStyle = {
    sista::ForegroundColor::MAGENTA,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BLINK   
};
void Mine::removeMine(Mine* mine) {
    auto it = std::find_if(Mine::mines.begin(), Mine::mines.end(),
        [mine](const std::shared_ptr<Mine>& z) { return z.get() == mine; });
    if (it != Mine::mines.end()) {
        field->erasePawn(mine);
        Mine::mines.erase(it);
    }
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
    settings.foregroundColor = sista::ForegroundColor::WHITE;
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

sista::ANSISettings Chest::chestStyle = {
    sista::ForegroundColor::YELLOW,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Chest::removeChest(Chest* chest) {
    auto it = std::find_if(Chest::chests.begin(), Chest::chests.end(),
        [chest](const std::shared_ptr<Chest>& z) { return z.get() == chest; });
    if (it != Chest::chests.end()) {
        field->erasePawn(chest);
        Chest::chests.erase(it);
    }
}
Chest::Chest(sista::Coordinates coordinates, Inventory inventory, bool _) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {}
Chest::Chest(sista::Coordinates coordinates, Inventory& inventory) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {}
Chest::Chest() : Entity('C', {0, 0}, chestStyle, Type::CHEST), inventory({0, 0}) {}

sista::ANSISettings Trap::trapStyle = {
    sista::ForegroundColor::CYAN,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Trap::removeTrap(Trap* trap) {
    auto it = std::find_if(Trap::traps.begin(), Trap::traps.end(),
        [trap](const std::shared_ptr<Trap>& z) { return z.get() == trap; });
    if (it != Trap::traps.end()) {
        field->erasePawn(trap);
        Trap::traps.erase(it);
    }
}
Trap::Trap(sista::Coordinates coordinates) : Entity('T', coordinates, trapStyle, Type::TRAP) {}
Trap::Trap() : Entity('T', {0, 0}, trapStyle, Type::TRAP) {}

sista::ANSISettings Weasel::weaselStyle = {
    sista::ForegroundColor::WHITE,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Weasel::removeWeasel(Weasel* weasel) {
    auto it = std::find_if(Weasel::weasels.begin(), Weasel::weasels.end(),
        [weasel](const std::shared_ptr<Weasel>& z) { return z.get() == weasel; });
    if (it != Weasel::weasels.end()) {
        field->erasePawn(weasel);
        Weasel::weasels.erase(it);
    }
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
            } // Otherwise change direction
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
        } else if (entity->type == Type::BULLET || entity->type == Type::ENEMYBULLET) {
            caught = true; // The weasel will be removed
            return;
        }
        direction = (direction == Direction::RIGHT) ? Direction::LEFT : Direction::RIGHT;
        symbol = (direction == Direction::RIGHT) ? '}' : '{';
        return;
    }
    field->movePawn(this, nextCoordinates);
    coordinates = nextCoordinates;
}

sista::ANSISettings Snake::snakeStyle = {
    sista::ForegroundColor::GREEN,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Snake::removeSnake(Snake* snake) {
    auto it = std::find_if(Snake::snakes.begin(), Snake::snakes.end(),
        [snake](const std::shared_ptr<Snake>& z) { return z.get() == snake; });
    if (it != Snake::snakes.end()) {
        field->erasePawn(snake);
        Snake::snakes.erase(it);
    }
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
            // Will just change direction, because traps are only meant for Weasels so far
        } else if (entity->type == Type::WEASEL) {
            Weasel::removeWeasel((Weasel*)entity);
        } else if (entity->type == Type::SNAKE) {
            Snake::removeSnake((Snake*)entity);
            return;
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

sista::ANSISettings Chicken::chickenStyle = {
    sista::ForegroundColor::YELLOW,
    sista::BackgroundColor::BLACK,
    sista::Attribute::ITALIC
};
void Chicken::removeChicken(Chicken* chicken) {
    auto it = std::find_if(Chicken::chickens.begin(), Chicken::chickens.end(),
        [chicken](const std::shared_ptr<Chicken>& z) { return z.get() == chicken; });
    if (it != Chicken::chickens.end()) {
        field->erasePawn(chicken);
        Chicken::chickens.erase(it);
    }
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
            Egg::eggs.push_back(std::make_shared<Egg>(oldCoordinates));
            field->addPrintPawn(Egg::eggs.back());
        }
    }
}

sista::ANSISettings Egg::eggStyle = {
    sista::ForegroundColor::WHITE,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Egg::removeEgg(Egg* egg) {
    auto it = std::find_if(Egg::eggs.begin(), Egg::eggs.end(),
        [egg](const std::shared_ptr<Egg>& z) { return z.get() == egg; });
    if (it != Egg::eggs.end()) {
        field->erasePawn(egg);
        Egg::eggs.erase(it);
    }
}
Egg::Egg(sista::Coordinates coordinates) : Entity('0', coordinates, eggStyle, Type::EGG) {}
Egg::Egg() : Entity('0', {0, 0}, eggStyle, Type::EGG) {}

sista::ANSISettings Gate::gateStyle = {
    sista::ForegroundColor::BLUE,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};
void Gate::removeGate(Gate* gate) {
    auto it = std::find_if(Gate::gates.begin(), Gate::gates.end(),
        [gate](const std::shared_ptr<Gate>& z) { return z.get() == gate; });
    if (it != Gate::gates.end()) {
        field->erasePawn(gate);
        Gate::gates.erase(it);
    }
}
Gate::Gate(sista::Coordinates coordinates) : Entity('=', coordinates, gateStyle, Type::GATE) {}
Gate::Gate() : Entity('=', {0, 0}, gateStyle, Type::GATE) {}

sista::ANSISettings Wall::wallStyle = {
    sista::ForegroundColor::RED,
    sista::BackgroundColor::BLUE,
    sista::Attribute::BRIGHT
};
void Wall::removeWall(Wall* wall) {
    auto it = std::find_if(Wall::walls.begin(), Wall::walls.end(),
        [wall](const std::shared_ptr<Wall>& z) { return z.get() == wall; });
    if (it != Wall::walls.end()) {
        field->erasePawn(wall);
        Wall::walls.erase(it);
    }
}
Wall::Wall(sista::Coordinates coordinates, short int strength) : Entity('#', coordinates, wallStyle, Type::WALL), strength(strength) {}
Wall::Wall() : Entity('#', {0, 0}, wallStyle, Type::WALL), strength(1) {}

sista::ANSISettings Walker::walkerStyle = {
    sista::ForegroundColor::GREEN,
    sista::BackgroundColor::BLACK,
    sista::Attribute::FAINT
};
void Walker::removeWalker(Walker* walker) {
    auto it = std::find_if(Walker::walkers.begin(), Walker::walkers.end(),
        [walker](const std::shared_ptr<Walker>& z) { return z.get() == walker; });
    if (it != Walker::walkers.end()) {
        field->erasePawn(walker);
        Walker::walkers.erase(it);
    }
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
            Weasel* weasel = (Weasel*)entity;
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates_ = weasel->getCoordinates() + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates_)) {
                    field->movePawn(weasel, nextCoordinates_);
                    weasel->setCoordinates(nextCoordinates_);
                    return;
                }
            }
        } else if (entity->type == Type::SNAKE) {
            // The two don't care about each other and just swap places
            field->swapTwoPawns(this, (Walker*)entity); // WARNING: really hope this works
        } else if (entity->type == Type::GATE || entity->type == Type::TRAP) {
            // The walker can't pass through the gate and the trap is too small to be triggered
        } else if (entity->type == Type::CHICKEN) {
            // The chicken is scared and moves randomly
            Chicken* chicken = (Chicken*)entity;
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates_ = chicken->getCoordinates() + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates_)) {
                    field->movePawn(chicken, nextCoordinates_);
                    chicken->setCoordinates(nextCoordinates_);
                    break;
                }
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

sista::ANSISettings Archer::archerStyle = {
    sista::ForegroundColor::CYAN,
    sista::BackgroundColor::BLACK,
    sista::Attribute::STRIKETHROUGH
};
void Archer::removeArcher(Archer* archer) {
    auto it = std::find_if(Archer::archers.begin(), Archer::archers.end(),
        [archer](const std::shared_ptr<Archer>& z) { return z.get() == archer; });
    if (it != Archer::archers.end()) {
        field->erasePawn(archer);
        Archer::archers.erase(it);
    }
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
            Weasel* weasel = (Weasel*)entity;
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates_ = weasel->getCoordinates() + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates_)) {
                    field->movePawn(weasel, nextCoordinates_);
                    weasel->setCoordinates(nextCoordinates_);
                    return;
                }
            }
        } else if (entity->type == Type::SNAKE) {
            // The snake is scared and moves randomly
            Snake* snake = (Snake*)entity;
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates_ = snake->getCoordinates() + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates_)) {
                    field->movePawn(snake, nextCoordinates_);
                    snake->setCoordinates(nextCoordinates_);
                    return;
                }
            }
        } else if (entity->type == Type::CHICKEN) {
            // The chicken is scared and moves randomly
            Chicken* chicken = (Chicken*)entity;
            for (int j=0; j<3; j++) {
                sista::Coordinates nextCoordinates_ = chicken->getCoordinates() + directionMap[(Direction)(rand() % 4)];
                if (field->isFree(nextCoordinates_)) {
                    field->movePawn(chicken, nextCoordinates_);
                    chicken->setCoordinates(nextCoordinates_);
                    return;
                }
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
        EnemyBullet::enemyBullets.push_back(std::make_shared<EnemyBullet>(nextCoordinates, direction));
        field->addPrintPawn(EnemyBullet::enemyBullets.back());
    } else {
        // For the moment I would just give up this option, because the player doesn't know what's going on
    }
}


void removeNullptrs(std::vector<std::shared_ptr<Entity>>& entities) {
    for (unsigned i=0; i<entities.size(); i++) {
        if (entities[i] == nullptr) {
            entities.erase(entities.begin() + i);
            i--;
        }
    }
}
