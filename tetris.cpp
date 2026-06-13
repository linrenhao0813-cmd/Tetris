#include "tetris.h"
#include <bits/stdc++.h>
#include <algorithm>
#include <fstream>
#include <sstream>

// ============================================================
//  Piece shape definitions (base / spawn orientation)
// ============================================================

static const Shape SHAPE_I = {
    {0,0,0,0},
    {1,1,1,1},
    {0,0,0,0},
    {0,0,0,0}
};

static const Shape SHAPE_O = {
    {1,1},
    {1,1}
};

static const Shape SHAPE_T = {
    {0,1,0},
    {1,1,1},
    {0,0,0}
};

static const Shape SHAPE_S = {
    {0,1,1},
    {1,1,0},
    {0,0,0}
};

static const Shape SHAPE_Z = {
    {1,1,0},
    {0,1,1},
    {0,0,0}
};

static const Shape SHAPE_J = {
    {1,0,0},
    {1,1,1},
    {0,0,0}
};

static const Shape SHAPE_L = {
    {0,0,1},
    {1,1,1},
    {0,0,0}
};

// ============================================================
//  Shape helpers
// ============================================================

int pieceSize(PieceType t) {
    switch (t) {
        case PieceType::I: return 4;
        case PieceType::O: return 2;
        default:           return 3;
    }
}

const Shape& baseShape(PieceType t) {
    static const Shape empty{};
    switch (t) {
        case PieceType::I: return SHAPE_I;
        case PieceType::O: return SHAPE_O;
        case PieceType::T: return SHAPE_T;
        case PieceType::S: return SHAPE_S;
        case PieceType::Z: return SHAPE_Z;
        case PieceType::J: return SHAPE_J;
        case PieceType::L: return SHAPE_L;
        default:           return empty;
    }
}

Shape rotateCW(const Shape& s) {
    int n = static_cast<int>(s.size());
    Shape out(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            out[j][n - 1 - i] = s[i][j];
    return out;
}

const std::array<Shape, 4>& allRotations(PieceType t) {
    static std::array<Shape, 4> cache[PIECE_COUNT];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < PIECE_COUNT; ++i) {
            auto pt = static_cast<PieceType>(i);
            cache[i][0] = baseShape(pt);
            for (int r = 1; r < 4; ++r)
                cache[i][r] = rotateCW(cache[i][r - 1]);
        }
        init = true;
    }
    return cache[static_cast<int>(t)];
}

// ============================================================
//  Board
// ============================================================

Board::Board()
    : grid_(TOTAL_H, std::vector<int>(BOARD_W, 0))
{}

int Board::at(int r, int c) const {
    if (!inBounds(r, c)) return 0;
    return grid_[r][c];
}

bool Board::occupied(int r, int c) const {
    return inBounds(r, c) && grid_[r][c] != 0;
}

bool Board::inBounds(int r, int c) const {
    return r >= 0 && r < TOTAL_H && c >= 0 && c < BOARD_W;
}

void Board::place(int r, int c, const Shape& s, int color) {
    int sz = static_cast<int>(s.size());
    for (int dr = 0; dr < sz; ++dr)
        for (int dc = 0; dc < sz; ++dc)
            if (s[dr][dc])
                grid_[r + dr][c + dc] = color;
}

int Board::clearLines() {
    int cleared = 0;
    for (int r = TOTAL_H - 1; r >= 0; --r) {
        bool full = true;
        for (int c = 0; c < BOARD_W; ++c) {
            if (grid_[r][c] == 0) { full = false; break; }
        }
        if (full) {
            ++cleared;
            for (int rr = r; rr > 0; --rr)
                for (int cc = 0; cc < BOARD_W; ++cc)
                    grid_[rr][cc] = grid_[rr - 1][cc];
            for (int cc = 0; cc < BOARD_W; ++cc)
                grid_[0][cc] = 0;
            ++r; // re-check this row
        }
    }
    return cleared;
}

// ============================================================
//  Piece
// ============================================================

Piece::Piece(PieceType t) : type_(t) {}

const Shape& Piece::shapeFor(PieceType t, int r) {
    return allRotations(t)[r % 4];
}

// ============================================================
//  Game
// ============================================================

