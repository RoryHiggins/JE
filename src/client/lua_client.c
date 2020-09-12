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
	int i;

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
	jeWindow *window = NULL;

	int stackPos = lua_gettop(lua);

	lua_settop(lua, 0);
	lua_getglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

	if (lua_islightuserdata(lua, JE_LUA_STACK_TOP) == 0) {
		JE_ERROR("jeLuaClient_getWindow(): %s is not set", JE_LUA_CLIENT_WINDOW_KEY);
		goto finalize;
	}

	window = (jeWindow*)lua_touserdata(lua, JE_LUA_STACK_TOP);

	finalize: {
		lua_settop(lua, stackPos);
	}
	return window;
}
void jeLuaClient_addWindow(lua_State* lua, jeWindow* window) {
	int stackPos = lua_gettop(lua);

	lua_pushlightuserdata(lua, (void*)window);
	lua_setglobal(lua, JE_LUA_CLIENT_WINDOW_KEY);

	/*finalize:*/ {
		lua_settop(lua, stackPos);
	}
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

	lua_pushnumber(lua, (lua_Number)jeWindow_getFPS(window));
	lua_setfield(lua, stateStackPos, "fps");

	lua_pushnumber(lua, (lua_Number)JE_LOG_LEVEL);
	lua_setfield(lua, stateStackPos, "logLevel");

	lua_pushboolean(lua, JE_TRUE);
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

	/*finalize:*/ {
		lua_settop(lua, stackPos);
	}
}

