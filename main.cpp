#include "tetris.h"
#include <ncurses.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cmath>

// ============================================================
//  Visual state
// ============================================================
struct VisualState {
    int shakeFrames = 0;
    int shakeIntensity = 0;
    int lineFlashRow = -1;
    int lineFlashFrames = 0;
    int comboDisplayFrames = 0;
    int comboDisplayValue = 0;
    int tetrisFlashFrames = 0;
    int hardDropTrail[BOARD_W] = {};
    int trailFrames = 0;
    int levelUpFrames = 0;
    int titleFrame = 0;
    int dropSparkRow = -1;
    int dropSparkCol = -1;
    int dropSparkFrames = 0;
    int clearAnimRow[4] = {-1,-1,-1,-1};
    int clearAnimCount = 0;
    int clearAnimFrames = 0;
    int lastLinesCleared = 0;
    int lastLevel = 1;
    int globalFrame = 0;
    int scoreFlyupValue = 0;
    int scoreFlyupFrames = 0;
    int scoreFlyupY = 0;
};

static VisualState vis;

static void triggerShake(int intensity = 2) {
    vis.shakeFrames = 6;
    vis.shakeIntensity = intensity;
}

static void triggerHardDropTrail(const Piece& p, int ghostRow) {
    for (int c = 0; c < BOARD_W; ++c)
        vis.hardDropTrail[c] = 0;
    const Shape& s = p.shape();
    for (int dr = 0; dr < p.size(); ++dr)
        for (int dc = 0; dc < p.size(); ++dc)
            if (s[dr][dc])
                vis.hardDropTrail[p.col() + dc] = ghostRow + dr;
    vis.trailFrames = 5;
}

static void triggerLineClear(int cleared) {
    vis.clearAnimFrames = 8;
    vis.lastLinesCleared = cleared;
}

static void triggerCombo(int combo) {
    if (combo > 1) {
        vis.comboDisplayFrames = 20;
        vis.comboDisplayValue = combo;
    }
}

static void triggerLevelUp() {
    vis.levelUpFrames = 30;
}

static void triggerScoreFlyup(int score) {
    vis.scoreFlyupValue = score;
    vis.scoreFlyupFrames = 15;
    vis.scoreFlyupY = 0;
}

// ============================================================
//  Sound (enhanced with more variety)
// ============================================================
static void playSound(int type) {
    switch (type) {
        case 0: beep(); break;
        case 1: flash(); break;
        case 2: beep(); flash(); break;
        case 3:
            beep(); flash();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            beep();
            break;
        case 4:
            beep(); flash();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            beep();
            break;
        case 5: // Level up / success
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            beep();
            flash();
            break;
        case 6: // Menu select
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            break;
        case 7: // Hard drop
            flash();
            beep();
            break;
        case 8: // Game over
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            flash();
            break;
        case 9: // Explosion
            flash(); beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            flash(); beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            flash(); beep();
            break;
        case 10: // Combo success
            beep(); flash();
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            beep();
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            beep(); flash();
            break;
        default: break;
    }
}

// ============================================================
//  Color palette - rich 256-color scheme
// ============================================================
static void initColors() {
    start_color();
    use_default_colors();

    // Piece colors: each piece gets primary + highlight for 3D effect
    // I=Cyan, O=Yellow, T=Magenta, S=Green, Z=Red, J=Blue, L=Orange
    init_pair(1,  195, 31);   // I - light cyan on cyan
    init_pair(2,  227, 178);  // O - light yellow on gold
    init_pair(3,  219, 133);  // T - light magenta on purple
    init_pair(4,  157, 34);   // S - light green on green
    init_pair(5,  217, 160);  // Z - light red on red
    init_pair(6,  153, 21);   // J - light blue on blue
    init_pair(7,  230, 172);  // L - light orange on orange

    // Darker shades for block bottom/shadow
    init_pair(21, 27, 195);   // I dark
    init_pair(22, 172, 227);  // O dark
    init_pair(23, 129, 219);  // T dark
    init_pair(24, 29, 157);   // S dark
    init_pair(25, 156, 217);  // Z dark
    init_pair(26, 17, 153);   // J dark
    init_pair(27, 166, 230);  // L dark

    // Ghost piece - very subtle
    init_pair(8,  240, -1);   // Ghost outline
    init_pair(58, 238, -1);   // Ghost fill

    // UI framework
    init_pair(9,  208, -1);   // Score - orange
    init_pair(10, 75, -1);    // Title/label - light blue
    init_pair(11, 240, -1);   // Border - gray
    init_pair(12, 237, -1);   // Empty cell - very dim
    init_pair(13, 226, -1);   // Combo - yellow
    init_pair(14, 196, -1);   // Danger/fire - red
    init_pair(15, 141, -1);   // Hold - purple
    init_pair(16, 46, -1);    // Green/success
    init_pair(17, 238, -1);   // Locked/disabled
    init_pair(18, 203, -1);   // Warning red
    init_pair(19, 214, -1);   // Gold
    init_pair(20, 250, -1);   // Subtle text

    // Background / decorative
    init_pair(30, 236, -1);   // Grid dots
    init_pair(31, 239, -1);   // Grid lines
    init_pair(32, 243, -1);   // Dim border

    // Animations
    init_pair(40, 255, 231);  // Flash white
    init_pair(41, 231, 196);  // Flash red
    init_pair(42, 231, 82);   // Flash green

    // Garbage block
    init_pair(50, 243, 59);   // Garbage green-gray
    init_pair(51, 243, 243);  // Garbage light

    // Panel backgrounds (dark bg for contrast)
    init_pair(60, 252, 233);  // Bright bg for title
    init_pair(61, 255, 255);  // White

    // Level-up flash
    init_pair(70, 226, 231);  // Level up yellow bg
}

// ============================================================
//  Unicode block characters for 3D rendering
// ============================================================
// Top-left filled: ▛  Top-right: ▜  Bottom-left: ▙  Bottom-right: ▟
// Full: █  Top half: ▀  Bottom half: ▄  Left: ▌  Right: ▐
// For 2-char cells, we use 2 chars to represent each cell

struct BlockChar {
    const char* top;     // top row (2 chars)
    const char* bottom;  // bottom row (2 chars)
};

// 3D block: bright top-left, darker bottom-right
static const BlockChar BLOCK_FULL[] = {
    // Index 0: I-piece style
    {"\xe2\x96\x88\xe2\x96\x88", "\xe2\x96\x88\xe2\x96\x88"},  // ██ ██
};

// Cell rendering for different states


// ============================================================
//  Board rendering helpers
// ============================================================
static void drawBoardFrame(int boardX, int boardY, int w, int h) {
    // Animated border color based on game state
    int borderColor = (vis.globalFrame / 10) % 2 == 0 ? 11 : 32;

    attron(COLOR_PAIR(borderColor) | A_BOLD);
    // Top
    mvprintw(boardY-1, boardX-1, "\xe2\x95\x94"); // ╔
    for (int i = 0; i < w; ++i) {
        int colorShift = (i + vis.globalFrame / 5) % 3;
        int charColor = (colorShift == 0) ? 11 : (colorShift == 1) ? 31 : 32;
        attron(COLOR_PAIR(charColor) | A_BOLD);
        mvprintw(boardY-1, boardX+i, "\xe2\x95\x90"); // ═
    }
    attron(COLOR_PAIR(borderColor) | A_BOLD);
    mvprintw(boardY-1, boardX+w, "\xe2\x95\x97"); // ╗
    // Bottom
    mvprintw(boardY+h, boardX-1, "\xe2\x95\x9a"); // ╚
    for (int i = 0; i < w; ++i) mvprintw(boardY+h, boardX+i, "\xe2\x95\x90");
    mvprintw(boardY+h, boardX+w, "\xe2\x95\x9d"); // ╝
    // Sides
    for (int i = 0; i < h; ++i) {
        mvprintw(boardY+i, boardX-1, "\xe2\x95\x91"); // ║
        mvprintw(boardY+i, boardX+w, "\xe2\x95\x91");
    }
    attroff(COLOR_PAIR(borderColor) | A_BOLD);

    // Corner decorations
    attron(COLOR_PAIR(19) | A_DIM);
    mvprintw(boardY-2, boardX-2, "\xe2\x98\x86"); // ☆
    mvprintw(boardY-2, boardX+w+1, "\xe2\x98\x86");
    mvprintw(boardY+h+1, boardX-2, "\xe2\x98\x86");
    mvprintw(boardY+h+1, boardX+w+1, "\xe2\x98\x86");
    attroff(COLOR_PAIR(19) | A_DIM);
}

static void drawPanel(int x, int y, int w, int h, const char* title, int titleColor) {
    // Background fill
    attron(COLOR_PAIR(32));
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c)
            mvprintw(y + r, x + c, " ");
    attroff(COLOR_PAIR(32));

    // Border
    attron(COLOR_PAIR(11));
    for (int r = 0; r < h; ++r) {
        mvprintw(y + r, x, "\xe2\x95\x91");
        mvprintw(y + r, x + w - 1, "\xe2\x95\x91");
    }
    for (int c = 0; c < w; ++c) {
        mvprintw(y, x + c, "\xe2\x95\x90");
        mvprintw(y + h - 1, x + c, "\xe2\x95\x90");
    }
    mvprintw(y, x, "\xe2\x95\x94");
    mvprintw(y, x + w - 1, "\xe2\x95\x97");
    mvprintw(y + h - 1, x, "\xe2\x95\x9a");
    mvprintw(y + h - 1, x + w - 1, "\xe2\x95\x9d");
    attroff(COLOR_PAIR(11));

    // Title
    if (title) {
        attron(COLOR_PAIR(titleColor) | A_BOLD);
        mvprintw(y + 1, x + 1, " %s", title);
        attroff(COLOR_PAIR(titleColor) | A_BOLD);
    }
}

static void drawProgressMini(int x, int y, int current, int max, int width, int fillPair, int emptyPair) {
    float ratio = (max > 0) ? std::min(1.0f, (float)current / max) : 0.0f;
    int filled = (int)(ratio * width);
    mvprintw(y, x, "\xe2\x95\x9a"); // ╚
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            attron(COLOR_PAIR(fillPair) | A_BOLD);
            mvprintw(y, x + 1 + i, "\xe2\x96\x88"); // █
            attroff(COLOR_PAIR(fillPair) | A_BOLD);
        } else {
            attron(COLOR_PAIR(emptyPair));
            mvprintw(y, x + 1 + i, "\xe2\x96\x91"); // ░
            attroff(COLOR_PAIR(emptyPair));
        }
    }
    mvprintw(y, x + 1 + width, "\xe2\x95\x9f"); // ╝
}

