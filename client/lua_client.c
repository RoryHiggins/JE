#include "stdafx.h"
#include "lua_client.h"
#include "debug.h"
#include "image.h"
#include "container.h"
#include "rendering.h"
#include "window.h"

/*https://www.lua.org/manual/5.1/manual.html*/
#define JE_LUA_STACK_TOP -1
#define JE_LUA_DATA_BUFFER_SIZE 8 * 1024 * 1024

#define JE_LUA_CLIENT_BINDINGS_KEY "jeLuaClientBindings"
#define JE_LUA_CLIENT_WINDOW_KEY "jeLuaClientWindow"
#define JE_LUA_CLIENT_BINDING(BINDING_NAME) {#BINDING_NAME, jeLuaClient_##BINDING_NAME}

#if JE_LOG_LEVEL > JE_LOG_LEVEL_DEBUG
#define JE_TESTS_LOG_LEVEL JE_LOG_LEVEL_WARN
#else
#define JE_TESTS_LOG_LEVEL JE_LOG_LEVEL
#endif

#if (LUA_VERSION_NUM < 520) && !(defined(LUAJIT_VERSION_NUM) && (LUAJIT_VERSION_NUM >= 20100))
/*Shim adapted from https://github.com/keplerproject/lua-compat-5.2/blob/master/c-api/compat-5.2.c#L119*/
void luaL_setfuncs(lua_State *lua, const luaL_Reg *functionsIter, int functionsCount) {
	luaL_checkstack(lua, functionsCount+1, "too many upvalues");

	for (; functionsIter->name != NULL; functionsIter++) {  /* fill the table with given functions */
		lua_pushstring(lua, functionsIter->name);

		/* copy upvalues to the top */
		for (int i = 0; i < functionsCount; i++) {
			lua_pushvalue(lua, -(functionsCount + 1));
		}

		lua_pushcclosure(lua, functionsIter->func, functionsCount);  /* closure with those upvalues */
		lua_settable(lua, -(functionsCount + 3)); /* table must be below the upvalues, the name and the closure */
	}
	lua_pop(lua, functionsCount);  /* remove upvalues */
}
#endif

#if LUA_VERSION_NUM >= 520
size_t lua_objlen(lua_State *lua, int i) {
	return lua_rawlen(lua, i);
}
#endif

const char* jeLuaClient_getError(lua_State* lua) {
	const char* error = lua_tostring(lua, JE_LUA_STACK_TOP);

	if (error == NULL) {
		error = "";
	}

	return error;
}
float jeLuaClient_getNumberField(lua_State* lua, int tableIndex, const char* field) {
	lua_getfield(lua, tableIndex, field);

	return luaL_checknumber(lua, JE_LUA_STACK_TOP);
}
float jeLuaClient_getOptionalNumberField(lua_State* lua, int tableIndex, const char* field, float defaultValue) {
	lua_getfield(lua, tableIndex, field);

	return luaL_optnumber(lua, JE_LUA_STACK_TOP, defaultValue);
}
const char* jeLuaClient_getStringField(lua_State* lua, int tableIndex, const char* field, int *optOutSize) {
	lua_getfield(lua, tableIndex, field);

	const char* result = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	if (optOutSize != NULL) {
		*optOutSize = lua_objlen(lua, JE_LUA_STACK_TOP);
	}

	return result;
}

