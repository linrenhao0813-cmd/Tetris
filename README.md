# Tetris

A feature-rich terminal Tetris game written in C++17 with **ncurses** wide-character support. 4 game modes, 256-color graphics, rich animated effects, and persistent high scores.

---

## Features

### 4 Game Modes

| Mode | Key | Description |
|------|-----|-------------|
| **Classic** | `[1]` | Infinite play with 5 difficulty levels (Beginner → Master, Lv.1–20) |
| **Stage** | `[2]` | 20 stages across 6 chapters, each with unique objectives and 1–3 star ratings |
| **Endless** | `[3]` | Speed starts fast and keeps accelerating — push your limits |
| **Hell** | `[4]` | Fixed maximum speed from the start — no mercy for mortals |

### Gameplay
- **7 standard tetrominoes** — I, O, T, S, Z, J, L with proper rotation and wall kicks
- **7-bag randomizer** — ensures fair piece distribution
- **Ghost piece** — pulsing outline showing where the piece lands
- **Hold piece** — swap current piece with held piece (once per drop)
- **Combo system** — consecutive line clears build combos with score bonuses
- **Scoring** — soft drops (+1/cell), hard drops (+2/cell), line clears (100/300/500/800 × level), combo bonus

### Stage Mode Objectives
- Clear N lines / Reach target score / Survive N seconds
- Clear N tetrises (4-line clears)
- Achieve Nx combo
- Clear N lines within a piece limit
- Clear N lines without using hold
- Speed clear — N lines within a time limit

Stages unlock sequentially. Rated 1–3 stars based on score and completion time.

### Visual Effects
- **256-color terminal graphics** with rich palette
- **3D block rendering** with highlights and shadows
- **Animated menus** — rainbow title, falling tetromino particles, sparkle effects
- **Game entrance animation** — expanding board outline before each game
- **Ghost piece** with pulsing animation
- **Hard drop trail** with landing flash and impact effect
- **Line clear animation** — color cycling, particle sparkles, side effects
- **Tetris banner** — dramatic overlay on 4-line clears
- **Combo display** — animated "COMBO xN" and "FIRE!" banners with particles
- **Level-up flash** with celebration sparkles
- **Score flyup** — floating "+N" text on line clears
- **Screen shake** — on hard drops and tetris clears
- **Danger warning** — flashing red border when blocks approach the top
- **Time urgency** — pulsing timer and gradient bar when time is low (Stage mode)
- **Mode-specific UI** — Endless shows speed stats, Hell shows pulsing red warning
- **Stage clear celebration** — confetti particles and animated text
- **Stage failure explosion** — bomb drop, shockwave rings, 60+ particles, screen shake
- **Taunt popup** — "哈哈哈 废物" appears after 10 consecutive stage failures
- **Ambient particles** — floating decorations around the game board

### Sound Effects
Terminal `beep()` and `flash()` for: moves, clears, combos, hard drops, level-ups, explosions, and menu selection.

### Persistence
- **High scores** — top scores saved to `highscores.txt`
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
| `3` | Endless mode |
| `4` | Hell mode |
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

### Linux / WSL

```bash
# Install ncurses (Debian/Ubuntu)
sudo apt install libncursesw5-dev

# Install ncurses (Arch)
sudo pacman -S ncurses

# Build and run
make
./tetris
```

### macOS

```bash
# ncurses is pre-installed
make
./tetris
```

### Windows

Use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) and follow the Linux instructions. Enable Unicode and UTF-8 in your terminal for box-drawing characters.

---

## Project Structure

| File | Description |
|------|-------------|
| `main.cpp` | ncurses UI, menus, input, animations (~3000 lines) |
| `tetris.h` | Header — Board, Piece, Game, Stage, HighScore types |
| `tetris.cpp` | Game logic: board mechanics, rotation, scoring, stages |
| `Makefile` | Build configuration (`make` / `make clean`) |
| `highscores.txt` | High score data (auto-generated) |
| `stagedata.txt` | Stage progress data (auto-generated) |

---

## Stages Overview

| # | Name | Chapter | Objective |
|---|------|---------|-----------|
| 1 | First Steps | Tutorial | Clear 5 lines |
| 2 | Getting Faster | Tutorial | Clear 10 lines in 120s |
| 3 | Score Hunter | Tutorial | Reach 2000 points |
| 4 | Hold Your Ground | Survival | Survive 60s |
| 5 | Junkyard | Survival | Clear 8 lines with garbage |
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
