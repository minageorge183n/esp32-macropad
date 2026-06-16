#include "snake_game.h"
#include "macropad.h"   // KEY_PINS, DEBOUNCE_MS

extern int gameKeyRaw(int idx);  // defined in games.cpp — thin wrapper

// ─────────────────────────────────────────────────────────────
void SnakeGame::init(TFT_eSPI* tft) {
    _tft      = tft;
    _len      = 4;
    _dx = 1; _dy = 0;
    _ndx = 1; _ndy = 0;
    _score    = 0;
    _stepMs   = SNG_START_MS;
    _lastStep = millis();
    _gameOver = false;
    _wantsExit = false;
    _needsRedraw = true;
    _atePrevStep = false;

    // Starting body: horizontal line in centre
    int8_t cx = SNG_COLS / 2, cy = SNG_ROWS / 2;
    for (int i = 0; i < _len; i++) {
        _body[i] = { (int8_t)(cx - i), cy };
    }
    _placeFood();
    _fullRedraw();
}

// ─────────────────────────────────────────────────────────────
bool SnakeGame::update() {
    if (_wantsExit) return false;

    // ── Button scan (direct digitalRead — debounce via hold time) ──
    // Layout on 3×3 grid:
    //  [0][1][2]
    //  [3][4][5]
    //  [6][7][8]
    // Up=key1, Left=key3, Right=key5, Down=key7, Exit=key4 long-held elsewhere
    bool up    = (digitalRead(KEY_PINS[1]) == LOW);
    bool left  = (digitalRead(KEY_PINS[3]) == LOW);
    bool right = (digitalRead(KEY_PINS[5]) == LOW);
    bool down  = (digitalRead(KEY_PINS[7]) == LOW);

    // Direction (ignore 180° reversal)
    if (up    && _dy !=  1) { _ndx =  0; _ndy = -1; }
    if (down  && _dy != -1) { _ndx =  0; _ndy =  1; }
    if (left  && _dx !=  1) { _ndx = -1; _ndy =  0; }
    if (right && _dx != -1) { _ndx =  1; _ndy =  0; }

    // Exit via key4 (centre) held
    static uint32_t exitHoldStart = 0;
    bool centre = (digitalRead(KEY_PINS[4]) == LOW);
    if (centre) {
        if (exitHoldStart == 0) exitHoldStart = millis();
        if (millis() - exitHoldStart > 1500) { _wantsExit = true; return false; }
    } else {
        exitHoldStart = 0;
    }

    if (_gameOver) {
        // Any key restarts
        if (up || down || left || right) {
            init(_tft);
        }
        return true;
    }

    // ── Timed step ───────────────────────────────────────────
    uint32_t now = millis();
    if (now - _lastStep < _stepMs) return true;
    _lastStep = now;

    _dx = _ndx; _dy = _ndy;

    // Save tail before shift
    _prevTail = _body[_len - 1];

    // Move body
    for (int i = _len - 1; i > 0; i--) _body[i] = _body[i - 1];
    _body[0].x += _dx;
    _body[0].y += _dy;

    // Wall collision
    if (_body[0].x < 0 || _body[0].x >= SNG_COLS ||
        _body[0].y < 0 || _body[0].y >= SNG_ROWS) {
        _gameOver = true;
        _drawGameOver();
        return true;
    }

    // Self collision
    for (int i = 1; i < _len; i++) {
        if (_body[i].x == _body[0].x && _body[i].y == _body[0].y) {
            _gameOver = true;
            _drawGameOver();
            return true;
        }
    }

    // Food
    bool ate = (_body[0].x == _food.x && _body[0].y == _food.y);
    if (ate) {
        _score++;
        _len++;
        if (_stepMs > SNG_MIN_MS) _stepMs -= 8;
        _placeFood();
    }
    _atePrevStep = ate;

    // ── Dirty-rect draw ──────────────────────────────────────
    // Erase tail (unless we ate)
    if (!ate) {
        _drawCell(_prevTail.x, _prevTail.y, SNG_C_BG);
    }
    // Repaint old head as body
    if (_len > 1) {
        _drawCell(_body[1].x, _body[1].y, SNG_C_BODY);
    }
    // Draw new head
    _drawCell(_body[0].x, _body[0].y, SNG_C_HEAD);

    if (ate) {
        _drawCell(_food.x, _food.y, SNG_C_FOOD);  // new food
        _drawScoreBar();
    }

    return true;
}

// ─────────────────────────────────────────────────────────────
void SnakeGame::draw() { /* dirty-rect handled in update */ }

void SnakeGame::_placeFood() {
    // Simple retry loop (fast enough at this scale)
    do {
        _food.x = random(0, SNG_COLS);
        _food.y = random(0, SNG_ROWS);
        bool onBody = false;
        for (int i = 0; i < _len; i++)
            if (_body[i].x == _food.x && _body[i].y == _food.y) { onBody = true; break; }
        if (!onBody) break;
    } while (true);
    _drawCell(_food.x, _food.y, SNG_C_FOOD);
}

void SnakeGame::_drawCell(int8_t x, int8_t y, uint16_t colour) {
    int px = SNG_ORIGIN_X + x * SNG_CELL;
    int py = SNG_ORIGIN_Y + y * SNG_CELL;
    _tft->fillRect(px, py, SNG_CELL - 1, SNG_CELL - 1, colour);
}

void SnakeGame::_drawScoreBar() {
    _tft->fillRect(0, 0, 240, SNG_ORIGIN_Y - 1, SNG_C_BG);
    _tft->setTextColor(SNG_C_TEXT, SNG_C_BG);
    _tft->setTextDatum(ML_DATUM);
    _tft->setTextSize(1);
    _tft->drawString("SNAKE", 4, 14);
    char buf[16]; snprintf(buf, sizeof(buf), "Score: %d", _score);
    _tft->setTextDatum(MR_DATUM);
    _tft->drawString(buf, 236, 14);
}

void SnakeGame::_drawGameOver() {
    _tft->setTextColor(SNG_C_HEAD, SNG_C_BG);
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextSize(2);
    _tft->drawString("GAME OVER", 120, 110);
    _tft->setTextSize(1);
    char buf[24]; snprintf(buf, sizeof(buf), "Score: %d", _score);
    _tft->drawString(buf, 120, 132);
    _tft->drawString("Any dir = restart", 120, 148);
    _tft->drawString("Hold centre = exit", 120, 160);
}

void SnakeGame::_fullRedraw() {
    _tft->fillScreen(SNG_C_BG);
    // Border
    _tft->drawRect(SNG_ORIGIN_X - 1, SNG_ORIGIN_Y - 1,
                   SNG_COLS * SNG_CELL + 2, SNG_ROWS * SNG_CELL + 2, SNG_C_BORDER);
    _drawScoreBar();
    // Food
    _drawCell(_food.x, _food.y, SNG_C_FOOD);
    // Body
    for (int i = _len - 1; i > 0; i--) _drawCell(_body[i].x, _body[i].y, SNG_C_BODY);
    _drawCell(_body[0].x, _body[0].y, SNG_C_HEAD);
}
