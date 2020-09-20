# J25 game engine
Lua/C game engine for game jams.  Very much WIP, only for personal use.


## Folder structure
*client/* - c client responsible for input/output for the game, including window creation, rendering, audio

*client/data/* - binary data used by the client

*engine/* - lua game engine providing a game loop, input/output, entity component system, and other game utilities

*engine/lib/* - lua libraries used by the engine

*games/(game)/data/* - binary data used by the given game


## Games
[games/game/](games/game/README.md) - platformer game built in parallel with the engine

*games/minimal/* - example game with the minimum code needed to do something interesting with the engine


## Getting set up
### Dependencies
LuaJIT ~ 2.0.5

SDL2 ~ 2.0.12

zlib ~ 1.2.11

libpng ~ 1.6.37


### Building the engine client
TODO


### Running the examples
TODO


### Running the tests
If the engine client is built with the DEVELOPMENT/DEBUG/TRACE targets, tests are run automatically at the beginning of a game.
