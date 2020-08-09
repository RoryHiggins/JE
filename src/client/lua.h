#pragma once
#include "stdafx.h"
#include "logging.h"
#include "rendering.h"
#include "window.h"

// https://www.lua.org/manual/5.3/manual.html
#define JE_LUA_DATA_BUFFER_SIZE 8 * 1024 * 1024
#define JE_LUA_CLIENT_BINDINGS_KEY "jeClientBindings"
#define JE_LUA_CLIENT_BINDING(BINDING_NAME) {#BINDING_NAME, jeLuaClient_##BINDING_NAME}

// Adapted from https://github.com/keplerproject/lua-compat-5.2/blob/master/c-api/compat-5.2.c#L119
#if LUA_VERSION_NUM < 520
void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
	luaL_checkstack(L, nup+1, "too many upvalues");

	for (; l->name != NULL; l++) {  /* fill the table with given functions */
		int i;
		lua_pushstring(L, l->name);

		/* copy upvalues to the top */
		for (i = 0; i < nup; i++) {
			lua_pushvalue(L, -(nup + 1));
		}

		lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
		lua_settable(L, -(nup + 3)); /* table must be below the upvalues, the name and the closure */
	}
	lua_pop(L, nup);  /* remove upvalues */
}
#endif  // END LUA_VERSION_NUM < 520

const char* jeLua_getError(lua_State* lua) {
	const char* error = "";

	if (lua != NULL) {
		error = lua_tostring(lua, -1);
	}

	if (error == NULL) {
		error = "";
	}

	return error;
}

