// ---------------- Client ----------------
// Includes
	// precompiled header
	#include "stdafx.h"

	#if defined(__cplusplus)
	extern "C" {
	#endif  // END defined(__cplusplus)

// Logging
	#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

	#if defined(NDEBUG)
	#define JE_DEBUG false
	#define JE_LOG(...)
	#define JE_ERR(...)
	#else
	#define JE_DEBUG true
	#define JE_LOG(...) jeLog_logImpl("LOG", __FILE__, __LINE__, __VA_ARGS__)
	#define JE_ERR(...) jeLog_logImpl("ERR", __FILE__, __LINE__, __VA_ARGS__)
	#endif

	void jeLog_logImpl(const char* label, const char* file, unsigned line, const char* formatStr, ...) {
		va_list args;

		fprintf(stdout, "[%s %s:%d] ", label, file, line);

		va_start(args, formatStr);
		vfprintf(stdout, formatStr, args);
		va_end(args);

		fputc('\n', stdout);
		fflush(stdout);
	}

// Rendering
	#if !defined(JE_HEADLESS)
	typedef struct {
		int z;
		sfVertexArray* vertexArray;
	} jeVertexArray;
	int jeVertexArray_compare(const void* a, const void* b) {
		return ((const jeVertexArray*)a)->z - ((const jeVertexArray*)b)->z;
	}
	bool jeVertexArray_create(jeVertexArray* vertexArray, int z) {
		vertexArray->z = z;
		vertexArray->vertexArray = sfVertexArray_create();
		sfVertexArray_setPrimitiveType(vertexArray->vertexArray, sfTriangles);

		return true;
	}
	void jeVertexArray_destroy(jeVertexArray* vertexArray) {
		if (vertexArray->vertexArray) {
			sfVertexArray_destroy(vertexArray->vertexArray);
			vertexArray->vertexArray = NULL;
		}
	}

	typedef struct {
		jeVertexArray* vertexArrays;

		int capacity;
		int count;
	} jeRenderQueue;
	jeVertexArray jeRenderQueue_get(jeRenderQueue* renderQueue, int z) {
		jeVertexArray vertexArray;
		vertexArray.z = z;
		jeVertexArray* vertexArrayPtr = (jeVertexArray*)bsearch(&vertexArray, renderQueue->vertexArrays, renderQueue->count, sizeof(jeVertexArray), &jeVertexArray_compare);

		if (vertexArrayPtr != NULL) {
			vertexArray = *vertexArrayPtr;
			goto cleanup;
		}

		if (renderQueue->count >= renderQueue->capacity) {
			int newCapacity = (renderQueue->capacity * 2) + 1;
			renderQueue->vertexArrays = (jeVertexArray*)realloc((void*)renderQueue->vertexArrays, sizeof(jeVertexArray) * newCapacity);
			renderQueue->capacity = newCapacity;
		}

		jeVertexArray_create(&renderQueue->vertexArrays[renderQueue->count], z);
		vertexArray = renderQueue->vertexArrays[renderQueue->count];
		renderQueue->count++;
		qsort((void*)renderQueue->vertexArrays, renderQueue->count, sizeof(jeVertexArray), &jeVertexArray_compare);

		cleanup: {
		}

		return vertexArray;
	}
	void jeRenderQueue_queueSprite(jeRenderQueue* renderQueue, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {
		jeVertexArray vertexArray;
		size_t vertexIndex;
		sfVertex* vertex;

		vertexArray = jeRenderQueue_get(renderQueue, z);
		vertexIndex = sfVertexArray_getVertexCount(vertexArray.vertexArray);
		sfVertexArray_resize(vertexArray.vertexArray, vertexIndex + 6);

		/*
		Render sprite via two triangles.  Triangle vertex indices are clockwise:

			0  1
			2  -

			-  4
			3  5
		*/

		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 0);
		vertex->position.x = x1;
		vertex->position.y = y1;
		vertex->texCoords.x = u1;
		vertex->texCoords.y = v1;

		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 1);
		vertex->position.x = x2;
		vertex->position.y = y1;
		vertex->texCoords.x = u2;
		vertex->texCoords.y = v1;

		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 2);
		vertex->position.x = x1;
		vertex->position.y = y2;
		vertex->texCoords.x = u1;
		vertex->texCoords.y = v2;

		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 3);
		vertex->position.x = x1;
		vertex->position.y = y2;
		vertex->texCoords.x = u1;
		vertex->texCoords.y = v2;

		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 4);
		vertex->position.x = x2;
		vertex->position.y = y1;
		vertex->texCoords.x = u2;
		vertex->texCoords.y = v1;

		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 5);
		vertex->position.x = x2;
		vertex->position.y = y2;
		vertex->texCoords.x = u2;
		vertex->texCoords.y = v2;

		for (int i = 0; i < 6; i++) {
			vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + i);
			vertex->color.r = r;
			vertex->color.g = g;
			vertex->color.b = b;
			vertex->color.a = a;
		}
	}
	void jeRenderQueue_draw(jeRenderQueue* renderQueue, sfRenderWindow *renderWindow, const sfRenderStates *renderStates) {
		// draw layers back-to-front (painters algorithm)
		for (int i = renderQueue->count - 1; i >= 0; i--) {
			sfRenderWindow_drawVertexArray(renderWindow, renderQueue->vertexArrays[i].vertexArray, renderStates);
		}
	}
	bool jeRenderQueue_create(jeRenderQueue* renderQueue) {
		const int startCapacity = 16;
		renderQueue->vertexArrays = (jeVertexArray*)malloc(sizeof(jeVertexArray) * startCapacity);
		renderQueue->count = 0;
		renderQueue->capacity = startCapacity;

		return true;
	}
	void jeRenderQueue_destroy(jeRenderQueue* renderQueue) {
		if (renderQueue->vertexArrays) {
			for (int i = 0; i < renderQueue->count; i++) {
				jeVertexArray_destroy(&renderQueue->vertexArrays[i]);
			}
			free((void*)renderQueue->vertexArrays);
			renderQueue->vertexArrays = NULL;

			renderQueue->count = 0;
			renderQueue->capacity = 0;
		}
	}
	#endif  // END !defined(JE_HEADLESS)

