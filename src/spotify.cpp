#include "spotify.h"
#include "display.h"      // C_* palette constants, SCREEN_W/H
#include "macropad.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TJpg_Decoder.h>   // lib: Bodmer/TJpg_Decoder

// ── Module state ─────────────────────────────────────────────
SpotifyTrack spotifyTrack  = {};
bool         spotifyReady  = false;

static TFT_eSPI* _tft      = nullptr;
static String    _accessToken;
static uint32_t  _tokenFetchedAt = 0;
static uint32_t  _lastPoll       = 0;
static char      _lastArtUrl[256] = "";   // skip re-download if same URL

// ── Layout (Spotify screen) ───────────────────────────────────
// ┌──────────────────────────────┐  y=0
// │     "Spotify"  top bar  28px │
// ├────────────┬─────────────────┤  y=30
// │            │  Title          │
// │  Album art │  Artist         │
// │  (88×88)   │                 │
// │            │                 │
// ├────────────┴─────────────────┤  y=122
// │  ████░░░░░░░░░░░  progress   │
// │  0:32 ───────────────── 3:14 │
// └──────────────────────────────┘

#define SP_ART_X    6
#define SP_ART_Y    32
#define SP_ART_W    88
#define SP_ART_H    88

#define SP_TEXT_X   (SP_ART_X + SP_ART_W + 8)
#define SP_TEXT_W   (SCREEN_W - SP_TEXT_X - 4)
#define SP_TITLE_Y  42
#define SP_ARTIST_Y 78

#define SP_BAR_X    6
#define SP_BAR_Y    128
#define SP_BAR_W    (SCREEN_W - 12)
#define SP_BAR_H    8
#define SP_TIME_Y   142

// Spotify green
#define C_SP_GREEN  0x0604   // #1DB954 → RGB565

// ── JPEG → TFT callback ───────────────────────────────────────
// TJpg_Decoder calls this for every MCU block
static bool _jpegRender(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= SP_ART_H) return 0;
    _tft->pushImage(SP_ART_X + x, SP_ART_Y + y, w, h, bitmap);
    return 1;
}

// ── Helpers ───────────────────────────────────────────────────
static void _wrapText(const char* src, char* line1, char* line2, int maxChars) {
    int len = strlen(src);
    if (len <= maxChars) {
        strncpy(line1, src, maxChars); line1[maxChars] = '\0';
        line2[0] = '\0';
        return;
    }
    // break at last space before maxChars
    int brk = maxChars;
    for (int i = maxChars - 1; i > 0; i--) {
        if (src[i] == ' ') { brk = i; break; }
    }
    strncpy(line1, src, brk); line1[brk] = '\0';
    strncpy(line2, src + brk + 1, maxChars); line2[maxChars] = '\0';
}

static void _fmtTime(int ms, char* buf) {
    int s = ms / 1000;
    snprintf(buf, 8, "%d:%02d", s / 60, s % 60);
}

// ─────────────────────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────────────────────
static void _connectWiFi() {
    _tft->fillScreen(C_BG);
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(C_TEXT, C_BG);
    _tft->setTextSize(1);
    _tft->drawString("Connecting to WiFi...", SCREEN_W / 2, 100);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(300);
    }

    if (WiFi.status() == WL_CONNECTED) {
        _tft->fillScreen(C_BG);
        _tft->drawString("WiFi OK", SCREEN_W / 2, 100);
        _tft->setTextColor(C_SP_GREEN, C_BG);
        _tft->drawString(WiFi.localIP().toString().c_str(), SCREEN_W / 2, 118);
        delay(800);
    } else {
        _tft->setTextColor(C_RED, C_BG);
        _tft->drawString("WiFi FAILED", SCREEN_W / 2, 118);
        delay(2000);
    }
}