int jeLuaClient_writeData(lua_State* lua) {
	bool success;
	char const* filename;
	char const* data;
	size_t dataSize;

	gzFile file;
	int dataSizeWritten;

	success = false;

	filename = luaL_checkstring(lua, 1);

	data = luaL_checkstring(lua, 2);
	dataSize = lua_objlen(lua, 2) + 1;

	file = gzopen(filename, "wb");
	if (file == NULL) {
		JE_ERR("jeLuaClient_writeData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto cleanup;
	}

	dataSizeWritten = gzwrite(file, data, dataSize);
	if (dataSizeWritten == 0) {
		JE_ERR("jeLuaClient_writeData(): gzwrite() failed to write data");
		goto cleanup;
	}

	success = true;
	JE_LOG("jeLuaClient_writeData(): gzwrite() bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);

	cleanup: {
		gzclose(file);
	}

	lua_pushboolean(lua, success);

	return 1;
}
int jeLuaClient_readData(lua_State* lua) {
	static char data[JE_LUA_DATA_BUFFER_SIZE];

	char const* filename;
	int dataSize;

	gzFile file;

	filename = luaL_checkstring(lua, 1);
	dataSize = 0;

	file = gzopen(filename, "rb");
	if (file == NULL) {
		JE_ERR("jeLuaClient_readData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto cleanup;
	}

	dataSize = gzread(file, data, JE_LUA_DATA_BUFFER_SIZE);
	JE_LOG("jeLuaClient_readData(): fread() bytes=%d (after decompression) read from filename=%s", dataSize, filename);

	cleanup: {
		gzclose(file);
	}

	lua_pushlstring(lua, data, dataSize - 1);

	return 1;
}
int jeLuaClient_isRunning(lua_State* lua) {
	bool isRunning = false;

	isRunning = jeWindow_isOpen(jeWindow_get());

	lua_pushboolean(lua, isRunning);

	return 1;
}
int jeLuaClient_step(lua_State* lua) {
	JE_MAYBE_UNUSED(lua);

	jeWindow_step(jeWindow_get());

	return 0;
}
int jeLuaClient_updateInputs(lua_State* lua) {
	luaL_checktype(lua, 1, LUA_TTABLE);

	lua_pushstring(lua, "left");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyLeft) || sfKeyboard_isKeyPressed(sfKeyA));
	lua_settable(lua, 1);

	lua_pushstring(lua, "right");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyRight) || sfKeyboard_isKeyPressed(sfKeyD));
	lua_settable(lua, 1);

	lua_pushstring(lua, "up");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyUp) || sfKeyboard_isKeyPressed(sfKeyW));
	lua_settable(lua, 1);

	lua_pushstring(lua, "down");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyDown) || sfKeyboard_isKeyPressed(sfKeyS));
	lua_settable(lua, 1);

	lua_pushstring(lua, "a");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyEnter) || sfKeyboard_isKeyPressed(sfKeyZ));
	lua_settable(lua, 1);

	lua_pushstring(lua, "b");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyBackspace) || sfKeyboard_isKeyPressed(sfKeyX));
	lua_settable(lua, 1);

	lua_pushstring(lua, "restart");
	lua_pushboolean(lua, sfKeyboard_isKeyPressed(sfKeyR));
	lua_settable(lua, 1);

	return 0;
}
int jeLuaClient_getCurrentFPS(lua_State* lua) {
	unsigned fps = 0;

	fps = jeWindow_get()->framesLastSecond;

	lua_pushnumber(lua, (lua_Number)fps);
	return 1;
}
int jeLuaClient_drawSprite(lua_State* lua) {
	// screen
	float screenX1;
	float screenY1;
	float screenX2;
	float screenY2;

	// sprite template
	float r;
	float g;
	float b;
	float a;

	float u1;
	float v1;
	float u2;
	float v2;

	// sprite
	float x1;
	float y1;
	float x2;
	float y2;
	int z;


	// screen
	luaL_checktype(lua, 3, LUA_TTABLE);
	lua_getfield(lua, 3, "x");
	screenX1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	screenY1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "w");
	screenX2 = screenX1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "h");
	screenY2 = screenY1 + luaL_checknumber(lua, -1);

	// sprite
	// TODO make sprite a seprate object fetched in lua code!
	lua_getfield(lua, 2, "r");
	r =  luaL_optnumber(lua, -1, 255.0f);
	lua_getfield(lua, 2, "g");
	g = luaL_optnumber(lua, -1, 255.0f);
	lua_getfield(lua, 2, "b");
	b =  luaL_optnumber(lua, -1, 255.0f);
	lua_getfield(lua, 2, "a");
	a = luaL_optnumber(lua, -1, 255.0f);

	lua_getfield(lua, 2, "u1");
	u1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v1");
	v1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "u2");
	u2 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v2");
	v2 = luaL_checknumber(lua, -1);

	// render object
	luaL_checktype(lua, 1, LUA_TTABLE);
	lua_getfield(lua, 1, "x");
	x1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "y");
	y1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "w");
	x2 = x1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "h");
	y2 = y1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "z");
	z = (int)luaL_optnumber(lua, -1, 0.0);

	if ((a == 0)
		|| (x1 > screenX2)
		|| (y1 > screenY2)
		|| (x2 <= screenX1)
		|| (y2 <= screenY1)
		|| (u1 == u2)
		|| (v1 == v2)
		|| (x1 == x2)
		|| (y1 == y2)) {
		goto cleanup;
	}

	x1 -= screenX1;
	x2 -= screenX1;
	y1 -= screenY1;
	y2 -= screenY1;

	jeRenderQueue_queueSprite(&(jeWindow_get()->renderQueue), z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);

	cleanup: {

	}

	return 0;
}
int jeLuaClient_drawSpriteText(lua_State* lua) {
	// screen
	float screenX1;
	float screenY1;
	float screenX2;
	float screenY2;

	// font
	float r;
	float g;
	float b;
	float a;

	float u;
	float v;

	int charW;
	int charH;
	const char* charFirst;
	const char* charLast;
	int charColumns;

	// text
	float x1;
	float y1;
	float x2;
	float y2;
	int z;

	const char* text;


	// screen
	luaL_checktype(lua, 3, LUA_TTABLE);

	lua_getfield(lua, 3, "x");
	screenX1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	screenY1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "w");
	screenX2 = screenX1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "h");
	screenY2 = screenY1 + luaL_checknumber(lua, -1);

	// font
	luaL_checktype(lua, 2, LUA_TTABLE);

	lua_getfield(lua, 2, "r");
	r = luaL_optnumber(lua, -1, 255.0f);
	lua_getfield(lua, 2, "g");
	g = luaL_optnumber(lua, -1, 255.0f);
	lua_getfield(lua, 2, "b");
	b = luaL_optnumber(lua, -1, 255.0f);
	lua_getfield(lua, 2, "a");
	a = luaL_optnumber(lua, -1, 255.0f);

	lua_getfield(lua, 2, "u");
	u = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v");
	v = luaL_checknumber(lua, -1);

	lua_getfield(lua, 2, "charW");
	charW = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "charH");
	charH = luaL_checknumber(lua, -1);

	lua_getfield(lua, 2, "charFirst");
	charFirst = luaL_checkstring(lua, -1);
	lua_getfield(lua, 2, "charLast");
	charLast = luaL_checkstring(lua, -1);
	lua_getfield(lua, 2, "charColumns");
	charColumns = luaL_checknumber(lua, -1);

	// render object
	luaL_checktype(lua, 1, LUA_TTABLE);

	lua_getfield(lua, 1, "x");
	x1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "y");
	y1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "w");
	x2 = x1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "h");
	y2 = y1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "z");
	z = (int)luaL_optnumber(lua, -1, 0.0f);

	lua_getfield(lua, 1, "text");
	text = luaL_checkstring(lua, -1);

	if ((a == 0)
		|| (x1 > screenX2)
		|| (y1 > screenY2)
		|| (x2 <= screenX1)
		|| (y2 <= screenY1)
		|| (x1 == x2)
		|| (y1 == y2)) {
		goto cleanup;
	}

	x1 -= screenX1;
	x2 -= screenX1;
	y1 -= screenY1;
	y2 -= screenY1;

	// TODO
	// jeRenderQueue_queueSprite(jeWindow_get()->renderQueue, z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);
	// screen
	JE_MAYBE_UNUSED(screenX1);
	JE_MAYBE_UNUSED(screenY1);
	JE_MAYBE_UNUSED(screenX2);
	JE_MAYBE_UNUSED(screenY2);

	// font
	JE_MAYBE_UNUSED(r);
	JE_MAYBE_UNUSED(g);
	JE_MAYBE_UNUSED(b);
	JE_MAYBE_UNUSED(a);

	JE_MAYBE_UNUSED(u);
	JE_MAYBE_UNUSED(v);

	JE_MAYBE_UNUSED(charW);
	JE_MAYBE_UNUSED(charH);
	JE_MAYBE_UNUSED(charFirst);
	JE_MAYBE_UNUSED(charLast);
	JE_MAYBE_UNUSED(charColumns);

	// text
	JE_MAYBE_UNUSED(x1);
	JE_MAYBE_UNUSED(y1);
	JE_MAYBE_UNUSED(x2);
	JE_MAYBE_UNUSED(y2);
	JE_MAYBE_UNUSED(z);

	JE_MAYBE_UNUSED(text);

	cleanup: {
	}

	return 0;
}
bool jeLuaClient_registerLuaClientBindings(lua_State* lua) {
	bool success = false;
	int created = 0;

	static const luaL_Reg clientBindings[] = {
		JE_LUA_CLIENT_BINDING(isRunning),
		JE_LUA_CLIENT_BINDING(step),
		JE_LUA_CLIENT_BINDING(drawSprite),
		JE_LUA_CLIENT_BINDING(updateInputs),
		JE_LUA_CLIENT_BINDING(getCurrentFPS),
		JE_LUA_CLIENT_BINDING(readData),
		JE_LUA_CLIENT_BINDING(writeData),
		{NULL, NULL}  // sentinel value
	};

	JE_LOG("jeLuaClient_registerLuaClientBindings()");

	created = luaL_newmetatable(lua, "jeClientMetatable");
	if (created != 1) {
		JE_ERR("jeLuaClient_registerLuaClientBindings(): luaL_newmetatable() failed, result=%d, metatableName=%s", created, JE_LUA_CLIENT_BINDINGS_KEY);
		goto cleanup;
	}

	luaL_setfuncs(lua, clientBindings, /* num upvalues */ 0);
	lua_pushvalue(lua, -1);
	lua_setfield(lua, -1, "__index");
	lua_setglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);


	success = true;
	cleanup: {
		lua_settop(lua, 0);
	}

	return success;
}
