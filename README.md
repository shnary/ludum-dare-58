# Ludum Dare 58

A 3D horror game made with raylib.

## Dependencies

### macOS
```bash
brew install raylib
```

### Linux
```bash
sudo apt install libraylib-dev  # Ubuntu/Debian
sudo pacman -S raylib          # Arch
```

### Windows
```bash
vcpkg install raylib
# or download from https://github.com/raysan5/raylib/releases
```

## Build

### macOS/Linux
```bash
mkdir build && cd build
cmake ..
cmake --build .
./ludum-dare-58
```

### Windows
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
.\Release\ludum-dare-58.exe
```