Game::Game()
    : rng_(std::random_device{}())
    , curr_(PieceType::I)
    , next_(PieceType::I)
    , startTime_(static_cast<int>(
          std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now().time_since_epoch()).count()))
{
    currentTime_ = startTime_;
    refillBag();
    curr_ = Piece(drawFromBag());
    next_ = Piece(drawFromBag());
    curr_.setCol((BOARD_W - curr_.size()) / 2);
    curr_.setRow(0);
    lastDrop_ = std::chrono::steady_clock::now();
}

Game::Game(int startLevel)
    : rng_(std::random_device{}())
    , curr_(PieceType::I)
    , next_(PieceType::I)
    , level_(std::max(1, std::min(startLevel, 20)))
    , startTime_(static_cast<int>(
          std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now().time_since_epoch()).count()))
{
    currentTime_ = startTime_;
    refillBag();
    curr_ = Piece(drawFromBag());
    next_ = Piece(drawFromBag());
    curr_.setCol((BOARD_W - curr_.size()) / 2);
    curr_.setRow(0);
    lastDrop_ = std::chrono::steady_clock::now();
}

void Game::reset() {
    *this = Game();
}

Game::Game(const Stage& stage)
    : rng_(std::random_device{}())
    , curr_(PieceType::I)
    , next_(PieceType::I)
    , level_(std::max(1, stage.startLevel))
    , mode_(GameMode::Stage)
    , stage_(stage)
    , startTime_(static_cast<int>(
          std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now().time_since_epoch()).count()))
{
    currentTime_ = startTime_;
    refillBag();
    initStageBoard();
    curr_ = Piece(drawFromBag());
    next_ = Piece(drawFromBag());
    curr_.setCol((BOARD_W - curr_.size()) / 2);
    curr_.setRow(0);
    lastDrop_ = std::chrono::steady_clock::now();
}

void Game::initStageBoard() {
    board_ = Board();
    for (auto& [r, c] : stage_.prefill) {
        if (board_.inBounds(r, c) && !board_.occupied(r, c)) {
            int garbageColor = 8 + (r % 3); // use different garbage colors
            board_.setCell(r, c, garbageColor);
        }
    }
}

void Game::checkStageComplete() {
    if (mode_ != GameMode::Stage || stageComplete_) return;
    
    bool met = false;
    switch (stage_.objective) {
        case ObjectiveType::ClearLines:
            met = (lines_ >= stage_.targetValue);
            break;
        case ObjectiveType::ReachScore:
            met = (score_ >= stage_.targetValue);
            break;
        case ObjectiveType::SurviveTime:
            met = (elapsedSeconds() >= stage_.targetValue);
            break;
        case ObjectiveType::ClearTetrises:
            met = (tetrisCount_ >= stage_.targetValue);
            break;
        case ObjectiveType::ClearWithCombo:
            met = (maxCombo_ >= stage_.targetValue);
            break;
        case ObjectiveType::PlacePieces:
            met = (lines_ >= stage_.targetValue);
            break;
        case ObjectiveType::NoHold:
            met = (noHoldLines_ >= stage_.targetValue);
            break;
        case ObjectiveType::SpeedClear:
            met = (lines_ >= stage_.targetValue);
            break;
    }
    if (met) {
        stageComplete_ = true;
        over_ = true;
    }
}

int Game::stageObjectiveProgress() const {
    if (mode_ != GameMode::Stage) return 0;
    switch (stage_.objective) {
        case ObjectiveType::ClearLines:
        case ObjectiveType::SpeedClear:
            return lines_;
        case ObjectiveType::ReachScore:
            return score_;
        case ObjectiveType::SurviveTime:
            return elapsedSeconds();
        case ObjectiveType::ClearTetrises:
            return tetrisCount_;
        case ObjectiveType::ClearWithCombo:
            return maxCombo_;
        case ObjectiveType::PlacePieces:
            return lines_;
        case ObjectiveType::NoHold:
            return noHoldLines_;
    }
    return 0;
}

std::string Game::stageObjectiveText() const {
    if (mode_ != GameMode::Stage) return "";
    switch (stage_.objective) {
        case ObjectiveType::ClearLines:
            return "Clear " + std::to_string(stage_.targetValue) + " lines";
        case ObjectiveType::ReachScore:
            return "Reach " + std::to_string(stage_.targetValue) + " pts";
        case ObjectiveType::SurviveTime:
            return "Survive " + std::to_string(stage_.targetValue) + "s";
        case ObjectiveType::ClearTetrises:
            return "Clear " + std::to_string(stage_.targetValue) + " tetris";
        case ObjectiveType::ClearWithCombo:
            return std::to_string(stage_.targetValue) + "x combo";
        case ObjectiveType::PlacePieces:
            return "Clear " + std::to_string(stage_.targetValue) + " lines in " + std::to_string(stage_.maxPieces) + " pieces";
        case ObjectiveType::NoHold:
            return "Clear " + std::to_string(stage_.targetValue) + " lines (no hold)";
        case ObjectiveType::SpeedClear:
            return "Clear " + std::to_string(stage_.targetValue) + " lines in " + std::to_string(stage_.timeLimit) + "s";
    }
    return "";
}

