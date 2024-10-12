# Inomhus

*Bättre att stanna **inomhus*** (better be **in the house**) is a [Sista](https://github.com/FLAK-ZOSO/Sista) based C++ terminal videogame.

## Description

https://github.com/user-attachments/assets/629db96e-2434-4d33-a6dc-4483e72f2e3b

Your character is represented by the `$` symbol and is happy to live in a 2D grid world; there are chickens (`%`) laying eggs (`0`) that the you can eat, chests (`C`) that may contain bricks and food, and comfortable walls (`#`) that can be used to build a house.

However the world is not as friendly as it may seem at daylight, there are snakes (`~`) that eat the precious eggs, weasels (`}`) that hunt the chickens, threatening blind zombies (`Z`) that will follow you around and that you should better not bump, archers (`A`) that will shoot you from a distance.

But, as it often happens in life, the worst enemy is yourself: when the night comes the character goes **out of control** and as a player you cannot control your character anymore, you can only watch it move around, destroy its own inventory and put itself in real danger.

## Installation

### From source

```bash
git clone https://github.com/FLAK-ZOSO/Inomhus
cd Inomhus
make
```

### From binary

Go to the [repository](https://github.com/FLAK-ZOSO/Inomhus) page and download the executable for your system.

## Usage

```bash
./inomhus
```

```batch
inomhus.exe
```

## Controls

Movement controls.

- `w` - Move up
- `a` - Move left
- `s` - Move down
- `d` - Move right

Shooting controls.

- `i` - Shoot up
- `j` - Shoot left
- `k` - Shoot down
- `l` - Shoot right

Mode controls.

- `c`/`C` - Collect
- `b`/`B` - Bullet
- `e`/`E` - Dump chest
- `=`/`0`/`#` - build Wall
- `g`/`G` - build Gate
- `t`/`T` - build Trap
- `m` - Mine
- `h` - Hatch

Game controls.

- `Q` - Quit
- `p`/`P`/`.` - Pause
- `+`/`-` - Speed up/down

## Credits

- FLAK-ZOSO for the Sista library
- Stefano Tocazzo for the trailer
- NIVIRO for the [trailer's soundtrack](https://ncs.io/TheRiot)
- ITÜK for the [event](https://gamecamp.ituk.ee/event/08dcca81-1c54-47d0-8eda-151aa7b1e956)
