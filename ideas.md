In order to pick up items, you must be standing next to them and press the key that corresponds to the direction you are facing; also you must be in {C}ollect mode (activated with the C key, the default mode at game start).

- with ijkl you can take up WALL which will go in the inventry (+wall.strength)

- if you pick up an EGG with ijkl it will go in the inventry
  - if you step on an egg you break it
  - if you throw the egg (with ijkl) it will be used as a bullet
  - if you hatch the egg (with ijkl when in {H}atch mode) it *may* turn into a chicken

- A GATE is open at day and closed at night
  - if you move over an *open* GATE you will be teleported to the other side
  - if you place a GATE (with ijkl) it will be placed in the direction you are facing
  - if you place a GATE in the same spot as a WALL, the WALL will be replaced with the GATE

The prime matters of the game are...
- WALL
- EGG
All other items can be crafted on the spot with the prime matters.

You also have meat which comes from what you kill and from what traps get.

## Night

During the night the Player will...
- move around randomly (or pseudo-randomly, tbd)
- destroy its own inventory

This is where the walls, to control the movement of the Player, and the chests, to store the items, come in handy.

## Crafting

- Gate: 2 WALL + 1 EGG
- Wall: 1 WALL
- Trap: 1 WALL + 1 MEAT
- Mine: 1 WALL + 3 EGG

```c++
enum Type {
    PLAYER, // $
    BULLET, // < ^ v >
    WALL, // #
    GATE, // =, only open during the day
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
```

### Meat

- Meat is obtained from eating a snake or a weasel

But what can we do with meat? Apart from preventing a zombie from breaking your chest, which is a rather marginal use.

- Meat can be used to craft a trap