int Game::stageTimeRemaining() const {
    if (mode_ != GameMode::Stage || stage_.timeLimit == 0) return -1;
    return std::max(0, stage_.timeLimit - elapsedSeconds());
}

void Game::refillBag() {
    bag_.clear();
    for (int i = 0; i < PIECE_COUNT; ++i)
        bag_.push_back(static_cast<PieceType>(i));
    std::shuffle(bag_.begin(), bag_.end(), rng_);
    bagIdx_ = 0;
}

PieceType Game::drawFromBag() {
    if (bagIdx_ >= static_cast<int>(bag_.size()))
        refillBag();
    return bag_[bagIdx_++];
}

bool Game::collides(const Piece& p) const {
    const Shape& s = p.shape();
    int sz = p.size();
    for (int dr = 0; dr < sz; ++dr) {
        for (int dc = 0; dc < sz; ++dc) {
            if (!s[dr][dc]) continue;
            int br = p.row() + dr;
            int bc = p.col() + dc;
            if (!board_.inBounds(br, bc) || board_.occupied(br, bc))
                return true;
        }
    }
    return false;
}

bool Game::tryMove(const Piece& cand) {
    if (!collides(cand)) {
        curr_ = cand;
        return true;
    }
    return false;
}

void Game::moveLeft() {
    Piece p = curr_;
    p.setCol(p.col() - 1);
    tryMove(p);
}

void Game::moveRight() {
    Piece p = curr_;
    p.setCol(p.col() + 1);
    tryMove(p);
}

void Game::softDrop() {
    if (mode_ == GameMode::Stage && stage_.disableSoftDrop) return;
    Piece p = curr_;
    p.setRow(p.row() + 1);
    if (tryMove(p))
        score_ += 1;
    lastDrop_ = std::chrono::steady_clock::now();
}

void Game::hardDrop() {
    int dist = 0;
    Piece p = curr_;
    while (true) {
        p.setRow(p.row() + 1);
        if (collides(p)) break;
        ++dist;
    }
    curr_.setRow(curr_.row() + dist);
    score_ += dist * 2;
    lockPiece();
}

void Game::rotate() {
    Piece p = curr_;
    p.setRot(p.nextRot());

    // Wall-kick offsets: 0, +/-1 col, +/-2 col, -1 row
    static const int kicks[][2] = {
        { 0, 0}, {-1, 0}, { 1, 0}, {-2, 0}, { 2, 0}, { 0,-1},
    };
    for (const auto& [dc, dr] : kicks) {
        Piece cand = p;
        cand.setCol(cand.col() + dc);
        cand.setRow(cand.row() + dr);
        if (!collides(cand)) {
            curr_ = cand;
            return;
        }
    }
}

void Game::lockPiece() {
    board_.place(curr_.row(), curr_.col(), curr_.shape(),
                 static_cast<int>(curr_.type()) + 1);
    int n = board_.clearLines();
    lastClear_ = ClearResult();
    lastClear_.linesCleared = n;
    piecesPlaced_++;
    
    if (n > 0) {
        static const int pts[] = {0, 100, 300, 500, 800};
        score_ += pts[n] * level_;
        lines_ += n;
        level_ = lines_ / 10 + 1;
        combo_++;
        lastClear_.combo = combo_;
        if (combo_ > maxCombo_) maxCombo_ = combo_;
        if (combo_ > 1) {
            score_ += 50 * combo_ * level_;
        }
        if (n == 4) {
            lastClear_.isTetris = true;
            tetrisCount_++;
        }
        // Track no-hold lines
        if (mode_ == GameMode::Stage && stage_.objective == ObjectiveType::NoHold) {
            noHoldLines_ += n;
        }
    } else {
        combo_ = 0;
    }
    canHold_ = true;

    // Check piece limit
    if (mode_ == GameMode::Stage && stage_.maxPieces > 0 && piecesPlaced_ >= stage_.maxPieces) {
        checkStageComplete();
        if (!stageComplete_) over_ = true;
        return;
    }

    // Check time limit
    if (mode_ == GameMode::Stage && stage_.timeLimit > 0 && elapsedSeconds() >= stage_.timeLimit) {
        checkStageComplete();
        if (!stageComplete_) over_ = true;
        return;
    }

    checkStageComplete();
    if (!stageComplete_) spawnPiece();
}

