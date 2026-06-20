#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// ── WiFi credentials (hardcode here) ─────────────────────────
#define WIFI_SSID     "TV"
#define WIFI_PASSWORD "go24212161@#$"

// ── Spotify OAuth (PKCE — no client secret needed) ───────────
// 1. Go to https://developer.spotify.com/dashboard → create an app
// 2. Under "Edit Settings" add redirect URI:
//       https://oauth.pstmn.io/v1/callback
// 3. Run tools/get_spotify_token.py — it opens a browser, you log in,
//    paste back the redirect URL, and it prints the values below.
//    Required scopes (granted automatically by the script):
//      user-read-currently-playing  user-read-playback-state
// 4. Paste the printed values here.
#define SPOTIFY_CLIENT_ID     "9f16bb15c7a947daa9236f36eb0ee4c4"
#define SPOTIFY_REFRESH_TOKEN "AQAWbFOKtNvJJobFa667upRHiPf87DixKXjyxN_mEh4RcvXn_3OtekKSBMTz362YakeH6vaPBLgTS-4StXGbOiFibljY9jM9rxpwwfL5K8pLQ00dO3l2EHJI6orNM70gBL8"

// ── Poll interval ─────────────────────────────────────────────
#define SPOTIFY_POLL_MS   3000   // how often to query /me/player
#define SPOTIFY_TOKEN_MS  3300000 // re-fetch access token every ~55 min

// ── Track info (populated after each poll) ───────────────────
struct SpotifyTrack {
    char  title[64];
    char  artist[64];
    char  albumArtUrl[256];   // 64×64 or smallest available
    int   progressMs;
    int   durationMs;
    bool  isPlaying;
    bool  valid;              // false = nothing playing / no data yet
};

extern SpotifyTrack spotifyTrack;
extern bool         spotifyReady;   // true once WiFi + first token fetched

// ── Public API ────────────────────────────────────────────────
void spotifyInit(TFT_eSPI* tft);   // connect WiFi, fetch token, first poll
void spotifyTick();                // call every loop; handles polling + display
void spotifyDrawFull();            // force full redraw of Spotify screen