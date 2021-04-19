# Inputs
# ---
# Client code build target.  One of:
# - RELEASE (optimized for runtime performance)
# - PROFILED (release build with profiler symbols for gprof)
# - DEVELOPMENT (optimized for developer iteration time: compilation time, runtime, and debugging)
# - DEBUG (optimized for debugging with gdb)
# - TRACE (debug build with extremely verbose logging)
TARGET := DEVELOPMENT
APP := example_platformer
HEADLESS_CLIENT := 0
UNITY_BUILD := 0

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
.PHONY: $(CLIENT) release run run_headless debug profile docs clean
.DEFAULT_GOAL := $(CLIENT)


CMAKE_BUILD_TYPE := DEBUG
ifeq ($(TARGET),RELEASE)
	CMAKE_BUILD_TYPE := RELEASE
endif
ifeq ($(TARGET),PROFILED)
	CMAKE_BUILD_TYPE := RELEASE
endif

$(CLIENT):
	$(CMAKE) -S . -B $(BUILD) -D CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -D JE_BUILD_TARGET=$(TARGET) -D JE_BUILD_HEADLESS=$(HEADLESS_CLIENT) -D JE_DEFAULT_APP=$(APP) -G"Ninja" -D CMAKE_C_COMPILER=$(CC) -D CMAKE_UNITY_BUILD=$(UNITY_BUILD)
	$(CMAKE) --build $(BUILD)
release:
	make TARGET=RELEASE APP=$(APP)

	rm -rf build/release
	mkdir build/release

	cp build/local/RELEASE/j25_client -- build/release
	cp build/local/RELEASE/*.dll -- build/release

	mkdir build/release/client
	cp -r client/data -- build/release/client

	# cp -r engine -- build/release
	mkdir build/release/engine
	cp -r engine/lib -- build/release/engine
	cp -r engine/systems -- build/release/engine
	cp -r engine/util -- build/release/engine
	cp engine/*.lua -- build/release/engine

	mkdir build/release/apps
	cp -r apps/$(APP) -- build/release/apps

	tar -C build/release -czf j25_release_`date +"%Y_%m_%d_%H_%M_%S"`.tar.gz .
run: $(CLIENT)
	$(CLIENT) --app apps/$(APP)
run_headless:
	$(LUA) apps/$(APP)/main.lua
run_debugger: $(CLIENT)
	gdb -ex 'break jeErr' --ex run --args $(CLIENT) --debug --app apps/$(APP)
profile: gmon.out
	gprof -b $(CLIENT)* gmon.out > profile.txt && cat profile.txt
tidy:
	find ./client -name '*.c' -or -name '*.h' | xargs -I TIDY_INPUT clang-tidy TIDY_INPUT -- -Iclient/include -Iclient/src
format:
	find ./client -name '*.c' -or -name '*.h' | xargs clang-format -i -Werror --
clean:
	rm -f game_dump.sav game_save.sav gmon.out profile.txt
	rm -rf build j25_release_*.tar.gz