jeWindow* jeLuaClient_getWindow(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	bool ok = true;
	jeWindow *window = NULL;

	int stackPos = lua_gettop(lua);

	if (ok) {
		lua_getglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

		if (lua_islightuserdata(lua, JE_LUA_STACK_TOP) == 0) {
			JE_ERROR("%s is not set", JE_LUA_CLIENT_WINDOW_KEY);
			ok = false;
		}
	}

	if (ok) {
		window = (jeWindow*)lua_touserdata(lua, JE_LUA_STACK_TOP);
		JE_TRACE("lua=%p, window=%p", lua, window);
	}

	lua_settop(lua, stackPos);

	return window;
}
void jeLuaClient_addWindow(lua_State* lua, jeWindow* window) {
	JE_TRACE("lua=%p, window=%p", lua, window);

	int stackPos = lua_gettop(lua);

	lua_pushlightuserdata(lua, (void*)window);
	lua_setglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

	lua_settop(lua, stackPos);
}
void jeLuaClient_updateStates(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	int stackPos = lua_gettop(lua);
	int stateStackPos = 0;
	jeWindow* window = jeLuaClient_getWindow(lua);

	lua_settop(lua, 0);
	lua_getglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);
	lua_getfield(lua, JE_LUA_STACK_TOP, "state");

	stateStackPos = lua_gettop(lua);

	lua_pushboolean(lua, jeWindow_getIsOpen(window));
	lua_setfield(lua, stateStackPos, "running");

	lua_pushnumber(lua, (lua_Number)JE_WINDOW_MIN_WIDTH);
	lua_setfield(lua, stateStackPos, "width");

	lua_pushnumber(lua, (lua_Number)JE_WINDOW_MIN_HEIGHT);
	lua_setfield(lua, stateStackPos, "height");

	lua_pushnumber(lua, (lua_Number)jeWindow_getFps(window));
	lua_setfield(lua, stateStackPos, "fps");

	lua_pushnumber(lua, (lua_Number)JE_LOG_LEVEL);
	lua_setfield(lua, stateStackPos, "logLevel");

	lua_pushboolean(lua, true);
	lua_setfield(lua, stateStackPos, "testsEnabled");

	lua_pushnumber(lua, (lua_Number)JE_TESTS_LOG_LEVEL);
	lua_setfield(lua, stateStackPos, "testsLogLevel");

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

	lua_settop(lua, stackPos);
}

