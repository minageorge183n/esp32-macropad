#include "tetris_game.h"
#include "macropad.h"

// ── Piece shapes [type][rotation][row][col] ───────────────────
static const uint8_t PIECES[7][4][4][4] = {
  // I
  {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
   {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
   {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
  // O
  {{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}},
  // T
  {{{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,1,0,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,1,0,0}},
   {{0,0,0,0},{0,1,0,0},{1,1,0,0},{0,1,0,0}}},
  // S
  {{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0}},
   {{0,0,0,0},{0,0,0,0},{0,1,1,0},{1,1,0,0}},
   {{0,0,0,0},{1,0,0,0},{1,1,0,0},{0,1,0,0}}},
  // Z
  {{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,0,1,0},{0,1,1,0},{0,1,0,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,0,0},{0,1,1,0}},
   {{0,0,0,0},{0,1,0,0},{1,1,0,0},{1,0,0,0}}},
  // J
  {{{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,0,0},{0,1,0,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,0,1,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,0,0},{1,1,0,0}}},
  // L
  {{{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,0,0},{0,1,1,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,0},{1,0,0,0}},
   {{0,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,0,0}}},
};

static const int SCORE_TABLE[5] = { 0, 100, 300, 500, 800 };

// ─────────────────────────────────────────────────────────────
void TetrisGame::init(TFT_eSPI* tft) {
    _tft = tft;
    memset(_board, 0, sizeof(_board));
    _score = 0; _level = 1; _lines = 0;
    _dropMs = 600;
    _lastDrop = millis();
    _lastInput = 0;
    _gameOver = false;
    _wantsExit = false;
    _boardDirty = true;
    memset(_btnLast, 0, sizeof(_btnLast));

    _tft->fillScreen(TET_C_BG);
    _spawnPiece();
    _drawBoard();
    _drawPanel();
}

// ── Edge-triggered button read with optional auto-repeat ─────
bool TetrisGame::_btnEdge(int idx, uint32_t repeatMs) {
    bool pressed = (digitalRead(KEY_PINS[idx]) == LOW);
    uint32_t now = millis();
    if (!pressed) { _btnLast[idx] = 0; return false; }
    if (_btnLast[idx] == 0) { _btnLast[idx] = now; return true; }  // first press
    if (repeatMs > 0 && (now - _btnLast[idx]) > repeatMs) {
        _btnLast[idx] = now - repeatMs / 2;   // half-rate repeat
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
bool TetrisGame::update() {
    if (_wantsExit) return false;

    // Exit: hold Enc2 SW (key pin = ENC2_SW defined in macropad.h)
    static uint32_t exitStart = 0;
    bool enc2sw = (digitalRead(ENC2_SW) == LOW);
    if (enc2sw) {
        if (!exitStart) exitStart = millis();
        if (millis() - exitStart > 1200) { _wantsExit = true; return false; }
    } else {
        exitStart = 0;
    }

    if (_gameOver) {
        // Any key in column 1 (key 1,4,7) restarts
        if (_btnEdge(1) || _btnEdge(4) || _btnEdge(7)) init(_tft);
        return true;
    }

    bool panelDirty = false;

    // ── Horizontal move (keys 3=left, 5=right, repeat 150ms) ─
    if (_btnEdge(3, 150)) {
        _drawPiece(_cur, true);   // erase ghost
        _drawPiece(_ghost, true, true);
        if (_valid(_cur.x - 1, _cur.y, _cur.type, _cur.rot)) _cur.x--;
        _calcGhost();
        _drawPiece(_ghost, false, true);
        _drawPiece(_cur);
    }
    if (_btnEdge(5, 150)) {
        _drawPiece(_cur, true);
        _drawPiece(_ghost, true, true);
        if (_valid(_cur.x + 1, _cur.y, _cur.type, _cur.rot)) _cur.x++;
        _calcGhost();
        _drawPiece(_ghost, false, true);
        _drawPiece(_cur);
    }

    // ── Rotate (key 1=CW, key 7=CCW) ─────────────────────────
    if (_btnEdge(1)) {
        _drawPiece(_cur, true);
        _drawPiece(_ghost, true, true);
        uint8_t nr = (_cur.rot + 1) % 4;
        if (_valid(_cur.x, _cur.y, _cur.type, nr)) _cur.rot = nr;
        else if (_valid(_cur.x + 1, _cur.y, _cur.type, nr)) { _cur.x++; _cur.rot = nr; }
        else if (_valid(_cur.x - 1, _cur.y, _cur.type, nr)) { _cur.x--; _cur.rot = nr; }
        _calcGhost();
        _drawPiece(_ghost, false, true);
        _drawPiece(_cur);
    }

    // ── Soft drop (key 4=down, repeat 80ms) ──────────────────
    uint32_t effectiveDrop = _dropMs;
    if (digitalRead(KEY_PINS[4]) == LOW) effectiveDrop = 60;

    // ── Hard drop (key 7) ────────────────────────────────────
    if (_btnEdge(7)) {
        _drawPiece(_cur, true);
        _drawPiece(_ghost, true, true);
        _cur.y = _ghost.y;
        _place();
        int cleared = _clearLines();
        _score += SCORE_TABLE[cleared] * _level;
        _lines += cleared;
        _level = _lines / 10 + 1;
        _dropMs = max(80, 600 - (_level - 1) * 50);
        panelDirty = true;
        _boardDirty = true;
        _spawnPiece();
        if (!_valid(_cur.x, _cur.y, _cur.type, _cur.rot)) {
            _gameOver = true;
            _drawGameOver();
            return true;
        }
        _drawBoard();
        _calcGhost();
        _drawPiece(_ghost, false, true);
        _drawPiece(_cur);
    }

    // ── Gravity drop ─────────────────────────────────────────
    uint32_t now = millis();
    if (now - _lastDrop >= effectiveDrop) {
        _lastDrop = now;
        if (_valid(_cur.x, _cur.y + 1, _cur.type, _cur.rot)) {
            _drawPiece(_cur, true);
            _cur.y++;
            _drawPiece(_cur);
        } else {
            // Lock
            _drawPiece(_ghost, true, true);
            _place();
            int cleared = _clearLines();
            _score += SCORE_TABLE[cleared] * _level;
            _lines += cleared;
            _level = _lines / 10 + 1;
            _dropMs = max(80, 600 - (_level - 1) * 50);
            panelDirty = true;
            _boardDirty = true;
            _spawnPiece();
            if (!_valid(_cur.x, _cur.y, _cur.type, _cur.rot)) {
                _gameOver = true;
                _drawBoard();
                _drawGameOver();
                return true;
            }
            _drawBoard();
            _calcGhost();
            _drawPiece(_ghost, false, true);
            _drawPiece(_cur);
        }
    }

    if (panelDirty) _drawPanel();
    return true;
}

void TetrisGame::draw() { /* dirty-rect handled in update */ }

// ─────────────────────────────────────────────────────────────
bool TetrisGame::_valid(int8_t x, int8_t y, uint8_t type, uint8_t rot) const {
    for (int py = 0; py < 4; py++)
        for (int px = 0; px < 4; px++)
            if (PIECES[type][rot][py][px]) {
                int bx = x + px, by = y + py;
                if (bx < 0 || bx >= TET_COLS || by >= TET_ROWS) return false;
                if (by >= 0 && _board[by][bx]) return false;
            }
    return true;
}

void TetrisGame::_spawnPiece() {
    _cur = { (int8_t)(TET_COLS / 2 - 2), 0, (uint8_t)random(0, 7), 0 };
    _calcGhost();
}

void TetrisGame::_calcGhost() {
    _ghost = _cur;
    while (_valid(_ghost.x, _ghost.y + 1, _ghost.type, _ghost.rot))
        _ghost.y++;
}

void TetrisGame::_place() {
    for (int py = 0; py < 4; py++)
        for (int px = 0; px < 4; px++)
            if (PIECES[_cur.type][_cur.rot][py][px]) {
                int by = _cur.y + py, bx = _cur.x + px;
                if (by >= 0) _board[by][bx] = _cur.type + 1; // 1-based colour idx
            }
}

int TetrisGame::_clearLines() {
    int cleared = 0;
    for (int y = TET_ROWS - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < TET_COLS; x++)
            if (!_board[y][x]) { full = false; break; }
        if (full) {
            for (int my = y; my > 0; my--)
                memcpy(_board[my], _board[my - 1], TET_COLS);
            memset(_board[0], 0, TET_COLS);
            cleared++; y++;
        }
    }
    return cleared;
}

// ─────────────────────────────────────────────────────────────
void TetrisGame::_drawCell(int x, int y, uint16_t colour) {
    int px = TET_ORIGIN_X + x * TET_CELL;
    int py = TET_ORIGIN_Y + y * TET_CELL;
    if (colour == TET_C_EMPTY)
        _tft->fillRect(px, py, TET_CELL - 1, TET_CELL - 1, TET_C_EMPTY);
    else
        _tft->fillRect(px, py, TET_CELL - 1, TET_CELL - 1, colour);
}

void TetrisGame::_drawBoard() {
    // Background + border
    _tft->fillRect(TET_ORIGIN_X, TET_ORIGIN_Y,
                   TET_COLS * TET_CELL, TET_ROWS * TET_CELL, TET_C_BG);
    _tft->drawRect(TET_ORIGIN_X - 1, TET_ORIGIN_Y - 1,
                   TET_COLS * TET_CELL + 2, TET_ROWS * TET_CELL + 2, TET_C_BORDER);
    for (int y = 0; y < TET_ROWS; y++)
        for (int x = 0; x < TET_COLS; x++)
            if (_board[y][x])
                _drawCell(x, y, TET_COLOURS[_board[y][x] - 1]);
}

void TetrisGame::_drawPiece(const TetPiece& p, bool erase, bool ghost) {
    for (int py = 0; py < 4; py++)
        for (int px = 0; px < 4; px++)
            if (PIECES[p.type][p.rot][py][px]) {
                int by = p.y + py, bx = p.x + px;
                if (by < 0 || by >= TET_ROWS || bx < 0 || bx >= TET_COLS) continue;
                uint16_t col;
                if (erase)       col = _board[by][bx] ? TET_COLOURS[_board[by][bx]-1] : TET_C_BG;
                else if (ghost)  col = TET_C_GHOST;
                else             col = TET_COLOURS[p.type];
                _drawCell(bx, by, col);
            }
}

void TetrisGame::_drawPanel() {
    int px = TET_PANEL_X, pw = TET_PANEL_W;
    _tft->fillRect(px, 0, pw, 240, TET_C_BG);

    _tft->setTextColor(TET_C_TEXT, TET_C_BG);
    _tft->setTextSize(1);
    _tft->setTextDatum(TL_DATUM);

    _tft->drawString("TETRIS", px, 4);

    _tft->drawString("SCR", px, 28);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", _score);
    _tft->drawString(buf, px, 40);

    _tft->drawString("LVL", px, 62);
    snprintf(buf, sizeof(buf), "%d", _level);
    _tft->drawString(buf, px, 74);

    _tft->drawString("LNS", px, 96);
    snprintf(buf, sizeof(buf), "%d", _lines);
    _tft->drawString(buf, px, 108);

    // Controls hint (tiny)
    _tft->setTextSize(1);
    _tft->setTextColor(0x4208, TET_C_BG);
    _tft->drawString("3<  5>", px, 160);
    _tft->drawString("1rot", px, 172);
    _tft->drawString("4dn", px, 184);
    _tft->drawString("7drp", px, 196);
}

void TetrisGame::_drawGameOver() {
    _tft->setTextColor(0xF800, TET_C_BG);
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextSize(2);
    _tft->drawString("GAME", 65, 100);
    _tft->drawString("OVER", 65, 120);
    _tft->setTextSize(1);
    _tft->setTextColor(TET_C_TEXT, TET_C_BG);
    char buf[16]; snprintf(buf, sizeof(buf), "Score:%d", _score);
    _tft->drawString(buf, 65, 145);
    _tft->drawString("1/4/7=restart", 65, 160);
}