// ─────────────────────────────────────────────────────────────
// Token refresh (client_credentials + refresh_token flow)
// ─────────────────────────────────────────────────────────────
static bool _refreshToken() {
    WiFiClientSecure client;
    client.setInsecure();   // skip cert verify for ESP32 simplicity

    HTTPClient http;
    http.begin(client, "https://accounts.spotify.com/api/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // PKCE public-client refresh — no client_secret, no Basic auth header.
    // Just send client_id in the body alongside the refresh_token.
    String body = "grant_type=refresh_token&refresh_token=";
    body += SPOTIFY_REFRESH_TOKEN;
    body += "&client_id=";
    body += SPOTIFY_CLIENT_ID;

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("[Spotify] Token refresh failed: %d\n", code);
        http.end();
        return false;
    }

    StaticJsonDocument<512> doc;
    deserializeJson(doc, http.getString());
    http.end();

    _accessToken = doc["access_token"].as<String>();
    _tokenFetchedAt = millis();
    Serial.println("[Spotify] Token refreshed OK");
    return true;
}

// ─────────────────────────────────────────────────────────────
// Poll currently playing
// ─────────────────────────────────────────────────────────────
static void _pollCurrentlyPlaying() {
    if (WiFi.status() != WL_CONNECTED) return;

    // Re-fetch token if needed (~55 min)
    if (_accessToken.isEmpty() || (millis() - _tokenFetchedAt) > SPOTIFY_TOKEN_MS) {
        if (!_refreshToken()) return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing?market=ES");
    http.addHeader("Authorization", "Bearer " + _accessToken);

    int code = http.GET();

    if (code == 204) {
        // Nothing playing
        spotifyTrack.valid = false;
        http.end();
        return;
    }
    if (code != 200) {
        Serial.printf("[Spotify] Poll failed: %d\n", code);
        http.end();
        return;
    }

    // Parse JSON — use a large buffer; Spotify responses are chunky
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err) {
        Serial.printf("[Spotify] JSON error: %s\n", err.c_str());
        return;
    }

    const char* title   = doc["item"]["name"]                    | "";
    const char* artist  = doc["item"]["artists"][0]["name"]      | "";
    int  progMs         = doc["progress_ms"]                     | 0;
    int  durMs          = doc["item"]["duration_ms"]             | 1;
    bool isPlaying      = doc["is_playing"]                      | false;

    // Pick smallest album art image (last in array = smallest)
    const char* artUrl = "";
    JsonArray images = doc["item"]["album"]["images"].as<JsonArray>();
    if (!images.isNull() && images.size() > 0) {
        artUrl = images[images.size() - 1]["url"] | "";
    }

    strncpy(spotifyTrack.title,       title,  63);
    strncpy(spotifyTrack.artist,      artist, 63);
    strncpy(spotifyTrack.albumArtUrl, artUrl, 255);
    spotifyTrack.progressMs = progMs;
    spotifyTrack.durationMs = durMs;
    spotifyTrack.isPlaying  = isPlaying;
    spotifyTrack.valid      = true;

    Serial.printf("[Spotify] %s — %s  %d/%d\n", title, artist, progMs, durMs);
}

// ─────────────────────────────────────────────────────────────
// Album art download + decode
// ─────────────────────────────────────────────────────────────
static void _fetchAlbumArt(const char* url) {
    if (!url || url[0] == '\0') return;
    if (strcmp(url, _lastArtUrl) == 0) return;   // same image, skip

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("Accept", "image/jpeg");

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[Spotify] Art fetch failed: %d\n", code);
        http.end();
        return;
    }

    int len = http.getSize();
    if (len <= 0 || len > 32000) {   // guard against huge images
        http.end();
        return;
    }

    uint8_t* buf = (uint8_t*)malloc(len);
    if (!buf) { http.end(); return; }

    WiFiClient* stream = http.getStreamPtr();
    int read = 0;
    uint32_t t = millis();
    while (read < len && (millis() - t) < 5000) {
        if (stream->available()) {
            buf[read++] = stream->read();
        }
    }
    http.end();

    if (read == len) {
        // Decode JPEG → TFT at SP_ART_X, SP_ART_Y, scaled to SP_ART_W × SP_ART_H
        TJpgDec.setJpgScale(1);                // 1 = full; Spotify sends ~64×64
        TJpgDec.setCallback(_jpegRender);
        TJpgDec.drawJpg(0, 0, buf, len);       // offset handled in callback
        strncpy(_lastArtUrl, url, 255);
    }

    free(buf);
}