/*Lua-client bindings.  Note: return value = num responses pushed to lua stack*/
int jeLuaClient_readData(lua_State* lua) {
	static char data[JE_LUA_DATA_BUFFER_SIZE] = {0};

	jeBool success = JE_FALSE;
	int numResponses = 0;
	const char* filename = "";
	int dataSize = 0;
	gzFile file = NULL;

	memset((void*)&data, 0, sizeof(data));

	filename = luaL_checkstring(lua, 1);

	file = gzopen(filename, "rb");

	if (file == NULL) {
		JE_ERROR("jeLuaClient_readData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto finalize;
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

	finalize: {
		if (file != NULL) {
			gzclose(file);
		}
	}
	
	return numResponses;
}
int jeLuaClient_writeData(lua_State* lua) {
	static const int filenameArg = 1;
	static const int dataArg = 2;

	jeBool success = JE_FALSE;
	const char* filename = "";
	const char* data = "";
	size_t dataSize = 0;
	int dataSizeWritten = 0;
	gzFile file = NULL;

	filename = luaL_checkstring(lua, filenameArg);
	data = luaL_checkstring(lua, dataArg);
	dataSize = lua_objlen(lua, dataArg) + 1;

	file = gzopen(filename, "wb");

	if (file == NULL) {
		JE_ERROR("jeLuaClient_writeData(): gzopen() failed with filename=%s, errno=%d err=%s",
			   filename, errno, strerror(errno));
		goto finalize;
	}

	dataSizeWritten = gzwrite(file, data, dataSize);

	if (dataSizeWritten == 0) {
		JE_ERROR("jeLuaClient_writeData(): gzwrite() failed to write data");
		goto finalize;
	}

	JE_DEBUG("jeLuaClient_writeData(): gzwrite() bytes=%d (before compression) written to filename=%s", dataSizeWritten, filename);

	success = JE_TRUE;
	finalize: {
		if (file != NULL) {
			gzclose(file);
		}
	}

	lua_pushboolean(lua, success);
	return 1;
}
int jeLuaClient_drawRenderableImpl(lua_State* lua, int primitiveType) {
	static const int cameraArg = 3;
	static const int defaultsArg = 2;
	static const int renderableArg = 1;

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

	memset(&renderable, 0, sizeof(renderable));

	luaL_checktype(lua, cameraArg, LUA_TTABLE);
	luaL_checktype(lua, defaultsArg, LUA_TTABLE);
	luaL_checktype(lua, renderableArg, LUA_TTABLE);

	renderable.primitiveType = primitiveType;

	/*camera*/
	lua_getfield(lua, cameraArg, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "y1");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "x2");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraX;

		lua_getfield(lua, cameraArg, "y2");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraY;
	} else {
		lua_getfield(lua, cameraArg, "x");
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "y");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "w");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "h");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	cameraOffsetX = -cameraX - (float)floor(cameraW / 2.0f);
	cameraOffsetY = -cameraY - (float)floor(cameraH / 2.0f);

	/*defaults*/
	lua_getfield(lua, defaultsArg, "r");
	renderable.r = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, defaultsArg, "g");
	renderable.g = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, defaultsArg, "b");
	renderable.b = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, defaultsArg, "a");
	renderable.a = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	lua_getfield(lua, defaultsArg, "u1");
	renderable.u1 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsArg, "v1");
	renderable.v1 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsArg, "u2");
	renderable.u2 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsArg, "v2");
	renderable.v2 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsArg, "u3");
	renderable.u3 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, defaultsArg, "v3");
	renderable.v3 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);

	/*render object*/
	lua_getfield(lua, renderableArg, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		renderable.x1 = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "y1");
		renderable.y1 = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "x2");
		renderable.x2 = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "y2");
		renderable.y2 = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "x3");
		renderable.x3 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);

		lua_getfield(lua, renderableArg, "y3");
		renderable.y3 = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	} else {
		lua_getfield(lua, renderableArg, "x");
		renderable.x1 = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "y");
		renderable.y1 = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "w");
		renderable.x2 = renderable.x1 + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "h");
		renderable.y2 = renderable.y1 + luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	lua_getfield(lua, renderableArg, "z");
	renderable.z = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);

	lua_getfield(lua, renderableArg, "renderScaleX");
	renderScaleX = renderScaleX * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableArg, "renderScaleY");
	renderScaleY = renderScaleY * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	renderable.x2 = renderable.x1 + ((renderable.x2 - renderable.x1) * renderScaleX);
	renderable.y2 = renderable.y1 + ((renderable.y2 - renderable.y1) * renderScaleY);
	renderable.x3 = renderable.x1 + ((renderable.x3 - renderable.x1) * renderScaleX);
	renderable.y3 = renderable.y1 + ((renderable.y3 - renderable.y1) * renderScaleY);

	lua_getfield(lua, renderableArg, "r");
	renderable.r = renderable.r * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.r);
	lua_getfield(lua, renderableArg, "g");
	renderable.g = renderable.g * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.g);
	lua_getfield(lua, renderableArg, "b");
	renderable.b = renderable.b * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.b);
	lua_getfield(lua, renderableArg, "a");
	renderable.a = renderable.a * luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.a);

	lua_getfield(lua, defaultsArg, "u1");
	renderable.u1 = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.u1);
	lua_getfield(lua, defaultsArg, "v1");
	renderable.v1 = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.v1);
	lua_getfield(lua, defaultsArg, "u2");
	renderable.u2 = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.u2);
	lua_getfield(lua, defaultsArg, "v2");
	renderable.v2 = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.v2);
	lua_getfield(lua, defaultsArg, "u3");
	renderable.u3 = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.u3);
	lua_getfield(lua, defaultsArg, "v3");
	renderable.v3 = luaL_optnumber(lua, JE_LUA_STACK_TOP, renderable.v3);

	JE_TRACE("jeLuaClient_drawRenderableImpl(): drawing renderable, %s", jeRenderable_toDebugString(&renderable));
	jeWindow_drawRenderable(jeLuaClient_getWindow(lua), &renderable);

	return 0;
}
int jeLuaClient_drawTextImpl(lua_State* lua) {
	static const int cameraArg = 3;
	static const int fontArg = 2;
	static const int renderableArg = 1;

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

	luaL_checktype(lua, cameraArg, LUA_TTABLE);
	luaL_checktype(lua, fontArg, LUA_TTABLE);
	luaL_checktype(lua, renderableArg, LUA_TTABLE);

	renderable.primitiveType = JE_PRIMITIVE_TYPE_SPRITES;

	/*camera*/
	lua_getfield(lua, cameraArg, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "y1");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "x2");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraX;

		lua_getfield(lua, cameraArg, "y2");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP) - cameraY;
	} else {
		lua_getfield(lua, cameraArg, "x");
		cameraX = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "y");
		cameraY = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "w");
		cameraW = luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, cameraArg, "h");
		cameraH = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}

	cameraOffsetX = -cameraX - (float)floor(cameraW / 2.0f);
	cameraOffsetY = -cameraY - (float)floor(cameraH / 2.0f);

	/*font*/
	lua_getfield(lua, fontArg, "r");
	renderable.r = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, fontArg, "g");
	renderable.g = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, fontArg, "b");
	renderable.b = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);
	lua_getfield(lua, fontArg, "a");
	renderable.a = luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	lua_getfield(lua, fontArg, "u1");
	renderable.u1 = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, fontArg, "v1");
	renderable.v1 = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	renderable.u2 = renderable.u1;
	renderable.v2 = renderable.v1;

	lua_getfield(lua, fontArg, "charW");
	charW = luaL_checknumber(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, fontArg, "charH");
	charH = luaL_checknumber(lua, JE_LUA_STACK_TOP);

	lua_getfield(lua, fontArg, "charFirst");
	charFirst = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, fontArg, "charLast");
	charLast = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	lua_getfield(lua, fontArg, "charColumns");
	charColumns = luaL_checknumber(lua, JE_LUA_STACK_TOP);

	/*render object*/
	lua_getfield(lua, renderableArg, "renderScaleX");
	charScaleX = charScaleX * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableArg, "renderScaleY");
	charScaleY = charScaleY * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	lua_getfield(lua, renderableArg, "x1");
	if (lua_isnumber(lua, JE_LUA_STACK_TOP)) {
		renderable.x1 = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "y1");
		renderable.y1 = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);
	} else {
		lua_getfield(lua, renderableArg, "x");
		renderable.x1 = cameraOffsetX + luaL_checknumber(lua, JE_LUA_STACK_TOP);

		lua_getfield(lua, renderableArg, "y");
		renderable.y1 = cameraOffsetY + luaL_checknumber(lua, JE_LUA_STACK_TOP);
	}
	renderable.x2 = renderable.x1;
	renderable.y2 = renderable.y1;
	lua_getfield(lua, renderableArg, "z");
	renderable.z = luaL_optnumber(lua, JE_LUA_STACK_TOP, 0.0f);

	lua_getfield(lua, renderableArg, "text");
	text = luaL_checkstring(lua, JE_LUA_STACK_TOP);
	textLength = lua_objlen(lua, JE_LUA_STACK_TOP);

	lua_getfield(lua, renderableArg, "r");
	renderable.r = renderable.r * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableArg, "g");
	renderable.g = renderable.g * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableArg, "b");
	renderable.b = renderable.b * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);
	lua_getfield(lua, renderableArg, "a");
	renderable.a = renderable.a * luaL_optnumber(lua, JE_LUA_STACK_TOP, 1.0f);

	JE_TRACE("jeLuaClient_drawText(): drawing text, text=%s, %s", text, jeRenderable_toDebugString(&renderable));
	for (i = 0; i < textLength; i++) {
		charVal = (char)toupper((int)text[i]);
		if ((charVal < charFirst[0]) || (charVal > charLast[0])) {
			JE_WARN("jeLuaClient_drawText(): character outside range, char=%d, min=%d, max=%d",
					(int)charVal, (int)charFirst[0], (int)charLast[0]);
			charVal = charDefault;
		}

		charIndex = (int)(charVal - charFirst[0]);

		charRenderable = renderable;
		charRenderable.x1 += (charW * charScaleX * i);
		charRenderable.x2 = charRenderable.x1 + (charW * charScaleX);
		charRenderable.y2 = charRenderable.y1 + (charH * charScaleY);

		charRenderable.u1 += (charW * (charIndex % charColumns));
		charRenderable.v1 += (charH * (charIndex / charColumns));
		charRenderable.u2 = charRenderable.u1 + charW;
		charRenderable.v2 = charRenderable.v1 + charH;

		JE_TRACE("jeLuaClient_drawText(): drawing renderable, %s", jeRenderable_toDebugString(&renderable));
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

jeBool jeLuaClient_addBindings(lua_State* lua) {
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

	jeBool success = JE_FALSE;
	int createdResult = 0;

	JE_DEBUG("jeLuaClient_addBindings()");

	createdResult = luaL_newmetatable(lua, "jeClientMetatable");
	if (createdResult != 1) {
		JE_ERROR("jeLuaClient_addBindings(): luaL_newmetatable() failed, result=%d, metatableName=%s", createdResult, JE_LUA_CLIENT_BINDINGS_KEY);
		goto finalize;
	}

	luaL_setfuncs(lua, clientBindings, /*numUpvalues*/ 0);
	lua_pushvalue(lua, JE_LUA_STACK_TOP);
	lua_setfield(lua, JE_LUA_STACK_TOP, "__index");

	lua_pushvalue(lua, JE_LUA_STACK_TOP);
	lua_setglobal(lua, JE_LUA_CLIENT_BINDINGS_KEY);

	lua_createtable(lua, /*numArrayElems*/ 0, /*numNonArrayElems*/ 16);
	lua_setfield(lua, JE_LUA_STACK_TOP - 1, "state");

	jeLuaClient_updateStates(lua);

	success = JE_TRUE;
	finalize: {
		lua_settop(lua, 0);
	}

	return success;
}
jeBool jeLuaClient_run(jeWindow* window, const char* filename) {
	jeBool success = JE_FALSE;
	int luaResponse = 0;
	lua_State* lua = NULL;

	JE_INFO("jeLuaClient_run()");

	lua = luaL_newstate();
	if (lua == NULL) {
		JE_ERROR("jeLuaClient_run(): luaL_newstate() failed");
		goto finalize;
	}

	luaL_openlibs(lua);

	jeLuaClient_addWindow(lua, window);

	if (jeLuaClient_addBindings(lua) == JE_FALSE) {
		JE_ERROR("jeLuaClient_run(): jeLuaClient_addBindings() failed");
		goto finalize;
	}

	luaResponse = luaL_loadfile(lua, filename);
	if (luaResponse != 0) {
		JE_ERROR("jeLuaClient_run(): luaL_loadfile() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLuaClient_getError(lua));
		goto finalize;
	}

	luaResponse = lua_pcall(lua, /*numArgs*/ 0, /*numReturnVals*/ LUA_MULTRET, /*errFunc*/ 0);
	if (luaResponse != 0) {
		JE_ERROR("jeLuaClient_run(): lua_pcall() failed, filename=%s luaResponse=%d error=%s", filename, luaResponse, jeLuaClient_getError(lua));
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
		if (lua != NULL) {
			lua_close(lua);
		}
	}

	return success;
}
