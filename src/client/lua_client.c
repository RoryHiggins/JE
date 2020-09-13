#include "stdafx.h"
#include "lua_client.h"
#include "debug.h"
#include "window.h"

/*https://www.lua.org/manual/5.1/manual.html*/
#define JE_LUA_STACK_TOP -1
#define JE_LUA_DATA_BUFFER_SIZE 8 * 1024 * 1024

#define JE_LUA_CLIENT_BINDINGS_KEY "jeLuaClientBindings"
#define JE_LUA_CLIENT_WINDOW_KEY "jeLuaClientWindow"
#define JE_LUA_CLIENT_BINDING(BINDING_NAME) {#BINDING_NAME, jeLuaClient_##BINDING_NAME}


#if LUA_VERSION_NUM < 520
/*Shim adapted from https://github.com/keplerproject/lua-compat-5.2/blob/master/c-api/compat-5.2.c#L119*/
void luaL_setfuncs(lua_State *lua, const luaL_Reg *functionsIter, int functionsCount) {
	int i = 0;

	luaL_checkstack(lua, functionsCount+1, "too many upvalues");

	for (; functionsIter->name != NULL; functionsIter++) {  /* fill the table with given functions */
		lua_pushstring(lua, functionsIter->name);

		/* copy upvalues to the top */
		for (i = 0; i < functionsCount; i++) {
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
	const char* error = "";

	error = lua_tostring(lua, JE_LUA_STACK_TOP);

	if (error == NULL) {
		error = "";
	}

	return error;
}
jeWindow* jeLuaClient_getWindow(lua_State* lua) {
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
	}

	lua_settop(lua, stackPos);

	return window;
}
void jeLuaClient_addWindow(lua_State* lua, jeWindow* window) {
	int stackPos = lua_gettop(lua);

	lua_pushlightuserdata(lua, (void*)window);
	lua_setglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

	lua_settop(lua, stackPos);
}
void jeLuaClient_updateStates(lua_State* lua) {
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

	lua_pushnumber(lua, (lua_Number)JE_LOG_LEVEL_WARN);
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
	static char data[JE_LUA_DATA_BUFFER_SIZE] = {0};

	bool ok = true;

	int numResponses = 0;
	const char* filename = "";
	int dataSize = 0;
	gzFile file = NULL;

	memset((void*)&data, 0, sizeof(data));

	if (ok) {
		filename = luaL_checkstring(lua, 1);

		file = gzopen(filename, "rb");

		if (file == NULL) {
			JE_ERROR("gzopen() failed with filename=%s, errno=%d err=%s",
				   filename, errno, strerror(errno));
			ok = false;
		}
	}

	if (ok) {
		dataSize = gzread(file, data, JE_LUA_DATA_BUFFER_SIZE);

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
	static const int filenameArg = 1;
	static const int dataArg = 2;

	bool ok = true;

	int numResponses = 0;
	const char* filename = "";
	const char* data = "";
	size_t dataSize = 0;
	int dataSizeWritten = 0;
	gzFile file = NULL;

	if (ok) {
		filename = luaL_checkstring(lua, filenameArg);
		data = luaL_checkstring(lua, dataArg);
		dataSize = lua_objlen(lua, dataArg) + 1;

		file = gzopen(filename, "wb");

		if (file == NULL) {
			JE_ERROR("gzopen() failed with filename=%s, errno=%d err=%s",
				   filename, errno, strerror(errno));
			ok = false;
		}
	}

	if (ok) {
		dataSizeWritten = gzwrite(file, data, dataSize);

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
int jeLuaClient_drawRenderableImpl(lua_State* lua, jePrimitiveType primitiveType) {
	static const int cameraIndex = 3;
	static const int defaultsIndex = 2;
	static const int renderableIndex = 1;

	jeRenderable renderable;

	/*camera*/
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float cameraW = 0.0f;
	float cameraH = 0.0f;

	float cameraOffsetX = 0.0f;
	float cameraOffsetY = 0.0f;

	/*primitive*/
	float renderScaleX = 1.0f;
	float renderScaleY = 1.0f;

	int i = 0;

	memset(&renderable, 0, sizeof(renderable));

	luaL_checktype(lua, cameraIndex, LUA_TTABLE);
	luaL_checktype(lua, defaultsIndex, LUA_TTABLE);
	luaL_checktype(lua, renderableIndex, LUA_TTABLE);

	renderable.primitiveType = primitiveType;

	/*camera*/
	lua_getfield(lua, cameraIndex, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "y1");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "x2");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraX;

		lua_getfield(lua, cameraIndex, "y2");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraY;
	} else {
		lua_getfield(lua, cameraIndex, "x");
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "y");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "w");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "h");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	cameraOffsetX = -cameraX - (float)floor(cameraW / 2.0f);
	cameraOffsetY = -cameraY - (float)floor(cameraH / 2.0f);

	/*defaults*/
	lua_getfield(lua, defaultsIndex, "r");
	renderable.vertex[0].r = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, defaultsIndex, "g");
	renderable.vertex[0].g = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, defaultsIndex, "b");
	renderable.vertex[0].b = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, defaultsIndex, "a");
	renderable.vertex[0].a = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	lua_getfield(lua, defaultsIndex, "u1");
	renderable.vertex[0].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "v1");
	renderable.vertex[0].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "u2");
	renderable.vertex[1].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "v2");
	renderable.vertex[1].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "u3");
	renderable.vertex[2].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "v3");
	renderable.vertex[2].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);

	/*render object*/
	lua_getfield(lua, renderableIndex, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		renderable.vertex[0].x = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "y1");
		renderable.vertex[0].y = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "x2");
		renderable.vertex[1].x = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "y2");
		renderable.vertex[1].y = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "x3");
		renderable.vertex[2].x = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);

		lua_getfield(lua, renderableIndex, "y3");
		renderable.vertex[2].y = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	} else {
		lua_getfield(lua, renderableIndex, "x");
		renderable.vertex[0].x = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "y");
		renderable.vertex[0].y = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "w");
		renderable.vertex[1].x = renderable.vertex[0].x + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "h");
		renderable.vertex[1].y = renderable.vertex[0].y + luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	lua_getfield(lua, renderableIndex, "z");
	renderable.z = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	for (i = 1; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].z = renderable.vertex[0].z;
	}

	lua_getfield(lua, renderableIndex, "renderScaleX");
	renderScaleX = renderScaleX * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableIndex, "renderScaleY");
	renderScaleY = renderScaleY * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	for (i = 1; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].x = renderable.vertex[0].x + ((renderable.vertex[i].x - renderable.vertex[0].x) * renderScaleX);
		renderable.vertex[i].y = renderable.vertex[0].y + ((renderable.vertex[i].y - renderable.vertex[0].y) * renderScaleY);
	}

	lua_getfield(lua, renderableIndex, "r");
	renderable.vertex[0].r = renderable.vertex[0].r * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].r);
	lua_getfield(lua, renderableIndex, "g");
	renderable.vertex[0].g = renderable.vertex[0].g * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].g);
	lua_getfield(lua, renderableIndex, "b");
	renderable.vertex[0].b = renderable.vertex[0].b * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].b);
	lua_getfield(lua, renderableIndex, "a");
	renderable.vertex[0].a = renderable.vertex[0].a * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].a);
	for (i = 1; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].r = renderable.vertex[0].r;
		renderable.vertex[i].g = renderable.vertex[0].g;
		renderable.vertex[i].b = renderable.vertex[0].b;
		renderable.vertex[i].a = renderable.vertex[0].a;
	}

	lua_getfield(lua, defaultsIndex, "u1");
	renderable.vertex[0].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].u);
	lua_getfield(lua, defaultsIndex, "v1");
	renderable.vertex[0].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].v);
	lua_getfield(lua, defaultsIndex, "u2");
	renderable.vertex[1].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[1].u);
	lua_getfield(lua, defaultsIndex, "v2");
	renderable.vertex[1].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[1].v);
	lua_getfield(lua, defaultsIndex, "u3");
	renderable.vertex[2].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[2].u);
	lua_getfield(lua, defaultsIndex, "v3");
	renderable.vertex[2].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[2].v);

	JE_TRACE("drawing renderable, renderable={%s}", jeRenderable_toDebugString(&renderable));
	jeWindow_drawRenderable(jeLuaClient_getWindow(lua), &renderable);

	return 0;
}
int jeLuaClient_drawTextImpl(lua_State* lua) {
	static const int cameraIndex = 3;
	static const int defaultsIndex = 2;
	static const int renderableIndex = 1;

	static const char charDefault = ' ';

	jeRenderable renderable;
	jeRenderable charRenderable;

	/*camera*/
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float cameraW = 0.0f;
	float cameraH = 0.0f;

	float cameraOffsetX = 0.0f;
	float cameraOffsetY = 0.0f;

	/*font*/
	int charW = 0;
	int charH = 0;
	int charColumns = 0;
	const char* charFirst = "\0";
	const char* charLast = "\0";

	/*char*/
	int i = 0;
	char charVal = charDefault;
	int charIndex = 0;

	float charScaleX = 1.0f;
	float charScaleY = 1.0f;

	const char* text = "";
	int textLength = 0;

	memset(&renderable, 0, sizeof(renderable));
	memset(&charRenderable, 0, sizeof(charRenderable));

	luaL_checktype(lua, cameraIndex, LUA_TTABLE);
	luaL_checktype(lua, defaultsIndex, LUA_TTABLE);
	luaL_checktype(lua, renderableIndex, LUA_TTABLE);

	renderable.primitiveType = JE_PRIMITIVE_TYPE_SPRITES;

	/*camera*/
	lua_getfield(lua, cameraIndex, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "y1");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "x2");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraX;

		lua_getfield(lua, cameraIndex, "y2");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraY;
	} else {
		lua_getfield(lua, cameraIndex, "x");
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "y");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "w");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraIndex, "h");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	cameraOffsetX = -cameraX - (float)floor(cameraW / 2.0f);
	cameraOffsetY = -cameraY - (float)floor(cameraH / 2.0f);

	/*font*/
	lua_getfield(lua, defaultsIndex, "r");
	renderable.vertex[0].r = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "g");
	renderable.vertex[0].g = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "b");
	renderable.vertex[0].b = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "a");
	renderable.vertex[0].a = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	lua_getfield(lua, defaultsIndex, "u1");
	renderable.vertex[0].u = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsIndex, "v1");
	renderable.vertex[0].v = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	for (i = 1; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].u = renderable.vertex[0].u;
		renderable.vertex[i].v = renderable.vertex[0].v;
	}

	lua_getfield(lua, defaultsIndex, "charW");
	charW = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, defaultsIndex, "charH");
	charH = luaL_checknumber(lua, JE_LUA_STACK_TOP);

	lua_getfield(lua, defaultsIndex, "charFirst");
	charFirst = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, defaultsIndex, "charLast");
	charLast = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, defaultsIndex, "charColumns");
	charColumns = luaL_checknumber(lua, JE_LUA_STACK_TOP);

	/*render object*/
	lua_getfield(lua, renderableIndex, "renderScaleX");
	charScaleX = charScaleX * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableIndex, "renderScaleY");
	charScaleY = charScaleY * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	lua_getfield(lua, renderableIndex, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		renderable.vertex[0].x = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "y1");
		renderable.vertex[0].y = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);
	} else {
		lua_getfield(lua, renderableIndex, "x");
		renderable.vertex[0].x = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableIndex, "y");
		renderable.vertex[0].y = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	for (i = 1; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].x = renderable.vertex[0].x;
		renderable.vertex[i].y = renderable.vertex[0].y;
	}

	lua_getfield(lua, renderableIndex, "z");
	renderable.z = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	for (i = 0; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].z = renderable.z;
	}

	lua_getfield(lua, renderableIndex, "text");
	text = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	textLength = lua_objlen(lua, JE_LUA_STACK_TOP);

	lua_getfield(lua, renderableIndex, "r");
	renderable.vertex[0].r = renderable.vertex[0].r * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].r);
	lua_getfield(lua, renderableIndex, "g");
	renderable.vertex[0].g = renderable.vertex[0].g * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].g);
	lua_getfield(lua, renderableIndex, "b");
	renderable.vertex[0].b = renderable.vertex[0].b * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].b);
	lua_getfield(lua, renderableIndex, "a");
	renderable.vertex[0].a = renderable.vertex[0].a * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.vertex[0].a);
	for (i = 1; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		renderable.vertex[i].r = renderable.vertex[0].r;
		renderable.vertex[i].g = renderable.vertex[0].g;
		renderable.vertex[i].b = renderable.vertex[0].b;
		renderable.vertex[i].a = renderable.vertex[0].a;
	}

	JE_TRACE("drawing text, text=%s, %s", text, jeRenderable_toDebugString(&renderable));
	for (i = 0; i < textLength; i++) {
		charVal = (char)toupper((int)text[i]);
		if ((charVal < charFirst[0]) || (charVal > charLast[0])) {
			JE_WARN("character outside range, char=%d, min=%d, max=%d",
					(int)charVal, (int)charFirst[0], (int)charLast[0]);
			charVal = charDefault;
		}

		charIndex = (int)(charVal - charFirst[0]);

		charRenderable = renderable;
		charRenderable.vertex[0].x += (charW * charScaleX * i);
		charRenderable.vertex[1].x = charRenderable.vertex[0].x + (charW * charScaleX);
		charRenderable.vertex[1].y = charRenderable.vertex[0].y + (charH * charScaleY);

		charRenderable.vertex[0].u += (charW * (charIndex % charColumns));
		charRenderable.vertex[0].v += (charH * (charIndex / charColumns));
		charRenderable.vertex[1].u = charRenderable.vertex[0].u + charW;
		charRenderable.vertex[1].v = charRenderable.vertex[0].v + charH;

		JE_TRACE("drawing char, charRenderable={%s}", jeRenderable_toDebugString(&charRenderable));
		jeWindow_drawRenderable(jeLuaClient_getWindow(lua), &charRenderable);
	}

	return 0;
}
int jeLuaClient_drawPoint(lua_State* lua) {
	return jeLuaClient_drawRenderableImpl(lua, JE_PRIMITIVE_TYPE_POINTS);
}
int jeLuaClient_drawLine(lua_State* lua) {
	return jeLuaClient_drawRenderableImpl(lua, JE_PRIMITIVE_TYPE_LINES);
}
int jeLuaClient_drawTriangle(lua_State* lua) {
	return jeLuaClient_drawRenderableImpl(lua, JE_PRIMITIVE_TYPE_TRIANGLES);
}
int jeLuaClient_drawSprite(lua_State* lua) {
	return jeLuaClient_drawRenderableImpl(lua, JE_PRIMITIVE_TYPE_SPRITES);
}
int jeLuaClient_drawText(lua_State* lua) {
	return jeLuaClient_drawTextImpl(lua);
}
int jeLuaClient_step(lua_State* lua) {
	jeWindow* window = jeLuaClient_getWindow(lua);

	jeWindow_step(window);

	jeLuaClient_updateStates(lua);

	return 0;
}

bool jeLuaClient_addBindings(lua_State* lua) {
	static const luaL_Reg clientBindings[] = {
		JE_LUA_CLIENT_BINDING(readData),
		JE_LUA_CLIENT_BINDING(writeData),
		JE_LUA_CLIENT_BINDING(drawPoint),
		JE_LUA_CLIENT_BINDING(drawLine),
		JE_LUA_CLIENT_BINDING(drawTriangle),
		JE_LUA_CLIENT_BINDING(drawSprite),
		JE_LUA_CLIENT_BINDING(drawText),
		JE_LUA_CLIENT_BINDING(drawLine),
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

	JE_DEBUG("");

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
		lua_close(lua);
	}

	return ok;
}
