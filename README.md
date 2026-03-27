# Jingle Palette — Qt/C++ Port

**Version:** 4.4.5  
**Platform:** Windows · macOS · Linux  
**Framework:** Qt 6 · CMake · C++17

This is a full cross-platform port of the original Jingle Palette (Visual Basic 6)  
to Qt 6 / C++, preserving all features of the original.

---

## Features

- 30 jingle buttons per palette, unlimited palettes
- MP3, WAV, OGG, MP2 playback via Qt Multimedia
- Touch-screen compatible interface
- Auto Mix, Auto Repeat, Loop per jingle
- Individual volume per jingle
- Time Announce function (play a sound + jingle at a set time)
- Internet stream playback
- VU meter
- Remaining time display with warning
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
JinglePalette/
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