// ─────────────────────────────────────────────────────────────
// Draw Spotify screen
// ─────────────────────────────────────────────────────────────
void spotifyDrawFull() {
    _tft->startWrite();
    _tft->fillScreen(C_BG);

    // ── Top bar ───────────────────────────────────────────────
    _tft->fillRoundRect(0, 0, SCREEN_W, MENUBAR_H, 10, C_PANEL);
    _tft->fillRect(0, 10, SCREEN_W, MENUBAR_H - 10, C_PANEL);
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextColor(C_SP_GREEN, C_PANEL);
    _tft->setTextSize(1);
    _tft->drawString("Spotify", SCREEN_W / 2, MENUBAR_H / 2 + 1);

    // WiFi indicator
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    _tft->fillCircle(SCREEN_W - 12, MENUBAR_H / 2, 4, wifiOk ? C_GREEN : C_RED);

    if (!spotifyTrack.valid) {
        _tft->setTextColor(C_TEXT_SEC, C_BG);
        _tft->setTextDatum(MC_DATUM);
        _tft->drawString("Nothing playing", SCREEN_W / 2, 120);
        _tft->endWrite();
        return;
    }

    // ── Album art placeholder (grey box while loading) ────────
    _tft->fillRect(SP_ART_X, SP_ART_Y, SP_ART_W, SP_ART_H, C_PANEL);
    _tft->endWrite();   // end before HTTP call (frees SPI bus briefly)

    _fetchAlbumArt(spotifyTrack.albumArtUrl);

    _tft->startWrite();

    // ── Track info ────────────────────────────────────────────
    char line1[22], line2[22];

    // Title (2 lines, font size 1 → ~18 chars per line in 134px)
    _wrapText(spotifyTrack.title, line1, line2, 18);
    _tft->setTextDatum(TL_DATUM);
    _tft->setTextColor(C_TEXT, C_BG);
    _tft->setTextSize(1);
    _tft->drawString(line1, SP_TEXT_X, SP_TITLE_Y);
    if (line2[0]) _tft->drawString(line2, SP_TEXT_X, SP_TITLE_Y + 14);

    // Artist
    _wrapText(spotifyTrack.artist, line1, line2, 18);
    _tft->setTextColor(C_TEXT_SEC, C_BG);
    _tft->drawString(line1, SP_TEXT_X, SP_ARTIST_Y);
    if (line2[0]) _tft->drawString(line2, SP_TEXT_X, SP_ARTIST_Y + 14);

    // Playing dot
    uint16_t dot = spotifyTrack.isPlaying ? C_SP_GREEN : C_TEXT_TER;
    _tft->fillCircle(SP_TEXT_X + 3, SP_ARTIST_Y + 34, 4, dot);
    _tft->setTextColor(C_TEXT_TER, C_BG);
    _tft->drawString(spotifyTrack.isPlaying ? "Playing" : "Paused",
                     SP_TEXT_X + 11, SP_ARTIST_Y + 28);

    // ── Progress bar ──────────────────────────────────────────
    _tft->fillRoundRect(SP_BAR_X, SP_BAR_Y, SP_BAR_W, SP_BAR_H, 4, C_PANEL);
    int filled = 0;
    if (spotifyTrack.durationMs > 0) {
        filled = (int)((long)spotifyTrack.progressMs * SP_BAR_W
                       / spotifyTrack.durationMs);
        filled = constrain(filled, 0, SP_BAR_W);
    }
    if (filled > 0)
        _tft->fillRoundRect(SP_BAR_X, SP_BAR_Y, filled, SP_BAR_H, 4, C_SP_GREEN);

    // Progress handle dot
    _tft->fillCircle(SP_BAR_X + filled, SP_BAR_Y + SP_BAR_H / 2, 5, C_SP_GREEN);

    // Time labels
    char tBuf[8];
    _fmtTime(spotifyTrack.progressMs, tBuf);
    _tft->setTextDatum(TL_DATUM);
    _tft->setTextColor(C_TEXT_SEC, C_BG);
    _tft->setTextSize(1);
    _tft->drawString(tBuf, SP_BAR_X, SP_TIME_Y);

    _fmtTime(spotifyTrack.durationMs, tBuf);
    _tft->setTextDatum(TR_DATUM);
    _tft->drawString(tBuf, SP_BAR_X + SP_BAR_W, SP_TIME_Y);

    // ── Key hints (bottom strip) ──────────────────────────────
    // Tiny hints showing what the keys do in Spotify mode
    const int hintY = 162;
    _tft->setTextColor(C_TEXT_TER, C_BG);
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextSize(1);

    // Row of 3 pills: [Vol+] [Mute] [Vol-]  /  [Prev] [Play] [Next]
    const char* hints[9] = {
        "Vol+", "Vol-", "Mute",
        "Prev", "Play", "Next",
        "Cut",  "Copy", "Paste"
    };
    for (int i = 0; i < 9; i++) {
        int col = i % 3, row = i / 3;
        int px = col * 80 + 40;
        int py = hintY + row * 18;
        _tft->fillRoundRect(col * 80 + 4, hintY + row * 18 - 7,
                            72, 14, 4, C_PANEL);
        _tft->drawString(hints[i], px, py);
    }

    _tft->endWrite();
}

