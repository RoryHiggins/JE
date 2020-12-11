TARGET := DEVELOPMENT  # RELEASE, PROFILED, DEVELOPMENT, DEBUG, TRACE
GAME := games/game

CMAKE := cmake
LUA := luajit
PYTHON := python3
CLIENT := build/client_launcher

CMAKE_BUILD_TYPE := DEBUG
ifeq ($(TARGET),RELEASE)
	CMAKE_BUILD_TYPE := RELEASE
endif
ifeq ($(TARGET),PROFILED)
	CMAKE_BUILD_TYPE := RELEASE
endif

CMAKE_FLAGS := -S . -B build -G"Ninja" -D CMAKE_C_COMPILER=gcc -D CMAKE_UNITY_BUILD=ON

$(CLIENT):
	$(CMAKE) $(CMAKE_FLAGS) -D CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -D JE_BUILD_TARGET=$(TARGET) 
	cmake --build build

release:
	$(CMAKE) $(CMAKE_FLAGS) -D CMAKE_BUILD_TYPE=RELEASE
	cmake --build build
	rm -rf release
	mkdir release

	cp -r $(CLIENT) -- release/client_launcher
	cp -r client/data -- release/client
	cp -r engine -- release

	mkdir release/games
	cp -r $(GAME) -- release/games

	tar -czf j25_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- release/*
run: $(CLIENT)
	$(CLIENT) -game $(GAME)
run_headless:
	$(LUA) $(GAME)/main.lua
run_debugger: $(CLIENT)
	gdb --args $(CLIENT) -game $(GAME)
profile: gmon.out
	gprof -b $(CLIENT)* gmon.out > profile.txt
docs:
	echo "Note: you may need to install python requirements first, via \"$(PYTHON) -m pip install -r scripts/requirements.txt\""
	$(PYTHON) scripts/build_docs.py --src-dir engine/docs/src --build-dir engine/docs/client/build
clean:
	rm -f game_dump.sav game_save.sav gmon.out profile.txt
	rm -rf build engine/docs/client/build

.PHONY: $(CLIENT) release run run_debugger run_headless profile docs release clean
.DEFAULT_GOAL := $(CLIENT)
