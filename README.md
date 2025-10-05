# MazeKiller3D

A 2D raycasting horror maze game made for Ludum Dare 58.

## About

MazeKiller3D is a first-person horror game built using 2D raycasting techniques (similar to classic Wolfenstein 3D). Navigate through mazes, collect gold, avoid the maze killer, and escape through locked doors.

## Features

- **2D Raycasting Engine**: Classic Wolfenstein-style 3D rendering from a 2D map
- **5 Levels**: Progressively harder mazes
- **Enemy AI**: maze killer that hunts you down through the corridors
- **Upgrade Shop**: Spend your gold on upgrades between levels

## How to Play

### Controls
- **WASD**: Move through the maze
- **Mouse**: Look around (first-person view)
- **ESC**: Exit game

### Menu Controls
- **W/S or Arrow Keys**: Navigate menu options
- **ENTER or SPACE**: Select menu option

### Objective
1. Collect enough gold to unlock the exit door
2. Avoid the maze killer who spawns far from you each level
3. Reach the red door when you have sufficient gold
4. Purchase upgrades in the shop between levels
5. Complete all 5 levels to escape!

## Building

### Requirements
- CMake 3.15+
- C++17 compiler
- raylib (install via homebrew on macOS: `brew install raylib`)

### Compile
```bash
cd build
cmake ..
make
```

### Run
```bash
./ludum-dare-58
```

## Credits

Made for Ludum Dare 58 - "Collector" theme

Game built using raylib and C++17.

## License

This project was created for Ludum Dare 58. Feel free to learn from the code!
