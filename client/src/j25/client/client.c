#include <j25/client/client.h>

#include <j25/core/common.h>
#include <j25/core/container.h>
#include <j25/platform/image.h>
#include <j25/platform/rendering.h>
#include <j25/platform/audio.h>
#include <j25/platform/window.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include <zlib.h>

/*
 * C and C++ have different default linkage.
 * Lua(jit) headers don't explicitly state linkage of symbols,
 * but libs are (normally) built with C, so we need to do their job for them.
 */
#if defined(__cplusplus)
extern "C" {
#endif
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>

/* Only used for version checking via LUAJIT_VERSION_NUM */
#include <luajit-2.1/luajit.h>
#if defined(__cplusplus)
} /*extern "C"*/
#endif

#if !defined(JE_DEFAULT_APP)
#define JE_DEFAULT_APP "game"
#endif
#define JE_DEFAULT_APP_DIR "apps/" JE_DEFAULT_APP

/*https://www.lua.org/manual/5.1/manual.html*/
#define JE_LUA_STACK_TOP (-1)
#define JE_LUA_DATA_BUFFER_SIZE (8 * 1024 * 1024)

#define JE_LUA_CLIENT_BINDINGS_KEY "jeLuaClientBindings"
#define JE_LUA_CLIENT_WINDOW_KEY "jeLuaWindow"
#define JE_LUA_CLIENT_BINDING(BINDING_NAME) \
	{ #BINDING_NAME, jeLua_##BINDING_NAME }

#if JE_LOG_LEVEL_DEFAULT > JE_LOG_LEVEL_DEBUG
#define JE_TESTS_LOG_LEVEL JE_LOG_LEVEL_WARN
#else
#define JE_TESTS_LOG_LEVEL JE_LOG_LEVEL_DEFAULT
#endif

struct jeWindow;
struct lua_State;

struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};

const char* jeLua_getError(lua_State* lua);
double jeLua_getNumberField(lua_State* lua, uint32_t tableIndex, const char* field);
double jeLua_getOptionalNumberField(lua_State* lua, uint32_t tableIndex, const char* field, double defaultValue);
const char* jeLua_getStringField(lua_State* lua, uint32_t tableIndex, const char* field, uint32_t* optOutSize);
struct jeWindow* jeLua_getWindow(lua_State* lua);
bool jeLua_addWindow(lua_State* lua, struct jeWindow* window);
void jeLua_updateStates(lua_State* lua);
int jeLua_readData(lua_State* lua);
int jeLua_writeData(lua_State* lua);
void jeLua_getPrimitiveImpl(lua_State* lua, struct jeVertex* vertices, uint32_t vertexCount);
void jeLua_drawPrimitiveImpl(lua_State* lua, uint32_t primitiveType);
int jeLua_drawPoint(lua_State* lua);
int jeLua_drawLine(lua_State* lua);
int jeLua_drawTriangle(lua_State* lua);
int jeLua_drawSprite(lua_State* lua);
int jeLua_drawText(lua_State* lua);
int jeLua_drawReset(lua_State* lua);
int jeLua_runTests(lua_State* lua);
int jeLua_step(lua_State* lua);
bool jeLua_addBindings(lua_State* lua);
bool jeLua_run(struct jeWindow* window, const char* filename, int argumentCount, char** arguments);

#if (LUA_VERSION_NUM < 520) && !(defined(LUAJIT_VERSION_NUM) && (LUAJIT_VERSION_NUM >= 20100))
/*Shim adapted from https://github.com/keplerproject/lua-compat-5.2/blob/master/c-api/compat-5.2.c#L119*/
void luaL_setfuncs(lua_State* lua, const luaL_Reg* functionsIter, int functionsCount);
void luaL_setfuncs(lua_State* lua, const luaL_Reg* functionsIter, int functionsCount) {
	luaL_checkstack(lua, functionsCount + 1, "too many upvalues");

	for (; functionsIter->name != NULL; functionsIter++) { /* fill the table with given functions */
		lua_pushstring(lua, functionsIter->name);

		/* copy upvalues to the top */
		for (int i = 0; i < functionsCount; i++) {
			lua_pushvalue(lua, -(functionsCount + 1));
		}

		lua_pushcclosure(lua, functionsIter->func, functionsCount); /* closure with those upvalues */
		lua_settable(lua, -(functionsCount + 3)); /* table must be below the upvalues, the name and the closure */
	}
	lua_pop(lua, functionsCount); /* remove upvalues */
}
#endif