void Game::spawnPiece() {
    curr_ = next_;
    next_ = Piece(drawFromBag());
    curr_.setCol((BOARD_W - curr_.size()) / 2);
    curr_.setRow(0);
    if (collides(curr_))
        over_ = true;
    lastDrop_ = std::chrono::steady_clock::now();
}

void Game::holdPiece() {
    if (!canHold_) return;
    if (mode_ == GameMode::Stage && stage_.disableHold) return;
    canHold_ = false;
    
    if (holdPiece_.has_value()) {
        Piece temp = holdPiece_.value();
        holdPiece_ = curr_;
        curr_ = temp;
        curr_.setCol((BOARD_W - curr_.size()) / 2);
        curr_.setRow(0);
    } else {
        holdPiece_ = curr_;
        spawnPiece();
    }
    lastDrop_ = std::chrono::steady_clock::now();
}

int Game::dropMs() const {
    return std::max(50, 800 - (level_ - 1) * 55);
}

void Game::tick() {
    if (over_ || paused_) return;
    auto now = std::chrono::steady_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now - lastDrop_).count();
    if (ms < dropMs()) return;w's

    Piece p = curr_;
    p.setRow(p.row() + 1);
    if (!collides(p)) {
        curr_ = p;
    } else {
        lockPiece();
    }
    lastDrop_ = std::chrono::steady_clock::now();
}

int Game::ghostRow() const {
    Piece g = curr_;
    while (true) {
        g.setRow(g.row() + 1);
        if (collides(g)) return g.row() - 1;
    }
}

// ============================================================
//  High scores
// ============================================================

static const std::string SCORES_FILE = "highscores.txt";
static const int MAX_HIGH_SCORES = 10;

std::vector<HighScore> loadHighScores() {
    std::vector<HighScore> scores;
    std::ifstream file(SCORES_FILE);
    if (!file.is_open()) return scores;
    
    std::string line;
    while (std::getline(file, line) && scores.size() < MAX_HIGH_SCORES) {
        std::istringstream iss(line);
        HighScore hs;
        if (iss >> hs.name >> hs.score >> hs.level >> hs.lines >> hs.time) {
            scores.push_back(hs);
        }
    }
    return scores;
}

void saveHighScore(const HighScore& hs) {
    auto scores = loadHighScores();
    scores.push_back(hs);
    std::sort(scores.begin(), scores.end(),
              [](const HighScore& a, const HighScore& b) { return a.score > b.score; });
    if (scores.size() > MAX_HIGH_SCORES)
        scores.resize(MAX_HIGH_SCORES);
    
    std::ofstream file(SCORES_FILE);
    for (const auto& s : scores) {
        file << s.name << " " << s.score << " " << s.level << " "
             << s.lines << " " << s.time << "\n";
    }
}

// ============================================================
//  Stage definitions
// ============================================================

