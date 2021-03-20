# Inputs
# ---
# Client code build target.  One of:
# - RELEASE (optimized for runtime performance)
# - PROFILED (release build with profiler symbols for gprof)
# - DEVELOPMENT (optimized for developer iteration time: compilation time, runtime, and debugging)
# - DEBUG (optimized for debugging with gdb)
# - TRACE (debug build with extremely verbose logging)
TARGET := DEVELOPMENT
APP := game


# Dependencies
# ---
CC := gcc
CMAKE := cmake
LUA := luajit
PYTHON := python3


# Outputs
# ---
BUILD := build/local/$(TARGET)
CLIENT := $(BUILD)/j25_client

# Commands
# ---
.PHONY: $(CLIENT) release run run_headless run_debugger profile docs clean
.DEFAULT_GOAL := $(CLIENT)


CMAKE_BUILD_TYPE := DEBUG
ifeq ($(TARGET),RELEASE)
	CMAKE_BUILD_TYPE := RELEASE
endif
ifeq ($(TARGET),PROFILED)
	CMAKE_BUILD_TYPE := RELEASE
endif

$(CLIENT):
	$(CMAKE) -S . -B $(BUILD) -D CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -D JE_BUILD_TARGET=$(TARGET) -G"Ninja" -D CMAKE_C_COMPILER=$(CC) -D CMAKE_UNITY_BUILD=false
	cmake --build $(BUILD)
release:
	make TARGET=RELEASE APP=$(APP)

	rm -rf build/release
	mkdir -p build/release/client build/release/engine build/release/apps

	cp -r build/local/RELEASE/j25_client -- build/release/j25_client
	cp -r client/data -- build/release/client
	cp -r engine/{lib,systems,util,*.lua} -- build/release/engine
	cp -r apps/$(APP) -- build/release/apps

	rm -rf build/release/{client,engine}/{docs,README.md} build/release/client/src

	tar -czf j25_release_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz -- build/release/*
run: $(CLIENT)
	$(CLIENT) --app apps/$(APP)
run_headless:
	$(LUA) apps/$(APP)/main.lua
run_debugger: $(CLIENT)
	gdb --args $(CLIENT) --app apps/$(APP)
profile: gmon.out
	gprof -b $(CLIENT)* gmon.out > profile.txt
docs:
	echo "Note: you may need to install python requirements first, via \"$(PYTHON) -m pip install -r scripts/requirements.txt\""
	$(PYTHON) scripts/build_docs.py --src-dir engine/docs/src --build-dir build/docs/engine
clean:
	rm -f game_dump.sav game_save.sav gmon.out profile.txt
	rm -rf build j25_release_*.tar.gz
