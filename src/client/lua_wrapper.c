#include "stdafx.h"
#include "lua_wrapper.h"
#include "core.h"
#include "rendering.h"
#include "window.h"

/*https://www.lua.org/manual/5.3/manual.html*/
#define JE_LUA_DATA_BUFFER_SIZE 8 * 1024 * 1024
#define JE_LUA_CLIENT_BINDINGS_KEY "jeClientBindings"
#define JE_LUA_CLIENT_BINDING(BINDING_NAME) {#BINDING_NAME, jeLuaClient_##BINDING_NAME}


/*Adapted from https://github.com/keplerproject/lua-compat-5.2/blob/master/c-api/compat-5.2.c#L119*/
#if LUA_VERSION_NUM < 520
void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
	int i;

	luaL_checkstack(L, nup+1, "too many upvalues");

	for (; l->name != NULL; l++) {  /* fill the table with given functions */
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
#else
size_t lua_objlen(lua_State *L, int index) {
	return lua_rawlen(l, int index);
}
#endif

const char* jeLua_getError(lua_State* lua) {
	const char* error = "";

	error = lua_tostring(lua, -1);

	if (error == NULL) {
		error = "";
	}

	return error;
}

int jeLuaClient_writeData(lua_State* lua) {
	bool success = false;
	char const* filename = "";
	char const* data = "";
	size_t dataSize = 0;
	int dataSizeWritten = 0;
	gzFile file = NULL;

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

	JE_LOG("jeLuaClient_writeData(): gzwrite() bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);

	success = true;
	cleanup: {
		if (file != NULL) {
			gzclose(file);
		}
	}

	lua_pushboolean(lua, success);
	return 1;
}
int jeLuaClient_readData(lua_State* lua) {
	static char data[JE_LUA_DATA_BUFFER_SIZE] = {0};

	bool success = false;
	int numResponses = 0;
	char const* filename = "";
	int dataSize = 0;
	gzFile file = NULL;

	memset((void*)&data, 0, sizeof(data));

	filename = luaL_checkstring(lua, 1);

	file = gzopen(filename, "rb");

	if (file == NULL) {
		JE_ERR("jeLuaClient_readData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto cleanup;
	}

	dataSize = gzread(file, data, JE_LUA_DATA_BUFFER_SIZE);

	/*Remove null terminator if string exists*/
	if (dataSize > 0) {
		dataSize--;
	}

	JE_LOG("jeLuaClient_readData(): fread() bytes=%d (after decompression) read from filename=%s", dataSize, filename);

	success = true;

	if (success == true) {
		lua_pushlstring(lua, data, dataSize);
		numResponses++;
	}

	cleanup: {
		if (file != NULL) {
			gzclose(file);
		}
	}
	
	return numResponses;
}
int jeLuaClient_isRunning(lua_State* lua) {
	lua_pushboolean(lua, jeWindow_isOpen(jeWindow_get()));
	return 1;
}
int jeLuaClient_step(lua_State* lua) {
	jeWindow_step(jeWindow_get());

	JE_MAYBE_UNUSED(lua);

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
	lua_pushnumber(lua, (lua_Number)jeWindow_get()->framesLastSecond);
	return 1;
}
int jeLuaClient_drawSprite(lua_State* lua) {
	/*screen*/
	float screenX1 = 0.0f;
	float screenY1 = 0.0f;
	float screenX2 = 0.0f;
	float screenY2 = 0.0f;

	/*sprite template*/
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;

	float u1 = 0.0f;
	float v1 = 0.0f;
	float u2 = 0.0f;
	float v2 = 0.0f;

	/*sprite*/
	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2 = 0.0f;
	float y2 = 0.0f;
	int z = 0;

	/*screen*/
	luaL_checktype(lua, 3, LUA_TTABLE);
	lua_getfield(lua, 3, "x");
	screenX1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	screenY1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "w");
	screenX2 = screenX1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "h");
	screenY2 = screenY1 + luaL_checknumber(lua, -1);

	/*sprite*/
	/*TODO make sprite a seprate object fetched in lua code!*/
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

	/*render object*/
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
	/*screen*/
	float screenX1 = 0.0f;
	float screenY1 = 0.0f;
	float screenX2 = 0.0f;
	float screenY2 = 0.0f;

	/*font*/
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;

	float u = 0.0f;
	float v = 0.0f;

	int charW = 0;
	int charH = 0;
	int charColumns = 0;
	const char* charFirst = "\0";
	const char* charLast = "\0";

	/*text*/
	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2 = 0.0f;
	float y2 = 0.0f;
	int z = 0;
	const char* text = "";

	/*screen*/
	luaL_checktype(lua, 3, LUA_TTABLE);

	lua_getfield(lua, 3, "x");
	screenX1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	screenY1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "w");
	screenX2 = screenX1 + luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "h");
	screenY2 = screenY1 + luaL_checknumber(lua, -1);

	/*font*/
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

	/*render object*/
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

	/*TODO*/
	/*jeRenderQueue_queueSprite(jeWindow_get()->renderQueue, z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);*/

	/*screen*/
	JE_MAYBE_UNUSED(screenX1);
	JE_MAYBE_UNUSED(screenY1);
	JE_MAYBE_UNUSED(screenX2);
	JE_MAYBE_UNUSED(screenY2);

	/*font*/
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

	/*text*/
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
	static const luaL_Reg clientBindings[] = {
		JE_LUA_CLIENT_BINDING(isRunning),
		JE_LUA_CLIENT_BINDING(step),
		JE_LUA_CLIENT_BINDING(drawSprite),
		JE_LUA_CLIENT_BINDING(updateInputs),
		JE_LUA_CLIENT_BINDING(getCurrentFPS),
		JE_LUA_CLIENT_BINDING(readData),
		JE_LUA_CLIENT_BINDING(writeData),
		{NULL, NULL}  /*sentinel value*/
	};

	bool success = false;
	int createdResult = 0;

	JE_LOG("jeLuaClient_registerLuaClientBindings()");

	createdResult = luaL_newmetatable(lua, "jeClientMetatable");
	if (createdResult != 1) {
		JE_ERR("jeLuaClient_registerLuaClientBindings(): luaL_newmetatable() failed, result=%d, metatableName=%s", createdResult, JE_LUA_CLIENT_BINDINGS_KEY);
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