#if LUA_VERSION_NUM >= 520
size_t lua_objlen(lua_State* lua, int i);
size_t lua_objlen(lua_State* lua, int i) {
	return lua_rawlen(lua, i);
}
#endif

const char* jeLua_getError(lua_State* lua) {
	const char* error = lua_tostring(lua, JE_LUA_STACK_TOP);

	if (error == NULL) {
		error = "";
	}

	return error;
}
lua_Number jeLua_getNumberField(lua_State* lua, uint32_t tableIndex, const char* field) {
	lua_getfield(lua, (int)tableIndex, field);

	return luaL_checknumber(lua, JE_LUA_STACK_TOP);
}
lua_Number
jeLua_getOptionalNumberField(lua_State* lua, uint32_t tableIndex, const char* field, lua_Number defaultValue) {
	lua_getfield(lua, (int)tableIndex, field);

	return luaL_optnumber(lua, JE_LUA_STACK_TOP, defaultValue);
}
const char* jeLua_getStringField(lua_State* lua, uint32_t tableIndex, const char* field, uint32_t* optOutSize) {
	lua_getfield(lua, (int)tableIndex, field);

	const char* result = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	if (optOutSize != NULL) {
		*optOutSize = (uint32_t)lua_objlen(lua, JE_LUA_STACK_TOP);
	}

	return result;
}

struct jeWindow* jeLua_getWindow(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	bool ok = true;
	struct jeWindow* window = NULL;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	int stackPos = 0;
	if (lua != NULL) {
		stackPos = lua_gettop(lua);
	}

	if (ok) {
		lua_getglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

		if (lua_islightuserdata(lua, JE_LUA_STACK_TOP) == 0) {
			JE_ERROR("%s is not set", JE_LUA_CLIENT_WINDOW_KEY);
			ok = false;
		}
	}

	if (ok) {
		window = (struct jeWindow*)lua_touserdata(lua, JE_LUA_STACK_TOP);
		JE_TRACE("lua=%p, window=%p", (void*)lua, (void*)window);
	}

	if (lua != NULL) {
		lua_settop(lua, stackPos);
	}

	return window;
}
bool jeLua_addWindow(lua_State* lua, struct jeWindow* window) {
	JE_TRACE("lua=%p, window=%p", (void*)lua, (void*)window);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	if (jeWindow_getIsValid(window) == false) {
		JE_ERROR("window is not valid");
		ok = false;
	}

	if (ok) {
		int stackPos = lua_gettop(lua);

		lua_pushlightuserdata(lua, (void*)window);
		lua_setglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

		lua_settop(lua, stackPos);
	}

	return ok;
}
void jeLua_updateStates(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	struct jeWindow* window = jeLua_getWindow(lua);

	if (ok) {
		int stackPos = lua_gettop(lua);
		int stateStackPos = 0;

		lua_settop(lua, 0);
		lua_getglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);
		lua_getfield(lua, JE_LUA_STACK_TOP, "state");

		stateStackPos = lua_gettop(lua);

		lua_pushnumber(lua, (lua_Number)JE_WINDOW_MIN_WIDTH);
		lua_setfield(lua, stateStackPos, "width");

		lua_pushnumber(lua, (lua_Number)JE_WINDOW_MIN_HEIGHT);
		lua_setfield(lua, stateStackPos, "height");

		lua_pushnumber(lua, (lua_Number)jeLogger_getLevel());
		lua_setfield(lua, stateStackPos, "logLevel");

		lua_pushboolean(lua, true);
		lua_setfield(lua, stateStackPos, "testsEnabled");

		lua_pushnumber(lua, (lua_Number)JE_TESTS_LOG_LEVEL);
		lua_setfield(lua, stateStackPos, "testsLogLevel");

		lua_pushboolean(lua, (window != NULL) && jeWindow_getIsOpen(window));
		lua_setfield(lua, stateStackPos, "running");

		if (window != NULL) {

			lua_pushnumber(lua, (lua_Number)jeWindow_getFps(window));
			lua_setfield(lua, stateStackPos, "fps");

			lua_pushnumber(lua, (lua_Number)jeWindow_getFrame(window));
			lua_setfield(lua, stateStackPos, "frame");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_LEFT));
			lua_setfield(lua, stateStackPos, "inputLeft");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_RIGHT));
			lua_setfield(lua, stateStackPos, "inputRight");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_UP));
			lua_setfield(lua, stateStackPos, "inputUp");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_DOWN));
			lua_setfield(lua, stateStackPos, "inputDown");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_A));
			lua_setfield(lua, stateStackPos, "inputA");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_B));
			lua_setfield(lua, stateStackPos, "inputB");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_X));
			lua_setfield(lua, stateStackPos, "inputX");

			lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_Y));
			lua_setfield(lua, stateStackPos, "inputY");

			int32_t mouseX = 0;
			int32_t mouseY = 0;
			jeWindow_getMousePos(window, &mouseX, &mouseY);

			lua_pushnumber(lua, (lua_Number)mouseX);
			lua_setfield(lua, stateStackPos, "inputMouseX");

			lua_pushnumber(lua, (lua_Number)mouseY);
			lua_setfield(lua, stateStackPos, "inputMouseY");

			lua_pushboolean(lua, jeWindow_getMouseButton(window, JE_MOUSE_BUTTON_LEFT));
			lua_setfield(lua, stateStackPos, "inputMouseLeft");

			lua_pushboolean(lua, jeWindow_getMouseButton(window, JE_MOUSE_BUTTON_MIDDLE));
			lua_setfield(lua, stateStackPos, "inputMouseMiddle");

			lua_pushboolean(lua, jeWindow_getMouseButton(window, JE_MOUSE_BUTTON_RIGHT));
			lua_setfield(lua, stateStackPos, "inputMouseRight");

			lua_pushnumber(lua, (lua_Number)jeBreakpoint_getCount());
			lua_setfield(lua, stateStackPos, "breakpointCount");
		}

		lua_settop(lua, stackPos);
	}
}

