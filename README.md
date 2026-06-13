# Tetris

A classic Tetris game for the terminal, written in C++ with ncurses.

## Features

- **7 standard tetrominoes** — I, O, T, S, Z, J, L with proper rotation
- **7-bag randomizer** — each bag contains one of each piece, shuffled, ensuring fair distribution
- **Wall kicks** — rotate pieces near walls with automatic offset attempts
- **Ghost piece** — faint preview of where the current piece will land
- **Next piece preview** — see what's coming next
- **Scoring** — points for soft drops (1/cell) and hard drops (2/cell); line clears: 100/300/500/800 (1/2/3/4 lines) multiplied by level
- **Levels** — level up every 10 lines cleared; gravity increases with each level
- **Pause** — toggle with `P`
- **Terminal UI** — clean ncurses interface with color-coded pieces and Unicode box-drawing borders

## Controls

| Key             | Action      |
|-----------------|-------------|
| `←` / `A`       | Move left   |
| `→` / `D`       | Move right  |
| `↑`             | Rotate CW   |
| `↓`             | Soft drop   |
| `Space`         | Hard drop   |
| `P`             | Pause       |
| `Q`             | Quit        |

## Build

### Prerequisites

- C++17 compiler (g++, clang++)
- ncurses library (with wide-char support, `ncursesw`)

### On Linux / WSL

```bash
# Install ncurses (Debian/Ubuntu)
sudo apt install libncursesw5-dev

# Install ncurses (Arch)
sudo pacman -S ncurses

# Build
make

# Run
./tetris
```

### On macOS

```bash
# ncurses is installed by default on macOS
make

# Run
./tetris
```

### On Windows

For Windows, use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) and follow the Linux instructions above, or build with MinGW and the appropriate ncurses port.

## Project Structure

| File         | Description                            |
|-------------|----------------------------------------|
| `main.cpp`  | ncurses renderer and input handling    |
| `tetris.h`  | Header — Board, Piece, and Game classes |
| `tetris.cpp`| Game logic implementation              |
| `Makefile`  | Build configuration                    |

## License

MIT
