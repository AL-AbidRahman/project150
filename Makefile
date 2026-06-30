# Simple Makefile for Snake (Linux / macOS / Windows-MinGW)
# Usage:  make          → build
#         make run      → build and run
#         make clean    → remove build files

CXX      := g++
TARGET   := snake
SRC      := snake.cpp
CXXFLAGS := -std=c++17 -O2 -Wall

# ── Detect OS ─────────────────────────────────────────────────────────────────
UNAME := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME), Linux)
    # Ubuntu/Debian: sudo apt install libsdl2-dev libsdl2-ttf-dev
    SDL_CFLAGS := $(shell sdl2-config --cflags)
    SDL_LIBS   := $(shell sdl2-config --libs) -lSDL2_ttf
    EXT        :=
endif

ifeq ($(UNAME), Darwin)
    # macOS (Homebrew): brew install sdl2 sdl2_ttf
    SDL_CFLAGS := $(shell sdl2-config --cflags)
    SDL_LIBS   := $(shell sdl2-config --libs) -lSDL2_ttf
    EXT        :=
endif

ifeq ($(UNAME), Windows)
    # MinGW-w64 on Windows
    SDL_CFLAGS := -I/mingw64/include
    SDL_LIBS   := -L/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
    EXT        := .exe
endif

# ── Rules ─────────────────────────────────────────────────────────────────────
all: $(TARGET)$(EXT)

$(TARGET)$(EXT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -o $@ $< $(SDL_LIBS)
	@echo "Build successful → $(TARGET)$(EXT)"

run: all
	./$(TARGET)$(EXT)

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: all run clean