// BEGIN LD48 TEMP CODE; TODO CLEANUP/REMOVE
static struct jeAudioMixer* jeAudioMixer_instance = NULL;
static struct jeAudio* jeAudio_prebaked[8] = {0};
static uint32_t jeAudio_numPrebaked = 0;
// END LD48 TEMP CODE; TODO CLEANUP/REMOVE

/*Lua-client bindings.  Note: return value = num responses pushed to lua stack*/
int jeLua_readData(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	bool ok = true;
	int numResponses = 0;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	gzFile file = NULL;
	const char* filename = "";
	if (ok) {
		filename = luaL_checkstring(lua, 1);

		JE_TRACE("lua=%p, filename=%s", (void*)lua, filename);

		file = gzopen(filename, "rb");

		if (file == NULL) {
			JE_ERROR("gzopen() failed with filename=%s, errno=%d err=%s", filename, errno, strerror(errno));
			ok = false;
		}
	}

	static char data[JE_LUA_DATA_BUFFER_SIZE] = {0};
	memset((void*)&data, 0, sizeof(data));

	int dataSize = 0;
	if (ok) {
		dataSize = gzread(file, data, JE_LUA_DATA_BUFFER_SIZE);
		if (dataSize < 0) {
			int errnum = 0;
			JE_ERROR("gzread() failed with filename=%s, gzerr=%s", filename, gzerror(file, &errnum));
			JE_MAYBE_UNUSED(errnum);
			ok = false;
		}
	}

	if (ok) {
		/*Exclude null terminator if string exists*/
		if (dataSize > 0) {
			dataSize--;
		}

		lua_pushlstring(lua, data, (size_t)dataSize);
		numResponses++;

		JE_DEBUG("fread() bytes=%d (after decompression) read from filename=%s", dataSize + 1, filename);
	}

	if (file != NULL) {
		gzclose(file);
	}

	return numResponses;
}
int jeLua_writeData(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	bool ok = true;
	int numResponses = 0;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	const char* filename = "";
	const char* data = "";
	size_t dataSize = 0;
	gzFile file = NULL;

	if (ok) {
		static const int filenameArg = 1;
		static const int dataArg = 2;

		filename = luaL_checkstring(lua, filenameArg);
		data = luaL_checkstring(lua, dataArg);
		dataSize = lua_objlen(lua, dataArg) + 1;

		JE_TRACE("lua=%p, filename=%s, dataSize=%u", (void*)lua, filename, (uint32_t)dataSize);

		file = gzopen(filename, "wb");

		if (file == NULL) {
			JE_ERROR("gzopen() failed with filename=%s, errno=%d err=%s", filename, errno, strerror(errno));
			ok = false;
		}
	}

	int dataSizeWritten = 0;
	if (ok) {
		dataSizeWritten = gzwrite(file, data, (unsigned)dataSize);

		if (dataSizeWritten < 0) {
			int errnum = 0;
			JE_ERROR("gzwrite() failed with filename=%s, gzerr=%s", filename, gzerror(file, &errnum));
			JE_MAYBE_UNUSED(errnum);
			ok = false;
		}
	}

	if (ok) {
		lua_pushboolean(lua, true);
		numResponses++;

		JE_DEBUG("bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);
	}

	if (file != NULL) {
		gzclose(file);
	}

	return numResponses;
}
void jeLua_getPrimitiveImpl(lua_State* lua, struct jeVertex* vertices, uint32_t vertexCount) {
	JE_TRACE("lua=%p, vertices=%p, vertexCount=%u", (void*)lua, (void*)vertices, vertexCount);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	if (vertices == NULL) {
		JE_ERROR("vertices=NULL");
		ok = false;
	}

	if (vertexCount == 0) {
		JE_ERROR("vertexCount=0");
		ok = false;
	}

	if (ok) {
		static const int renderableIndex = 1;
		static const int defaultsIndex = 2;
		static const int cameraIndex = 3;

		luaL_checktype(lua, renderableIndex, LUA_TTABLE);
		luaL_checktype(lua, defaultsIndex, LUA_TTABLE);
		luaL_checktype(lua, cameraIndex, LUA_TTABLE);

		vertices[0].x = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "x", 0.0F);
		vertices[0].y = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "y", 0.0F);
		vertices[0].x = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "x1", vertices[0].x);
		vertices[0].y = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "y1", vertices[0].y);
		vertices[0].z = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "z", 0.0F);
		vertices[0].u = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "u1", 0.0F);
		vertices[0].v = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "v1", 0.0F);

		vertices[0].r = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "r", 1.0F);
		vertices[0].g = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "g", 1.0F);
		vertices[0].b = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "b", 1.0F);
		vertices[0].a = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "a", 1.0F);
		vertices[0].r = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "r", vertices[0].r);
		vertices[0].g = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "g", vertices[0].g);
		vertices[0].b = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "b", vertices[0].b);
		vertices[0].a = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "a", vertices[0].a);

		if (vertexCount >= 2) {
			vertices[1].x = vertices[0].x + (float)jeLua_getOptionalNumberField(lua, renderableIndex, "w", 0.0F);
			vertices[1].y = vertices[0].y + (float)jeLua_getOptionalNumberField(lua, renderableIndex, "h", 0.0F);
			vertices[1].x = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "x2", vertices[1].x);
			vertices[1].y = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "y2", vertices[1].y);
			vertices[1].u = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "u2", 1.0F);
			vertices[1].v = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "v2", 1.0F);
		}

		if (vertexCount >= 3) {
			vertices[2].x = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "x3", 0.0F);
			vertices[2].y = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "y3", 0.0F);
			vertices[2].u = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "u3", 1.0F);
			vertices[2].v = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "v3", 1.0F);
		}

		for (uint32_t i = 1; i < vertexCount; i++) {
			vertices[i].z = vertices[0].z;
			vertices[i].r = vertices[0].r;
			vertices[i].g = vertices[0].g;
			vertices[i].b = vertices[0].b;
			vertices[i].a = vertices[0].a;
		}

		float cameraX1 = (float)jeLua_getOptionalNumberField(lua, cameraIndex, "x", 0.0F);
		float cameraY1 = (float)jeLua_getOptionalNumberField(lua, cameraIndex, "y", 0.0F);
		float cameraX2 = cameraX1 + (float)jeLua_getOptionalNumberField(lua, cameraIndex, "w", 0.0F);
		float cameraY2 = cameraY1 + (float)jeLua_getOptionalNumberField(lua, cameraIndex, "h", 0.0F);
		cameraX1 = (float)jeLua_getOptionalNumberField(lua, cameraIndex, "x1", cameraX1);
		cameraY1 = (float)jeLua_getOptionalNumberField(lua, cameraIndex, "y1", cameraY1);
		cameraX2 = (float)jeLua_getOptionalNumberField(lua, cameraIndex, "x2", cameraX2);
		cameraY2 = (float)jeLua_getOptionalNumberField(lua, cameraIndex, "y2", cameraY2);

		float offsetX = 0.0F;
		float offsetY = 0.0F;
		offsetX = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "offsetX", 0.0F);
		offsetY = (float)jeLua_getOptionalNumberField(lua, renderableIndex, "offsetY", 0.0F);

		offsetX -= cameraX1 + floorf((cameraX2 - cameraX1) / 2.0F);
		offsetY -= cameraY1 + floorf((cameraY2 - cameraY1) / 2.0F);

		for (uint32_t i = 0; i < vertexCount; i++) {
			vertices[i].x += offsetX;
			vertices[i].y += offsetY;
		}
	}
}
void jeLua_drawPrimitiveImpl(lua_State* lua, uint32_t primitiveType) {
	struct jeWindow* window = jeLua_getWindow(lua);

	JE_TRACE("lua=%p, window=%p", (void*)lua, (void*)window);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	if (jeWindow_getIsValid(window) == false) {
		// JE_ERROR("window is not valid");
		ok = false;
	}

	if (jePrimitiveType_getValid(primitiveType) == false) {
		JE_ERROR("not a valid primitiveType, primitiveType=%u", primitiveType);
		ok = false;
	}

	struct jeVertex vertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	memset(&vertices, 0, sizeof(vertices));

	uint32_t vertexCount = jePrimitiveType_getVertexCount(primitiveType);

	if (ok) {
		jeLua_getPrimitiveImpl(lua, vertices, vertexCount);

		JE_TRACE(
			"lua=%p, primitiveType=%u, vertexCount=%u, vertices={%s}",
			(void*)lua,
			primitiveType,
			vertexCount,
			jeVertex_arrayGetDebugString(vertices, vertexCount));

		jeWindow_pushPrimitive(window, vertices, primitiveType);
	}
}
int jeLua_drawPoint(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);
	jeLua_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_POINTS);
	return 0;
}
int jeLua_drawLine(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);
	jeLua_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_LINES);
	return 0;
}
int jeLua_drawTriangle(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);
	jeLua_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_TRIANGLES);
	return 0;
}
int jeLua_drawSprite(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);
	jeLua_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_SPRITES);
	return 0;
}
int jeLua_drawText(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	if (ok) {
		static const int renderableIndex = 1;
		static const int defaultsIndex = 2;

		luaL_checktype(lua, defaultsIndex, LUA_TTABLE);
		luaL_checktype(lua, renderableIndex, LUA_TTABLE);

		uint32_t charW = (uint32_t)jeLua_getNumberField(lua, defaultsIndex, "charW");
		uint32_t charH = (uint32_t)jeLua_getNumberField(lua, defaultsIndex, "charH");

		const char* charFirst = jeLua_getStringField(lua, defaultsIndex, "charFirst", NULL);
		const char* charLast = jeLua_getStringField(lua, defaultsIndex, "charLast", NULL);
		uint32_t charColumns = (uint32_t)jeLua_getNumberField(lua, defaultsIndex, "charColumns");

		uint32_t textLength = 0;
		const char* text = jeLua_getStringField(lua, renderableIndex, "text", &textLength);

		struct jeVertex textBoundsVertices[2];
		memset(&textBoundsVertices, 0, sizeof(textBoundsVertices));

		jeLua_getPrimitiveImpl(lua, textBoundsVertices, 1);
		textBoundsVertices[0].z = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "textZ", textBoundsVertices[0].z);
		textBoundsVertices[0].r = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "textR", textBoundsVertices[0].r);
		textBoundsVertices[0].g = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "textG", textBoundsVertices[0].g);
		textBoundsVertices[0].b = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "textB", textBoundsVertices[0].b);
		textBoundsVertices[0].a = (float)jeLua_getOptionalNumberField(lua, defaultsIndex, "textA", textBoundsVertices[0].a);
		textBoundsVertices[1] = textBoundsVertices[0];

		JE_TRACE(
			"lua=%p, text=%s, textBoundsVertices=%s",
			(void*)lua,
			text,
			jeVertex_arrayGetDebugString(textBoundsVertices, 2));
		for (uint32_t i = 0; i < textLength; i++) {
			static const char charDefault = ' ';

			char charVal = (char)toupper((uint32_t)text[i]);
			if ((charVal < charFirst[0]) || (charVal > charLast[0])) {
				JE_WARN(
					"character outside range, char=%u, min=%u, max=%u",
					(uint32_t)charVal,
					(uint32_t)charFirst[0],
					(uint32_t)charLast[0]);
				charVal = charDefault;
			}

			uint32_t charIndex = (uint32_t)(charVal - charFirst[0]);

			static const uint32_t renderScaleX = 1;
			static const uint32_t renderScaleY = 1;

			struct jeVertex charVertices[2];
			memcpy(charVertices, textBoundsVertices, sizeof(charVertices));
			charVertices[0].x += (float)(charW * renderScaleX * i);
			charVertices[0].u += (float)(charW * (charIndex % charColumns));
			charVertices[0].v += (float)(charH * (charIndex / charColumns));

			charVertices[1] = charVertices[0];
			charVertices[1].x += (float)(charW * renderScaleX);
			charVertices[1].y += (float)(charH * renderScaleY);
			charVertices[1].u += (float)charW;
			charVertices[1].v += (float)charH;

			JE_TRACE("lua=%p, charVertices={%s}", (void*)lua, jeVertex_arrayGetDebugString(charVertices, 2));
			jeWindow_pushPrimitive(jeLua_getWindow(lua), charVertices, JE_PRIMITIVE_TYPE_SPRITES);
		}
	}

	return 0;
}
int jeLua_drawReset(lua_State* lua) {
	struct jeWindow* window = jeLua_getWindow(lua);
	JE_TRACE("lua=%p, window=%p", (void*)lua, (void*)window);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	if (jeWindow_getIsValid(window) == false) {
		JE_ERROR("window is not valid");
		ok = false;
	}

	if (ok) {
		jeWindow_resetPrimitives(window);
	}

	return 0;
}
int jeLua_playSound(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	if (jeAudioMixer_instance == NULL) {
		JE_ERROR("jeAudioMixer_instance=NULL");
		ok = false;
	}

	uint32_t index = 0;
	if (ok) {
		index = (uint32_t)luaL_checknumber(lua, 1);

		if (index > jeAudio_numPrebaked) {
			JE_ERROR("index > jeAudio_numPrebaked, index=%u", index);
			ok = false;
		}
	}

	if (jeAudio_prebaked[index] == NULL) {
		JE_ERROR("jeAudio_prebaked[index]=NULL, index=%u", index);
		ok = false;
	}

	if (ok) {
		jeAudioMixer_playSound(jeAudioMixer_instance, jeAudio_prebaked[index]);
	}

	return 0;
}
int jeLua_runTests(lua_State* lua) {
	uint32_t numTestSuites = 0;

#if JE_DEBUGGING
	JE_TRACE("lua=%p", (void*)lua);

	uint32_t logLevelbackup = jeLogger_getLevel();
	jeLogger_setLevelOverride(JE_TESTS_LOG_LEVEL);

	jeContainer_runTests();
	numTestSuites++;

	jeImage_runTests();
	numTestSuites++;

	jeRendering_runTests();
	numTestSuites++;

	jeAudio_runTests();
	numTestSuites++;

	jeWindow_runTests();
	numTestSuites++;

	jeLogger_setLevelOverride(logLevelbackup);
#endif

	lua_pushnumber(lua, numTestSuites);
	return 1;
}
int jeLua_step(lua_State* lua) {
	// BEGIN LD48 TEMP CODE; TODO CLEANUP/REMOVE
	// This is garbage that doesn't clean up after itself.  Gamejam time limits demand it!
	{
		if (jeAudioMixer_instance == NULL) {
			jeAudioMixer_instance = jeAudioMixer_create();
			struct jeAudioDevice* reference_device = jeAudioMixer_getMusicAudioDevice(jeAudioMixer_instance);

			struct jeAudio* music = jeAudio_createFromWavFile(reference_device, "apps/ld48/data/song1.wav");
			jeAudioMixer_loopMusic(jeAudioMixer_instance, music);

			jeAudio_prebaked[jeAudio_numPrebaked++] = jeAudio_createFromWavFile(reference_device, "apps/ld48/data/bump.wav");
			jeAudio_prebaked[jeAudio_numPrebaked++] = jeAudio_createFromWavFile(reference_device, "apps/ld48/data/jump.wav");
			jeAudio_prebaked[jeAudio_numPrebaked++] = jeAudio_createFromWavFile(reference_device, "apps/ld48/data/death.wav");
		}
		jeAudioMixer_step(jeAudioMixer_instance);

	}
	// END LD48 TEMP CODE; TODO CLEANUP/REMOVE

	struct jeWindow* window = jeLua_getWindow(lua);

	JE_TRACE("lua=%p, window=%p", (void*)lua, (void*)window);

	bool ok = true;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	bool performStep = true;
	if (jeWindow_getIsValid(window) == false) {
		JE_TRACE("window is not valid, skipping step");
		performStep = false;
	}

	if (performStep) {
		ok = ok && jeWindow_step(window);
	}

	if (lua != NULL) {
		jeLua_updateStates(lua);

		lua_pushboolean(lua, ok);
	}

	return 1;
}

