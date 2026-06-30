# 🐍 Classic Snake — SDL2 / C++

A polished classic Snake game built with C++17 and SDL2.

## Features
- Smooth grid-based movement with speed that increases as you eat
- Pulsing animated food with glow effect
- Snake eyes on the head + alternating body shading
- HUD showing score, best score, and current level
- Pause (P / ESC) and restart (R / Enter) support
- Game-over overlay with NEW HIGH SCORE detection

---

## Controls

| Key              | Action          |
|------------------|-----------------|
| Arrow keys / WASD| Move snake      |
| P or ESC         | Pause / Resume  |
| R or Enter       | Restart (on death) |
| Q                | Quit            |

---

## How to Build & Run

### ── Linux (Ubuntu / Debian) ──────────────────────────────────────────────────

**1. Install dependencies**
```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev
```

**2. Build**
```bash
cd project_150
make
```

**3. Run**
```bash
./snake
# or just:
make run
```

---

### ── macOS (Homebrew) ──────────────────────────────────────────────────────────

**1. Install dependencies**
```bash
brew install sdl2 sdl2_ttf
```

**2. Build**
```bash
cd project_150
make
```

**3. Run**
```bash
./snake
```

---

### ── Windows (MSYS2 / MinGW-w64) ─────────────────────────────────────────────

**1. Install MSYS2** from https://www.msys2.org/

**2. Open "MSYS2 MinGW 64-bit" terminal, install dependencies**
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf make
```

**3. Build**
```bash
cd project_150
make
```

**4. Run**
```bash
./snake.exe
```

> **Note:** If you get a "DLL not found" error, copy these DLLs from
> `C:\msys64\mingw64\bin\` into your `project_150\` folder:
> `SDL2.dll`, `SDL2_ttf.dll`, `libfreetype-6.dll`, `libbrotlidec.dll`,
> `libbrotlicommon.dll`, `libpng16-16.dll`, `zlib1.dll`, `libbz2-1.dll`

---

### ── Windows (Visual Studio + vcpkg) ─────────────────────────────────────────

**1. Install vcpkg** (if not already)
```powershell
git clone https://github.com/microsoft/vcpkg
cd vcpkg && bootstrap-vcpkg.bat
vcpkg install sdl2 sdl2-ttf
```

**2. Open CMake project in Visual Studio**
```
File → Open → CMake → select project_150/CMakeLists.txt
Build → Build All
```

---

### ── Using CMake (any platform) ──────────────────────────────────────────────

```bash
cd project_150
mkdir build && cd build
cmake ..
cmake --build . --config Release
./snake        # Linux/macOS
.\snake.exe    # Windows
```

---

## File Structure

```
project_150/
├── snake.cpp        ← all game code (single file)
├── Makefile         ← simple build (Linux/macOS/MinGW)
├── CMakeLists.txt   ← CMake build (cross-platform)
└── README.md        ← this file
```

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `sdl2-config: not found` | Install `libsdl2-dev` |
| `SDL_ttf.h: No such file` | Install `libsdl2-ttf-dev` |
| Fonts show as boxes | Install `fonts-dejavu` (`sudo apt install fonts-dejavu`) |
| Black screen on macOS | Try running from terminal, not double-click |
| `./snake: Permission denied` | Run `chmod +x snake` first |