// Window
	#if !defined(JE_HEADLESS)
	#define JE_WINDOW_WIDTH 640
	#define JE_WINDOW_HEIGHT 480
	#define JE_WINDOW_SCALE 4
	#define JE_WINDOW_SPRITES "sprites.png"

	typedef struct {
		sfRenderWindow* window;
		sfRenderStates renderStates;
		sfTexture* spriteTexture;
		// TODO remove in favor of a render queue
		sfSprite* sprite;
		jeRenderQueue renderQueue;

		bool closing;
	} jeWindow;

	jeWindow* jeWindow_get() {
		static jeWindow window;
		return &window;
	}
	bool jeWindow_isOpen(jeWindow* window) {
		return (window->window != NULL) && !window->closing;
	}
	void jeWindow_destroy(jeWindow* window) {
		JE_LOG("jeWindow_destroy()");

		if (window->sprite != NULL) {
			sfSprite_destroy(window->sprite);
			window->sprite = NULL;
		}

		if (window->spriteTexture != NULL) {
			sfTexture_destroy(window->spriteTexture);
			window->spriteTexture = NULL;
		}

		if (window->window != NULL) {
			sfRenderWindow_destroy(window->window);
			window->window = NULL;
		}
	}
	bool jeWindow_create(jeWindow* window) {
		bool success = false;
		sfVideoMode videoMode;

		JE_LOG("jeWindow_create()");

		videoMode.width = JE_WINDOW_WIDTH;
		videoMode.height = JE_WINDOW_HEIGHT;
		videoMode.bitsPerPixel = 32;

		window->window = sfRenderWindow_create(videoMode, "", sfTitlebar | sfClose, NULL);
		if (window->window == NULL) {
			JE_ERR("jeWindow_create(): sfRenderWindow_create() failed");
			goto cleanup;
		}

		sfRenderWindow_setFramerateLimit(window->window, 30);
		sfRenderWindow_clear(window->window, sfWhite);

		window->renderStates.blendMode = sfBlendAlpha;
		window->renderStates.transform = sfTransform_Identity;
		window->renderStates.texture = NULL;
		window->renderStates.shader = NULL;
		sfTransform_scale(&window->renderStates.transform, (float)JE_WINDOW_SCALE, (float)JE_WINDOW_SCALE);

		window->spriteTexture = sfTexture_createFromFile(JE_WINDOW_SPRITES, /*area*/ NULL);
		if (window->spriteTexture == NULL) {
			JE_ERR("jeWindow_create(): sfTexture_createFromFile() failed");
			goto cleanup;
		}
		window->sprite = sfSprite_create();
		if (window->sprite == NULL) {
			JE_ERR("jeWindow_create(): sfSprite_create() failed");
			goto cleanup;
		}
		sfSprite_setTexture(window->sprite, window->spriteTexture, /* resetRect */ true);
		window->renderStates.texture = window->spriteTexture;

		if (!jeRenderQueue_create(&window->renderQueue)) {
			goto cleanup;
		}

		window->closing = false;

		success = true;
		cleanup: {
			if (!success) {
				jeWindow_destroy(window);
			}
		}

		return success;
	}
	void jeWindow_step(jeWindow* window) {
		sfEvent event;

		while (sfRenderWindow_pollEvent(window->window, &event)) {
			if (event.type == sfEvtClosed) {
				window->closing = true;
			}
		}

		if (sfKeyboard_isKeyPressed(sfKeyEscape)) {
			window->closing = true;
		}

		jeRenderQueue_draw(&window->renderQueue, window->window, &window->renderStates);

		sfRenderWindow_display(window->window);
		sfRenderWindow_clear(window->window, sfWhite);
	}
	#endif  // END !defined(JE_HEADLESS)