std::vector<Stage> getAllStages() {
    return {
        // --- Chapter 1: Tutorial ---
        {1,  "First Steps",      "Clear 5 lines",
         ObjectiveType::ClearLines, 5, 0, 1, 0, false, false, {}},
        {2,  "Getting Faster",   "Clear 10 lines within 120s",
         ObjectiveType::SpeedClear, 10, 120, 1, 0, false, false, {}},
        {3,  "Score Hunter",     "Reach 2000 points",
         ObjectiveType::ReachScore, 2000, 0, 2, 0, false, false, {}},

        // --- Chapter 2: Survival ---
        {4,  "Hold Your Ground", "Survive 60 seconds",
         ObjectiveType::SurviveTime, 60, 60, 2, 0, false, false, {}},
        {5,  "Junkyard",         "Clear 8 lines with garbage",
         ObjectiveType::ClearLines, 8, 0, 2, 0, false, false,
         {{19,0},{19,1},{19,3},{19,4},{19,5},{19,6},{19,8},{19,9},
          {18,0},{18,2},{18,3},{18,5},{18,7},{18,8}}},
        {6,  "Combo Master",     "Achieve a 3x combo",
         ObjectiveType::ClearWithCombo, 3, 0, 3, 0, false, false, {}},

        // --- Chapter 3: Tetris ---
        {7,  "Tetris Time",      "Clear 3 tetris (4-line) clears",
         ObjectiveType::ClearTetrises, 3, 0, 3, 0, false, false, {}},
        {8,  "Piece Limit",      "Clear 15 lines in 40 pieces",
         ObjectiveType::PlacePieces, 15, 0, 3, 40, false, false, {}},
        {9,  "No Hold Challenge","Clear 10 lines without hold",
         ObjectiveType::NoHold, 10, 0, 4, 0, true, false, {}},

        // --- Chapter 4: Intense ---
        {10, "Speed Demon",      "Clear 12 lines within 90s",
         ObjectiveType::SpeedClear, 12, 90, 5, 0, false, false, {}},
        {11, "Deep Garbage",     "Clear 10 lines with deep garbage",
         ObjectiveType::ClearLines, 10, 0, 5, 0, false, false,
         {{19,0},{19,2},{19,4},{19,6},{19,8},
          {18,1},{18,3},{18,5},{18,7},{18,9},
          {17,0},{17,2},{17,4},{17,6},{17,8},
          {16,1},{16,3},{16,5},{16,7},{16,9}}},
        {12, "Tetris Storm",     "Clear 5 tetris clears",
         ObjectiveType::ClearTetrises, 5, 0, 6, 0, false, false, {}},

        // --- Chapter 5: Expert ---
        {13, "Speed Tetris",     "Clear 4 tetris in 60s",
         ObjectiveType::SpeedClear, 16, 60, 7, 0, false, false, {}},
        {14, "Minimalist",       "Clear 20 lines in 30 pieces",
         ObjectiveType::PlacePieces, 20, 0, 8, 30, false, false, {}},
        {15, "No Mercy",         "Clear 15 lines with no hold & garbage",
         ObjectiveType::ClearLines, 15, 0, 8, 0, true, false,
         {{19,1},{19,3},{19,5},{19,7},{19,9},
          {18,0},{18,2},{18,4},{18,6},{18,8}}},
        {16, "Combo King",       "Achieve a 5x combo",
         ObjectiveType::ClearWithCombo, 5, 0, 9, 0, false, false, {}},

        // --- Chapter 6: Master ---
        {17, "Grand Tetris",     "Clear 8 tetris clears",
         ObjectiveType::ClearTetrises, 8, 0, 10, 0, false, false, {}},
        {18, "Marathon",         "Survive 180 seconds",
         ObjectiveType::SurviveTime, 180, 180, 12, 0, false, false, {}},
        {19, "Gauntlet",         "Clear 25 lines within 120s",
         ObjectiveType::SpeedClear, 25, 120, 15, 0, false, false, {}},
        {20, "Final Challenge",  "Clear 30 lines, no hold, max speed",
         ObjectiveType::ClearLines, 30, 0, 20, 0, true, false,
         {{19,0},{19,1},{19,2},{19,4},{19,5},{19,7},{19,8},{19,9},
          {18,0},{18,3},{18,6},{18,9}}},
    };
}

static const std::string STAGE_FILE = "stagedata.txt";

std::vector<StageProgress> loadStageProgress() {
    std::vector<StageProgress> progress(20);
    std::ifstream file(STAGE_FILE);
    if (!file.is_open()) return progress;
    
    std::string line;
    int idx = 0;
    while (std::getline(file, line) && idx < 20) {
        std::istringstream iss(line);
        iss >> progress[idx].stars >> progress[idx].bestScore >> progress[idx].bestTime;
        idx++;
    }
    return progress;
}

void saveStageProgress(const std::vector<StageProgress>& progress) {
    std::ofstream file(STAGE_FILE);
    for (const auto& p : progress) {
        file << p.stars << " " << p.bestScore << " " << p.bestTime << "\n";
    }
}

// ============================================================
//  Board helpers
// ============================================================

void Board::setCell(int r, int c, int val) {
    if (inBounds(r, c))
        grid_[r][c] = val;
}

void Board::addGarbage(int rows) {
    for (int r = 0; r < TOTAL_H - rows; ++r)
        for (int c = 0; c < BOARD_W; ++c)
            grid_[r + rows][c] = grid_[r][c];
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < BOARD_W; ++c)
            grid_[r][c] = 8; // garbage color
}
