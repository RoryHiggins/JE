#include "stdafx.h"
#include "lua_wrapper.h"
#include "debug.h"
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
	jeBool success = JE_FALSE;
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

	JE_DEBUG("jeLuaClient_writeData(): gzwrite() bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);

	success = JE_TRUE;
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

	jeBool success = JE_FALSE;
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

	JE_DEBUG("jeLuaClient_readData(): fread() bytes=%d (after decompression) read from filename=%s", dataSize, filename);

	success = JE_TRUE;

	if (success == JE_TRUE) {
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
int jeLuaClient_drawSprite(lua_State* lua) {
	/*screen*/
	float screenOriginX = 0.0f;
	float screenOriginY = 0.0f;

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
	float z = 0;

	luaL_checktype(lua, 3, LUA_TTABLE);
	luaL_checktype(lua, 2, LUA_TTABLE);
	luaL_checktype(lua, 1, LUA_TTABLE);

	/*screen*/
	lua_getfield(lua, 3, "x");
	screenOriginX = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 3, "y");
	screenOriginY = luaL_optnumber(lua, -1, 0.0f);

	/*sprite*/
	/*TODO make sprite a seprate object fetched in lua code!*/
	lua_getfield(lua, 2, "r");
	r =  luaL_optnumber(lua, -1, 256.0f);
	lua_getfield(lua, 2, "g");
	g = luaL_optnumber(lua, -1, 256.0f);
	lua_getfield(lua, 2, "b");
	b =  luaL_optnumber(lua, -1, 256.0f);
	lua_getfield(lua, 2, "a");
	a = luaL_optnumber(lua, -1, 256.0f);

	lua_getfield(lua, 2, "u1");
	u1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v1");
	v1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "u2");
	u2 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v2");
	v2 = luaL_checknumber(lua, -1);

	/*render object*/
	lua_getfield(lua, 1, "x");
	x1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "y");
	y1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "spriteTranslationX");
	x1 = x1 + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "spriteTranslationY");
	y1 = y1 + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "z");
	z = luaL_optnumber(lua, -1, 0.0f);

	lua_getfield(lua, 1, "w");
	x2 = x1 + luaL_optnumber(lua, -1, u2 - u1);
	lua_getfield(lua, 1, "h");
	y2 = y1 + luaL_optnumber(lua, -1, y2 - y1);

	x1 -= screenOriginX;
	x2 -= screenOriginX;
	y1 -= screenOriginY;
	y2 -= screenOriginY;

	jeWindow_drawSprite(jeWindow_get(), z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);

	return 0;
}
int jeLuaClient_drawText(lua_State* lua) {
	static const char charDefault = ' ';
	/*screen*/
	float screenOriginX = 0.0f;
	float screenOriginY = 0.0f;

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
	float x = 0.0f;
	float y = 0.0f;
	float z = 0;
	const char* text = "";
	int textLength = 0;

	/*char*/
	int i = 0;
	char charVal = charDefault;
	int charIndex = 0;
	float charX = 0.0f;
	float charY = 0.0f;
	float charU = 0.0f;
	float charV = 0.0f;

	luaL_checktype(lua, 3, LUA_TTABLE);
	luaL_checktype(lua, 2, LUA_TTABLE);
	luaL_checktype(lua, 1, LUA_TTABLE);

	/*screen*/
	lua_getfield(lua, 3, "x");
	screenOriginX = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	screenOriginY = luaL_checknumber(lua, -1);

	/*font*/
	lua_getfield(lua, 2, "r");
	r = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 2, "g");
	g = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 2, "b");
	b = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 2, "a");
	a = luaL_optnumber(lua, -1, 256.0f);

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
	lua_getfield(lua, 1, "x");
	x = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "y");
	y = luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "textTranslationX");
	x = x + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "textTranslationY");
	y = y + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "z");
	z = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "textZ");
	z = luaL_optnumber(lua, -1, z);

	lua_getfield(lua, 1, "text");
	text = luaL_optstring(lua, -1, "");
	textLength = strnlen(text, 256);

	x -= screenOriginX;
	y -= screenOriginY;

	/*TODO*/
	for (i = 0; i < textLength; i++) {
		charVal = (char)toupper((int)text[i]);
		if ((charVal < charFirst[0]) || (charVal > charLast[0])) {
			JE_DEBUG("jeLuaClient_drawText(): character outside range, char=%d, min=%d, max=%d", (int)charVal, (int)charFirst[0], (int)charLast[0]);
			charVal = charDefault;
		}

		charX = x + (charW * i);
		charY = y;

		charIndex = (int)(charVal - charFirst[0]);
		charU = u + (charW * (charIndex % charColumns));
		charV = v + (charH * (charIndex / charColumns));
		jeWindow_drawSprite(jeWindow_get(), z, charX, charY, charX + charW, charY + charH, r, g, b, a, charU, charV, charU + charW, charV + charH);
	}

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
	JE_MAYBE_UNUSED(x);
	JE_MAYBE_UNUSED(y);
	JE_MAYBE_UNUSED(z);

	JE_MAYBE_UNUSED(text);

	return 0;
}
int jeLuaClient_step(lua_State* lua) {
	jeWindow* window = jeWindow_get();

	jeWindow_step(window);

	luaL_checktype(lua, 1, LUA_TTABLE);

	lua_pushstring(lua, "running");
	lua_pushboolean(lua, jeWindow_isOpen(window));
	lua_settable(lua, 1);

	lua_pushstring(lua, "fps");
	lua_pushnumber(lua, (lua_Number)jeWindow_getFramesPerSecond(window));
	lua_settable(lua, 1);

	lua_pushstring(lua, "logLevel");
	lua_pushnumber(lua, (lua_Number)JE_LOG_LEVEL);
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputLeft");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_LEFT));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputRight");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_RIGHT));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputUp");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_UP));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputDown");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_DOWN));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputA");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_A));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputB");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_B));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputX");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_X));
	lua_settable(lua, 1);

	lua_pushstring(lua, "inputY");
	lua_pushboolean(lua, jeWindow_getInput(window, JE_INPUT_Y));
	lua_settable(lua, 1);

	return 0;
}
jeBool jeLuaClient_registerLuaClientBindings(lua_State* lua) {
	static const luaL_Reg clientBindings[] = {
		JE_LUA_CLIENT_BINDING(readData),
		JE_LUA_CLIENT_BINDING(writeData),
		JE_LUA_CLIENT_BINDING(drawSprite),
		JE_LUA_CLIENT_BINDING(drawText),
		JE_LUA_CLIENT_BINDING(step),
		{NULL, NULL}  /*sentinel value*/
	};

	jeBool success = JE_FALSE;
	int createdResult = 0;

	JE_DEBUG("jeLuaClient_registerLuaClientBindings()");

	createdResult = luaL_newmetatable(lua, "jeClientMetatable");
	if (createdResult != 1) {
		JE_ERR("jeLuaClient_registerLuaClientBindings(): luaL_newmetatable() failed, result=%d, metatableName=%s", createdResult, JE_LUA_CLIENT_BINDINGS_KEY);
		goto cleanup;
	}

	luaL_setfuncs(lua, clientBindings, /* num upvalues */ 0);
	lua_pushvalue(lua, -1);
	lua_setfield(lua, -1, "__index");
	lua_setglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);

	success = JE_TRUE;
	cleanup: {
		lua_settop(lua, 0);
	}

	return success;
}
