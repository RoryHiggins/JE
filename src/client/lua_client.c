#include "stdafx.h"
#include "lua_client.h"
#include "debug.h"
#include "window.h"

/*https://www.lua.org/manual/5.3/manual.html*/
#define JE_LUA_DATA_BUFFER_SIZE 8 * 1024 * 1024
#define JE_LUA_CLIENT_BINDINGS_KEY "jeClientBindings"
#define JE_LUA_CLIENT_BINDING(BINDING_NAME) {#BINDING_NAME, jeLua_##BINDING_NAME}


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
jeWindow* jeLua_getWindow(lua_State* lua) {
	jeWindow *window = NULL;

	lua_settop(lua, 0);
	lua_getglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);
	lua_getfield(lua, -1, "window");

	if (lua_islightuserdata(lua, -1) == 0) {
		JE_ERROR("jeLua_getWindow(): %s.window is not set", JE_LUA_CLIENT_BINDINGS_KEY);
		goto finalize;
	}

	window = (jeWindow*)lua_touserdata(lua, -1);

	finalize: {

	}
	return window;
}
void jeLua_updateStates(lua_State* lua) {
	jeWindow* window = jeLua_getWindow(lua);

	lua_settop(lua, 0);
	lua_getglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);
	lua_getfield(lua, -1, "state");

	lua_pushboolean(lua, jeWindow_getIsOpen(window));
	lua_setfield(lua, 2, "running");

	lua_pushnumber(lua, (lua_Number)JE_WINDOW_MIN_WIDTH);
	lua_setfield(lua, 2, "width");

	lua_pushnumber(lua, (lua_Number)JE_WINDOW_MIN_HEIGHT);
	lua_setfield(lua, 2, "height");

	lua_pushnumber(lua, (lua_Number)jeWindow_getFramesPerSecond(window));
	lua_setfield(lua, 2, "fps");

	lua_pushnumber(lua, (lua_Number)JE_LOG_LEVEL);
	lua_setfield(lua, 2, "logLevel");

	lua_pushboolean(lua, JE_TRUE);
	lua_setfield(lua, 2, "testsEnabled");

	lua_pushnumber(lua, (lua_Number)JE_LOG_LEVEL_WARN);
	lua_setfield(lua, 2, "testsLogLevel");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_LEFT));
	lua_setfield(lua, 2, "inputLeft");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_RIGHT));
	lua_setfield(lua, 2, "inputRight");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_UP));
	lua_setfield(lua, 2, "inputUp");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_DOWN));
	lua_setfield(lua, 2, "inputDown");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_A));
	lua_setfield(lua, 2, "inputA");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_B));
	lua_setfield(lua, 2, "inputB");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_X));
	lua_setfield(lua, 2, "inputX");

	lua_pushboolean(lua, jeWindow_getInputState(window, JE_INPUT_Y));
	lua_setfield(lua, 2, "inputY");

	/*finalize:*/ {
		lua_settop(lua, 0);
	}
}

