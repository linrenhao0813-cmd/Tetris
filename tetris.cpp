#include "tetris.h"
#include <bits/stdc++.h>

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
{
    refillBag();
    curr_ = Piece(drawFromBag());
    next_ = Piece(drawFromBag());
    curr_.setCol((BOARD_W - curr_.size()) / 2);
    curr_.setRow(0);
    lastDrop_ = std::chrono::steady_clock::now();
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
    if (n > 0) {
        static const int pts[] = {0, 100, 300, 500, 800};
        score_ += pts[n] * level_;
        lines_ += n;
        level_ = lines_ / 10 + 1;
    }
    spawnPiece();
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

int Game::dropMs() const {
    return std::max(50, 800 - (level_ - 1) * 55);
}

void Game::tick() {
    if (over_ || paused_) return;
    auto now = std::chrono::steady_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now - lastDrop_).count();
    if (ms < dropMs()) return;

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
