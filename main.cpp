#include "tetris.h"
#include <ncurses.h>
#include <cstdlib>
#include <thread>

static void initColors() {
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_CYAN);     // I
    init_pair(2, COLOR_BLACK, COLOR_YELLOW);   // O
    init_pair(3, COLOR_BLACK, COLOR_MAGENTA);  // T
    init_pair(4, COLOR_BLACK, COLOR_GREEN);    // S
    init_pair(5, COLOR_BLACK, COLOR_RED);      // Z
    init_pair(6, COLOR_BLACK, COLOR_BLUE);     // J
    init_pair(7, COLOR_BLACK, COLOR_WHITE);    // L
}

static void drawCell(int y, int x, int colorPair, const char* str) {
    attron(COLOR_PAIR(colorPair));
    mvprintw(y, x, "%s", str);
    attroff(COLOR_PAIR(colorPair));
}

static void render(const Game& game) {
    const Board& b = game.board();
    const Piece& cur = game.current();

    erase();

    // Board
    for (int vr = 0; vr < BOARD_H; ++vr) {
        int br = vr + HIDDEN;
        for (int c = 0; c < BOARD_W; ++c) {
            int py = 1 + vr;
            int px = 2 + c * 2;

            // Current piece
            {
                int dr = br - cur.row();
                int dc = c - cur.col();
                if (dr >= 0 && dr < cur.size() && dc >= 0 && dc < cur.size()
                    && cur.shape()[dr][dc]) {
                    drawCell(py, px, static_cast<int>(cur.type()) + 1, "[]");
                    continue;
                }
            }
            // Locked block
            int color = b.at(br, c);
            if (color) {
                drawCell(py, px, color, "[]");
            } else {
                mvprintw(py, px, " ·");
            }
        }
    }

    // Border around board
    for (int r = 0; r <= BOARD_H + 1; ++r) {
        mvprintw(r, 1, "│");
        mvprintw(r, 2 + BOARD_W * 2, "│");
    }
    for (int c = 1; c <= 2 + BOARD_W * 2; ++c) {
        mvprintw(0, c, "─");
        mvprintw(BOARD_H + 1, c, "─");
    }
    mvprintw(0, 1, "┌");
    mvprintw(0, 2 + BOARD_W * 2, "┐");
    mvprintw(BOARD_H + 1, 1, "└");
    mvprintw(BOARD_H + 1, 2 + BOARD_W * 2, "┘");

    // Score
    mvprintw(1, 2 + BOARD_W * 2 + 3, "SCORE: %d", game.score());
    mvprintw(2, 2 + BOARD_W * 2 + 3, "LEVEL: %d", game.level());
    mvprintw(3, 2 + BOARD_W * 2 + 3, "LINES: %d", game.lines());

    // Next piece
    mvprintw(5, 2 + BOARD_W * 2 + 3, "NEXT:");
    const Piece& nxt = game.next();
    const Shape& ns = nxt.shape();
    int nsz = nxt.size();
    for (int dr = 0; dr < nsz; ++dr)
        for (int dc = 0; dc < nsz; ++dc)
            if (ns[dr][dc])
                drawCell(6 + dr, 2 + BOARD_W * 2 + 3 + dc * 2,
                         static_cast<int>(nxt.type()) + 1, "[]");

    // Status messages
    if (game.gameOver())
        mvprintw(BOARD_H / 2, 2 + BOARD_W * 2 + 3, "GAME OVER");
    else if (game.paused())
        mvprintw(BOARD_H / 2, 2 + BOARD_W * 2 + 3, "PAUSED");

    mvprintw(BOARD_H + 3, 2, "← →/AD  Move  ↑ Rotate  ↓ Drop  Space Hard  P Pause  Q Quit");

    refresh();
}

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(5);
    initColors();

    Game game;

    while (true) {
        int ch;
        while ((ch = getch()) != ERR) {
            switch (ch) {
                case KEY_LEFT:
                case 'a':
                case 'A':       game.moveLeft();   break;
                case KEY_RIGHT:
                case 'd':
                case 'D':       game.moveRight();  break;
                case KEY_UP:    game.rotate();     break;
                case KEY_DOWN:  game.softDrop();   break;
                case ' ':       game.hardDrop();   break;
                case 'p':
                case 'P':       game.togglePause(); break;
                case 'q':
                case 'Q':       goto quit;
            }
        }

        game.tick();
        render(game);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

quit:
    endwin();
    printf("Final score: %d  Level: %d  Lines: %d\n",
           game.score(), game.level(), game.lines());
    return 0;
}
