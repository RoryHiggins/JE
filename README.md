# J25 game engine

Lua/C game engine for game jams.  Very much WIP, only for personal use.

<!--TOC-->

- [Folder structure](#folder-structure)
- [Games](#games)
- [Getting set up](#getting-set-up)
  - [Building and running - Windows](#building-and-running---windows)
  - [Building and running - Linux](#building-and-running---linux)
  - [Dependency versions](#dependency-versions)
  - [Running the tests](#running-the-tests)

<!--TOC-->

## Folder structure
*games/* - folder containing games using the engine

*games/(game)/data/* - binary data used by the given game

*engine/* - lua game engine providing a game loop, input/output, entity component system, and other game utilities

*engine/lib/* - lua libraries used by the engine

*client/* - c client responsible for input/output for the engine, including window creation, rendering, audio

*client/data/* - binary data used by the client

*scripts/* - utility scripts for the project


## Games
[games/game/](games/game/README.md) - platformer game built in parallel with the engine

[games/minimal/](games/minimal/README.md) - example game with the minimum code needed to do something interesting with the engine


## Getting set up

### Building - Windows

1. Install [MSYS2](https://www.msys2.org/)

2. Run a MSYS2 MinGW-w64 shell (open start menu, search for mingw-w64)

3. Install dependencies:
```
# some dependencies may come with MSYS2, --needed only installs ones you don't have
pacman -S --needed \
	mingw-w64-x86_64-gcc \
	mingw-w64-x86_64-make \
	mingw-w64-x86_64-luajit \
	mingw-w64-x86_64-SDL2 \
	mingw-w64-x86_64-zlib \
	mingw-w64-x86_64-libpng
```

4. Checkout repository:
```
cd /c/Users/$(whoami)/Downloads
git clone git@github.com:RoryHiggins/JE.git

cd JE
```

### Building - Linux
Untested on Linux.  Package manager dependencies should be identical to Windows, but the Makefile may require tweaking.


### Running
Everything can be done from the Makefile:
```
# build the client
make

# run the default game using the built client.  builds client if not already built
make run

# override the game to run
make run GAME=games/game

# override target and rebuild with debug logging and gdb-friendly debug symbols
make -B TARGET=DEBUG

# other targets are:
# - RELEASE - all optimizations, no logging, no debug symbols.  for releases
# - PROFILED - release build with enough extra information to generate a profile with gprof
# - DEVELOPMENT (default) - optimized for compile-time, minimal logging
# - DEBUG - optimized for debugging with gdb.  some extra logging enabled
# - TRACE - debug build with some extremely verbose logging enabled

# run client in gdb.  for debugging client crashes
make run_debugger

# create a fully packaged release.tar.gz which can be delivered standalone
make release GAME=games/game

# generate a performance profile using gprof.  build with TARGET=PROFILED, run game, then run this command
make profile

# clean artefacts
make clean
```


### Dependency versions

GCC == 10.2.0

LuaJIT == 2.0.5

SDL2 == 2.0.12

zlib == 1.2.11

libpng == 1.6.37

These are the only tested versions.  Earlier versions are likely supported for these libraries.


### Running the tests
When the client is built with either the DEVELOPMENT/DEBUG/TRACE build modes, tests are run automatically at the beginning of a game.  If the build mode is DEVELOPMENT, test logging is suppressed to warnings only.