// ============================================================
//  Main menu - animated with fade-in
// ============================================================
static int showMainMenu() {
    int frame = 0;

    // Fade-in effect
    for (int fade = 0; fade < 6; ++fade) {
        erase();
        int bgColor = 238 - fade * 8;
        if (bgColor < 232) bgColor = 232;
        attron(COLOR_PAIR(32));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(32));

        if (fade > 2) {
            int titleColor = 10;
            attron(COLOR_PAIR(titleColor) | A_BOLD);
            mvprintw(5, 25, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88 T E T R I S \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            attroff(COLOR_PAIR(titleColor) | A_BOLD);
        }
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }

    // Falling block particles
    struct FallingBlock {
        int x, y, speed, color, shape;
    };
    FallingBlock blocks[15];
    for (int i = 0; i < 15; ++i) {
        blocks[i].x = (i * 5 + 3) % 75;
        blocks[i].y = (i * 13) % 25;
        blocks[i].speed = 1 + (i % 3);
        blocks[i].color = (i % 7) + 1;
        blocks[i].shape = i % 4;
    }

    while (true) {
        erase();
        frame++;

        int cx = 18, sy = 3;

        // Animated rainbow title
        int titleColors[] = {1, 2, 3, 4, 5, 6, 7};

        // Title banner with animated border and glow
        int titleColor = titleColors[(frame / 4) % 7];
        int nextColor = titleColors[((frame / 4) + 1) % 7];
        attron(COLOR_PAIR(titleColor) | A_BOLD);
        mvprintw(sy,   cx, "\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97");
        mvprintw(sy+1, cx, "\xe2\x95\x91                                   \xe2\x95\x91");
        mvprintw(sy+2, cx, "\xe2\x95\x91      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88      \xe2\x95\x91");
        mvprintw(sy+3, cx, "\xe2\x95\x91      \xe2\x95\xad\xe2\x95\xad\xe2\x96\x88\xe2\x96\x88\xe2\x95\xbc\xe2\x95\xad\xe2\x96\x88\xe2\x96\x88\xe2\x95\xad\xe2\x95\xad\xe2\x95\xad\xe2\x95\xad\xe2\x96\x88\xe2\x96\x88\xe2\x95\xad\xe2\x95\xad\xe2\x95\xad\xe2\x95\xad      \xe2\x95\x91");
        mvprintw(sy+4, cx, "\xe2\x95\x91         \xe2\x96\x88\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88      \xe2\x95\x91");
        mvprintw(sy+5, cx, "\xe2\x95\x91         \xe2\x96\x88\xe2\x96\x88   \xe2\x96\x88\xe2\x96\x88\xe2\x95\xbc         \xe2\x96\x88\xe2\x96\x88      \xe2\x95\x91");
        mvprintw(sy+6, cx, "\xe2\x95\x91         \xe2\x96\x88\xe2\x96\x88   \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88   \xe2\x96\x88\xe2\x96\x88      \xe2\x95\x91");
        mvprintw(sy+7, cx, "\xe2\x95\x91         \xe2\x95\xaf   \xe2\x95\xaf\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88   \xe2\x95\xaf      \xe2\x95\x91");
        mvprintw(sy+8, cx, "\xe2\x95\x91                                   \xe2\x95\x91");
        attroff(COLOR_PAIR(titleColor) | A_BOLD);

        // Animated underline with gradient
        attron(COLOR_PAIR(nextColor) | A_BOLD);
        mvprintw(sy+9, cx, "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d");
        attroff(COLOR_PAIR(nextColor) | A_BOLD);

        // Falling tetromino blocks in background
        for (int i = 0; i < 15; ++i) {
            blocks[i].y += blocks[i].speed;
            if (blocks[i].y > 24) {
                blocks[i].y = -2;
                blocks[i].x = (frame * 3 + i * 7) % 75;
            }
            if (blocks[i].x > 2 && blocks[i].x < 72 && blocks[i].y > 1 && blocks[i].y < 22) {
                attron(COLOR_PAIR(blocks[i].color) | A_DIM);
                switch (blocks[i].shape) {
                    case 0: // I-piece
                        mvprintw(blocks[i].y, blocks[i].x, "\xe2\x96\x88\xe2\x96\x88");
                        break;
                    case 1: // O-piece
                        mvprintw(blocks[i].y, blocks[i].x, "\xe2\x96\x88\xe2\x96\x88");
                        mvprintw(blocks[i].y+1, blocks[i].x, "\xe2\x96\x88\xe2\x96\x88");
                        break;
                    case 2: // T-piece
                        mvprintw(blocks[i].y, blocks[i].x, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
                        mvprintw(blocks[i].y+1, blocks[i].x+2, "\xe2\x96\x88");
                        break;
                    case 3: // L-piece
                        mvprintw(blocks[i].y, blocks[i].x, "\xe2\x96\x88\xe2\x96\x88");
                        mvprintw(blocks[i].y+1, blocks[i].x+2, "\xe2\x96\x88");
                        break;
                }
                attroff(COLOR_PAIR(blocks[i].color) | A_DIM);
            }
        }

        // Sparkle particles around title
        for (int i = 0; i < 12; ++i) {
            int sx = cx + 2 + (frame * 2 + i * 9) % 33;
            int sy2 = sy + (frame / 2 + i * 3) % 9;
            if ((frame + i * 7) % 8 < 3) {
                attron(COLOR_PAIR(19) | A_BOLD);
                mvprintw(sy2, sx, "\xe2\x98\x86");
                attroff(COLOR_PAIR(19) | A_BOLD);
            }
        }

        // Mode selection
        int menuY = sy + 12;
        attron(COLOR_PAIR(19) | A_BOLD);
        mvprintw(menuY, cx + 10, "\xe2\x96\xb6  SELECT MODE  \xe2\x96\xb6");
        attroff(COLOR_PAIR(19) | A_BOLD);

        // Classic mode card with glow effect
        int classicGlow = (frame / 2) % 8;
        int classicBorderColor = (classicGlow < 4) ? 10 : 32;
        attron(COLOR_PAIR(classicBorderColor) | A_BOLD);
        // Card frame
        for (int i = 0; i < 26; ++i) {
            mvprintw(menuY + 2, cx + 5 + i, "\xe2\x95\x90");
            mvprintw(menuY + 6, cx + 5 + i, "\xe2\x95\x90");
        }
        for (int i = 0; i < 4; ++i) {
            mvprintw(menuY + 3 + i, cx + 5, "\xe2\x95\x91");
            mvprintw(menuY + 3 + i, cx + 30, "\xe2\x95\x91");
        }
        mvprintw(menuY + 2, cx + 5, "\xe2\x95\x94");
        mvprintw(menuY + 2, cx + 30, "\xe2\x95\x97");
        mvprintw(menuY + 6, cx + 5, "\xe2\x95\x9a");
        mvprintw(menuY + 6, cx + 30, "\xe2\x95\x9d");
        attroff(COLOR_PAIR(classicBorderColor) | A_BOLD);

        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(menuY + 3, cx + 8, "\xe2\x96\xb6 [1] CLASSIC");
        attroff(COLOR_PAIR(10) | A_BOLD);
        attron(COLOR_PAIR(20));
        mvprintw(menuY + 4, cx + 8, "  Infinite play");
        mvprintw(menuY + 5, cx + 8, "  Choose difficulty");
        attroff(COLOR_PAIR(20));

        // Stage mode card with glow effect
        int stageGlow = ((frame / 2) + 4) % 8;
        int stageBorderColor = (stageGlow < 4) ? 15 : 32;
        attron(COLOR_PAIR(stageBorderColor) | A_BOLD);
        for (int i = 0; i < 26; ++i) {
            mvprintw(menuY + 2, cx + 34 + i, "\xe2\x95\x90");
            mvprintw(menuY + 6, cx + 34 + i, "\xe2\x95\x90");
        }
        for (int i = 0; i < 4; ++i) {
            mvprintw(menuY + 3 + i, cx + 34, "\xe2\x95\x91");
            mvprintw(menuY + 3 + i, cx + 59, "\xe2\x95\x91");
        }
        mvprintw(menuY + 2, cx + 34, "\xe2\x95\x94");
        mvprintw(menuY + 2, cx + 59, "\xe2\x95\x97");
        mvprintw(menuY + 6, cx + 34, "\xe2\x95\x9a");
        mvprintw(menuY + 6, cx + 59, "\xe2\x95\x9d");
        attroff(COLOR_PAIR(stageBorderColor) | A_BOLD);

        attron(COLOR_PAIR(15) | A_BOLD);
        mvprintw(menuY + 3, cx + 37, "\xe2\x96\xb6 [2] STAGE");
        attroff(COLOR_PAIR(15) | A_BOLD);
        attron(COLOR_PAIR(20));
        mvprintw(menuY + 4, cx + 37, "  20 stages");
        mvprintw(menuY + 5, cx + 37, "  Objectives & rewards");
        attroff(COLOR_PAIR(20));

        // High scores preview with better styling
        auto highScores = loadHighScores();
        if (!highScores.empty()) {
            int hsY = menuY + 9;
            // Decorative line
            attron(COLOR_PAIR(19));
            mvprintw(hsY - 1, cx + 10, "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80");
            attroff(COLOR_PAIR(19));

            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(hsY, cx + 14, "\xe2\x98\x85 HIGH SCORES \xe2\x98\x85");
            attroff(COLOR_PAIR(19) | A_BOLD);

            // Rank icons
            const char* rankIcons[] = {"\xe2\x98\x85", "\xe2\x98\x86", "\xe2\x98\x86"};
            for (int i = 0; i < std::min(3, (int)highScores.size()); ++i) {
                int rankColor = (i == 0) ? 19 : (i == 1) ? 20 : 11;
                attron(COLOR_PAIR(rankColor) | A_BOLD);
                mvprintw(hsY + 1 + i, cx + 10, "%s", rankIcons[i]);
                attroff(COLOR_PAIR(rankColor) | A_BOLD);
                attron(COLOR_PAIR(rankColor));
                mvprintw(hsY + 1 + i, cx + 13, "#%d %-8s %8d pts",
                         i + 1, highScores[i].name.c_str(), highScores[i].score);
                attroff(COLOR_PAIR(rankColor));
            }
        }

        // Footer with animated border
        attron(COLOR_PAIR(11));
        mvprintw(25, cx - 2, "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80");
        attroff(COLOR_PAIR(11));

        int dots = frame % 4;
        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(26, cx + 8, "Press [1] CLASSIC  [2] STAGE  [Q] QUIT");
        for (int i = 0; i < dots; ++i)
            mvprintw(26, cx + 46 + i, ".");
        attroff(COLOR_PAIR(10) | A_BOLD);

        // Version text
        attron(COLOR_PAIR(32));
        mvprintw(27, cx + 14, "v2.0 - Enhanced Edition");
        attroff(COLOR_PAIR(32));

        refresh();

        int ch = getch();
        if (ch == '1') { playSound(6); return 1; }
        if (ch == '2') { playSound(6); return 2; }
        if (ch == 'q' || ch == 'Q') return 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ============================================================
//  Classic mode difficulty menu (enhanced)
// ============================================================
static int showClassicMenu() {
    int frame = 0;
    int cursor = 0;
    const char* diffNames[]  = {"BEGINNER", "NORMAL", "HARD", "EXPERT", "MASTER"};
    const int  diffLevels[]  = {1, 5, 10, 15, 20};
    const int  diffColors[]  = {16, 10, 13, 14, 18};
    const char* diffIcons[]  = {"\xe2\x98\x86", "\xe2\x98\x85", "\xe2\x9a\xa1", "\xe2\x9a\xa2", "\xe2\x9c\xa8"};
    const char* diffDescs[]  = {"Perfect for newcomers", "Standard gameplay", "Fast & challenging", "For experienced players", "Ultimate test"};

    while (true) {
        erase();
        frame++;

        int cx = 15, sy = 3;

        // Header with animated border
        int headerColor = (frame / 5) % 2 == 0 ? 10 : 15;
        attron(COLOR_PAIR(headerColor) | A_BOLD);
        mvprintw(sy, cx + 5, "\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97");
        mvprintw(sy + 1, cx + 5, "\xe2\x95\x91          CLASSIC MODE          \xe2\x95\x91");
        mvprintw(sy + 2, cx + 5, "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d");
        attroff(COLOR_PAIR(headerColor) | A_BOLD);

        // Difficulty cards with enhanced design
        for (int i = 0; i < 5; ++i) {
            int cardY = sy + 4 + i * 3;
            bool selected = (i == cursor);

            if (selected) {
                // Selected card with glow effect
                int glowColor = (frame / 2) % 2 == 0 ? diffColors[i] : 19;
                attron(COLOR_PAIR(glowColor) | A_BOLD);
                // Card border
                mvprintw(cardY, cx, "\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97");
                // Card content
                mvprintw(cardY+1, cx, "\xe2\x95\x91  %s  [%d] %-10s  Lv.%-2d  \xe2\x95\x91", diffIcons[i], i+1, diffNames[i], diffLevels[i]);
                // Description
                mvprintw(cardY+2, cx, "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d");
                attroff(COLOR_PAIR(glowColor) | A_BOLD);

                // Description below card
                attron(COLOR_PAIR(20) | A_DIM);
                mvprintw(cardY + 3, cx + 4, "%s", diffDescs[i]);
                attroff(COLOR_PAIR(20) | A_DIM);
            } else {
                // Normal card
                attron(COLOR_PAIR(32));
                mvprintw(cardY, cx + 1, "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80");
                mvprintw(cardY+1, cx + 2, "  [%d] %-10s  Lv.%-2d", i+1, diffNames[i], diffLevels[i]);
                mvprintw(cardY+2, cx + 1, "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80");
                attroff(COLOR_PAIR(32));
            }
        }

        // High scores panel with better design
        auto highScores = loadHighScores();
        if (!highScores.empty()) {
            int hsX = cx + 40, hsY = sy + 4;

            // Panel background
            attron(COLOR_PAIR(237));
            for (int r = 0; r < 10; ++r)
                for (int c = 0; c < 28; ++c)
                    mvprintw(hsY + r, hsX + c, " ");
            attroff(COLOR_PAIR(237));

            // Panel border
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(hsY, hsX, "\xe2\x95\x94");
            for (int i = 1; i < 27; ++i) mvprintw(hsY, hsX + i, "\xe2\x95\x90");
            mvprintw(hsY, hsX + 27, "\xe2\x95\x97");
            for (int i = 1; i < 9; ++i) {
                mvprintw(hsY + i, hsX, "\xe2\x95\x91");
                mvprintw(hsY + i, hsX + 27, "\xe2\x95\x91");
            }
            mvprintw(hsY + 9, hsX, "\xe2\x95\x9a");
            for (int i = 1; i < 27; ++i) mvprintw(hsY + 9, hsX + i, "\xe2\x95\x90");
            mvprintw(hsY + 9, hsX + 27, "\xe2\x95\x9d");
            attroff(COLOR_PAIR(19) | A_BOLD);

            // Title
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(hsY + 1, hsX + 6, "\xe2\x98\x85 HIGH SCORES \xe2\x98\x85");
            attroff(COLOR_PAIR(19) | A_BOLD);

            // Scores
            for (int i = 0; i < std::min(5, (int)highScores.size()); ++i) {
                int rc = (i == 0) ? 19 : (i == 1) ? 9 : 11;
                attron(COLOR_PAIR(rc));
                mvprintw(hsY + 3 + i, hsX + 2, "%d. %-8s %7d",
                         i + 1, highScores[i].name.c_str(), highScores[i].score);
                attroff(COLOR_PAIR(rc));
            }
        }

        // Footer with animation
        attron(COLOR_PAIR(11));
        mvprintw(23, cx, "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80");
        attroff(COLOR_PAIR(11));

        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(24, cx + 4, "\xe2\x86\x90\xe2\x86\x91\xe2\x86\x93\xe2\x86\x92 Navigate  |  ENTER Select  |  ESC Back");
        attroff(COLOR_PAIR(10) | A_BOLD);

        // Selected difficulty preview
        attron(COLOR_PAIR(diffColors[cursor]) | A_BOLD);
        mvprintw(26, cx + 8, "Starting at Level %d", diffLevels[cursor]);
        attroff(COLOR_PAIR(diffColors[cursor]) | A_BOLD);

        refresh();

        int ch = getch();
        if (ch == KEY_UP && cursor > 0) { cursor--; playSound(6); }
        if (ch == KEY_DOWN && cursor < 4) { cursor++; playSound(6); }
        if (ch == '\n' || ch == KEY_ENTER) { playSound(6); return diffLevels[cursor]; }
        if (ch == 27) return -1;
        if (ch == 'q' || ch == 'Q') return 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ============================================================
//  Stage select menu - visual cards
// ============================================================
static int showStageSelect() {
    auto stages = getAllStages();
    auto progress = loadStageProgress();
    int cursor = 0;
    int frame = 0;

    while (true) {
        erase();
        frame++;

        int cx = 2, sy = 1;

        // Header
        int hc = (frame / 8) % 7;
        int headerColors[] = {1, 2, 3, 4, 5, 6, 7};
        attron(COLOR_PAIR(headerColors[hc]) | A_BOLD);
        mvprintw(sy, cx + 15, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88 STAGE MODE \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
        attroff(COLOR_PAIR(headerColors[hc]) | A_BOLD);

        // Chapter info
        auto getChapter = [](int id) -> std::pair<std::string, int> {
            if (id <= 3)  return {"Ch.1 Tutorial",  10};
            if (id <= 6)  return {"Ch.2 Survival",  16};
            if (id <= 9)  return {"Ch.3 Tetris",    13};
            if (id <= 12) return {"Ch.4 Intense",   14};
            if (id <= 16) return {"Ch.5 Expert",    15};
            return {"Ch.6 Master", 18};
        };

        // Stage grid layout: 4 columns x 5 rows
        std::string lastCh;
        int gridStartY = sy + 2;
        for (int i = 0; i < (int)stages.size(); ++i) {
            int col = i % 4;
            int row = i / 4;
            int cardX = cx + col * 18;
            int cardY = gridStartY + row * 5;

            auto [chName, chColor] = getChapter(stages[i].id);

            bool unlocked = (i == 0 || progress[i-1].stars > 0);
            bool selected = (i == cursor);

            if (selected) {
                // Selected - glowing border
                int glowColor = headerColors[(frame / 3) % 7];
                attron(COLOR_PAIR(glowColor) | A_BOLD);
                mvprintw(cardY, cardX, "\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97");
                attroff(COLOR_PAIR(glowColor) | A_BOLD);
            } else {
                attron(COLOR_PAIR(11));
                mvprintw(cardY, cardX, "\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97");
                attroff(COLOR_PAIR(11));
            }

            if (unlocked) {
                attron(COLOR_PAIR(selected ? chColor : 10));
                mvprintw(cardY + 1, cardX, "\xe2\x95\x91 #%-2d          \xe2\x95\x91", stages[i].id);
                attroff(COLOR_PAIR(selected ? chColor : 10));

                attron(COLOR_PAIR(selected ? chColor : 20));
                mvprintw(cardY + 2, cardX, "\xe2\x95\x91 %-12s  \xe2\x95\x91", stages[i].name.substr(0, 12).c_str());
                attroff(COLOR_PAIR(selected ? chColor : 20));

                // Stars
                attron(COLOR_PAIR(19));
                std::string starStr;
                for (int s = 0; s < 3; ++s)
                    starStr += (s < progress[i].stars) ? "\xe2\x98\x85" : "\xe2\x98\x86";
                mvprintw(cardY + 3, cardX, "\xe2\x95\x91  %s          \xe2\x95\x91", starStr.c_str());
                attroff(COLOR_PAIR(19));
            } else {
                attron(COLOR_PAIR(17));
                mvprintw(cardY + 1, cardX, "\xe2\x95\x91 #%-2d  \xf0\x9f\x94\x92      \xe2\x95\x91", stages[i].id);
                mvprintw(cardY + 2, cardX, "\xe2\x95\x91  locked       \xe2\x95\x91");
                mvprintw(cardY + 3, cardX, "\xe2\x95\x91              \xe2\x95\x91");
                attroff(COLOR_PAIR(17));
            }

            attron(COLOR_PAIR(11));
            mvprintw(cardY + 4, cardX, "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d");
            attroff(COLOR_PAIR(11));
        }

        // Detail panel on the right
        int detX = cx + 73, detY = sy + 1;
        auto [chName, chColor] = getChapter(stages[cursor].id);
        drawPanel(detX, detY, 30, 12, "STAGE INFO", chColor);

        attron(COLOR_PAIR(chColor) | A_BOLD);
        mvprintw(detY + 2, detX + 2, "Stage %d: %s", stages[cursor].id, stages[cursor].name.c_str());
        attroff(COLOR_PAIR(chColor) | A_BOLD);

        attron(COLOR_PAIR(20));
        mvprintw(detY + 3, detX + 2, "Chapter: %s", chName.c_str());
        attroff(COLOR_PAIR(20));

        attron(COLOR_PAIR(9));
        mvprintw(detY + 5, detX + 2, "Objective:");
        attroff(COLOR_PAIR(9));
        attron(COLOR_PAIR(10));
        mvprintw(detY + 6, detX + 2, "  %s", stages[cursor].description.c_str());
        attroff(COLOR_PAIR(10));

        attron(COLOR_PAIR(20));
        mvprintw(detY + 8, detX + 2, "Start Lv: %d", stages[cursor].startLevel);
        if (stages[cursor].timeLimit > 0)
            mvprintw(detY + 9, detX + 2, "Time: %ds", stages[cursor].timeLimit);
        else
            mvprintw(detY + 9, detX + 2, "Time: --");
        if (stages[cursor].disableHold) {
            attron(COLOR_PAIR(18));
            mvprintw(detY + 10, detX + 2, "Hold: DISABLED");
            attroff(COLOR_PAIR(18));
        } else {
            mvprintw(detY + 10, detX + 2, "Hold: enabled");
        }
        attroff(COLOR_PAIR(20));

        // Controls with better styling
        attron(COLOR_PAIR(11));
        for (int i = 0; i < 60; ++i)
            mvprintw(24, cx + i, "\xe2\x94\x80");
        attroff(COLOR_PAIR(11));

        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(25, cx + 2, "\xe2\x86\x90\xe2\x86\x91\xe2\x86\x93\xe2\x86\x92 Navigate  |  ENTER Play  |  ESC Back");
        attroff(COLOR_PAIR(10) | A_BOLD);

        // Ambient floating particles
        for (int i = 0; i < 15; ++i) {
            float angle = (frame * 0.08f + i * 0.42f);
            float radius = 35.0f + std::sin(frame * 0.05f + i * 0.7f) * 8.0f;
            int px = (int)(40 + std::cos(angle) * radius);
            int py = (int)(14 + std::sin(angle) * radius * 0.4f);
            if (px >= 0 && px < 78 && py >= 0 && py < 28) {
                int pc = (i % 4 == 0) ? 18 : (i % 4 == 1) ? 19 : (i % 4 == 2) ? 16 : 13;
                attron(COLOR_PAIR(pc) | A_DIM);
                mvprintw(py, px, "\xe2\x98\x86");
                attroff(COLOR_PAIR(pc) | A_DIM);
            }
        }

        refresh();

        while (true) {
            int ch = getch();
            if (ch == KEY_UP && cursor >= 4) { cursor -= 4; playSound(6); break; }
            if (ch == KEY_DOWN && cursor < (int)stages.size() - 4) { cursor += 4; playSound(6); break; }
            if (ch == KEY_LEFT && cursor > 0) { cursor--; playSound(6); break; }
            if (ch == KEY_RIGHT && cursor < (int)stages.size() - 1) { cursor++; playSound(6); break; }
            if (ch == '\n' || ch == KEY_ENTER) {
                bool unlocked = (cursor == 0 || progress[cursor - 1].stars > 0);
                if (unlocked) { playSound(6); return cursor; }
                playSound(1);
                break;
            }
            if (ch == 27) return -1;
            if (ch == 'q' || ch == 'Q') return -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

// ============================================================
//  Render the game board - the visual core
// ============================================================
static void renderGame(const Game& game) {
    vis.globalFrame++;
    const Board& b = game.board();
    const Piece& cur = game.current();
    int ghost = game.ghostRow();

    erase();

    // Screen shake offset
    int shakeX = 0, shakeY = 0;
    if (vis.shakeFrames > 0) {
        shakeX = (rand() % 3 - 1) * vis.shakeIntensity;
        shakeY = (rand() % 2) * vis.shakeIntensity;
        vis.shakeFrames--;
    }

    // Board position
    int boardX = 2 + shakeX;
    int boardY = 1 + shakeY;
    int cellW = 2;

    // Title bar
    if (game.mode() == GameMode::Stage) {
        const Stage& stg = game.currentStage();
        attron(COLOR_PAIR(19) | A_BOLD);
        mvprintw(0, 2, "\xe2\x98\x85 STAGE %d: %s", stg.id, stg.name.c_str());
        attroff(COLOR_PAIR(19) | A_BOLD);

        // Time remaining indicator (enhanced with urgency effects)
        int timeLeft = game.stageTimeRemaining();
        if (timeLeft >= 0) {
            // Pulsing time display when low
            int timePulse = (vis.globalFrame / 3) % 4;
            int tc;
            if (timeLeft <= 5) tc = (timePulse < 2) ? 18 : 32;  // Very urgent - flashing
            else if (timeLeft <= 10) tc = 18;  // Urgent - red
            else if (timeLeft <= 20) tc = 14;  // Warning - orange
            else tc = 9;  // Normal

            attron(COLOR_PAIR(tc) | A_BOLD);
            mvprintw(0, 50, "TIME: %3ds", timeLeft);
            attroff(COLOR_PAIR(tc) | A_BOLD);

            // Urgency bar with gradient
            if (game.currentStage().timeLimit > 0) {
                float tRatio = (float)timeLeft / game.currentStage().timeLimit;
                int barW = 12;
                int filled = (int)(tRatio * barW);
                for (int i = 0; i < barW; ++i) {
                    int barColor;
                    if (i < filled) {
                        float posRatio = (float)i / barW;
                        if (posRatio < 0.3) barColor = 18;      // Red zone
                        else if (posRatio < 0.6) barColor = 14; // Orange zone
                        else barColor = 16;                      // Green zone
                    } else {
                        barColor = 32;
                    }
                    attron(COLOR_PAIR(barColor));
                    mvprintw(0, 62 + i, "\xe2\x96\x88");
                    attroff(COLOR_PAIR(barColor));
                }
            }

            // Urgent time warning flash
            if (timeLeft <= 10 && timePulse < 2) {
                attron(COLOR_PAIR(18) | A_BOLD);
                mvprintw(0, 75, "\xe2\x9a\xa0");
                attroff(COLOR_PAIR(18) | A_BOLD);
            }
        }
    } else {
        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(0, 4, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88 T E T R I S \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
        attroff(COLOR_PAIR(10) | A_BOLD);
    }

    // Danger warning when blocks are near the top
    if (!game.gameOver() && !game.paused()) {
        // Check if any block is in the top 4 rows
        bool danger = false;
        for (int r = HIDDEN; r < HIDDEN + 4; ++r) {
            for (int c = 0; c < BOARD_W; ++c) {
                if (b.occupied(r, c)) { danger = true; break; }
            }
            if (danger) break;
        }

        if (danger) {
            int dangerPulse = (vis.globalFrame / 2) % 4;
            int dangerColor = (dangerPulse < 2) ? 18 : 32;

            // Flashing border effect
            attron(COLOR_PAIR(dangerColor) | A_BOLD);
            for (int c = 0; c < BOARD_W * cellW; ++c) {
                mvprintw(boardY - 1, boardX + c, "\xe2\x96\x88");
            }
            attroff(COLOR_PAIR(dangerColor) | A_BOLD);

            // Danger text
            if (dangerPulse < 3) {
                attron(COLOR_PAIR(18) | A_BOLD);
                mvprintw(0, boardX + BOARD_W * cellW + 3, "\xe2\x9a\xa0 DANGER! \xe2\x9a\xa0");
                attroff(COLOR_PAIR(18) | A_BOLD);
            }
        }
    }

    // Board background with subtle animated grid
    attron(COLOR_PAIR(12));
    for (int vr = 0; vr < BOARD_H; ++vr) {
        for (int c = 0; c < BOARD_W; ++c) {
            int py = boardY + vr;
            int px = boardX + c * cellW;
            mvprintw(py, px, "  ");
        }
    }
    attroff(COLOR_PAIR(12));

    // Animated grid lines for atmosphere
    for (int vr = 0; vr < BOARD_H; vr += 4) {
        for (int c = 0; c < BOARD_W; ++c) {
            int py = boardY + vr;
            int px = boardX + c * cellW;
            if ((vis.globalFrame / 20 + vr + c) % 8 == 0) {
                attron(COLOR_PAIR(30) | A_DIM);
                mvprintw(py, px, "\xe2\x80\xa2 ");
                attroff(COLOR_PAIR(30) | A_DIM);
            }
        }
    }

    // Hard drop trail (enhanced with landing flash)
    if (vis.trailFrames > 0) {
        for (int c = 0; c < BOARD_W; ++c) {
            if (vis.hardDropTrail[c] >= HIDDEN) {
                int vr = vis.hardDropTrail[c] - HIDDEN;
                int py = boardY + vr;
                int px = boardX + c * cellW;
                int trailColor = static_cast<int>(cur.type()) + 1;

                // Trail with fading effect
                if (vis.trailFrames > 3) {
                    attron(COLOR_PAIR(trailColor) | A_BOLD);
                    mvprintw(py, px, "\xe2\x96\x91\xe2\x96\x91"); // ░░
                    attroff(COLOR_PAIR(trailColor) | A_BOLD);
                } else {
                    // Landing flash at the bottom of trail
                    attron(COLOR_PAIR(19) | A_BOLD);
                    mvprintw(py, px, "\xe2\x96\x88\xe2\x96\x88"); // ██ bright
                    attroff(COLOR_PAIR(19) | A_BOLD);
                }

                // Landing impact effect at the bottom
                if (vis.trailFrames == 5 && vr < BOARD_H - 1) {
                    attron(COLOR_PAIR(19) | A_BOLD);
                    mvprintw(py + 1, px - 2, "\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x90"); // ╭──╮
                    attroff(COLOR_PAIR(19) | A_BOLD);
                }
            }
        }
        vis.trailFrames--;
    }

    // Clear animation (enhanced with particle effects)
    if (vis.clearAnimFrames > 0) {
        int flashCycle = vis.clearAnimFrames % 4;
        int clearStartRow = BOARD_H - vis.lastLinesCleared;

        // Flash rows with color cycling
        for (int r = 0; r < BOARD_H; ++r) {
            if (r >= clearStartRow) {
                for (int c = 0; c < BOARD_W; ++c) {
                    int py = boardY + r;
                    int px = boardX + c * cellW;

                    // Color cycle through different flash colors
                    int flashColor;
                    if (flashCycle == 0) flashColor = 40;      // White
                    else if (flashCycle == 1) flashColor = 19;  // Gold
                    else if (flashCycle == 2) flashColor = 42;  // Green
                    else flashColor = 16;                        // Bright green

                    attron(COLOR_PAIR(flashColor) | A_BOLD);
                    mvprintw(py, px, "\xe2\x96\x88\xe2\x96\x88");
                    attroff(COLOR_PAIR(flashColor) | A_BOLD);
                }

                // Scattered particles on cleared rows
                if (flashCycle % 2 == 0) {
                    for (int p = 0; p < 3; ++p) {
                        int particleX = boardX + ((vis.clearAnimFrames * 3 + p * 7 + r * 5) % (BOARD_W * 2));
                        int particleY = boardY + r;
                        int pColor = (p % 3 == 0) ? 19 : (p % 3 == 1) ? 16 : 13;
                        attron(COLOR_PAIR(pColor) | A_BOLD);
                        mvprintw(particleY, particleX, "\xe2\x98\x86");
                        attroff(COLOR_PAIR(pColor) | A_BOLD);
                    }
                }
            }
        }

        // Side sparkles for dramatic effect
        if (vis.clearAnimFrames > 4) {
            for (int i = 0; i < vis.lastLinesCleared; ++i) {
                int sparkleY = boardY + clearStartRow + i;
                attron(COLOR_PAIR(19) | A_BOLD);
                mvprintw(sparkleY, boardX - 2, "\xe2\x98\xa2"); // Left
                mvprintw(sparkleY, boardX + BOARD_W * 2 + 1, "\xe2\x98\xa2"); // Right
                attroff(COLOR_PAIR(19) | A_BOLD);
            }
        }

        vis.clearAnimFrames--;
    }

    // Ghost piece (enhanced with pulsing animation and piece color)
    int ghostPulse = (vis.globalFrame / 3) % 4;
    int ghostTopColors[] = {8, 8, 8, 58};  // cycle through subtle styles

    for (int vr = 0; vr < BOARD_H; ++vr) {
        int br = vr + HIDDEN;
        for (int c = 0; c < BOARD_W; ++c) {
            int dr = br - ghost;
            int dc = c - cur.col();
            bool isGhost = (dr >= 0 && dr < cur.size() && dc >= 0 && dc < cur.size()
                          && cur.shape()[dr][dc] && !b.occupied(br, c));
            int cdr = br - cur.row();
            bool isCur = (cdr >= 0 && cdr < cur.size() && dc >= 0 && dc < cur.size()
                         && cur.shape()[cdr][dc]);
            isGhost = isGhost && !isCur;

            if (isGhost) {
                int py = boardY + vr;
                int px = boardX + c * cellW;
                // Pulsing ghost with piece-type color hint
                int gp = ghostTopColors[ghostPulse];
                attron(COLOR_PAIR(gp));
                mvprintw(py, px, "\xe2\x94\x8c\xe2\x94\x90"); // ╭╮
                attroff(COLOR_PAIR(gp));
            }
        }
    }

    // Current piece (enhanced with glow)
    int pieceGlowPhase = (vis.globalFrame / 4) % 3;
    for (int vr = 0; vr < BOARD_H; ++vr) {
        int br = vr + HIDDEN;
        for (int c = 0; c < BOARD_W; ++c) {
            int dr = br - cur.row();
            int dc = c - cur.col();
            bool isCurrent = (dr >= 0 && dr < cur.size() && dc >= 0 && dc < cur.size()
                            && cur.shape()[dr][dc]);

            if (isCurrent) {
                int py = boardY + vr;
                int px = boardX + c * cellW;
                int cp = static_cast<int>(cur.type()) + 1;

                // Subtle glow effect around piece
                if (pieceGlowPhase == 0) {
                    // Check if this is an edge cell (adjacent to empty)
                    bool isEdge = false;
                    for (int adj = 0; adj < 4; ++adj) {
                        int ar = dr + "0102"[adj] - '1';
                        int ac = dc + "1021"[adj] - '1';
                        if (ar < 0 || ar >= cur.size() || ac < 0 || ac >= cur.size() || !cur.shape()[ar][ac]) {
                            isEdge = true;
                            break;
                        }
                    }
                    if (isEdge) {
                        attron(COLOR_PAIR(cp) | A_DIM);
                        // Top glow
                        if (py > boardY) mvprintw(py - 1, px, "\xe2\x96\x81\xe2\x96\x81");
                        // Left glow
                        if (px > boardX) mvprintw(py, px - 2, "\xe2\x96\x8c");
                        attroff(COLOR_PAIR(cp) | A_DIM);
                    }
                }

                attron(COLOR_PAIR(cp) | A_BOLD);
                mvprintw(py, px, "\xe2\x96\x88\xe2\x96\x88"); // ██
                attroff(COLOR_PAIR(cp) | A_BOLD);
            }
        }
    }

    // Board cells (locked blocks) with enhanced 3D effect
    for (int vr = 0; vr < BOARD_H; ++vr) {
        int br = vr + HIDDEN;
        for (int c = 0; c < BOARD_W; ++c) {
            int py = boardY + vr;
            int px = boardX + c * cellW;

            // Skip if current piece is here
            int dr = br - cur.row();
            int dc = c - cur.col();
            bool isCurrent = (dr >= 0 && dr < cur.size() && dc >= 0 && dc < cur.size()
                            && cur.shape()[dr][dc]);
            if (isCurrent) continue;

            int color = b.at(br, c);
            if (color) {
                int cpair;
                if (color >= 8) {
                    cpair = 50;   // garbage
                } else {
                    cpair = color;
                }
                // 3D block effect: bright top, darker bottom
                attron(COLOR_PAIR(cpair) | A_BOLD);
                mvprintw(py, px, "\xe2\x96\x88\xe2\x96\x88"); // ██
                attroff(COLOR_PAIR(cpair) | A_BOLD);

                // Subtle highlight on top-left corner
                if (vr > 0 && !b.occupied(br - 1, c)) {
                    attron(COLOR_PAIR(cpair) | A_DIM);
                    mvprintw(py, px, "\xe2\x96\x88");
                    attroff(COLOR_PAIR(cpair) | A_DIM);
                }
            } else {
                // Empty cell with animated subtle grid
                int gridPattern = (vr + c + (vis.globalFrame / 15)) % 4;
                if (gridPattern == 0) {
                    attron(COLOR_PAIR(30));
                    mvprintw(py, px, "  ");
                    attroff(COLOR_PAIR(30));
                } else if (gridPattern == 1) {
                    attron(COLOR_PAIR(30) | A_DIM);
                    mvprintw(py, px, " \xe2\x80\xa2");
                    attroff(COLOR_PAIR(30) | A_DIM);
                } else {
                    attron(COLOR_PAIR(31) | A_DIM);
                    mvprintw(py, px, " .");
                    attroff(COLOR_PAIR(31) | A_DIM);
                }
            }
        }
    }

    // Ghost piece bottom half (enhanced)
    for (int vr = 0; vr < BOARD_H; ++vr) {
        int br = vr + HIDDEN;
        for (int c = 0; c < BOARD_W; ++c) {
            int dr = br - ghost;
            int dc = c - cur.col();
            bool isGhost = (dr >= 0 && dr < cur.size() && dc >= 0 && dc < cur.size()
                          && cur.shape()[dr][dc] && !b.occupied(br, c));
            int cdr = br - cur.row();
            bool isCur = (cdr >= 0 && cdr < cur.size() && dc >= 0 && dc < cur.size()
                         && cur.shape()[cdr][dc]);
            isGhost = isGhost && !isCur;

            if (isGhost) {
                int py = boardY + vr;
                int px = boardX + c * cellW;
                int gp = ghostTopColors[ghostPulse];
                attron(COLOR_PAIR(gp));
                mvprintw(py + 1, px, "\xe2\x94\x94\xe2\x94\x98"); // ╰╯
                attroff(COLOR_PAIR(gp));
            }
        }
    }

    // Board frame
    drawBoardFrame(boardX, boardY, BOARD_W * cellW, BOARD_H);

    // ---- Right Panel (enhanced) ----
    int panelX = boardX + BOARD_W * cellW + 3;

    // Score panel with animated border and dynamic styling
    int scorePanelColor = (game.score() > 10000) ? 19 : (game.score() > 5000) ? 14 : 9;
    drawPanel(panelX, 1, 22, 6, "SCORE", scorePanelColor);

    // Score with pulsing effect for high scores
    int scorePulse = (vis.globalFrame / 5) % 4;
    int scoreStyle = A_BOLD;
    if (game.score() > 10000 && scorePulse < 2) scoreStyle = A_BOLD;

    attron(COLOR_PAIR(scorePanelColor) | scoreStyle);
    mvprintw(3, panelX + 2, "%-18d", game.score());
    attroff(COLOR_PAIR(scorePanelColor) | scoreStyle);

    // Level and lines with color coding
    int levelColor = (game.level() >= 15) ? 18 : (game.level() >= 10) ? 14 : (game.level() >= 5) ? 13 : 10;
    attron(COLOR_PAIR(levelColor));
    mvprintw(4, panelX + 2, "Lv:%-3d", game.level());
    attroff(COLOR_PAIR(levelColor));

    attron(COLOR_PAIR(10));
    mvprintw(4, panelX + 10, "Ln:%-4d", game.lines());
    attroff(COLOR_PAIR(10));

    // Speed bar with gradient colors
    int speedMs = game.dropMs();
    int speedPct = (int)(100.0f * (1.0f - (float)(speedMs - 50) / 750.0f));
    int speedColor = (speedPct > 80) ? 18 : (speedPct > 50) ? 14 : 16;
    attron(COLOR_PAIR(20));
    mvprintw(5, panelX + 2, "SPD: ");
    attroff(COLOR_PAIR(20));
    drawProgressMini(panelX + 7, 5, speedPct, 100, 10, speedColor, 32);

    // Next pieces panel with glow
    drawPanel(panelX, 8, 22, 10, "NEXT", 10);

    // Show 3 next pieces (we only have 1, but render it centered nicely)
    const Piece& nxt = game.next();
    const Shape& ns = nxt.shape();
    int nsz = nxt.size();
    int np = static_cast<int>(nxt.type()) + 1;

    // Center the piece in the panel with glow effect
    int nextCenterX = panelX + 2;
    int nextCenterY = 10;

    // Draw piece glow
    for (int dr = 0; dr < nsz; ++dr) {
        for (int dc = 0; dc < nsz; ++dc) {
            if (ns[dr][dc]) {
                int py = nextCenterY + dr;
                int px = nextCenterX + dc * 2;
                // Glow effect
                attron(COLOR_PAIR(np) | A_DIM);
                mvprintw(py - 1, px, "  ");
                attroff(COLOR_PAIR(np) | A_DIM);
            }
        }
    }

    // Draw actual piece
    for (int dr = 0; dr < nsz; ++dr) {
        for (int dc = 0; dc < nsz; ++dc) {
            if (ns[dr][dc]) {
                int py = nextCenterY + dr;
                int px = nextCenterX + dc * 2;
                attron(COLOR_PAIR(np) | A_BOLD);
                mvprintw(py, px, "\xe2\x96\x88\xe2\x96\x88");
                attroff(COLOR_PAIR(np) | A_BOLD);
            }
        }
    }

    // Hold panel with enhanced styling
    drawPanel(panelX, 19, 22, 6, "HOLD", 15);

    if (game.mode() == GameMode::Stage && game.currentStage().disableHold) {
        attron(COLOR_PAIR(18) | A_BOLD);
        mvprintw(21, panelX + 4, "\xf0\x9f\x94\x92 LOCKED");
        attroff(COLOR_PAIR(18) | A_BOLD);
        attron(COLOR_PAIR(20) | A_DIM);
        mvprintw(22, panelX + 3, "Cannot use hold");
        attroff(COLOR_PAIR(20) | A_DIM);
    } else if (game.hasHold()) {
        const Piece& h = game.hold();
        const Shape& hs = h.shape();
        int hsz = h.size();
        int hp = static_cast<int>(h.type()) + 1;
        int holdCX = panelX + 2;
        int holdCY = 21;

        // Draw piece with glow
        for (int dr = 0; dr < hsz; ++dr)
            for (int dc = 0; dc < hsz; ++dc)
                if (hs[dr][dc]) {
                    // Glow effect
                    attron(COLOR_PAIR(hp) | A_DIM);
                    mvprintw(holdCY + dr - 1, holdCX + dc * 2, "  ");
                    attroff(COLOR_PAIR(hp) | A_DIM);

                    // Actual piece
                    attron(COLOR_PAIR(hp) | A_BOLD);
                    mvprintw(holdCY + dr, holdCX + dc * 2, "\xe2\x96\x88\xe2\x96\x88");
                    attroff(COLOR_PAIR(hp) | A_BOLD);
                }
        if (!game.canHold()) {
            attron(COLOR_PAIR(17) | A_BOLD);
            mvprintw(24, panelX + 2, "(used)");
            attroff(COLOR_PAIR(17) | A_BOLD);
        }
    } else {
        attron(COLOR_PAIR(20) | A_DIM);
        mvprintw(21, panelX + 4, "(empty)");
        mvprintw(22, panelX + 3, "Press H to hold");
        attroff(COLOR_PAIR(20) | A_DIM);
    }

    // Stage objective or stats
    if (game.mode() == GameMode::Stage) {
        const Stage& stg = game.currentStage();

        // Objective panel with dynamic color based on progress
        int prog = game.stageObjectiveProgress();
        int tgt = stg.targetValue;
        float progressRatio = (tgt > 0) ? (float)prog / tgt : 0.0f;
        int objColor = (progressRatio >= 1.0f) ? 16 : (progressRatio >= 0.8f) ? 19 : (progressRatio >= 0.5f) ? 10 : 9;

        drawPanel(panelX, 26, 22, 4, "OBJECTIVE", objColor);

        attron(COLOR_PAIR(10));
        mvprintw(28, panelX + 2, "%s", stg.description.c_str());
        attroff(COLOR_PAIR(10));

        // Progress bar with gradient colors
        int barColor;
        if (progressRatio >= 1.0f) barColor = 16;       // Complete - green
        else if (progressRatio >= 0.8f) barColor = 19;  // Almost - gold
        else if (progressRatio >= 0.5f) barColor = 14;  // Halfway - orange
        else barColor = 10;                               // Start - blue

        drawProgressMini(panelX + 1, 29, prog, tgt, 16, barColor, 32);

        // Percentage display
        attron(COLOR_PAIR(barColor) | A_BOLD);
        mvprintw(29, panelX + 18, "%d/%d", prog, tgt);
        attroff(COLOR_PAIR(barColor) | A_BOLD);

        // Celebration sparkle when complete
        if (progressRatio >= 1.0f && vis.globalFrame % 6 < 3) {
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(29, panelX + 20, "\xe2\x98\x85");
            attroff(COLOR_PAIR(19) | A_BOLD);
        }
    } else {
        drawPanel(panelX, 26, 22, 4, "STATS", 9);
        int elapsed = game.elapsedSeconds();

        // Time with color coding
        int timeColor = (elapsed > 300) ? 14 : (elapsed > 180) ? 10 : 9;
        attron(COLOR_PAIR(timeColor));
        mvprintw(28, panelX + 2, "Time: %d:%02d", elapsed / 60, elapsed % 60);
        attroff(COLOR_PAIR(timeColor));

        // Pieces and combo with styling
        attron(COLOR_PAIR(20));
        mvprintw(29, panelX + 2, "Pcs:%-3d", game.piecesPlaced());
        attroff(COLOR_PAIR(20));

        int comboColor = (game.maxCombo() >= 5) ? 14 : (game.maxCombo() >= 3) ? 13 : 19;
        attron(COLOR_PAIR(comboColor));
        mvprintw(29, panelX + 10, "Combo:%-2d", game.maxCombo());
        attroff(COLOR_PAIR(comboColor));
    }

    // ---- Combo / Effects display (enhanced) ----
    if (vis.comboDisplayFrames > 0 && vis.comboDisplayValue > 1) {
        int comboColor = (vis.comboDisplayValue >= 5) ? 14 : (vis.comboDisplayValue >= 3) ? 13 : 19;
        int yOffset = 0;
        if (vis.comboDisplayFrames > 15) yOffset = (20 - vis.comboDisplayFrames);

        // Animated combo background
        if (vis.comboDisplayFrames > 12) {
            attron(COLOR_PAIR(comboColor) | A_DIM);
            for (int i = 0; i < 12; ++i) {
                int sparkleX = boardX + 1 + ((vis.comboDisplayFrames * 2 + i * 3) % 18);
                int sparkleY = BOARD_H / 2 - 2 + yOffset + ((i * 2) % 5) - 2;
                mvprintw(sparkleY, sparkleX, "\xe2\x98\x86");
            }
            attroff(COLOR_PAIR(comboColor) | A_DIM);
        }

        attron(COLOR_PAIR(comboColor) | A_BOLD);
        if (vis.comboDisplayValue >= 5) {
            mvprintw(BOARD_H / 2 - 3 + yOffset, boardX + 1,
                     "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            mvprintw(BOARD_H / 2 - 2 + yOffset, boardX + 2, "  FIRE!  ");
            mvprintw(BOARD_H / 2 - 1 + yOffset, boardX + 1,
                     "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
        }
        mvprintw(BOARD_H / 2 + 1 + yOffset, boardX + 3,
                 "COMBO x%d", vis.comboDisplayValue);
        attroff(COLOR_PAIR(comboColor) | A_BOLD);

        // Score popup with animation
        if (vis.comboDisplayValue >= 3) {
            int scorePopY = BOARD_H / 2 + 3 + yOffset;
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(scorePopY, boardX + 2, "+%d bonus!",
                     50 * vis.comboDisplayValue * game.level());
            attroff(COLOR_PAIR(19) | A_BOLD);
        }
        vis.comboDisplayFrames--;
    }

    // Tetris flash (enhanced)
    if (vis.tetrisFlashFrames > 0) {
        int flashColor = (vis.tetrisFlashFrames % 2 == 0) ? 3 : 13;
        int flashOffset = (vis.tetrisFlashFrames % 4 < 2) ? 0 : 1;

        attron(COLOR_PAIR(flashColor) | A_BOLD);
        // Tetris banner with border
        for (int i = 0; i < 16; ++i) {
            mvprintw(BOARD_H / 2 - 3, boardX + 1 + i, "\xe2\x96\x88");
            mvprintw(BOARD_H / 2 + 1, boardX + 1 + i, "\xe2\x96\x88");
        }
        mvprintw(BOARD_H / 2 - 2 + flashOffset, boardX + 2,
                 " T E T R I S ! ");
        attroff(COLOR_PAIR(flashColor) | A_BOLD);

        vis.tetrisFlashFrames--;
    }

    // Level up flash (enhanced)
    if (vis.levelUpFrames > 0) {
        int luColor = (vis.levelUpFrames % 3 == 0) ? 19 : 9;
        int luOffset = (vis.levelUpFrames % 4 < 2) ? 0 : 1;

        // Level up background glow
        attron(COLOR_PAIR(luColor) | A_DIM);
        for (int i = 0; i < 8; ++i) {
            int sparkleX = boardX + 2 + ((vis.levelUpFrames * 2 + i * 3) % 14);
            int sparkleY = BOARD_H / 2 + 2 + ((i * 2) % 3) - 1;
            mvprintw(sparkleY, sparkleX, "\xe2\x98\x86");
        }
        attroff(COLOR_PAIR(luColor) | A_DIM);

        attron(COLOR_PAIR(luColor) | A_BOLD);
        // Level up banner
        for (int i = 0; i < 16; ++i) {
            mvprintw(BOARD_H / 2 + 1, boardX + 2 + i, "\xe2\x96\x88");
            mvprintw(BOARD_H / 2 + 5, boardX + 2 + i, "\xe2\x96\x88");
        }
        mvprintw(BOARD_H / 2 + 2 + luOffset, boardX + 3,
                 "LEVEL UP! Lv.%d", game.level());
        attroff(COLOR_PAIR(luColor) | A_BOLD);
        vis.levelUpFrames--;
    }

    // Score flyup effect
    if (vis.scoreFlyupFrames > 0) {
        int flyY = boardY + 5 - (15 - vis.scoreFlyupFrames);
        int flyAlpha = vis.scoreFlyupFrames;

        // Fade effect based on remaining frames
        int flyColor = (flyAlpha > 10) ? 19 : (flyAlpha > 5) ? 16 : 20;
        attron(COLOR_PAIR(flyColor) | A_BOLD);
        mvprintw(flyY, boardX + 3, "+%d", vis.scoreFlyupValue);
        attroff(COLOR_PAIR(flyColor) | A_BOLD);

        // Trailing sparkle
        if (flyAlpha > 8) {
            attron(COLOR_PAIR(19) | A_DIM);
            mvprintw(flyY + 1, boardX + 5, "\xe2\x98\x86");
            attroff(COLOR_PAIR(19) | A_DIM);
        }

        vis.scoreFlyupFrames--;
    }

    // Game over overlay
    if (game.gameOver()) {
        // Semi-transparent overlay with gradient effect
        for (int vr = BOARD_H / 2 - 4; vr <= BOARD_H / 2 + 4; ++vr) {
            for (int c = 0; c < BOARD_W; ++c) {
                if (vr >= 0 && vr < BOARD_H) {
                    int dist = abs(vr - BOARD_H / 2);
                    int overlayColor = (dist < 2) ? 41 : (dist < 3) ? 40 : 41;
                    attron(COLOR_PAIR(overlayColor) | A_BOLD);
                    mvprintw(boardY + vr, boardX + c * 2, "  ");
                    attroff(COLOR_PAIR(overlayColor) | A_BOLD);
                }
            }
        }

        if (game.mode() == GameMode::Stage && game.stageComplete()) {
            // Stage clear with celebration effects
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 - 3, boardX + 1, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            attroff(COLOR_PAIR(19) | A_BOLD);

            attron(COLOR_PAIR(16) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 - 2, boardX + 2, "\xe2\x9c\x88 STAGE CLEAR! \xe2\x9c\x88");
            attroff(COLOR_PAIR(16) | A_BOLD);

            attron(COLOR_PAIR(16));
            mvprintw(boardY + BOARD_H / 2 - 1, boardX + 4, "Congratulations!");
            attroff(COLOR_PAIR(16));

            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 + 1, boardX + 3, "Score: %d", game.score());
            attroff(COLOR_PAIR(19) | A_BOLD);

            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 + 3, boardX + 1, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            attroff(COLOR_PAIR(19) | A_BOLD);
        } else {
            // Game over with dramatic effect
            attron(COLOR_PAIR(5) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 - 3, boardX + 1, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            attroff(COLOR_PAIR(5) | A_BOLD);

            attron(COLOR_PAIR(5) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 - 2, boardX + 2, "\xe2\x9b\xb1 GAME OVER \xe2\x9b\xb1");
            attroff(COLOR_PAIR(5) | A_BOLD);

            attron(COLOR_PAIR(9));
            mvprintw(boardY + BOARD_H / 2 - 1, boardX + 4, "Better luck next time!");
            attroff(COLOR_PAIR(9));

            attron(COLOR_PAIR(9) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 + 1, boardX + 3, "Score: %d", game.score());
            attroff(COLOR_PAIR(9) | A_BOLD);

            attron(COLOR_PAIR(5) | A_BOLD);
            mvprintw(boardY + BOARD_H / 2 + 3, boardX + 1, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            attroff(COLOR_PAIR(5) | A_BOLD);
        }
        attron(COLOR_PAIR(20));
        mvprintw(boardY + BOARD_H / 2 + 5, boardX + 1, "[R] Retry [M] Menu [Q] Quit");
        attroff(COLOR_PAIR(20));
    }

    // Pause overlay (enhanced)
    if (game.paused() && !game.gameOver()) {
        int pausePulse = (vis.globalFrame / 4) % 4;

        // Frosted glass effect with gradient
        for (int vr = BOARD_H / 2 - 4; vr <= BOARD_H / 2 + 4; ++vr) {
            for (int c = 0; c < BOARD_W; ++c) {
                if (vr >= 0 && vr < BOARD_H) {
                    int dist = abs(vr - BOARD_H / 2);
                    int pauseColor;
                    if (dist < 2) pauseColor = 40;
                    else if (dist < 3) pauseColor = 32;
                    else pauseColor = 32;
                    attron(COLOR_PAIR(pauseColor) | A_BOLD);
                    mvprintw(boardY + vr, boardX + c * 2, "  ");
                    attroff(COLOR_PAIR(pauseColor) | A_BOLD);
                }
            }
        }

        // Animated pause icon
        int iconColor = (pausePulse < 2) ? 10 : 32;
        attron(COLOR_PAIR(iconColor) | A_BOLD);
        mvprintw(boardY + BOARD_H / 2 - 2, boardX + 6, "\xe2\x8f\xb8");
        attroff(COLOR_PAIR(iconColor) | A_BOLD);

        attron(COLOR_PAIR(10) | A_BOLD);
        mvprintw(boardY + BOARD_H / 2 - 1, boardX + 4, "  P A U S E D  ");
        attroff(COLOR_PAIR(10) | A_BOLD);

        attron(COLOR_PAIR(20));
        mvprintw(boardY + BOARD_H / 2 + 1, boardX + 2, "Press P to resume");
        attroff(COLOR_PAIR(20));

        // Floating decorative particles
        for (int i = 0; i < 6; ++i) {
            int px = boardX + 2 + ((vis.globalFrame * 2 + i * 5) % 16);
            int py = boardY + BOARD_H / 2 - 3 + ((vis.globalFrame + i * 7) % 7);
            if ((vis.globalFrame + i) % 8 < 3) {
                attron(COLOR_PAIR(31) | A_DIM);
                mvprintw(py, px, "\xe2\x98\x86");
                attroff(COLOR_PAIR(31) | A_DIM);
            }
        }
    }

    // Ambient floating particles around the board
    if (!game.gameOver() && !game.paused()) {
        for (int i = 0; i < 8; ++i) {
            float angle = (vis.globalFrame * 0.05f + i * 0.79f);
            float radius = 25.0f + std::sin(vis.globalFrame * 0.03f + i) * 3.0f;
            int px = (int)(boardX + BOARD_W + std::cos(angle) * radius + 10);
            int py = (int)(boardY + BOARD_H / 2 + std::sin(angle) * radius * 0.5f);
            if (px > boardX + BOARD_W * cellW + 2 && px < 78 && py >= 1 && py < 27) {
                int pc = (i % 3 == 0) ? 30 : (i % 3 == 1) ? 31 : 32;
                attron(COLOR_PAIR(pc) | A_DIM);
                mvprintw(py, px, "\xe2\x98\x86");
                attroff(COLOR_PAIR(pc) | A_DIM);
            }
        }
    }

    // ---- Bottom Controls Bar (enhanced) ----
    int ctrlY = boardY + BOARD_H + 1;

    attron(COLOR_PAIR(11));
    mvprintw(ctrlY, boardX - 1, "\xe2\x95\x94");
    for (int i = 0; i < BOARD_W * cellW; ++i) mvprintw(ctrlY, boardX + i, "\xe2\x95\x90");
    mvprintw(ctrlY, boardX + BOARD_W * cellW, "\xe2\x95\x97");
    attroff(COLOR_PAIR(11));

    // Controls with color coding
    attron(COLOR_PAIR(11) | A_BOLD);
    mvprintw(ctrlY + 1, boardX, "\xe2\x86\x90\xe2\x86\x92");
    attroff(COLOR_PAIR(11) | A_BOLD);
    attron(COLOR_PAIR(10));
    mvprintw(ctrlY + 1, boardX + 4, "Move");
    attroff(COLOR_PAIR(10));

    attron(COLOR_PAIR(15) | A_BOLD);
    mvprintw(ctrlY + 1, boardX + 9, "\xe2\x86\x91");
    attroff(COLOR_PAIR(15) | A_BOLD);
    attron(COLOR_PAIR(10));
    mvprintw(ctrlY + 1, boardX + 11, "Rot");
    attroff(COLOR_PAIR(10));

    attron(COLOR_PAIR(14) | A_BOLD);
    mvprintw(ctrlY + 1, boardX + 15, "\xe2\x86\x93");
    attroff(COLOR_PAIR(14) | A_BOLD);
    attron(COLOR_PAIR(10));
    mvprintw(ctrlY + 1, boardX + 17, "Soft");
    attroff(COLOR_PAIR(10));

    attron(COLOR_PAIR(18) | A_BOLD);
    mvprintw(ctrlY + 1, boardX + 22, "Spc");
    attroff(COLOR_PAIR(18) | A_BOLD);
    attron(COLOR_PAIR(10));
    mvprintw(ctrlY + 1, boardX + 26, "Hard");
    attroff(COLOR_PAIR(10));

    attron(COLOR_PAIR(16) | A_BOLD);
    mvprintw(ctrlY + 1, boardX + 31, "H");
    attroff(COLOR_PAIR(16) | A_BOLD);
    attron(COLOR_PAIR(10));
    mvprintw(ctrlY + 1, boardX + 33, "Hold");
    attroff(COLOR_PAIR(10));

    attron(COLOR_PAIR(13) | A_BOLD);
    mvprintw(ctrlY + 1, boardX + 38, "P");
    attroff(COLOR_PAIR(13) | A_BOLD);
    attron(COLOR_PAIR(10));
    mvprintw(ctrlY + 1, boardX + 40, "Pause");
    attroff(COLOR_PAIR(10));

    // ---- Score flyup effect ----
    // (handled in combo display above)

    refresh();
}

// ============================================================
//  Bomb explosion animation for stage failure (enhanced)
// ============================================================
static void showBombExplosion(const Game& game, int stageIdx) {
    // Play explosion sound
    playSound(9);
    struct Particle {
        float x, y;
        float dx, dy;
        int life;
        int maxLife;
        int colorPair;
        const char* symbol;
    };

    std::vector<Particle> particles;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 6.2832f);
    std::uniform_real_distribution<float> speedDist(1.0f, 5.0f);
    std::uniform_int_distribution<int> colorDist(0, 4);

    int boardX = 2;
    int boardY = 1;
    float centerX = boardX + BOARD_W;
    float centerY = boardY + BOARD_H / 2.0f;
    int cellW = 2;

    (void)stageIdx;

    // Phase 1: Bomb drops in with trail
    for (int frame = 0; frame < 8; ++frame) {
        erase();

        // Draw board background
        attron(COLOR_PAIR(12));
        for (int vr = 0; vr < BOARD_H; ++vr) {
            for (int c = 0; c < BOARD_W; ++c) {
                int py = boardY + vr;
                int px = boardX + c * cellW;
                mvprintw(py, px, "  ");
            }
        }
        attroff(COLOR_PAIR(12));
        drawBoardFrame(boardX, boardY, BOARD_W * cellW, BOARD_H);

        // Bomb trail
        int bombY = centerY - 4 + (frame < 4 ? frame * 2 : 7 - frame);
        for (int t = 0; t < 3; ++t) {
            int trailY = bombY - t - 1;
            if (trailY >= boardY && trailY < boardY + BOARD_H) {
                int trailColor = (t == 0) ? 19 : (t == 1) ? 9 : 11;
                attron(COLOR_PAIR(trailColor) | A_DIM);
                mvprintw(trailY, (int)centerX, "\xe2\x80\xa2");  // •
                attroff(COLOR_PAIR(trailColor) | A_DIM);
            }
        }

        // Bomb with bounce animation
        attron(COLOR_PAIR(18) | A_BOLD);
        mvprintw(bombY, (int)centerX - 1, "\xf0\x9f\x94\x8b");  // 💣
        attroff(COLOR_PAIR(18) | A_BOLD);

        // Fuse spark with animation
        if (frame % 2 == 0) {
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(bombY - 1, (int)centerX, "\xe2\x98\x86");  // ☀
            attroff(COLOR_PAIR(19) | A_BOLD);
        } else {
            attron(COLOR_PAIR(14) | A_BOLD);
            mvprintw(bombY - 1, (int)centerX, "\xe2\x9c\xa8");  // ✨
            attroff(COLOR_PAIR(14) | A_BOLD);
        }

        // "STAGE FAILED" text with glow
        int glowIntensity = (frame % 2 == 0) ? 18 : 14;
        attron(COLOR_PAIR(glowIntensity) | A_BOLD);
        mvprintw((int)centerY + 5, (int)centerX - 9, ">>> STAGE FAILED <<<");
        attroff(COLOR_PAIR(glowIntensity) | A_BOLD);

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Phase 2: Dramatic flash sequence
    for (int frame = 0; frame < 4; ++frame) {
        erase();
        // Alternating red/white flash
        int flashColor = (frame % 2 == 0) ? 41 : 40;
        attron(COLOR_PAIR(flashColor) | A_BOLD);
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(flashColor) | A_BOLD);

        // Expanding BOOM text
        if (frame >= 1) {
            int boomColor = (frame < 3) ? 18 : 19;
            attron(COLOR_PAIR(boomColor) | A_BOLD);
            int boomSize = frame * 2;
            // Draw BOOM with border
            for (int dy = -boomSize; dy <= boomSize; ++dy) {
                for (int dx = -boomSize * 2; dx <= boomSize * 2; ++dx) {
                    int bx = (int)centerX + dx;
                    int by = (int)centerY + dy;
                    if (bx >= 0 && bx < 78 && by >= 0 && by < 28) {
                        if (abs(dy) + abs(dx/2) == boomSize) {
                            mvprintw(by, bx, "\xe2\x96\x88");  // █
                        } else if (abs(dy) + abs(dx/2) == boomSize - 1) {
                            mvprintw(by, bx, "\xe2\x96\x92");  // ▒
                        }
                    }
                }
            }
            // Center text
            mvprintw((int)centerY, (int)centerX - 3, "BOOM!");
            attroff(COLOR_PAIR(boomColor) | A_BOLD);
        }

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
    }

    // Phase 3: Explosion particles with improved physics
    int totalFrames = 20;
    // Create explosion particles
    for (int i = 0; i < 60; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        float dx = std::cos(angle) * speed;
        float dy = std::sin(angle) * speed;

        const char* fireSymbols[] = {
            "\xe2\x98\xa2",  // ☢
            "\xe2\x98\xa1",  // ☡
            "\xe2\x9c\xa8",  // ✨
            "*", "#", "X", "@", "~"
        };
        int fireColors[] = {18, 19, 14, 9, 5};

        particles.push_back({
            centerX * 2, centerY,
            dx * 2, dy,
            8 + (int)(speedDist(rng) * 5),
            8 + (int)(speedDist(rng) * 5),
            fireColors[colorDist(rng)],
            fireSymbols[i % 8]
        });
    }

    // Screen shake during explosion
    int shakeFrames = 12;

    for (int frame = 0; frame < totalFrames; ++frame) {
        erase();

        // Screen shake offset
        int shakeX = 0, shakeY = 0;
        if (shakeFrames > 0) {
            shakeX = (rng() % 5 - 2);
            shakeY = (rng() % 3 - 1);
            shakeFrames--;
        }

        // Background with fade
        int bg = (frame < 4) ? 18 : (frame < 8) ? 32 : 236;
        attron(COLOR_PAIR(bg));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r + shakeY, c + shakeX, " ");
        attroff(COLOR_PAIR(bg));

        // Shockwave rings (multiple expanding rings)
        for (int ring = 0; ring < 3; ++ring) {
            int ringFrame = frame - ring * 2;
            if (ringFrame >= 0 && ringFrame < 10) {
                int ringR = ringFrame * 4;
                int ringAlpha = 240 - ringFrame * 15;
                if (ringAlpha < 236) ringAlpha = 236;

                attron(COLOR_PAIR(31));
                for (int a = 0; a < 360; a += 8) {
                    float rad = a * 3.14159f / 180.0f;
                    int rx = (int)centerX + (int)(std::cos(rad) * ringR) + shakeX;
                    int ry = (int)centerY + (int)(std::sin(rad) * ringR) + shakeY;
                    if (rx >= 0 && rx < 78 && ry >= 0 && ry < 28) {
                        const char* ringChars[] = {"\xe2\x96\x91", "\xe2\x96\x92", "\xe2\x96\x93"};
                        mvprintw(ry, rx, "%s", ringChars[ring % 3]);
                    }
                }
                attroff(COLOR_PAIR(31));
            }
        }

        // Fire explosion core
        if (frame < 10) {
            int coreR = 8 - frame;
            if (coreR > 0) {
                int coreColor = (frame < 3) ? 18 : (frame < 6) ? 19 : 9;
                attron(COLOR_PAIR(coreColor) | A_BOLD);
                for (int dy = -coreR; dy <= coreR; ++dy) {
                    for (int dx = -coreR * 2; dx <= coreR * 2; ++dx) {
                        float dist = std::sqrt(dy * dy + (dx * 0.5f) * (dx * 0.5f));
                        if (dist <= coreR) {
                            int gx = (int)centerX / 2 + dx / 2 + shakeX;
                            int gy = (int)centerY + dy + shakeY;
                            if (gx >= 0 && gx < 78 && gy >= 0 && gy < 28) {
                                const char* coreChars[] = {"\xe2\x96\x88", "\xe2\x96\x92", "\xe2\x96\x91"};
                                int charIdx = (dist < coreR * 0.5f) ? 0 : (dist < coreR * 0.8f) ? 1 : 2;
                                mvprintw(gy, gx, "%s", coreChars[charIdx]);
                            }
                        }
                    }
                }
                attroff(COLOR_PAIR(coreColor) | A_BOLD);
            }
        }

        // Particles with trails
        for (auto& p : particles) {
            if (p.life > 0) {
                // Store previous position for trail
                float prevX = p.x - p.dx;
                float prevY = p.y - p.dy;

                p.x += p.dx;
                p.y += p.dy;
                p.dx *= 0.95f;  // Friction
                p.dy += 0.15f;  // Gravity

                float lifeRatio = (float)p.life / p.maxLife;
                int drawX = (int)(p.x / 2) + shakeX;
                int drawY = (int)p.y + shakeY;

                if (drawX >= 0 && drawX < 78 && drawY >= 0 && drawY < 28) {
                    int cp = p.colorPair;
                    if (lifeRatio < 0.3) cp = 31;

                    attron(COLOR_PAIR(cp) | A_BOLD);
                    if (lifeRatio > 0.7) {
                        mvprintw(drawY, drawX, "%s", p.symbol);
                    } else if (lifeRatio > 0.4) {
                        mvprintw(drawY, drawX, "*");
                    } else {
                        mvprintw(drawY, drawX, ".");
                    }
                    attroff(COLOR_PAIR(cp) | A_BOLD);

                    // Draw trail
                    if (lifeRatio > 0.5 && frame < 12) {
                        int trailX = (int)(prevX / 2) + shakeX;
                        int trailY = (int)prevY + shakeY;
                        if (trailX >= 0 && trailX < 78 && trailY >= 0 && trailY < 28) {
                            attron(COLOR_PAIR(31));
                            mvprintw(trailY, trailX, "\xe2\x80\xa2");
                            attroff(COLOR_PAIR(31));
                        }
                    }
                }
                p.life--;
            }
        }

        // Sparks flying outward
        if (frame < 15) {
            for (int i = 0; i < 20; ++i) {
                float sparkAngle = angleDist(rng) + frame * 0.3f;
                float sparkDist = frame * 3 + (i * 2);
                int sx = (int)(centerX + std::cos(sparkAngle) * sparkDist) + shakeX;
                int sy = (int)(centerY + std::sin(sparkAngle) * sparkDist * 0.6f) + shakeY;
                if (sx >= 0 && sx < 78 && sy >= 0 && sy < 28 && (frame + i) % 3 == 0) {
                    attron(COLOR_PAIR(19) | A_BOLD);
                    mvprintw(sy, sx, "\xe2\x98\x86");
                    attroff(COLOR_PAIR(19) | A_BOLD);
                }
            }
        }

        // Smoke rising at the end
        if (frame >= 8) {
            for (int i = 0; i < 12; ++i) {
                int smokeX = (int)centerX + ((i * 5 + frame * 2) % 20) - 10 + shakeX;
                int smokeY = (int)centerY - (frame - 8) * 2 - (i % 4) + shakeY;
                if (smokeX >= 0 && smokeX < 78 && smokeY >= 0 && smokeY < 28) {
                    attron(COLOR_PAIR(31));
                    mvprintw(smokeY, smokeX, "\xe2\x96\x91");
                    attroff(COLOR_PAIR(31));
                }
            }
        }

        // Debris falling
        if (frame >= 6) {
            for (int d = 0; d < 15; ++d) {
                int debrisX = (int)centerX + ((d * 7 + frame * 3) % 25) - 12 + shakeX;
                int debrisY = (int)centerY + (frame - 6) * 2 + (d % 5) + shakeY;
                if (debrisX >= 0 && debrisX < 78 && debrisY >= 0 && debrisY < 28) {
                    attron(COLOR_PAIR((d % 3 == 0) ? 18 : 31));
                    const char* debrisChars[] = {"\xe2\x96\x92", "\xe2\x96\x93", ".", ":"};
                    mvprintw(debrisY, debrisX, "%s", debrisChars[d % 3]);
                    attroff(COLOR_PAIR((d % 3 == 0) ? 18 : 31));
                }
            }
        }

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    // Final dramatic fade
    for (int fade = 0; fade < 5; ++fade) {
        erase();
        // Gradient fade
        int bgColor = (fade < 2) ? 236 : (fade < 3) ? 237 : 238;
        attron(COLOR_PAIR(bgColor));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(bgColor));

        if (fade == 2) {
            // Dramatic GAME OVER reveal
            attron(COLOR_PAIR(18) | A_BOLD);
            mvprintw((int)centerY - 1, (int)centerX - 9, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            mvprintw((int)centerY, (int)centerX - 9, " \xe2\x9b\xb1  GAME  OVER  \xe2\x9b\xb1 ");
            mvprintw((int)centerY + 1, (int)centerX - 9, "\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88");
            attroff(COLOR_PAIR(18) | A_BOLD);

            attron(COLOR_PAIR(9));
            mvprintw((int)centerY + 3, (int)centerX - 6, "Final Score: %d", game.score());
            attroff(COLOR_PAIR(9));
        }
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

// ============================================================
//  Stage clear celebration animation
// ============================================================
static void showStageClearCelebration(const Game& game, int stageIdx) {
    auto stages = getAllStages();
    std::mt19937 rng(std::random_device{}());

    // Create celebration particles
    struct CelebrParticle {
        float x, y, dx, dy;
        int life, color;
        const char* symbol;
    };

    std::vector<CelebrParticle> parts;
    for (int i = 0; i < 40; ++i) {
        float angle = (i * 0.157f);
        float speed = 2.0f + (i % 5) * 0.5f;
        const char* syms[] = {"\xe2\x98\x86", "\xe2\x98\x85", "\xe2\x9c\xa8", "*", "+"};
        int colors[] = {19, 16, 13, 9, 10};
        parts.push_back({
            40.0f, 15.0f,
            std::cos(angle) * speed, std::sin(angle) * speed * 0.6f - 2.0f,
            15 + (i % 10),
            colors[i % 5],
            syms[i % 5]
        });
    }

    for (int frame = 0; frame < 25; ++frame) {
        erase();

        // Background
        attron(COLOR_PAIR(32));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(32));

        // Central stage clear text with glow
        int textGlow = (frame / 2) % 3;
        int textColor = (textGlow == 0) ? 16 : (textGlow == 1) ? 19 : 10;

        // Top decoration
        attron(COLOR_PAIR(19) | A_BOLD);
        for (int i = 0; i < 30; ++i) {
            if ((frame + i) % 3 == 0) {
                mvprintw(8, 25 + i, "\xe2\x96\x88");
            }
        }
        attroff(COLOR_PAIR(19) | A_BOLD);

        // Main text
        attron(COLOR_PAIR(textColor) | A_BOLD);
        mvprintw(11, 28, "\xe2\x9c\x88  STAGE %d CLEAR!  \xe2\x9c\x88", stages[stageIdx].id);
        attroff(COLOR_PAIR(textColor) | A_BOLD);

        attron(COLOR_PAIR(16) | A_BOLD);
        mvprintw(13, 30, "%s", stages[stageIdx].name.c_str());
        attroff(COLOR_PAIR(16) | A_BOLD);

        // Score display
        if (frame > 5) {
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(16, 32, "Score: %d", game.score());
            attroff(COLOR_PAIR(19) | A_BOLD);
        }

        // Bottom decoration
        attron(COLOR_PAIR(19) | A_BOLD);
        for (int i = 0; i < 30; ++i) {
            if ((frame + i) % 3 == 0) {
                mvprintw(19, 25 + i, "\xe2\x96\x88");
            }
        }
        attroff(COLOR_PAIR(19) | A_BOLD);

        // Celebration particles
        for (auto& p : parts) {
            if (p.life > 0) {
                p.x += p.dx;
                p.y += p.dy;
                p.dy += 0.1f;

                int px = (int)p.x;
                int py = (int)p.y;

                if (px >= 0 && px < 78 && py >= 0 && py < 28) {
                    attron(COLOR_PAIR(p.color) | A_BOLD);
                    mvprintw(py, px, "%s", p.symbol);
                    attroff(COLOR_PAIR(p.color) | A_BOLD);
                }
                p.life--;
            }
        }

        // Additional sparkles
        for (int i = 0; i < 15; ++i) {
            int sx = 20 + ((frame * 3 + i * 7) % 40);
            int sy2 = 6 + ((frame + i * 5) % 18);
            if ((frame + i) % 4 < 2) {
                attron(COLOR_PAIR((i % 3 == 0) ? 19 : 16) | A_BOLD);
                mvprintw(sy2, sx, "\xe2\x98\x86");
                attroff(COLOR_PAIR((i % 3 == 0) ? 19 : 16) | A_BOLD);
            }
        }

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Fade out
    for (int fade = 0; fade < 4; ++fade) {
        erase();
        attron(COLOR_PAIR(32));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(32));
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
}

// ============================================================
//  Stage result screen - enhanced
// ============================================================
static int showStageResult(const Game& game, int stageIdx) {
    auto stages = getAllStages();
    auto progress = loadStageProgress();

    bool cleared = game.stageComplete();
    int currentProgress = game.stageObjectiveProgress();
    int target = stages[stageIdx].targetValue;
    bool passed = cleared || currentProgress >= target;

    int stars = 0;
    if (passed) {
        stars = 1;
        int elapsed = game.elapsedSeconds();
        int timeLimit = stages[stageIdx].timeLimit;
        if (timeLimit > 0 && elapsed < timeLimit / 2) stars = 3;
        else if (timeLimit > 0 && elapsed < timeLimit * 3 / 4) stars = 2;
        else if (game.score() > target * 10) stars = 3;
        else if (game.score() > target * 5) stars = 2;
    }

    if (stars > progress[stageIdx].stars)
        progress[stageIdx].stars = stars;
    if (game.score() > progress[stageIdx].bestScore)
        progress[stageIdx].bestScore = game.score();
    if (passed && (game.elapsedSeconds() < progress[stageIdx].bestTime || progress[stageIdx].bestTime == 0))
        progress[stageIdx].bestTime = game.elapsedSeconds();
    saveStageProgress(progress);

    if (passed) playSound(5);

    int frame = 0;
    while (true) {
        erase();
        frame++;

        int cx = 12, sy = 3;

        if (passed) {
            // Animated celebration
            int starColor = (frame % 8 < 4) ? 19 : 9;
            attron(COLOR_PAIR(starColor) | A_BOLD);
            mvprintw(sy, cx + 8, "\xe2\x98\x85\xe2\x98\x85\xe2\x98\x85\xe2\x98\x85\xe2\x98\x85");
            attroff(COLOR_PAIR(starColor) | A_BOLD);

            attron(COLOR_PAIR(16) | A_BOLD);
            mvprintw(sy + 1, cx + 2, "\xe2\x9c\x88  S T A G E   C L E A R !  \xe2\x9c\x88");
            attroff(COLOR_PAIR(16) | A_BOLD);

            attron(COLOR_PAIR(starColor) | A_BOLD);
            mvprintw(sy + 2, cx + 8, "\xe2\x98\x85\xe2\x98\x85\xe2\x98\x85\xe2\x98\x85\xe2\x98\x85");
            attroff(COLOR_PAIR(starColor) | A_BOLD);
        } else {
            attron(COLOR_PAIR(18) | A_BOLD);
            mvprintw(sy + 1, cx + 4, "\xe2\x9a\xa0  S T A G E   F A I L E D  \xe2\x9a\xa0");
            attroff(COLOR_PAIR(18) | A_BOLD);
        }

        // Stage info
        auto [chName, chColor] = std::pair<std::string, int>{"", 10};
        if (stages[stageIdx].id <= 3)  { chName = "Tutorial";  chColor = 10; }
        else if (stages[stageIdx].id <= 6)  { chName = "Survival";  chColor = 16; }
        else if (stages[stageIdx].id <= 9)  { chName = "Tetris";    chColor = 13; }
        else if (stages[stageIdx].id <= 12) { chName = "Intense";   chColor = 14; }
        else if (stages[stageIdx].id <= 16) { chName = "Expert";    chColor = 15; }
        else { chName = "Master"; chColor = 18; }

        drawPanel(cx, sy + 4, 45, 16, nullptr, chColor);

        attron(COLOR_PAIR(chColor) | A_BOLD);
        mvprintw(sy + 5, cx + 2, "Stage %d: %s", stages[stageIdx].id, stages[stageIdx].name.c_str());
        attroff(COLOR_PAIR(chColor) | A_BOLD);

        attron(COLOR_PAIR(20));
        mvprintw(sy + 6, cx + 2, "Chapter: %s", chName.c_str());
        attroff(COLOR_PAIR(20));

        // Stars with animation
        attron(COLOR_PAIR(19) | A_BOLD);
        for (int s = 0; s < 3; ++s) {
            if (s < stars && frame > s * 8) {
                // Animated star reveal
                int starFrame = frame - s * 8;
                if (starFrame < 5) {
                    mvprintw(sy + 8, cx + 10 + s * 8, "\xe2\x98\x86");  // Empty first
                } else {
                    mvprintw(sy + 8, cx + 10 + s * 8, "\xe2\x98\x85");  // Then filled
                }
            } else {
                mvprintw(sy + 8, cx + 10 + s * 8, "\xe2\x98\x86");
            }
        }
        attroff(COLOR_PAIR(19) | A_BOLD);

        // Stats with better formatting
        attron(COLOR_PAIR(9) | A_BOLD);
        mvprintw(sy + 10, cx + 2, "  SCORE   : %d", game.score());
        attroff(COLOR_PAIR(9) | A_BOLD);
        attron(COLOR_PAIR(10));
        mvprintw(sy + 11, cx + 2, "  Lines   : %d", game.lines());
        mvprintw(sy + 12, cx + 2, "  Level   : %d", game.level());
        int mins = game.elapsedSeconds() / 60;
        int secs = game.elapsedSeconds() % 60;
        mvprintw(sy + 13, cx + 2, "  Time    : %d:%02d", mins, secs);
        mvprintw(sy + 14, cx + 2, "  Pieces  : %d", game.piecesPlaced());
        attroff(COLOR_PAIR(10));

        // Best records with highlight
        if (progress[stageIdx].bestScore > 0) {
            attron(COLOR_PAIR(19) | A_BOLD);
            mvprintw(sy + 16, cx + 2, "  Best Score: %d", progress[stageIdx].bestScore);
            attroff(COLOR_PAIR(19) | A_BOLD);
        }

        // Actions with better styling
        bool hasNext = (stageIdx + 1 < (int)stages.size());
        int actY = sy + 18;

        // Action panel
        attron(COLOR_PAIR(11));
        for (int i = 0; i < 20; ++i) {
            mvprintw(actY - 1, cx + 2 + i, "\xe2\x94\x80");
        }
        attroff(COLOR_PAIR(11));

        attron(COLOR_PAIR(10));
        mvprintw(actY, cx + 2, "[R] Retry");
        if (hasNext)
            mvprintw(actY + 1, cx + 2, "[N] Next Stage");
        mvprintw(actY + 2, cx + 2, "[M] Stage Select");
        mvprintw(actY + 3, cx + 2, "[Q] Quit");
        attroff(COLOR_PAIR(10));

        // Celebration particles for stage clear
        if (passed) {
            for (int i = 0; i < 20; ++i) {
                // Spiral particle pattern
                float angle = (frame * 0.15f + i * 0.31f);
                float radius = 25.0f + std::sin(frame * 0.1f + i) * 5.0f;
                int px = (int)(cx + 22 + std::cos(angle) * radius);
                int py = (int)(sy + 12 + std::sin(angle) * radius * 0.5f);

                if (px > cx && px < cx + 45 && py > sy && py < sy + 20) {
                    int pc;
                    if (frame % 10 < 3) pc = 19;
                    else if (frame % 10 < 6) pc = 16;
                    else pc = 13;

                    attron(COLOR_PAIR(pc) | A_BOLD);
                    const char* celebChars[] = {"\xe2\x98\x86", "\xe2\x98\x85", "\xe2\x9c\xa8", "*"};
                    mvprintw(py, px, "%s", celebChars[i % 4]);
                    attroff(COLOR_PAIR(pc) | A_BOLD);
                }
            }
        } else {
            // Sad particles for failure
            for (int i = 0; i < 10; ++i) {
                int px = cx + (frame * 2 + i * 11) % 45;
                int py = sy + 1 + (frame + i * 3) % 5;
                if (px > cx && px < cx + 45) {
                    attron(COLOR_PAIR(31) | A_DIM);
                    mvprintw(py, px, "\xe2\x98\x86");
                    attroff(COLOR_PAIR(31) | A_DIM);
                }
            }
        }

        refresh();

        int ch = getch();
        if (ch == 'r' || ch == 'R') return 1;
        if ((ch == 'n' || ch == 'N') && hasNext) return 2;
        if (ch == 'm' || ch == 'M') return 0;
        if (ch == 'q' || ch == 'Q') return -1;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ============================================================
//  Game start entrance animation
// ============================================================
static void showGameStartAnimation(const Game& game) {
    for (int frame = 0; frame < 12; ++frame) {
        erase();

        // Background
        attron(COLOR_PAIR(32));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(32));

        // Expanding game board outline
        int expandW = (frame * 2);
        int expandH = (frame * 1);
        int boardCX = 40;
        int boardCY = 15;

        if (expandW > 0 && expandH > 0) {
            int minX = boardCX - expandW;
            int maxX = boardCX + expandW;
            int minY = boardCY - expandH;
            int maxY = boardCY + expandH;

            // Border with color cycling
            int borderColor = (frame / 2) % 7 + 1;
            attron(COLOR_PAIR(borderColor) | A_BOLD);

            // Top and bottom
            for (int x = minX; x <= maxX; ++x) {
                if (x >= 0 && x < 78) {
                    if (minY >= 0 && minY < 28) mvprintw(minY, x, "\xe2\x95\x90");
                    if (maxY >= 0 && maxY < 28) mvprintw(maxY, x, "\xe2\x95\x90");
                }
            }
            // Sides
            for (int y = minY; y <= maxY; ++y) {
                if (y >= 0 && y < 28) {
                    if (minX >= 0 && minX < 78) mvprintw(y, minX, "\xe2\x95\x91");
                    if (maxX >= 0 && maxX < 78) mvprintw(y, maxX, "\xe2\x95\x91");
                }
            }

            // Corners
            if (minX >= 0 && maxX < 78 && minY >= 0 && maxY < 28) {
                mvprintw(minY, minX, "\xe2\x95\x94");
                mvprintw(minY, maxX, "\xe2\x95\x97");
                mvprintw(maxY, minX, "\xe2\x95\x9a");
                mvprintw(maxY, maxX, "\xe2\x95\x9d");
            }
            attroff(COLOR_PAIR(borderColor) | A_BOLD);
        }

        // Center text appears after expansion
        if (frame > 4) {
            int textAlpha = (frame - 4);
            int textColor = (textAlpha > 6) ? 10 : 32;

            if (game.mode() == GameMode::Stage) {
                attron(COLOR_PAIR(textColor) | A_BOLD);
                mvprintw(13, 30, "STAGE %d", game.currentStage().id);
                attroff(COLOR_PAIR(textColor) | A_BOLD);
                attron(COLOR_PAIR(19));
                mvprintw(15, 28, "%s", game.currentStage().name.c_str());
                attroff(COLOR_PAIR(19));
            } else {
                attron(COLOR_PAIR(textColor) | A_BOLD);
                mvprintw(13, 32, "T E T R I S");
                attroff(COLOR_PAIR(textColor) | A_BOLD);
                attron(COLOR_PAIR(10));
                mvprintw(15, 30, "Level %d", game.level());
                attroff(COLOR_PAIR(10));
            }
        }

        // Corner sparkles
        for (int i = 0; i < 8; ++i) {
            int sx = 10 + ((frame * 5 + i * 9) % 60);
            int sy = 3 + ((frame * 3 + i * 7) % 22);
            int sc = (i % 3 == 0) ? 19 : (i % 3 == 1) ? 16 : 10;
            if (frame > 2) {
                attron(COLOR_PAIR(sc) | A_BOLD);
                mvprintw(sy, sx, "\xe2\x98\x86");
                attroff(COLOR_PAIR(sc) | A_BOLD);
            }
        }

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    // Brief pause before game starts
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

// ============================================================
//  Taunt popup after 10 consecutive failures (enhanced)
// ============================================================
static void showTauntPopup() {
    int frame = 0;
    int totalFrames = 75;

    // Background particles
    struct BgParticle {
        int x, y, speed, color;
    };
    BgParticle bgParticles[20];
    for (int i = 0; i < 20; ++i) {
        bgParticles[i].x = (i * 4) % 75;
        bgParticles[i].y = (i * 7) % 28;
        bgParticles[i].speed = 1 + (i % 3);
        bgParticles[i].color = (i % 3 == 0) ? 18 : (i % 3 == 1) ? 19 : 14;
    }

    while (frame < totalFrames) {
        erase();
        frame++;

        // Animated background with particles
        attron(COLOR_PAIR(236));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(236));

        // Moving background particles
        for (int i = 0; i < 20; ++i) {
            bgParticles[i].y += bgParticles[i].speed;
            if (bgParticles[i].y > 29) {
                bgParticles[i].y = 0;
                bgParticles[i].x = (frame * 2 + i * 5) % 75;
            }
            if ((frame + i) % 4 < 2) {
                attron(COLOR_PAIR(bgParticles[i].color) | A_DIM);
                mvprintw(bgParticles[i].y, bgParticles[i].x, "\xe2\x98\xa2");
                attroff(COLOR_PAIR(bgParticles[i].color) | A_DIM);
            }
        }

        // Popup box with animated entrance
        int bx = 20, by = 6, bw = 40, bh = 16;
        int expandFrame = std::min(frame, 8);
        int drawW = (bw * expandFrame) / 8;
        int drawH = (bh * expandFrame) / 8;
        int drawBx = bx + (bw - drawW) / 2;
        int drawBy = by + (bh - drawH) / 2;

        if (expandFrame > 0) {
            // Glow effect behind box
            if (frame > 5) {
                int glowColor = (frame % 4 < 2) ? 18 : 14;
                attron(COLOR_PAIR(glowColor) | A_DIM);
                for (int r = -1; r <= drawH; ++r) {
                    for (int c = -1; c <= drawW; ++c) {
                        int gx = drawBx + c;
                        int gy = drawBy + r;
                        if (gx >= 0 && gx < 79 && gy >= 0 && gy < 29) {
                            if (r == -1 || r == drawH || c == -1 || c == drawW) {
                                mvprintw(gy, gx, " ");
                            }
                        }
                    }
                }
                attroff(COLOR_PAIR(glowColor) | A_DIM);
            }

            // Box border with color cycling
            int borderColor;
            if (frame < 20) borderColor = 18;
            else if (frame < 40) borderColor = 19;
            else borderColor = (frame % 6 < 2) ? 18 : (frame % 6 < 4) ? 19 : 14;

            attron(COLOR_PAIR(borderColor) | A_BOLD);
            // Top border
            mvprintw(drawBy, drawBx, "\xe2\x95\x94");
            for (int i = 1; i < drawW - 1; ++i) mvprintw(drawBy, drawBx + i, "\xe2\x95\x90");
            mvprintw(drawBy, drawBx + drawW - 1, "\xe2\x95\x97");
            // Bottom border
            mvprintw(drawBy + drawH - 1, drawBx, "\xe2\x95\x9a");
            for (int i = 1; i < drawW - 1; ++i) mvprintw(drawBy + drawH - 1, drawBx + i, "\xe2\x95\x90");
            mvprintw(drawBy + drawH - 1, drawBx + drawW - 1, "\xe2\x95\x9d");
            // Sides
            for (int i = 1; i < drawH - 1; ++i) {
                mvprintw(drawBy + i, drawBx, "\xe2\x95\x91");
                mvprintw(drawBy + i, drawBx + drawW - 1, "\xe2\x95\x91");
            }
            attroff(COLOR_PAIR(borderColor) | A_BOLD);

            // Inner fill with gradient
            for (int r = 1; r < drawH - 1; ++r) {
                for (int c = 1; c < drawW - 1; ++c) {
                    int fillColor = (r < drawH / 3) ? 237 : (r < drawH * 2 / 3) ? 238 : 236;
                    attron(COLOR_PAIR(fillColor));
                    mvprintw(drawBy + r, drawBx + c, " ");
                    attroff(COLOR_PAIR(fillColor));
                }
            }
        }

        // Content appears after box expands
        if (frame > 10) {
            // Animated warning symbols
            int warnColor = (frame % 3 == 0) ? 18 : (frame % 3 == 1) ? 19 : 14;
            attron(COLOR_PAIR(warnColor) | A_BOLD);
            // Top decorative line
            for (int i = 0; i < drawW - 4; ++i) {
                if ((frame + i) % 4 < 2) {
                    mvprintw(drawBy + 2, drawBx + 2 + i, "\xe2\x96\x88");
                }
            }
            attroff(COLOR_PAIR(warnColor) | A_BOLD);

            // Title with pulsing effect
            int titlePulse = (frame / 3) % 4;
            int titleColor = (titlePulse < 2) ? 18 : 14;
            attron(COLOR_PAIR(titleColor) | A_BOLD);
            mvprintw(drawBy + 3, drawBx + (drawW - 16) / 2, "\xe2\x9d\x8f \xe8\xad\xa6\xe5\x91\x8a \xe2\x9d\x8f");
            attroff(COLOR_PAIR(titleColor) | A_BOLD);

            // Main taunt text with dramatic reveal
            if (frame > 15) {
                // Yellow "哈哈哈"
                attron(COLOR_PAIR(19) | A_BOLD);
                mvprintw(drawBy + 5, drawBx + (drawW - 18) / 2, "\xe2\x80\x9c\xe5\x93\x88\xe5\x93\x88\xe5\x93\x88");
                attroff(COLOR_PAIR(19) | A_BOLD);

                // Red "废物"
                attron(COLOR_PAIR(18) | A_BOLD);
                mvprintw(drawBy + 5, drawBx + (drawW - 18) / 2 + 9, "\xe5\xba\x9f\xe7\x89\xa9\xe2\x80\x9d");
                attroff(COLOR_PAIR(18) | A_BOLD);
            }

            // Decorative skulls
            if (frame > 18) {
                attron(COLOR_PAIR(18) | A_BOLD);
                mvprintw(drawBy + 5, drawBx + 2, "\xe2\x98\xa0");
                mvprintw(drawBy + 5, drawBx + drawW - 3, "\xe2\x98\xa0");
                attroff(COLOR_PAIR(18) | A_BOLD);
            }

            // Subtitle text
            if (frame > 25) {
                attron(COLOR_PAIR(9));
                mvprintw(drawBy + 7, drawBx + (drawW - 28) / 2, "You have failed 10 times in a row!");
                attroff(COLOR_PAIR(9));
            }

            // Score reminder
            if (frame > 30) {
                attron(COLOR_PAIR(20));
                mvprintw(drawBy + 9, drawBx + (drawW - 20) / 2, "Maybe try again later?");
                attroff(COLOR_PAIR(20));
            }

            // Bottom decorative line
            if (frame > 20) {
                attron(COLOR_PAIR(warnColor));
                for (int i = 0; i < drawW - 4; ++i) {
                    if ((frame + i) % 4 < 2) {
                        mvprintw(drawBy + drawH - 3, drawBx + 2 + i, "\xe2\x96\x88");
                    }
                }
                attroff(COLOR_PAIR(warnColor));
            }
        }

        // Flying particles around the box
        for (int i = 0; i < 15; ++i) {
            float angle = (frame * 0.1f + i * 0.42f);
            float radius = 22.0f + std::sin(frame * 0.1f + i) * 3.0f;
            int px = (int)(40 + std::cos(angle) * radius);
            int py = (int)(14 + std::sin(angle) * radius * 0.6f);
            if (px >= 0 && px < 78 && py >= 0 && py < 28) {
                int pc = (i % 4 == 0) ? 18 : (i % 4 == 1) ? 19 : (i % 4 == 2) ? 14 : 9;
                attron(COLOR_PAIR(pc) | A_BOLD);
                mvprintw(py, px, "\xe2\x98\xa2");
                attroff(COLOR_PAIR(pc) | A_BOLD);
            }
        }

        // Press any key hint (blinking with fade)
        if (frame > 40) {
            int hintAlpha = (frame % 12 < 8) ? 20 : 32;
            attron(COLOR_PAIR(hintAlpha));
            mvprintw(drawBy + drawH - 2, drawBx + (drawW - 24) / 2, "Press any key to continue");
            attroff(COLOR_PAIR(hintAlpha));
        }

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(70));

        // Check for key press to dismiss early
        int ch = getch();
        if (ch != ERR && frame > 20) break;
    }

    // Final fade out
    for (int fade = 0; fade < 3; ++fade) {
        erase();
        int bgColor = 238 - fade;
        attron(COLOR_PAIR(bgColor));
        for (int r = 0; r < 30; ++r)
            for (int c = 0; c < 80; ++c)
                mvprintw(r, c, " ");
        attroff(COLOR_PAIR(bgColor));
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ============================================================
//  Main
// ============================================================
int main() {
    srand(time(nullptr));
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(5);
    initColors();

restart:
    vis = VisualState();
    int mode = showMainMenu();
    if (mode == 0) { endwin(); return 0; }

    if (mode == 1) {
        // Classic mode
        int startLevel = showClassicMenu();
        if (startLevel == -1) goto restart;
        if (startLevel == 0) { endwin(); return 0; }

        Game game(startLevel);
        showGameStartAnimation(game);

        while (true) {
            game.setCurrentTime(static_cast<int>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count()));

            int prevLevel = game.level();
            int prevCombo = game.combo();

            int ch;
            while ((ch = getch()) != ERR) {
                switch (ch) {
                    case KEY_LEFT:  case 'a': case 'A': game.moveLeft();   break;
                    case KEY_RIGHT: case 'd': case 'D': game.moveRight();  break;
                    case KEY_UP:    game.rotate();     break;
                    case KEY_DOWN:  game.softDrop();   break;
                    case ' ':
                        triggerHardDropTrail(game.current(), game.ghostRow());
                        game.hardDrop();
                        triggerShake(3);
                        playSound(1);
                        break;
                    case 'h': case 'H': game.holdPiece(); playSound(6); break;
                    case 'p': case 'P': game.togglePause(); break;
                    case 'r': case 'R':
                        if (game.gameOver()) {
                            saveHighScore({"PLAYER", game.score(), game.level(),
                                          game.lines(), game.elapsedSeconds()});
                            game = Game(startLevel);
                            vis = VisualState();
                        }
                        break;
                    case 'm': case 'M':
                        if (game.gameOver()) goto restart;
                        break;
                    case 'q': case 'Q': goto quit;
                }
            }

            game.tick();

            // Post-move visual effects
            ClearResult clear = game.lastClear();
            if (clear.linesCleared > 0) {
                triggerLineClear(clear.linesCleared);
                triggerShake(clear.isTetris ? 5 : 2);
                // Score flyup for line clears
                int lineScore = clear.linesCleared * 100 * game.level();
                if (clear.isTetris) lineScore *= 2;
                triggerScoreFlyup(lineScore);
                if (clear.isTetris) {
                    vis.tetrisFlashFrames = 15;
                    playSound(3);
                } else {
                    playSound(2);
                }
            }
            if (game.combo() > prevCombo && game.combo() > 1) {
                triggerCombo(game.combo());
                if (game.combo() >= 5) playSound(4);
                else playSound(2);
            }
            if (game.level() > prevLevel) {
                triggerLevelUp();
                playSound(5);
            }

            renderGame(game);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    } else {
        // Stage mode
        while (true) {
            int stageIdx = showStageSelect();
            if (stageIdx == -1) goto restart;

            auto stages = getAllStages();
            int consecutiveFails = 0;

stage_retry:
            vis = VisualState();
            Game game(stages[stageIdx]);
            showGameStartAnimation(game);

            while (true) {
                game.setCurrentTime(static_cast<int>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count()));

                int prevCombo = game.combo();
                int prevLevel = game.level();

                int ch;
                while ((ch = getch()) != ERR) {
                    switch (ch) {
                        case KEY_LEFT:  case 'a': case 'A': game.moveLeft();   break;
                        case KEY_RIGHT: case 'd': case 'D': game.moveRight();  break;
                        case KEY_UP:    game.rotate();     break;
                        case KEY_DOWN:  game.softDrop();   break;
                        case ' ':
                            triggerHardDropTrail(game.current(), game.ghostRow());
                            game.hardDrop();
                            triggerShake(3);
                            playSound(1);
                            break;
                        case 'h': case 'H': game.holdPiece(); playSound(6); break;
                        case 'p': case 'P': game.togglePause(); break;
                        case 'q': case 'Q': goto quit;
                    }
                }

                game.tick();

                ClearResult clear = game.lastClear();
                if (clear.linesCleared > 0) {
                    triggerLineClear(clear.linesCleared);
                    triggerShake(clear.isTetris ? 5 : 2);
                    // Score flyup for line clears
                    int lineScore = clear.linesCleared * 100 * game.level();
                    if (clear.isTetris) lineScore *= 2;
                    triggerScoreFlyup(lineScore);
                    if (clear.isTetris) {
                        vis.tetrisFlashFrames = 15;
                        playSound(3);
                    } else {
                        playSound(2);
                    }
                }
                if (game.combo() > prevCombo && game.combo() > 1) {
                    triggerCombo(game.combo());
                    if (game.combo() >= 5) playSound(4);
                    else playSound(2);
                }
                if (game.level() > prevLevel) {
                    triggerLevelUp();
                    playSound(5);
                }

                renderGame(game);

                if (game.gameOver()) {
                    bool stageCleared = game.stageComplete();
                    if (!stageCleared) {
                        consecutiveFails++;
                        showBombExplosion(game, stageIdx);
                        if (consecutiveFails >= 10) {
                            showTauntPopup();
                            consecutiveFails = 0;
                        }
                    } else {
                        consecutiveFails = 0;
                        playSound(5);  // Victory sound
                        showStageClearCelebration(game, stageIdx);
                    }
                    int result = showStageResult(game, stageIdx);
                    if (result == -1) goto quit;
                    if (result == 0) goto restart;
                    if (result == 1) goto stage_retry;
                    if (result == 2) {
                        stageIdx++;
                        if (stageIdx >= (int)stages.size()) goto restart;
                        goto stage_retry;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
    }

quit:
    endwin();
    printf("Thanks for playing TETRIS!\n");
    return 0;
}
