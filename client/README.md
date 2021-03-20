# J25 client
Responsible for starting the game engine, and handling input/output between a player and the engine.  This includes a window, input, rendering, and sound.

The engine client is hooked up to the engine using an interface exposed in lua.

Usage notes:
- Not meant to be used directly by a game.  The lua engine encapsulates all client usage

Building notes:
- Built via the Makefile in the repository root
- Built as a unity build (with all c files #included in main.c)
- All external dependencies are included in stdafx.h and built as a precompiled header to improve subsequent engine build time
