#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <random>
#include <string>
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

// --- Stage objective types ---
enum class ObjectiveType {
    ClearLines,       // Clear N lines
    ReachScore,       // Reach target score
    SurviveTime,      // Survive for N seconds
    ClearTetrises,    // Clear N tetris (4-line) clears
    ClearWithCombo,   // Achieve N combo
    PlacePieces,      // Place N pieces without topping out
    NoHold,           // Clear N lines without using hold
    SpeedClear,       // Clear N lines within time limit
};

// --- Stage definition ---
struct Stage {
    int id;
    std::string name;
    std::string description;
    ObjectiveType objective;
    int targetValue;        // target for objective
    int timeLimit;          // seconds, 0=no limit
    int startLevel;         // starting level
    int maxPieces;          // max pieces allowed, 0=unlimited
    bool disableHold;       // hold disabled
    bool disableSoftDrop;   // soft drop disabled
    std::vector<std::pair<int,int>> prefill; // pre-filled cells (row, col) - garbage
};

// --- Stage progress ---
struct StageProgress {
    int stars = 0;          // 0=not cleared, 1-3 stars
    int bestScore = 0;
    int bestTime = 0;       // seconds
};

std::vector<Stage> getAllStages();
std::vector<StageProgress> loadStageProgress();
void saveStageProgress(const std::vector<StageProgress>& progress);

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
    void addGarbage(int rows);
    void setCell(int r, int c, int val);

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

// --- Line clear result ---
struct ClearResult {
    int linesCleared = 0;
    int combo = 0;
    bool isTetris = false;
    bool isTSpin = false;
};

// --- High score ---
struct HighScore {
    std::string name;
    int score;
    int level;
    int lines;
    int time;
};

std::vector<HighScore> loadHighScores();
void saveHighScore(const HighScore& hs);

// --- Game modes ---
enum class GameMode { Classic, Stage, Endless, Hell };

// --- Game ---
class Game {
public:
    Game();
    explicit Game(int startLevel);
    explicit Game(const Stage& stage);
    explicit Game(GameMode mode, int startLevel = 1);

    bool gameOver() const { return over_; }
    bool paused()   const { return paused_; }
    int  score()    const { return score_; }
    int  level()    const { return level_; }
    int  lines()    const { return lines_; }
    int  combo()    const { return combo_; }
    bool hasHold()  const { return holdPiece_.has_value(); }
    const Piece& hold() const { return holdPiece_.value(); }
    bool canHold()  const { return canHold_; }
    int  piecesPlaced() const { return piecesPlaced_; }
    int  tetrisCount()  const { return tetrisCount_; }
    int  maxCombo()     const { return maxCombo_; }
    int  startTime()    const { return startTime_; }

    void setCurrentTime(int t) { currentTime_ = t; }
    int  elapsedSeconds() const { return currentTime_ - startTime_; }

    const Board& board()   const { return board_; }
    const Piece& current() const { return curr_; }
    const Piece& next()    const { return next_; }

    void togglePause() { paused_ = !paused_; }
    void moveLeft();
    void moveRight();
    void softDrop();
    void hardDrop();
    void rotate();
    void holdPiece();
    void tick();        // gravity per frame
    int  ghostRow() const; // where current piece lands
    int  dropMs() const;
    void reset();       // reset game

    ClearResult lastClear() const { return lastClear_; }

    // Stage mode
    GameMode mode() const { return mode_; }
    const Stage& currentStage() const { return stage_; }
    bool stageComplete() const { return stageComplete_; }
    int  stageObjectiveProgress() const;
    std::string stageObjectiveText() const;
    int  stageTimeRemaining() const;

private:
    Board board_;
    std::mt19937 rng_;
    std::vector<PieceType> bag_;
    int  bagIdx_ = 0;

    Piece curr_;
    Piece next_;
    std::optional<Piece> holdPiece_;
    bool canHold_ = true;

    int  score_ = 0;
    int  level_ = 1;
    int  lines_ = 0;
    int  combo_ = 0;
    bool over_  = false;
    bool paused_= false;

    int  piecesPlaced_ = 0;
    int  tetrisCount_ = 0;
    int  maxCombo_ = 0;

    ClearResult lastClear_;

    std::chrono::steady_clock::time_point lastDrop_;

    // Stage mode
    GameMode mode_ = GameMode::Classic;
    Stage stage_;
    bool stageComplete_ = false;
    int  noHoldLines_ = 0;

    int  startTime_ = 0;
    int  currentTime_ = 0;

    // --- Internal ---
    PieceType drawFromBag();
    void      refillBag();
    bool      collides(const Piece& p) const;
    void      lockPiece();
    void      spawnPiece();
    bool      tryMove(const Piece& cand); // true if valid & applied
    void      checkStageComplete();
    void      initStageBoard();
};
