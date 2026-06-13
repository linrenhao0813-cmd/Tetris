# Tetris — Enhanced Edition

A feature-rich Tetris game for the terminal, written in C++17 with **ncurses** wide-character support. Features 256-color graphics, two game modes (Classic + 20-stage campaign), hold piece, combo scoring, animated visual effects, and persistent high scores.

![Preview](screenshot.png)

> **Note:** Replace `screenshot.png` with an actual screenshot of the game running in your terminal.

---

## Features

### Gameplay
- **7 standard tetrominoes** — I, O, T, S, Z, J, L with proper rotation and wall kicks
- **7-bag randomizer** — each bag contains one of each piece, shuffled, ensuring fair distribution
- **Ghost piece** — pulsing preview of where the current piece lands
- **Hold piece** — swap current piece with held piece (once per drop)
- **Combo system** — consecutive line clears build combos with score bonuses
- **Levels** — level up every 10 lines; gravity increases with each level
- **Scoring** — soft drops (+1/cell), hard drops (+2/cell); line clears (100/300/500/800 points); combo bonus; all multiplied by level
- **T-spin detection** — tracks T-spin clears

### Two Game Modes

| Mode | Description |
|------|-------------|
| **Classic** | Infinite play with difficulty selection (Beginner → Master, levels 1–20) |
| **Stage** | 20 stages across 6 chapters, each with unique objectives |

**Stage objectives:**
- Clear N lines
- Reach target score
- Survive N seconds
- Clear N tetrises (4-line clears)
- Achieve Nx combo
- Clear N lines within a piece limit
- Clear N lines without using hold
- Speed clear — N lines within a time limit

Stages unlock sequentially. Each stage is rated 1–3 stars based on score and completion time.

### Visual Effects
- **256-color terminal graphics** with rich color palette
- **3D block rendering** with highlights and shadows
- **Animated menu system** with fade-in, falling particle effects, and rainbow title
- **Animated game board** with subtle grid patterns
- **Ghost piece** with pulsing animation
- **Hard drop trail** with landing flash effect
- **Line clear animation** with flash cycling and particle sparkles
- **Tetris banner** — dramatic "TETRIS!" overlay on 4-line clears
- **Combo display** — animated "COMBO xN" and "FIRE!" banners
- **Level-up flash** with celebration sparkles
- **Score flyup** — floating "+N" text on line clears
- **Screen shake** — on hard drops and tetris clears
- **Danger warning** — flashing red border when blocks approach the top
- **Time urgency** — pulsing timer and gradient bar when time is low
- **Stage clear celebration** — particles and animated text
- **Stage failure explosion** — bomb drop, shockwave rings, particle physics, screen shake
- **Taunt popup** — appears after 10 consecutive stage failures
- **Game entrance animation** — expanding board outline

### Sound Effects
Uses terminal `beep()` and `flash()` for audio feedback on moves, clears, combos, level-ups, and game events.

### Persistence
- **High scores** — top 10 scores saved to `highscores.txt`
- **Stage progress** — star ratings and best scores saved to `stagedata.txt`

---

## Controls

| Key | Action |
|-----|--------|
| `←` / `A` | Move left |
| `→` / `D` | Move right |
| `↑` | Rotate clockwise |
| `↓` | Soft drop |
| `Space` | Hard drop |
| `H` | Hold piece |
| `P` | Pause / Resume |
| `R` | Retry (on game over) |
| `M` | Return to menu (on game over) |
| `Q` | Quit |

### Menu Controls
| Key | Action |
|-----|--------|
| `1` | Classic mode |
| `2` | Stage mode |
| `↑` / `↓` | Navigate difficulty / stages |
| `←` / `→` | Navigate stage grid |
| `Enter` | Select |
| `Esc` | Go back |
| `Q` | Quit |

---

## Build

### Prerequisites

- **C++17 compiler** (g++, clang++)
- **ncurses library** with wide-character support (`ncursesw`)

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

Use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) and follow the Linux instructions above. The game requires a terminal with Unicode and 256-color support.

> **Note for Windows Terminal users:** Enable Unicode support and set your terminal to render UTF-8 for the box-drawing and emoji characters to display correctly.

---

## Project Structure

| File | Description |
|------|-------------|
| `main.cpp` | ncurses UI renderer, menus, input handling, animations (~2800 lines) |
| `tetris.h` | Header — Board, Piece, Game, Stage, and HighScore types |
| `tetris.cpp` | Game logic: board mechanics, piece rotation, scoring, stage definitions |
| `Makefile` | Build configuration (`make` / `make clean`) |
| `highscores.txt` | Persistent high score data (auto-generated) |
| `stagedata.txt` | Persistent stage progress data (auto-generated) |

---

## Stages Overview

| # | Name | Chapter | Objective |
|---|------|---------|-----------|
| 1 | First Steps | Tutorial | Clear 5 lines |
| 2 | Getting Faster | Tutorial | Clear 10 lines in 120s |
| 3 | Score Hunter | Tutorial | Reach 2000 points |
| 4 | Hold Your Ground | Survival | Survive 60s |
| 5 | Junkyard | Survival | Clear 8 lines with garbage blocks |
| 6 | Combo Master | Survival | Achieve 3x combo |
| 7 | Tetris Time | Tetris | Clear 3 tetrises |
| 8 | Piece Limit | Tetris | Clear 15 lines in 40 pieces |
| 9 | No Hold Challenge | Tetris | Clear 10 lines without hold |
| 10 | Speed Demon | Intense | Clear 12 lines in 90s |
| 11 | Deep Garbage | Intense | Clear 10 lines with deep garbage |
| 12 | Tetris Storm | Intense | Clear 5 tetrises |
| 13 | Speed Tetris | Expert | Clear 4 tetrises in 60s |
| 14 | Minimalist | Expert | Clear 20 lines in 30 pieces |
| 15 | No Mercy | Expert | Clear 15 lines, no hold, with garbage |
| 16 | Combo King | Expert | Achieve 5x combo |
| 17 | Grand Tetris | Master | Clear 8 tetrises |
| 18 | Marathon | Master | Survive 180s |
| 19 | Gauntlet | Master | Clear 25 lines in 120s |
| 20 | Final Challenge | Master | Clear 30 lines, no hold, max speed |

---

## License

MIT
