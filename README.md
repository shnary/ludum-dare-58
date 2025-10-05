# MazeKiller3D

A 2D raycasting horror maze game made for Ludum Dare 58.

<img width="1268" height="704" alt="Screenshot 2025-10-05 203034" src="https://github.com/user-attachments/assets/471514bd-c20e-4c34-80cc-7532fddf2f21" />
<img width="1267" height="706" alt="Screenshot 2025-10-05 203111" src="https://github.com/user-attachments/assets/1463d584-8079-4548-a52a-a0a655dc5b24" />

## About

MazeKiller3D is a first-person horror game built using 2D raycasting techniques (similar to classic Wolfenstein 3D). Navigate through mazes, collect gold, avoid the maze killer, and escape through locked doors.

## How to Play

### Controls
- **WASD**: Move through the maze
- **Mouse**: Look around (first-person view)
- **ESC**: Exit game

### Menu Controls
- **W/S or Arrow Keys**: Navigate menu options
- **ENTER or SPACE**: Select menu option

### Objective
Collect enough gold to unlock the exit door

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