/*Lua-client bindings.  Note: return value = num responses pushed to lua stack*/
int jeLuaClient_readData(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	bool ok = true;
	int numResponses = 0;

	gzFile file = NULL;
	const char* filename = "";
	if (ok) {
		filename = luaL_checkstring(lua, 1);

		JE_TRACE("lua=%p, filename=%s", lua, filename);

		file = gzopen(filename, "rb");

		if (file == NULL) {
			JE_ERROR("gzopen() failed with filename=%s, errno=%d err=%s",
				   filename, errno, strerror(errno));
			ok = false;
		}
	}

	static char data[JE_LUA_DATA_BUFFER_SIZE] = {0};
	memset((void*)&data, 0, sizeof(data));
	if (ok) {
		int dataSize = gzread(file, data, JE_LUA_DATA_BUFFER_SIZE);

		/*Remove null terminator if string exists*/
		if (dataSize > 0) {
			dataSize--;
		}

		lua_pushlstring(lua, data, dataSize);
		numResponses++;

		JE_DEBUG("fread() bytes=%d (after decompression) read from filename=%s", dataSize, filename);
	}

	if (file != NULL) {
		gzclose(file);
	}

	return numResponses;
}
int jeLuaClient_writeData(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	bool ok = true;
	int numResponses = 0;

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

		JE_TRACE("lua=%p, filename=%s, dataSize=%d", lua, filename, dataSize);

		file = gzopen(filename, "wb");

		if (file == NULL) {
			JE_ERROR("gzopen() failed with filename=%s, errno=%d err=%s",
				   filename, errno, strerror(errno));
			ok = false;
		}
	}

	if (ok) {
		int dataSizeWritten = gzwrite(file, data, dataSize);

		lua_pushboolean(lua, ok);
		numResponses++;

		JE_DEBUG("bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);

		JE_MAYBE_UNUSED(dataSizeWritten);
	}

	if (file != NULL) {
		gzclose(file);
	}

	return numResponses;
}
void jeLuaClient_getPrimitiveImpl(lua_State* lua, jeVertex *vertices, int vertexCount) {
	JE_TRACE("lua=%p, vertices=%p, vertexCount=%d", lua, vertices, vertexCount);

	static const int renderableIndex = 1;
	static const int defaultsIndex = 2;
	static const int cameraIndex = 3;

	luaL_checktype(lua, renderableIndex, LUA_TTABLE);
	luaL_checktype(lua, defaultsIndex, LUA_TTABLE);
	luaL_checktype(lua, cameraIndex, LUA_TTABLE);

	vertices[0].x = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "x", 0.0f);
	vertices[0].y = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "y", 0.0f);
	vertices[0].x = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "x1", vertices[0].x);
	vertices[0].y = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "y1", vertices[0].y);
	vertices[0].z = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "z1", 0.0f);
	vertices[0].u = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "u1", 0.0f);
	vertices[0].v = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "v1", 0.0f);

	vertices[0].r = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "r", 1.0f);
	vertices[0].g = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "g", 1.0f);
	vertices[0].b = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "b", 1.0f);
	vertices[0].a = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "a", 1.0f);
	vertices[0].r = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "r", vertices[0].r);
	vertices[0].g = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "g", vertices[0].g);
	vertices[0].b = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "b", vertices[0].b);
	vertices[0].a = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "a", vertices[0].a);

	if (vertexCount >= 2) {
		vertices[1].x = vertices[0].x + jeLuaClient_getOptionalNumberField(lua, renderableIndex, "w", 0.0f);
		vertices[1].y = vertices[0].y + jeLuaClient_getOptionalNumberField(lua, renderableIndex, "h", 0.0f);
		vertices[1].x = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "x2", vertices[1].x);
		vertices[1].y = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "y2", vertices[1].y);
		vertices[1].u = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "u2", 1.0f);
		vertices[1].v = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "v2", 1.0f);
	}

	if (vertexCount >= 3) {
		vertices[2].x = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "x3", 0.0f);
		vertices[2].y = jeLuaClient_getOptionalNumberField(lua, renderableIndex, "y3", 0.0f);
		vertices[2].u = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "u3", 1.0f);
		vertices[2].v = jeLuaClient_getOptionalNumberField(lua, defaultsIndex, "v3", 1.0f);
	}

	for (int i = 0; i < vertexCount; i++) {
		vertices[i].z = vertices[0].z;
		vertices[i].r = vertices[0].r;
		vertices[i].g = vertices[0].g;
		vertices[i].b = vertices[0].b;
		vertices[i].a = vertices[0].a;
	}

	float cameraX1 = jeLuaClient_getOptionalNumberField(lua, cameraIndex, "x", 0.0f);
	float cameraY1 = jeLuaClient_getOptionalNumberField(lua, cameraIndex, "y", 0.0f);
	float cameraX2 = cameraX1 + jeLuaClient_getOptionalNumberField(lua, cameraIndex, "w", 0.0f);
	float cameraY2 = cameraY1 + jeLuaClient_getOptionalNumberField(lua, cameraIndex, "h", 0.0f);
	cameraX1 = jeLuaClient_getOptionalNumberField(lua, cameraIndex, "x1", cameraX1);
	cameraY1 = jeLuaClient_getOptionalNumberField(lua, cameraIndex, "y1", cameraY1);
	cameraX2 = jeLuaClient_getOptionalNumberField(lua, cameraIndex, "x2", cameraX2);
	cameraY2 = jeLuaClient_getOptionalNumberField(lua, cameraIndex, "y2", cameraY2);

	float cameraOffsetX = -cameraX1 - (float)floor((cameraX2 - cameraX1) / 2.0f);
	float cameraOffsetY = -cameraY1 - (float)floor((cameraY2 - cameraY1) / 2.0f);
	for (int i = 0; i < vertexCount; i++) {
		vertices[i].x += cameraOffsetX;
		vertices[i].y += cameraOffsetY;
	}
}
void jeLuaClient_drawPrimitiveImpl(lua_State* lua, jePrimitiveType primitiveType) {
	jeWindow* window = jeLuaClient_getWindow(lua);

	JE_TRACE("lua=%p, window=%p", lua, window);

	jeVertex vertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	memset(&vertices, 0, sizeof(vertices));

	int vertexCount = jePrimitiveType_getVertexCount(primitiveType);
	jeLuaClient_getPrimitiveImpl(lua, vertices, vertexCount);

	JE_TRACE("lua=%p, primitiveType=%d, vertexCount=%d, vertices={%s}",
			 lua, primitiveType, vertexCount, jeVertex_arrayToDebugString(vertices, vertexCount));
	jeWindow_pushPrimitive(window, vertices, primitiveType);
}
int jeLuaClient_drawPoint(lua_State* lua) {
	JE_TRACE("lua=%p", lua);
	jeLuaClient_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_POINTS);
	return 0;
}
int jeLuaClient_drawLine(lua_State* lua) {
	JE_TRACE("lua=%p", lua);
	jeLuaClient_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_LINES);
	return 0;
}
int jeLuaClient_drawTriangle(lua_State* lua) {
	JE_TRACE("lua=%p", lua);
	jeLuaClient_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_TRIANGLES);
	return 0;
}
int jeLuaClient_drawSprite(lua_State* lua) {
	JE_TRACE("lua=%p", lua);
	jeLuaClient_drawPrimitiveImpl(lua, JE_PRIMITIVE_TYPE_SPRITES);
	return 0;
}
int jeLuaClient_drawText(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	static const int renderableIndex = 1;
	static const int defaultsIndex = 2;

	luaL_checktype(lua, defaultsIndex, LUA_TTABLE);
	luaL_checktype(lua, renderableIndex, LUA_TTABLE);

	int charW = jeLuaClient_getNumberField(lua, defaultsIndex, "charW");
	int charH = jeLuaClient_getNumberField(lua, defaultsIndex, "charH");

	const char* charFirst = jeLuaClient_getStringField(lua, defaultsIndex, "charFirst", NULL);
	const char* charLast = jeLuaClient_getStringField(lua, defaultsIndex, "charLast", NULL);
	int charColumns = jeLuaClient_getNumberField(lua, defaultsIndex, "charColumns");

	int textLength = 0;
	const char* text = jeLuaClient_getStringField(lua, renderableIndex, "text", &textLength);

	jeVertex textBoundsVertices[2];
	memset(&textBoundsVertices, 0, sizeof(textBoundsVertices));

	jeLuaClient_getPrimitiveImpl(lua, textBoundsVertices, 1);
	textBoundsVertices[1] = textBoundsVertices[0];

	JE_TRACE("lua=%p, text=%s, textBoundsVertices=%s", lua, text, jeVertex_arrayToDebugString(textBoundsVertices, 2));
	for (int i = 0; i < textLength; i++) {
		static const char charDefault = ' ';

		char charVal = (char)toupper((int)text[i]);
		if ((charVal < charFirst[0]) || (charVal > charLast[0])) {
			JE_WARN("character outside range, char=%d, min=%d, max=%d",
					(int)charVal, (int)charFirst[0], (int)charLast[0]);
			charVal = charDefault;
		}

		int charIndex = (int)(charVal - charFirst[0]);

		static const float renderScaleX = 1.0f;
		static const float renderScaleY = 1.0f;

		jeVertex charVertices[2];
		memcpy(charVertices, textBoundsVertices, sizeof(charVertices));
		charVertices[0].x += (charW * renderScaleX * i);
		charVertices[0].u += (charW * (charIndex % charColumns));
		charVertices[0].v += (charH * (charIndex / charColumns));

		charVertices[1] = charVertices[0];
		charVertices[1].x += charW * renderScaleX;
		charVertices[1].y += charH * renderScaleY;
		charVertices[1].u += charW;
		charVertices[1].v += charH;

		JE_TRACE("lua=%p, charVertices={%s}", lua, jeVertex_arrayToDebugString(charVertices, 2));
		jeWindow_pushPrimitive(jeLuaClient_getWindow(lua), charVertices, JE_PRIMITIVE_TYPE_SPRITES);
	}

	return 0;
}
int jeLuaClient_drawReset(lua_State* lua) {
	jeWindow* window = jeLuaClient_getWindow(lua);
	JE_TRACE("lua=%p, window=%p", lua, window);

	jeWindow_resetPrimitives(window);

	return 0;
}
int jeLuaClient_runTests(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	jeLoggerLevel logLevelbackup = jeLoggerLevel_override;
	jeLoggerLevel_override = JE_TESTS_LOG_LEVEL;

	int numTestSuites = 0;
	jeContainerRunTests();
	numTestSuites++;

	jeImageRunTests();
	numTestSuites++;

	jeRenderingRunTests();
	numTestSuites++;

	jeWindowRunTests();
	numTestSuites++;

	jeLoggerLevel_override = logLevelbackup;
	lua_pushnumber(lua, numTestSuites);
	return 1;
}
int jeLuaClient_step(lua_State* lua) {
	jeWindow* window = jeLuaClient_getWindow(lua);

	JE_TRACE("lua=%p, window=%p", lua, window);

	bool ok = jeWindow_step(window);

	jeLuaClient_updateStates(lua);

	lua_pushboolean(lua, ok);
	return 1;
}

