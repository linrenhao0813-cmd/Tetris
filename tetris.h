#pragma once

#include <array>
#include <chrono>
#include <random>
#include <vector>

// --- Dimensions ---
constexpr int BOARD_W = 10;
constexpr int BOARD_H = 20;   // visible rows
constexpr int HIDDEN  = 2;    // hidden rows above visible area
constexpr int TOTAL_H = BOARD_H + HIDDEN;

// --- Piece types ---
enum class PieceType { I = 0, O, T, S, Z, J, L };
constexpr int PIECE_COUNT = 7;

// Shape = square matrix, 1=filled 0=empty
using Shape = std::vector<std::vector<int>>;

// --- Shape helpers ---
int  pieceSize(PieceType t);
const Shape& baseShape(PieceType t);
Shape rotateCW(const Shape& s);
const std::array<Shape, 4>& allRotations(PieceType t);

// --- Board ---
class Board {
public:
    Board();
    int  at(int r, int c) const;
    bool occupied(int r, int c) const;
    bool inBounds(int r, int c) const;
    void place(int r, int c, const Shape& s, int color);
    int  clearLines();

private:
    std::vector<std::vector<int>> grid_; // [row][col], 0=empty, 1-7=color
};

// --- Tetromino ---
class Piece {
public:
    explicit Piece(PieceType t);

    PieceType type() const { return type_; }
    int       rot()  const { return rot_; }
    int       row()  const { return row_; }
    int       col()  const { return col_; }
    int       size() const { return pieceSize(type_); }

    const Shape& shape() const { return shapeFor(type_, rot_); }

    void setRow(int r) { row_ = r; }
    void setCol(int c) { col_ = c; }
    void setRot(int r) { rot_ = r; }

    int  nextRot() const { return (rot_ + 1) % 4; }

    static const Shape& shapeFor(PieceType t, int r);

private:
    PieceType type_;
    int rot_ = 0;
    int row_ = 0;
    int col_ = 0;
};

// --- Game ---
class Game {
public:
    Game();

    bool gameOver() const { return over_; }
    bool paused()   const { return paused_; }
    int  score()    const { return score_; }
    int  level()    const { return level_; }
    int  lines()    const { return lines_; }

    const Board& board()   const { return board_; }
    const Piece& current() const { return curr_; }
    const Piece& next()    const { return next_; }

    void togglePause() { paused_ = !paused_; }
    void moveLeft();
    void moveRight();
    void softDrop();
    void hardDrop();
    void rotate();
    void tick();        // gravity per frame
    int  ghostRow() const; // where current piece lands

private:
    Board board_;
    std::mt19937 rng_;
    std::vector<PieceType> bag_;
    int  bagIdx_ = 0;

    Piece curr_;
    Piece next_;

    int  score_ = 0;
    int  level_ = 1;
    int  lines_ = 0;
    bool over_  = false;
    bool paused_= false;

    std::chrono::steady_clock::time_point lastDrop_;

    // --- Internal ---
    PieceType drawFromBag();
    void      refillBag();
    bool      collides(const Piece& p) const;
    void      lockPiece();
    void      spawnPiece();
    int       dropMs() const;
    bool      tryMove(const Piece& cand); // true if valid & applied
};
