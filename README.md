# 🔘 ESP32 Bluetooth + WiFi Macropad

A custom-built macropad featuring **9 programmable keys** and **2 rotary encoders**, designed for productivity, streaming, editing, and general automation.
It connects via **Bluetooth** for real-time input and **WiFi** for easy configuration through a web-based UI.

---

## ✨ Features

* 🎹 **9 Mechanical Keys**
  Fully programmable keys that can send:

  * Keyboard shortcuts
  * Media controls
  * Text strings

* 🔄 **2 Rotary Encoders**
  Each encoder supports:

  * Clockwise / Counterclockwise rotation
  * Push button click

* 📡 **Dual Connectivity**

  * **Bluetooth (BLE)** → Acts as a wireless keyboard
  * **WiFi** → Hosts a configuration web interface

* 🌐 **Web UI Configuration**

  * Access via browser (phone or PC)
  * Remap keys and encoders بسهولة
  * No need to reflash firmware for changes

---

## ⚙️ How It Works

### Bluetooth Mode

* The macropad connects to your device as a **BLE keyboard**
* Pressing keys sends predefined actions instantly

### WiFi Mode

* The device creates or connects to a WiFi network
* You access a **local web page** hosted on the ESP32
* From there, you can:

  * Change key mappings
  * Assign new actions
  * Save configurations

---

## 🚀 Getting Started

### 1. Power On

* Connect the macropad via USB or battery
* It will automatically start Bluetooth and WiFi services

### 2. Connect via Bluetooth

* Pair it like a normal keyboard
* Start using default key mappings

### 3. Access Web Interface

* Connect to the same WiFi network (or device hotspot mode)
* Open browser and go to:

  ```
  http://<device-ip>
  ```
* Configure keys and encoders from the UI

---

## 🧠 Key Mapping Capabilities

Each key can be configured to:

* 🔹 Send a single key (e.g., `A`, `Enter`)
* 🔹 Use modifiers (`Ctrl`, `Alt`, `Shift`)
* 🔹 Trigger media actions (volume, play/pause)
* 🔹 Type full strings (macros)

---

## 🎛️ Rotary Encoder Functions

Each encoder supports:

* ↻ Rotate Left → Action 1
* ↺ Rotate Right → Action 2
* ⏺ Press → Action 3

Example uses:

* Volume control
* Timeline scrubbing
* Zoom in/out

---

## 🛠️ Hardware Overview

* ESP32 microcontroller
* 9 mechanical switches
* 2 rotary encoders with push buttons
* Optional battery module

---

## 📌 Use Cases

* 🎥 Streaming (OBS, shortcuts)
* 🎨 Design (Photoshop, Illustrator)
* 🎬 Video editing (Premiere, DaVinci)
* 💻 General productivity
* 🎮 Gaming macros

---

## 🔮 Future Improvements

* Profiles switching
* OLED display support
* Cloud sync for configs
* Mobile app companion

---

## 📄 Notes

* Make sure Bluetooth is enabled on your device
* Web UI works best on modern browsers
* Keep firmware updated for new features

---

## 🤝 Contributing

This is a personal project, but ideas and improvements are always welcome.

---

## 📜 License



---