// Lua
	// https://www.lua.org/manual/5.3/manual.html
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
	int jeLuaClient_isRunning(lua_State* lua) {
		bool isRunning = false;

		#if !defined(JE_HEADLESS)
			isRunning = jeWindow_isOpen(jeWindow_get());
		#endif  // END !defined(JE_HEADLESS)

		lua_pushboolean(lua, isRunning);

		return 1;  // num return values
	}
	int jeLuaClient_step(lua_State* lua) {
		JE_MAYBE_UNUSED(lua);

		#if !defined(JE_HEADLESS)
			jeWindow_step(jeWindow_get());
		#endif  // END !defined(JE_HEADLESS)

		return 0;  // num return values
	}
	int jeLuaClient_drawSprite(lua_State* lua) {
		#if !defined(JE_HEADLESS)
			jeWindow* window;
		#endif  // END !defined(JE_HEADLESS)

		// screen
		float screenX1;
		float screenY1;
		float screenX2;
		float screenY2;

		// sprite
		float x1;
		float y1;
		float x2;
		float y2;
		int z;
		float r;
		float g;
		float b;
		float a;
		float u1;
		float v1;
		float u2;
		float v2;


		// screen
		luaL_checktype(lua, 2, LUA_TTABLE);
		lua_getfield(lua, 2, "x");
		screenX1 = luaL_checknumber(lua, -1);
		lua_getfield(lua, 2, "y");
		screenY1 = luaL_checknumber(lua, -1);

		lua_getfield(lua, 2, "w");
		screenX2 = screenX1 + luaL_checknumber(lua, -1);
		lua_getfield(lua, 2, "h");
		screenY2 = screenY1 + luaL_checknumber(lua, -1);

		// sprite
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
		z = (int)luaL_checknumber(lua, -1);

		lua_getfield(lua, 1, "r");
		r =  luaL_checknumber(lua, -1);
		lua_getfield(lua, 1, "g");
		g = luaL_checknumber(lua, -1);
		lua_getfield(lua, 1, "b");
		b =  luaL_checknumber(lua, -1);
		lua_getfield(lua, 1, "a");
		a = luaL_checknumber(lua, -1);

		lua_getfield(lua, 1, "u1");
		u1 =  luaL_checknumber(lua, -1);
		lua_getfield(lua, 1, "v1");
		v1 = luaL_checknumber(lua, -1);

		lua_getfield(lua, 1, "u2");
		u2 = luaL_checknumber(lua, -1);
		lua_getfield(lua, 1, "v2");
		v2 = luaL_checknumber(lua, -1);

		if ((a == 0)
			|| (x1 > screenX2)
			|| (y1 > screenY2)
			|| (y2 <= screenY1)
			|| (x2 <= screenX1)
			|| (y2 <= screenY1)
			|| (u1 == u2)
			|| (v1 == u2)
			|| (x1 == x2)
			|| (y1 == y2)) {
			goto cleanup;
		}

		x1 += screenX1;
		x2 += screenX1;
		y1 += screenY1;
		y2 += screenY1;

		#if !defined(JE_HEADLESS)
			window = jeWindow_get();
			jeRenderQueue_queueSprite(&window->renderQueue, z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);
		#else  // END !defined(JE_HEADLESS)
			JE_MAYBE_UNUSED(screenX1);
			JE_MAYBE_UNUSED(screenY1);
			JE_MAYBE_UNUSED(x1);
			JE_MAYBE_UNUSED(y1);
			JE_MAYBE_UNUSED(x2);
			JE_MAYBE_UNUSED(y2);
			JE_MAYBE_UNUSED(z);
			JE_MAYBE_UNUSED(r);
			JE_MAYBE_UNUSED(g);
			JE_MAYBE_UNUSED(b);
			JE_MAYBE_UNUSED(a);
			JE_MAYBE_UNUSED(u1);
			JE_MAYBE_UNUSED(v1);
			JE_MAYBE_UNUSED(u2);
			JE_MAYBE_UNUSED(v2);
		#endif

		cleanup: {

		}

		return 0;  // num return values
	}

