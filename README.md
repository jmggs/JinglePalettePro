# Jingle Palette Pro — Qt/C++ Port

**Version:** v0.2
**Platform:** Windows · macOS · Linux  
**Framework:** Qt 6 · CMake · C++17

This is Jingle Palette Pro, a full cross-platform port of the original Jingle Palette (Visual Basic 6)  
to Qt 6 / C++, preserving all features of the original.

<img width="1000" height="870" alt="Screenshot 2026-04-01 at 15 10 26" src="https://github.com/user-attachments/assets/93b4f7d3-c8f7-4484-b86c-6886dd0560e8" />

---

## Features

- 30 jingle buttons per palette, unlimited palettes
- WAV, MP3, MP2, MP1, MPA, OGG, FLAC, AIFF, AIF, M4A, AAC playback via Qt Multimedia
- Touch-screen compatible interface
- Auto Mix, Auto Repeat, Loop per jingle
- Individual volume per jingle
- Time Announce function (play a sound + jingle at a set time)
- Internet stream playback
- VU meter
- Remaining time display with warning
- Keyboard shortcuts for fast trigger (1-20) + Space for Pause
- HTTP remote control API (play, pause, stop, automix, autorepeat)
- Multi-language support (language.ini)
- Settings saved to jp.ini (portable, no registry)

---

## Requirements

| Tool         | Version     |
|--------------|-------------|
| Qt           | 6.4 or later |
| CMake        | 3.16 or later |
| Compiler     | GCC 9+, Clang 10+, MSVC 2019+ |

Install Qt from: https://www.qt.io/download

---

## Build Instructions

### Linux / macOS
```bash
chmod +x build.sh
./build.sh
```

### Windows (Qt Creator)
1. Open **Qt Creator**
2. File → Open → select `CMakeLists.txt`
3. Choose a Qt 6 kit
4. Press **Build** (Ctrl+B)

### Manual CMake
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

---

## Project Structure

```
JinglePalette/  ← source tree for Jingle Palette Pro
├── CMakeLists.txt
├── build.sh
├── src/
│   ├── main.cpp               ← Entry point
│   ├── mainwindow.h/.cpp      ← Main window (buttons, display, timers)
│   ├── audioengine.h/.cpp     ← All audio playback (Qt Multimedia)
│   ├── palettemanager.h/.cpp  ← palette.ini read/write
│   ├── settingsmanager.h/.cpp ← jp.ini read/write
│   ├── languagemanager.h/.cpp ← language.ini multi-language
│   ├── settingsdialog.h/.cpp  ← Settings window
│   ├── aboutdialog.h/.cpp     ← About window
│   ├── vumeter.h/.cpp         ← Custom VU meter widget
│   ├── jinglebutton.h/.cpp    ← Jingle button widget
│   ├── errorlogger.h/.cpp     ← Error logging
│   └── globals.h              ← Shared data structures
└── resources/
    ├── resources.qrc
    ├── Time_Announce.wav
    └── West_Musette.wav
```

---

## Files to include alongside the executable

| File             | Purpose                          |
|------------------|----------------------------------|
| `jp.ini`         | Settings (created on first run)  |
| `palette.ini`    | Palettes (created on first run)  |
| `language.ini`   | Language strings                 |
| `Time_Announce.wav` | Default time announce sound   |

---

## Keyboard Shortcuts

- `1-5` -> Jingles 1-5
- `Q W E R T` -> Jingles 6-10
- `A S D F G` -> Jingles 11-15
- `Z X C V B` -> Jingles 16-20
- `Space` -> Pause

---

## HTTP API

- Base URL: `http://<IP>:8000/`
- `GET /01` to `GET /30` -> Play jingle 1..30
- `GET /pause` -> Pause / Resume
- `GET /stop` -> Stop all
- `GET /automix` -> Toggle AutoMix
- `GET /autorepeat` -> Toggle AutoRepeat

---

## VB6 → Qt/C++ key mappings

| VB6                         | Qt/C++                              |
|-----------------------------|-------------------------------------|
| `ImpulseRegistryAndINI`     | `QSettings` (INI format)            |
| `BASS_StreamCreateFile`     | `QMediaPlayer::setSource()`         |
| `BASS_ChannelPlay`          | `QMediaPlayer::play()`              |
| `BASS_ChannelStop`          | `QMediaPlayer::stop()`              |
| `BASS_ChannelGetLevel`      | VU approximation (level via state)  |
| `Timer1..5`                 | `QTimer` (5 separate timers)        |
| `LevelMeter OCX`            | Custom `VuMeter` widget             |
| `SSTab`                     | `QTabWidget`                        |
| `IniSet.Entry(...)`         | `QSettings::value()/setValue()`     |