bool jeLua_addBindings(lua_State* lua) {
	JE_TRACE("lua=%p", (void*)lua);

	static const luaL_Reg clientBindings[] = {
		JE_LUA_CLIENT_BINDING(readData),
		JE_LUA_CLIENT_BINDING(writeData),
		JE_LUA_CLIENT_BINDING(drawPoint),
		JE_LUA_CLIENT_BINDING(drawLine),
		JE_LUA_CLIENT_BINDING(drawTriangle),
		JE_LUA_CLIENT_BINDING(drawSprite),
		JE_LUA_CLIENT_BINDING(drawText),
		JE_LUA_CLIENT_BINDING(drawReset),
		JE_LUA_CLIENT_BINDING(playSound),
		JE_LUA_CLIENT_BINDING(runTests),
		JE_LUA_CLIENT_BINDING(step),
		{NULL, NULL} /*sentinel value*/
	};

	bool ok = true;
	int createdResult = 0;

	if (lua == NULL) {
		JE_ERROR("lua=NULL");
		ok = false;
	}

	JE_DEBUG(" ");

	if (ok) {
		createdResult = luaL_newmetatable(lua, "jeClientMetatable");
		if (createdResult != 1) {
			JE_ERROR(
				"luaL_newmetatable() failed, result=%d, metatableName=%s", createdResult, JE_LUA_CLIENT_BINDINGS_KEY);
			ok = false;
		}
	}

	if (ok) {
		luaL_setfuncs(lua, clientBindings, /*numUpvalues*/ 0);
		lua_pushvalue(lua, JE_LUA_STACK_TOP);
		lua_setfield(lua, JE_LUA_STACK_TOP, "__index");

		lua_pushvalue(lua, JE_LUA_STACK_TOP);
		lua_setglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);

		lua_createtable(lua, /*numArrayElems*/ 0, /*numNonArrayElems*/ 16);
		lua_setfield(lua, JE_LUA_STACK_TOP - 1, "state");

		jeLua_updateStates(lua);

		lua_settop(lua, 0);
	}

	return ok;
}
bool jeLua_run(struct jeWindow* window, const char* filename, int argumentCount, char** arguments) {
	bool ok = true;
	int luaResponse = 0;
	lua_State* lua = NULL;

	if (jeWindow_getIsValid(window) == false) {
		JE_ERROR("window is not valid");
		ok = false;
	}

	if (filename == NULL) {
		JE_ERROR("filename=NULL");
		ok = false;
	}

	if ((arguments == NULL) && (argumentCount > 0)) {
		JE_ERROR("arguments=NULL when argumentCount > 0");
		ok = false;
	}

	JE_DEBUG("window=%p, filename=%s", (void*)window, filename);

	if (ok) {
		lua = luaL_newstate();
		if (lua == NULL) {
			JE_ERROR("luaL_newstate() failed");
			ok = false;
		}
	}

	if (ok) {
		luaL_openlibs(lua);

		ok = jeLua_addWindow(lua, window);
	}

	ok = ok && jeLua_addBindings(lua);

	if (ok) {
		luaResponse = luaL_loadfile(lua, filename);
		if (luaResponse != 0) {
			JE_ERROR(
				"luaL_loadfile() failed, filename=%s luaResponse=%d error=%s",
				filename,
				luaResponse,
				jeLua_getError(lua));
			ok = false;
		}
	}

	if (ok) {
		/*push string arguments to the lua stack*/
		for (int i = 0; i < argumentCount; i++) {
			JE_TRACE("arg[%d] = \"%s\"", i, arguments[i]);
			lua_pushstring(lua, arguments[i]);
		}
		JE_TRACE("%d command-line arguments", argumentCount);
	}

	if (ok) {
		/*THIS WILL BLOCK UNTIL THE CALLED LUA SCRIPT ENDS!*/
		luaResponse = lua_pcall(lua, /*numArgs*/ argumentCount, /*numReturnVals*/ LUA_MULTRET, /*errFunc*/ 0);
		if (luaResponse != LUA_OK) {
			JE_ERROR(
				"lua_pcall() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLua_getError(lua));
			ok = false;
		}
	}

	if (lua != NULL) {
		JE_DEBUG("closing, lua=%p, window=%p, filename=%s", (void*)lua, (void*)window, filename);
		lua_close(lua);
	}

	return ok;
}

bool jeClient_run(int argumentCount, char** arguments) {
	bool ok = true;

	struct jeClient client;
	memset((void*)&client, 0, sizeof(struct jeClient));

	if ((arguments == NULL) && (argumentCount > 0)) {
		JE_ERROR("arguments=NULL when argumentCount > 0");
		ok = false;
	}

	const char* appDir = JE_DEFAULT_APP_DIR;
	const uint32_t maxArgLen = 32;
	if (ok) {
		for (int i = 0; i < argumentCount; i++) {
			if (strncmp(arguments[i], "--appdir", maxArgLen) == 0) {
				ok = ok && ((i + 1) < argumentCount);

				if (ok) {
					appDir = arguments[i + 1];
				}
			}
			if (strncmp(arguments[i], "--debug", maxArgLen) == 0) {
				jeLogger_setLevelOverride(JE_LOG_LEVEL_DEBUG);
			}
		}
	}

	JE_DEBUG("client=%p, appDir=%s", (void*)&client, appDir);

	struct jeString luaMainFilename = {0};

	ok = ok && jeString_create(&luaMainFilename);
	ok = ok && jeString_setFormatted(&luaMainFilename, "%s/main.lua", appDir);

	struct jeString spritesFilename = {0};
	ok = ok && jeString_create(&spritesFilename);
	ok = ok && jeString_setFormatted(&spritesFilename, "%s/data/sprites.png", appDir);

	if (ok) {
		client.window = jeWindow_create(/*startVisible*/ true, jeString_get(&spritesFilename, 0));
	}

	if (ok) {
		if (jeWindow_getIsValid(client.window) == false) {
			JE_ERROR("window is not valid");
			ok = false;
		}
	}

	ok = ok && jeLua_run(client.window, jeString_get(&luaMainFilename, 0), argumentCount, arguments);

	jeWindow_destroy(client.window);

	jeString_destroy(&spritesFilename);
	jeString_destroy(&luaMainFilename);

	return ok;
}