// Game
	#define JE_GAME_FILENAME "game.lua"

	typedef struct {
		lua_State* lua;
	} jeGame;

	bool jeGame_registerLuaClientBindings(jeGame* game) {
		bool success = false;
		int created = 0;

		static const luaL_Reg clientBindings[] = {
			JE_LUA_CLIENT_BINDING(isRunning),
			JE_LUA_CLIENT_BINDING(step),
			JE_LUA_CLIENT_BINDING(drawSprite),
			{NULL, NULL}  // sentinel value
		};

		JE_LOG("jeGame_registerLuaClientBindings()");

		created = luaL_newmetatable(game->lua, "jeGameMetatable");
		if (created != 1) {
			JE_ERR("jeGame_registerLuaClientBindings(): luaL_newmetatable() failed, result=%d, metatableName=%s", created, JE_LUA_CLIENT_BINDINGS_KEY);
			goto cleanup;
		}

		luaL_setfuncs(game->lua, clientBindings, /* num upvalues */ 0);
		lua_pushvalue(game->lua, -1);
		lua_setfield(game->lua, -1, "__index");
		lua_setglobal(game->lua, JE_LUA_CLIENT_BINDINGS_KEY);


		success = true;
		cleanup: {
			lua_settop(game->lua, 0);
		}

		return success;
	}
	bool jeGame_create(jeGame* game) {
		bool success = false;

		JE_LOG("jeGame_create()");

		game->lua = NULL;

		#if !defined(JE_HEADLESS)
			if (!jeWindow_create(jeWindow_get())) {
				goto cleanup;
			}
		#endif  // END !defined(JE_HEADLESS)

		game->lua = luaL_newstate();
		if (game->lua == NULL) {
			JE_ERR("jeGame_create(): luaL_newstate() failed");
			goto cleanup;
		}

		luaL_openlibs(game->lua);

		if (!jeGame_registerLuaClientBindings(game)) {
			goto cleanup;
		}

		success = true;
		cleanup: {
		}
		return success;
	}
	void jeGame_destroy(jeGame* game) {
		if (game->lua != NULL) {
			lua_close(game->lua);
			game->lua = NULL;
		}

		#if !defined(JE_HEADLESS)
			jeWindow_destroy(jeWindow_get());
		#endif  // END !defined(JE_HEADLESS)
	}
	bool jeGame_run(jeGame* game) {
		bool success = false;
		int response = 0;

		if (!jeGame_create(game)) {
			goto cleanup;
		}

		response = luaL_loadfile(game->lua, JE_GAME_FILENAME);
		if (response != 0) {
			JE_ERR("jeGame_create(): luaL_loadfile() failed, filename=%s response=%d error=%s", JE_GAME_FILENAME, response, jeLua_getError(game->lua));
			goto cleanup;
		}

		response = lua_pcall(game->lua, /* num args */ 0, /* num return vals */ LUA_MULTRET, /* err func */ 0);
		if (response != 0) {
			JE_ERR("jeGame_create(): lua_pcall() failed, filename=%s response=%d error=%s", JE_GAME_FILENAME, response, jeLua_getError(game->lua));
			goto cleanup;
		}

		success = true;
		cleanup: {
			jeGame_destroy(game);
		}

		return success;
	}

// ----------------- Main -----------------
// Main
	int main(int argc, char** argv) {
		bool success = true;
		jeGame game;

		JE_MAYBE_UNUSED(argc);
		JE_MAYBE_UNUSED(argv);

		success = jeGame_run(&game);

		return success ? EXIT_SUCCESS : EXIT_FAILURE;
	}
	void jeMain(int argc, char** argv) {
		main(argc, argv);
	}

	#if defined(__cplusplus)
	}  // END extern "C"
	#endif  // END defined(__cplusplus)