// ─────────────────────────────────────────────────────────────
// Incrementally update only the progress bar each tick
// (avoids full redraw every second)
// ─────────────────────────────────────────────────────────────
static void _updateProgressBar() {
    if (!spotifyTrack.valid) return;

    _tft->startWrite();
    _tft->fillRoundRect(SP_BAR_X, SP_BAR_Y, SP_BAR_W, SP_BAR_H + 10 + 10, 4, C_BG);

    _tft->fillRoundRect(SP_BAR_X, SP_BAR_Y, SP_BAR_W, SP_BAR_H, 4, C_PANEL);

    int filled = 0;
    if (spotifyTrack.durationMs > 0) {
        filled = (int)((long)spotifyTrack.progressMs * SP_BAR_W
                       / spotifyTrack.durationMs);
        filled = constrain(filled, 0, SP_BAR_W);
    }
    if (filled > 0)
        _tft->fillRoundRect(SP_BAR_X, SP_BAR_Y, filled, SP_BAR_H, 4, C_SP_GREEN);
    _tft->fillCircle(SP_BAR_X + filled, SP_BAR_Y + SP_BAR_H / 2, 5, C_SP_GREEN);

    char tBuf[8];
    _fmtTime(spotifyTrack.progressMs, tBuf);
    _tft->setTextDatum(TL_DATUM);
    _tft->setTextColor(C_TEXT_SEC, C_BG);
    _tft->setTextSize(1);
    _tft->drawString(tBuf, SP_BAR_X, SP_TIME_Y);

    _fmtTime(spotifyTrack.durationMs, tBuf);
    _tft->setTextDatum(TR_DATUM);
    _tft->drawString(tBuf, SP_BAR_X + SP_BAR_W, SP_TIME_Y);

    _tft->endWrite();
}

// ─────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────
void spotifyInit(TFT_eSPI* tft) {
    _tft = tft;

    TJpgDec.setSwapBytes(true);   // ESP32 = little-endian, TFT wants big-endian

    _connectWiFi();
    _refreshToken();
    _pollCurrentlyPlaying();
    spotifyReady = true;
    spotifyDrawFull();
}

static uint32_t _lastProgressTick = 0;
static char _lastTitle[64] = "";

void spotifyTick() {
    if (!spotifyReady) return;

    uint32_t now = millis();

    // Poll Spotify API every SPOTIFY_POLL_MS
    if (now - _lastPoll >= SPOTIFY_POLL_MS) {
        _lastPoll = now;

        bool wasValid = spotifyTrack.valid;
        char prevTitle[64];
        strncpy(prevTitle, spotifyTrack.title, 63);

        _pollCurrentlyPlaying();

        // Full redraw if track changed or valid state changed
        bool trackChanged = (strcmp(spotifyTrack.title, prevTitle) != 0);
        bool validChanged = (spotifyTrack.valid != wasValid);

        if (trackChanged || validChanged) {
            _lastArtUrl[0] = '\0';   // force art re-download
            spotifyDrawFull();
            return;
        }
    }

    // Locally advance progress between polls (1 second tick)
    if (spotifyTrack.valid && spotifyTrack.isPlaying) {
        if (now - _lastProgressTick >= 1000) {
            _lastProgressTick = now;
            spotifyTrack.progressMs += 1000;
            if (spotifyTrack.progressMs > spotifyTrack.durationMs)
                spotifyTrack.progressMs = spotifyTrack.durationMs;
            _updateProgressBar();
        }
    }
}