bool jeLuaClient_addBindings(lua_State* lua) {
	JE_TRACE("lua=%p", lua);

	static const luaL_Reg clientBindings[] = {
		JE_LUA_CLIENT_BINDING(readData),
		JE_LUA_CLIENT_BINDING(writeData),
		JE_LUA_CLIENT_BINDING(drawPoint),
		JE_LUA_CLIENT_BINDING(drawLine),
		JE_LUA_CLIENT_BINDING(drawTriangle),
		JE_LUA_CLIENT_BINDING(drawSprite),
		JE_LUA_CLIENT_BINDING(drawText),
		JE_LUA_CLIENT_BINDING(drawReset),
		JE_LUA_CLIENT_BINDING(runTests),
		JE_LUA_CLIENT_BINDING(step),
		{NULL, NULL}  /*sentinel value*/
	};

	bool ok = true;
	int createdResult = 0;

	JE_DEBUG("");

	if (ok) {
		createdResult = luaL_newmetatable(lua, "jeClientMetatable");
		if (createdResult != 1) {
			JE_ERROR("luaL_newmetatable() failed, result=%d, metatableName=%s", createdResult, JE_LUA_CLIENT_BINDINGS_KEY);
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

		jeLuaClient_updateStates(lua);

		lua_settop(lua, 0);
	}

	return ok;
}
bool jeLuaClient_run(jeWindow* window, const char* filename) {
	bool ok = true;
	int luaResponse = 0;
	lua_State* lua = NULL;

	JE_DEBUG("window=%p, filename=%s", window, filename);

	if (ok) {
		lua = luaL_newstate();
		if (lua == NULL) {
			JE_ERROR("luaL_newstate() failed");
			ok = false;
		}
	}

	if (ok) {
		luaL_openlibs(lua);

		jeLuaClient_addWindow(lua, window);
	}

	ok = ok && jeLuaClient_addBindings(lua);

	if (ok) {
		luaResponse = luaL_loadfile(lua, filename);
		if (luaResponse != 0) {
			JE_ERROR("luaL_loadfile() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLuaClient_getError(lua));
			ok = false;
		}
	}

	if (ok) {
		luaResponse = lua_pcall(lua, /*numArgs*/ 0, /*numReturnVals*/ LUA_MULTRET, /*errFunc*/ 0);
		if (luaResponse != 0) {
			JE_ERROR("lua_pcall() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLuaClient_getError(lua));
			ok = false;
		}
	}

	if (lua != NULL) {
		JE_DEBUG("closing, lua=%p, window=%p, filename=%s", lua, window, filename);
		lua_close(lua);
	}

	return ok;
}