int jeLua_writeData(lua_State* lua) {
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
		JE_ERROR("jeLua_writeData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto finalize;
	}

	dataSizeWritten = gzwrite(file, data, dataSize);

	if (dataSizeWritten == 0) {
		JE_ERROR("jeLua_writeData(): gzwrite() failed to write data");
		goto finalize;
	}

	JE_DEBUG("jeLua_writeData(): gzwrite() bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);

	success = JE_TRUE;
	finalize: {
		if (file != NULL) {
			gzclose(file);
		}
	}

	lua_pushboolean(lua, success);
	return 1;
}
int jeLua_readData(lua_State* lua) {
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
		JE_ERROR("jeLua_readData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto finalize;
	}

	dataSize = gzread(file, data, JE_LUA_DATA_BUFFER_SIZE);

	/*Remove null terminator if string exists*/
	if (dataSize > 0) {
		dataSize--;
	}

	JE_DEBUG("jeLua_readData(): fread() bytes=%d (after decompression) read from filename=%s", dataSize, filename);

	success = JE_TRUE;

	if (success == JE_TRUE) {
		lua_pushlstring(lua, data, dataSize);
		numResponses++;
	}

	finalize: {
		if (file != NULL) {
			gzclose(file);
		}
	}
	
	return numResponses;
}
int jeLua_drawSprite(lua_State* lua) {
	/*camera*/
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float cameraW = 0.0f;
	float cameraH = 0.0f;

	float cameraOffsetX = 0.0f;
	float cameraOffsetY = 0.0f;

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
	float spriteScaleX = 1.0f;
	float spriteScaleY = 1.0f;
	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2 = 0.0f;
	float y2 = 0.0f;
	float z = 0;

	luaL_checktype(lua, 3, LUA_TTABLE);
	luaL_checktype(lua, 2, LUA_TTABLE);
	luaL_checktype(lua, 1, LUA_TTABLE);

	/*camera*/
	lua_getfield(lua, 3, "x");
	cameraX = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	cameraY = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "w");
	cameraW = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "h");
	cameraH = luaL_checknumber(lua, -1);

	cameraOffsetX = -cameraX - (float)floor(cameraW / 2.0f);
	cameraOffsetY = -cameraY - (float)floor(cameraH / 2.0f);

	/*sprite*/
	/*TODO make sprite a seprate object fetched in lua code!*/
	lua_getfield(lua, 2, "r");
	r = luaL_optnumber(lua, -1, 1.0f);
	lua_getfield(lua, 2, "g");
	g = luaL_optnumber(lua, -1, 1.0f);
	lua_getfield(lua, 2, "b");
	b = luaL_optnumber(lua, -1, 1.0f);
	lua_getfield(lua, 2, "a");
	a = luaL_optnumber(lua, -1, 1.0f);

	lua_getfield(lua, 2, "u1");
	u1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v1");
	v1 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "u2");
	u2 = luaL_checknumber(lua, -1);
	lua_getfield(lua, 2, "v2");
	v2 = luaL_checknumber(lua, -1);

	/*render object*/
	lua_getfield(lua, 1, "spriteScaleX");
	spriteScaleX = spriteScaleX * luaL_optnumber(lua, -1, 1.0f);
	lua_getfield(lua, 1, "spriteScaleY");
	spriteScaleY = spriteScaleY * luaL_optnumber(lua, -1, 1.0f);

	lua_getfield(lua, 1, "x");
	x1 = cameraOffsetX + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "y");
	y1 = cameraOffsetY + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "spriteTranslationX");
	x1 = x1 + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "spriteTranslationY");
	y1 = y1 + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "z");
	z = luaL_optnumber(lua, -1, 0.0f);

	lua_getfield(lua, 1, "w");
	x2 = x1 + (luaL_optnumber(lua, -1, u2 - u1) * spriteScaleX);
	lua_getfield(lua, 1, "h");
	y2 = y1 + (luaL_optnumber(lua, -1, y2 - y1) * spriteScaleY);

	jeWindow_drawSprite(jeLua_getWindow(lua), z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);

	return 0;
}
int jeLua_drawText(lua_State* lua) {
	static const char charDefault = ' ';
	/*camera*/
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float cameraW = 0.0f;
	float cameraH = 0.0f;

	float cameraOffsetX = 0.0f;
	float cameraOffsetY = 0.0f;

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

	/*char*/
	int i = 0;
	char charVal = charDefault;
	int charIndex = 0;
	float charX = 0.0f;
	float charY = 0.0f;
	float charU = 0.0f;
	float charV = 0.0f;

	float charScaleX = 1.0f;
	float charScaleY = 1.0f;

	/*render object*/
	float x = 0.0f;
	float y = 0.0f;
	float z = 0;

	const char* text = "";
	int textLength = 0;

	luaL_checktype(lua, 3, LUA_TTABLE);
	luaL_checktype(lua, 2, LUA_TTABLE);
	luaL_checktype(lua, 1, LUA_TTABLE);

	/*camera*/
	lua_getfield(lua, 3, "x");
	cameraX = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "y");
	cameraY = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "w");
	cameraW = luaL_checknumber(lua, -1);
	lua_getfield(lua, 3, "h");
	cameraH = luaL_checknumber(lua, -1);

	cameraOffsetX = -cameraX - (float)floor(cameraW / 2.0f);
	cameraOffsetY = -cameraY - (float)floor(cameraH / 2.0f);

	/*font*/
	lua_getfield(lua, 2, "r");
	r = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 2, "g");
	g = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 2, "b");
	b = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 2, "a");
	a = luaL_optnumber(lua, -1, 1.0f);

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
	lua_getfield(lua, 1, "textScaleX");
	charScaleX = charScaleX * luaL_optnumber(lua, -1, 1.0f);
	lua_getfield(lua, 1, "textScaleY");
	charScaleY = charScaleY * luaL_optnumber(lua, -1, 1.0f);

	lua_getfield(lua, 1, "x");
	x = cameraOffsetX + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "y");
	y = cameraOffsetY + luaL_checknumber(lua, -1);
	lua_getfield(lua, 1, "textTranslationX");
	x = x + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "textTranslationY");
	y = y + luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "z");
	z = luaL_optnumber(lua, -1, 0.0f);
	lua_getfield(lua, 1, "textZ");
	z = luaL_optnumber(lua, -1, z);

	lua_getfield(lua, 1, "text");
	text = luaL_checkstring(lua, -1);
	textLength = strnlen(text, 256);

	/*TODO*/
	for (i = 0; i < textLength; i++) {
		charVal = (char)toupper((int)text[i]);
		if ((charVal < charFirst[0]) || (charVal > charLast[0])) {
			JE_DEBUG("jeLua_drawText(): character outside range, char=%d, min=%d, max=%d", (int)charVal, (int)charFirst[0], (int)charLast[0]);
			charVal = charDefault;
		}

		charX = x + (charW * charScaleX * i);
		charY = y;

		charIndex = (int)(charVal - charFirst[0]);
		charU = u + (charW * (charIndex % charColumns));
		charV = v + (charH * (charIndex / charColumns));
		jeWindow_drawSprite(
			jeLua_getWindow(lua),
			z,
			charX,
			charY,
			charX + (charW * charScaleX),
			charY + (charH * charScaleY),
			r,
			g,
			b,
			a,
			charU,
			charV,
			charU + charW,
			charV + charH
		);
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
int jeLua_step(lua_State* lua) {
	jeWindow* window = jeLua_getWindow(lua);

	jeWindow_step(window);

	jeLua_updateStates(lua);

	return 0;
}
jeBool jeLuaClient_registerLuaClientBindings(lua_State* lua, jeWindow* window) {
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
		JE_ERROR("jeLuaClient_registerLuaClientBindings(): luaL_newmetatable() failed, result=%d, metatableName=%s", createdResult, JE_LUA_CLIENT_BINDINGS_KEY);
		goto finalize;
	}

	luaL_setfuncs(lua, clientBindings, /* num upvalues */ 0);
	lua_pushvalue(lua, -1);
	lua_setfield(lua, -1, "__index");

	lua_pushvalue(lua, -1);
	lua_setglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);

	lua_createtable(lua, /*numArrayElems*/ 0, /*numNonArrayElems*/ 16);
	lua_setfield(lua, -2, "state");

	lua_pushlightuserdata(lua, (void*)window);
	lua_setfield(lua, -2, "window");

	jeLua_updateStates(lua);

	success = JE_TRUE;
	finalize: {
		lua_settop(lua, 0);
	}

	return success;
}
void jeLuaClient_destroy(jeLuaClient* luaClient) {
	JE_INFO("jeLuaClient_destroy()");

	if (luaClient->lua != NULL) {
		lua_close(luaClient->lua);
		luaClient->lua = NULL;
	}
}
jeBool jeLuaClient_create(jeLuaClient* luaClient, jeWindow* window) {
	jeBool success = JE_FALSE;

	JE_INFO("jeLuaClient_create()");

	memset((void*)luaClient, 0, sizeof(*luaClient));

	luaClient->lua = luaL_newstate();

	if (luaClient->lua == NULL) {
		JE_ERROR("jeLuaClient_create(): luaL_newstate() failed");
		goto finalize;
	}

	luaL_openlibs(luaClient->lua);

	if (jeLuaClient_registerLuaClientBindings(luaClient->lua, window) == JE_FALSE) {
		JE_ERROR("jeLuaClient_create(): jeLuaClient_registerLuaClientBindings() failed");
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
	}

	return success;
}
jeBool jeLuaClient_run(jeLuaClient* luaClient, jeWindow* window, const char* filename) {
	jeBool success = JE_FALSE;
	int luaResponse = 0;

	JE_INFO("jeLuaClient_run()");

	if (jeLuaClient_create(luaClient, window) == JE_FALSE) {
		JE_ERROR("jeLuaClient_run(): jeLuaClient_create() failed");
		goto finalize;
	}

	luaResponse = luaL_loadfile(luaClient->lua, filename);
	if (luaResponse != 0) {
		JE_ERROR("jeLuaClient_run(): luaL_loadfile() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLua_getError(luaClient->lua));
		goto finalize;
	}

	luaResponse = lua_pcall(luaClient->lua, /* num args */ 0, /* num return vals */ LUA_MULTRET, /* err func */ 0);
	if (luaResponse != 0) {
		JE_ERROR("jeLuaClient_run(): lua_pcall() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLua_getError(luaClient->lua));
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
		jeLuaClient_destroy(luaClient);
	}

	return success;
}

