#!/usr/bin/env python3
"""
get_spotify_token.py
────────────────────
Run once on your computer to get a Spotify refresh token for the macropad.
Uses PKCE + manual redirect (no local server, no http:// URI issues).

Requirements:
    pip install requests

Setup (one-time):
    1. Go to https://developer.spotify.com/dashboard → create an app
    2. Under "Edit Settings" add this exact Redirect URI:
           https://oauth.pstmn.io/v1/callback
    3. Set CLIENT_ID below (no secret needed with PKCE)
    4. Run:  python3 get_spotify_token.py
    5. Log in to Spotify in the browser that opens
    6. You'll be redirected to a Postman page — copy the full URL from
       the browser address bar and paste it back into this terminal
    7. Copy the printed refresh_token into include/spotify.h
"""

import os, sys, base64, hashlib, secrets, urllib.parse, webbrowser
import requests

# ── Fill these in ─────────────────────────────────────────────
CLIENT_ID    = os.getenv("SPOTIFY_CLIENT_ID", "9f16bb15c7a947daa9236f36eb0ee4c4")
# No client secret required for PKCE public-client flow
REDIRECT_URI = "https://oauth.pstmn.io/v1/callback"
SCOPES       = "user-read-currently-playing user-read-playback-state"

if CLIENT_ID == "YOUR_CLIENT_ID":
    print("ERROR: set CLIENT_ID in the script or export SPOTIFY_CLIENT_ID=...")
    sys.exit(1)

# ── PKCE verifier + challenge ─────────────────────────────────
verifier  = secrets.token_urlsafe(64)
challenge = base64.urlsafe_b64encode(
    hashlib.sha256(verifier.encode()).digest()
).rstrip(b"=").decode()

auth_url = (
    "https://accounts.spotify.com/authorize"
    f"?client_id={CLIENT_ID}"
    f"&response_type=code"
    f"&redirect_uri={urllib.parse.quote(REDIRECT_URI, safe='')}"
    f"&scope={urllib.parse.quote(SCOPES, safe='')}"
    f"&code_challenge_method=S256"
    f"&code_challenge={challenge}"
)

print("\n→ Opening Spotify login in your browser…")
print("  (If it doesn't open, visit this URL manually:)")
print(f"\n  {auth_url}\n")
webbrowser.open(auth_url)

# ── Manual redirect capture ───────────────────────────────────
print("After authorising, you'll land on a Postman page.")
print("Copy the FULL URL from your browser's address bar and paste it here.\n")
redirected = input("Paste the redirect URL: ").strip()

parsed = urllib.parse.urlparse(redirected)
params = urllib.parse.parse_qs(parsed.query)

if "error" in params:
    print(f"\nERROR from Spotify: {params['error']}")
    sys.exit(1)

code = params.get("code", [None])[0]
if not code:
    print("\nERROR: no 'code' found in that URL. Did you copy the full address bar URL?")
    sys.exit(1)

# ── Exchange code → tokens (PKCE, no client secret) ──────────
r = requests.post(
    "https://accounts.spotify.com/api/token",
    data={
        "grant_type":    "authorization_code",
        "code":          code,
        "redirect_uri":  REDIRECT_URI,
        "client_id":     CLIENT_ID,
        "code_verifier": verifier,
    },
    headers={"Content-Type": "application/x-www-form-urlencoded"},
)

if r.status_code != 200:
    print(f"\nERROR: token exchange failed ({r.status_code})")
    print(r.text)
    sys.exit(1)

data = r.json()

if "refresh_token" not in data:
    print("\nERROR: no refresh_token in response. Scopes may have been rejected.")
    print(data)
    sys.exit(1)

print("\n✅  Success! Paste these into include/spotify.h:\n")
print(f'#define SPOTIFY_CLIENT_ID     "{CLIENT_ID}"')
print(f'#define SPOTIFY_CLIENT_SECRET ""          // not needed with PKCE')
print(f'#define SPOTIFY_REFRESH_TOKEN "{data["refresh_token"]}"')
print()
print(f'(access_token for testing: {data["access_token"][:40]}…)')