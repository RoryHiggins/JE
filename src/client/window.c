#include "stdafx.h"
#include "window.h"
#include "debug.h"
#include "image.h"

#define JE_WINDOW_CAPTION ""
#define JE_WINDOW_SCALE 8
#define JE_WINDOW_WIDTH (160 * JE_WINDOW_SCALE)
#define JE_WINDOW_HEIGHT (120 * JE_WINDOW_SCALE)
#define JE_WINDOW_FRAME_RATE 60
#define JE_WINDOW_SPRITE_FILENAME "data/sprites.png"
#define JE_WINDOW_CONTROLLER_DB_FILENAME "data/gamecontrollerdb.txt"
#define JE_WINDOW_VERTEX_BUFFER_CAPACITY 16 * 1024

/*https://www.khronos.org/registry/OpenGL/specs/gl/glspec21.pdf*/
/*https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.20.pdf*/
#define JE_WINDOW_VERT_SHADER \
	"#version 120\n" \
	\
	"attribute vec3 srcPos;" \
	"attribute vec4 srcCol;" \
	"attribute vec2 srcUv;" \
	\
	"varying vec4 col;" \
	"varying vec2 uv;" \
	\
	"void main() {" \
		"col = srcCol /*- vec4(srcPos, 0)*/;" \
		"uv = srcUv;" \
		"gl_Position = vec4(srcPos, 1);" \
	"}"
#define JE_WINDOW_FRAG_SHADER \
	"#version 120\n" \
	\
	"uniform sampler2D srcTexture;" \
	\
	"varying vec4 col;" \
	"varying vec2 uv;" \
	\
	"void main() {" \
		"gl_FragColor = texture2D(srcTexture, uv).rgba * col;" \
	"}"


static const GLchar *jeWindow_vertShaderPtr = JE_WINDOW_VERT_SHADER;
static const GLchar *jeWindow_fragShaderPtr = JE_WINDOW_FRAG_SHADER;
static const GLint jeWindow_vertShaderSize = sizeof(JE_WINDOW_VERT_SHADER);
static const GLint jeWindow_fragShaderSize = sizeof(JE_WINDOW_FRAG_SHADER);

struct jeVertex {
	float x;
	float y;
	float z;

	float r;
	float g;
	float b;
	float a;

	float u;
	float v;
};
typedef struct jeVertex jeVertex;

struct jeSprite {
	float z;

	float x1;
	float y1;
	float x2;
	float y2;

	float r;
	float g;
	float b;
	float a;

	float u1;
	float v1;
	float u2;
	float v2;
};
typedef struct jeSprite jeSprite;
int jeSprite_less(const void* a, const void* b) {
	return ((const jeSprite*)a)->z < ((const jeSprite*)b)->z;
}

/*Z-sorted queue of sprites, used for sorting translucent sprites before drawing*/
struct jeSpriteDepthQueue {
	jeSprite* sprites;
	int capacity;
	int count;
};
typedef struct jeSpriteDepthQueue jeSpriteDepthQueue;
void jeSpriteDepthQueue_destroy(jeSpriteDepthQueue* spriteDepthQueue) {
	spriteDepthQueue->count = 0;
	spriteDepthQueue->capacity = 0;

	if (spriteDepthQueue->sprites != NULL) {
		free(spriteDepthQueue->sprites);
		spriteDepthQueue->sprites = NULL;
	}
}
void jeSpriteDepthQueue_create(jeSpriteDepthQueue* spriteDepthQueue) {
	spriteDepthQueue->sprites = NULL;
	spriteDepthQueue->capacity = 0;
	spriteDepthQueue->count = 0;
}
void jeSpriteDepthQueue_setCapacity(jeSpriteDepthQueue* spriteDepthQueue, int capacity) {
	JE_DEBUG("jeSpriteDepthQueue_setCapacity(): newCapacity=%d, currentCapacity=%d", capacity, spriteDepthQueue->capacity);

	if (capacity == spriteDepthQueue->capacity) {
		goto finalize;
	}

	if (capacity == 0) {
		jeSpriteDepthQueue_destroy(spriteDepthQueue);
		goto finalize;
	}

	if (spriteDepthQueue->sprites == NULL) {
		spriteDepthQueue->sprites = (jeSprite*)malloc(sizeof(jeSprite) * capacity);
	} else {
		spriteDepthQueue->sprites = (jeSprite*)realloc(spriteDepthQueue->sprites, sizeof(jeSprite) * capacity);
	}

	spriteDepthQueue->capacity = capacity;

	if (spriteDepthQueue->count > capacity) {
		spriteDepthQueue->count = capacity;
	}

	finalize: {
	}
}
void jeSpriteDepthQueue_insert(jeSpriteDepthQueue* spriteDepthQueue, jeSprite sprite) {
	static const int startCapacity = 32;

	int newCapacity = 0;

	if (spriteDepthQueue->count >= spriteDepthQueue->capacity) {
		newCapacity = startCapacity;
		if (spriteDepthQueue->capacity >= startCapacity) {
			newCapacity = spriteDepthQueue->capacity * 4;
		}
		jeSpriteDepthQueue_setCapacity(spriteDepthQueue, newCapacity);
	}

	spriteDepthQueue->sprites[spriteDepthQueue->count] = sprite;
	spriteDepthQueue->count++;
}
void jeSpriteDepthQueue_sort(jeSpriteDepthQueue* spriteDepthQueue) {
	qsort(spriteDepthQueue->sprites, spriteDepthQueue->count, sizeof(jeSprite), jeSprite_less);
}

struct jeController {
	SDL_GameController* controller;

	SDL_Scancode keys[JE_INPUT_COUNT];
	SDL_Scancode altKeys[JE_INPUT_COUNT];
	SDL_GameControllerButton controllerButtons[JE_INPUT_COUNT];
	SDL_GameControllerAxis controllerAxis[JE_INPUT_COUNT];
	float controllerAxisDir[JE_INPUT_COUNT];

	float controllerAxisThreshold;
};
typedef struct jeController jeController;
void jeController_destroy(jeController* controller) {
	if (controller->controller != NULL) {
		SDL_GameControllerClose(controller->controller);
		controller->controller = NULL;
	}
}
void jeController_create(jeController* controller) {
	int i = 0;

	memset((void*)controller, 0, sizeof(*controller));
	controller->keys[JE_INPUT_LEFT] = SDL_GetScancodeFromKey(SDLK_LEFT);
	controller->altKeys[JE_INPUT_LEFT] = SDL_GetScancodeFromKey(SDLK_a);
	controller->controllerButtons[JE_INPUT_LEFT] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
	controller->controllerAxis[JE_INPUT_LEFT] = SDL_CONTROLLER_AXIS_LEFTX;
	controller->controllerAxisDir[JE_INPUT_LEFT] = -1.0f;

	controller->keys[JE_INPUT_RIGHT] = SDL_GetScancodeFromKey(SDLK_RIGHT);
	controller->altKeys[JE_INPUT_RIGHT] = SDL_GetScancodeFromKey(SDLK_d);
	controller->controllerButtons[JE_INPUT_RIGHT] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
	controller->controllerAxis[JE_INPUT_RIGHT] = SDL_CONTROLLER_AXIS_LEFTX;
	controller->controllerAxisDir[JE_INPUT_RIGHT] = 1.0f;

	controller->keys[JE_INPUT_UP] = SDL_GetScancodeFromKey(SDLK_UP);
	controller->altKeys[JE_INPUT_UP] = SDL_GetScancodeFromKey(SDLK_w);
	controller->controllerButtons[JE_INPUT_UP] = SDL_CONTROLLER_BUTTON_DPAD_UP;
	controller->controllerAxis[JE_INPUT_UP] = SDL_CONTROLLER_AXIS_LEFTY;
	controller->controllerAxisDir[JE_INPUT_UP] = -1.0f;

	controller->keys[JE_INPUT_DOWN] = SDL_GetScancodeFromKey(SDLK_DOWN);
	controller->altKeys[JE_INPUT_DOWN] = SDL_GetScancodeFromKey(SDLK_s);
	controller->controllerButtons[JE_INPUT_DOWN] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
	controller->controllerAxis[JE_INPUT_DOWN] = SDL_CONTROLLER_AXIS_LEFTY;
	controller->controllerAxisDir[JE_INPUT_DOWN] = 1.0f;

	controller->keys[JE_INPUT_A] = SDL_GetScancodeFromKey(SDLK_RETURN);
	controller->altKeys[JE_INPUT_A] = SDL_GetScancodeFromKey(SDLK_z);
	controller->controllerButtons[JE_INPUT_A] = SDL_CONTROLLER_BUTTON_A;
	controller->controllerAxis[JE_INPUT_A] = SDL_CONTROLLER_AXIS_INVALID;
	controller->controllerAxisDir[JE_INPUT_A] = 0.0f;

	controller->keys[JE_INPUT_B] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
	controller->altKeys[JE_INPUT_B] = SDL_GetScancodeFromKey(SDLK_x);
	controller->controllerButtons[JE_INPUT_B] = SDL_CONTROLLER_BUTTON_B;
	controller->controllerAxis[JE_INPUT_B] = SDL_CONTROLLER_AXIS_INVALID;
	controller->controllerAxisDir[JE_INPUT_B] = 0.0f;

	controller->keys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_c);
	controller->altKeys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_q);
	controller->controllerButtons[JE_INPUT_X] = SDL_CONTROLLER_BUTTON_X;
	controller->controllerAxis[JE_INPUT_X] = SDL_CONTROLLER_AXIS_INVALID;
	controller->controllerAxisDir[JE_INPUT_X] = 0.0f;

	controller->keys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_v);
	controller->altKeys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_e);
	controller->controllerButtons[JE_INPUT_Y] = SDL_CONTROLLER_BUTTON_Y;
	controller->controllerAxis[JE_INPUT_Y] = SDL_CONTROLLER_AXIS_INVALID;
	controller->controllerAxisDir[JE_INPUT_Y] = 0.0f;

	JE_DEBUG("jeController_create(): numJoysticks=%d", SDL_NumJoysticks());
	for (i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			JE_DEBUG("jeController_create(): index=%d", i);
			controller->controller = SDL_GameControllerOpen(i);
			break;
		}
	}
	if (controller->controller == NULL) {
		JE_LOG("jeController_create(): No compatible game controller found", i);
	}

	controller->controllerAxisThreshold = 0.1;
}

struct jeWindow {
	jeBool open;
	Uint32 nextFrameTimeMs;
	jeImage image;
	jeSpriteDepthQueue spriteDepthQueue;
	SDL_Window* window;

	jeController controller;
	const Uint8* keyState;

	SDL_GLContext context;
	GLuint vertShader;
	GLuint fragShader;
	GLuint program;
	GLuint texture;
	GLuint vbo;
	GLuint vao;

	jeVertex vboData[JE_WINDOW_VERTEX_BUFFER_CAPACITY];
	GLuint vboVertexCount;
};
jeWindow* jeWindow_get() {
	static jeWindow window;
	return &window;
}
jeBool jeWindow_getGlOk(jeWindow* window, const char* file, int line, const char* function) {
	jeBool ok = JE_TRUE;
	GLenum glError = GL_NO_ERROR;

	for (glError = glGetError(); glError != GL_NO_ERROR; glError = glGetError()) {
#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_logPrefixImpl(JE_LOG_LABEL_ERR, file, line);
		jeLogger_logImpl("%s(): OpenGL error, glError=%d, message=%s", function, glError, gluErrorString(glError));
#endif
		ok = JE_FALSE;
	}

	JE_MAYBE_UNUSED(window);

	return ok;
}
jeBool jeWindow_getShaderCompiledOk(jeWindow* window, GLuint shader, const char* file, int line, const char* function) {
	jeBool ok = JE_TRUE;
	GLint compileStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;
	void* buffer = NULL;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msgMaxSize);
		buffer = malloc(msgMaxSize);

		glGetShaderInfoLog(shader, msgMaxSize, NULL, (GLchar*)buffer);

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_logPrefixImpl(JE_LOG_LABEL_ERR, file, line);
		jeLogger_logImpl("%s(): OpenGL shader compilation failed, error=\n%s", function, (const char*)buffer);
#endif

		ok = JE_FALSE;
	}

	/*finalize:*/ {
		if (buffer != NULL) {
			free(buffer);
		}
	}

	JE_MAYBE_UNUSED(window);

	return ok;
}
jeBool jeWindow_getProgramLinkedOk(jeWindow* window, GLuint program, const char* file, int line, const char* function) {
	jeBool ok = JE_TRUE;
	GLint linkStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;
	void* buffer = NULL;

	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &msgMaxSize);
		buffer = malloc(msgMaxSize);

		glGetProgramInfoLog(program, msgMaxSize, NULL, (GLchar*)buffer);

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_logPrefixImpl(JE_LOG_LABEL_ERR, file, line);
		jeLogger_logImpl("%s(): OpenGL program linking failed, error=\n%s", function, (const char*)buffer);
#endif

		ok = JE_FALSE;
	}

	/*finalize:*/ {
		if (buffer != NULL) {
			free(buffer);
		}
	}

	JE_MAYBE_UNUSED(window);

	return ok;
}
jeBool jeWindow_isOpen(jeWindow* window) {
	return window->open;
}
int jeWindow_getWidth(jeWindow* window) {
	int width = 0;
	int height = 0;

	SDL_GetWindowSize(window->window, &width, &height);

	return width;
}
int jeWindow_getHeight(jeWindow* window) {
	int width = 0;
	int height = 0;

	SDL_GetWindowSize(window->window, &width, &height);

	return height;
}
void jeWindow_flushVertexBuffer(jeWindow* window) {
	glUseProgram(window->program);
	glBindVertexArray(window->vao);

	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, window->vboVertexCount);

	glBindVertexArray(0);
	glUseProgram(0);

	jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_flushVertexBuffer()");

	window->vboVertexCount = 0;
	memset((void*)window->vboData, 0, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex));
}
void jeWindow_drawSpriteImpl(jeWindow* window, jeSprite sprite) {
	static const GLuint spriteVertexCount = 6;

	float worldScaleX = 0.0f;
	float worldScaleY = 0.0f;

	/*Flush vertex buffer if there is not enough space for this sprite*/
	if ((window->vboVertexCount + spriteVertexCount) >= JE_WINDOW_VERTEX_BUFFER_CAPACITY) {
		jeWindow_flushVertexBuffer(window);
	}

	/*Transform pos from world coords (+/- windowSize) to screen coords (-1.0 to 1.0)*/
	worldScaleX = (2.0f * (float)JE_WINDOW_SCALE) / (float)jeWindow_getWidth(window);
	worldScaleY = (2.0f * (float)JE_WINDOW_SCALE) / (float)jeWindow_getHeight(window);
	sprite.x1 = (sprite.x1 * worldScaleX);
	sprite.x2 = (sprite.x2 * worldScaleX);
	sprite.y1 = (-sprite.y1 * worldScaleY);
	sprite.y2 = (-sprite.y2 * worldScaleY);
	sprite.z = sprite.z / (float)(1 << 20);

	/*Normalize pixel uvs to between 0.0 and 1.0*/
	sprite.u1 = sprite.u1 / (float)window->image.width;
	sprite.u2 = sprite.u2 / (float)window->image.width;
	sprite.v1 = sprite.v1 / (float)window->image.height;
	sprite.v2 = sprite.v2 / (float)window->image.height;

	/* Render sprites as two triangles represented by 6 clockwise vertices.
	 *
	 * Triangle 1 indices:
	 * 0 1
	 * 2
	 *
	 * Triangle 2 indices:
	 *   4
	 * 3 5
	*/

	window->vboData[window->vboVertexCount].x = sprite.x1;
	window->vboData[window->vboVertexCount].y = sprite.y1;
	window->vboData[window->vboVertexCount].z = sprite.z;
	window->vboData[window->vboVertexCount].r = sprite.r;
	window->vboData[window->vboVertexCount].g = sprite.g;
	window->vboData[window->vboVertexCount].b = sprite.b;
	window->vboData[window->vboVertexCount].a = sprite.a;
	window->vboData[window->vboVertexCount].u = sprite.u1;
	window->vboData[window->vboVertexCount].v = sprite.v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = sprite.x2;
	window->vboData[window->vboVertexCount].y = sprite.y1;
	window->vboData[window->vboVertexCount].z = sprite.z;
	window->vboData[window->vboVertexCount].r = sprite.r;
	window->vboData[window->vboVertexCount].g = sprite.g;
	window->vboData[window->vboVertexCount].b = sprite.b;
	window->vboData[window->vboVertexCount].a = sprite.a;
	window->vboData[window->vboVertexCount].u = sprite.u2;
	window->vboData[window->vboVertexCount].v = sprite.v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = sprite.x1;
	window->vboData[window->vboVertexCount].y = sprite.y2;
	window->vboData[window->vboVertexCount].z = sprite.z;
	window->vboData[window->vboVertexCount].r = sprite.r;
	window->vboData[window->vboVertexCount].g = sprite.g;
	window->vboData[window->vboVertexCount].b = sprite.b;
	window->vboData[window->vboVertexCount].a = sprite.a;
	window->vboData[window->vboVertexCount].u = sprite.u1;
	window->vboData[window->vboVertexCount].v = sprite.v2;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = sprite.x1;
	window->vboData[window->vboVertexCount].y = sprite.y2;
	window->vboData[window->vboVertexCount].z = sprite.z;
	window->vboData[window->vboVertexCount].r = sprite.r;
	window->vboData[window->vboVertexCount].g = sprite.g;
	window->vboData[window->vboVertexCount].b = sprite.b;
	window->vboData[window->vboVertexCount].a = sprite.a;
	window->vboData[window->vboVertexCount].u = sprite.u1;
	window->vboData[window->vboVertexCount].v = sprite.v2;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = sprite.x2;
	window->vboData[window->vboVertexCount].y = sprite.y1;
	window->vboData[window->vboVertexCount].z = sprite.z;
	window->vboData[window->vboVertexCount].r = sprite.r;
	window->vboData[window->vboVertexCount].g = sprite.g;
	window->vboData[window->vboVertexCount].b = sprite.b;
	window->vboData[window->vboVertexCount].a = sprite.a;
	window->vboData[window->vboVertexCount].u = sprite.u2;
	window->vboData[window->vboVertexCount].v = sprite.v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = sprite.x2;
	window->vboData[window->vboVertexCount].y = sprite.y2;
	window->vboData[window->vboVertexCount].z = sprite.z;
	window->vboData[window->vboVertexCount].r = sprite.r;
	window->vboData[window->vboVertexCount].g = sprite.g;
	window->vboData[window->vboVertexCount].b = sprite.b;
	window->vboData[window->vboVertexCount].a = sprite.a;
	window->vboData[window->vboVertexCount].u = sprite.u2;
	window->vboData[window->vboVertexCount].v = sprite.v2;
	window->vboVertexCount++;
}
void jeWindow_drawSprite(jeWindow* window, float z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {
	jeSprite sprite;
	sprite.z = z;
	sprite.x1 = x1;
	sprite.y1 = y1;
	sprite.x2 = x2;
	sprite.y2 = y2;
	sprite.r = r;
	sprite.g = g;
	sprite.b = b;
	sprite.a = a;
	sprite.u1 = u1;
	sprite.v1 = v1;
	sprite.u2 = u2;
	sprite.v2 = v2;

	/*If this is a translucent sprite, defer drawing to end of frame, after sorting all translucent sprites*/
	if ((a > 0.0f) && (a < 1.0f)) {
		jeSpriteDepthQueue_insert(&window->spriteDepthQueue, sprite);
		goto finalize;
	}

	jeWindow_drawSpriteImpl(window, sprite);

	finalize: {
	}
}
void jeWindow_flushSpriteDepthQueue(jeWindow* window) {
	int i = 0;
	jeSprite sprite;

	jeSpriteDepthQueue_sort(&window->spriteDepthQueue);

	for (i = (window->spriteDepthQueue.count - 1); i >= 0; i--) {
		sprite = window->spriteDepthQueue.sprites[i];
		jeWindow_drawSpriteImpl(window, sprite);
	}
	window->spriteDepthQueue.count = 0;
}
void jeWindow_destroyGL(jeWindow* window) {
	if (window->vao != 0) {
		glBindVertexArray(window->vao);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteVertexArrays(1, &window->vao);
		window->vao = 0;
	}

	glBindVertexArray(0);

	if (window->vbo != 0) {
		glDeleteBuffers(1, &window->vbo);
		window->vbo = 0;
	}

	if (window->texture != 0) {
		glDeleteTextures(1, &window->texture);
		window->texture = 0;
	}

	if (window->program != 0) {
		glDetachShader(window->program, window->fragShader);
		glDetachShader(window->program, window->vertShader);
		glDeleteProgram(window->program);
		window->program = 0;
	}

	if (window->fragShader != 0) {
		glDeleteShader(window->fragShader);
		window->fragShader = 0;
	}

	if (window->vertShader != 0) {
		glDeleteShader(window->vertShader);
		window->vertShader = 0;
	}

	if (window->context != NULL) {
		SDL_GL_DeleteContext(window->context);
		window->context = NULL;
	}
}
jeBool jeWindow_initGL(jeWindow* window) {
	jeBool success = JE_FALSE;

	window->context = SDL_GL_CreateContext(window->window);
	if (window->context == 0) {
		JE_ERR("jeWindow_create(): SDL_GL_CreateContext() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
		JE_ERR("jeWindow_create(): SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	if (SDL_GL_SetSwapInterval(0) < 0) {
		JE_ERR("jeWindow_create(): SDL_GL_SetSwapInterval() failed to disable vsync, error=%s", SDL_GetError());
	}

	glewExperimental = JE_TRUE;
	if (glewInit() != GLEW_OK) {
		JE_ERR("jeWindow_create(): glewInit() failed");
		goto finalize;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glDisable(GL_CULL_FACE);

	glViewport(0, 0, jeWindow_getWidth(window), jeWindow_getHeight(window));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	window->vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(window->vertShader, 1, &jeWindow_vertShaderPtr, &jeWindow_vertShaderSize);
	glCompileShader(window->vertShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}
	if (!jeWindow_getShaderCompiledOk(window, window->vertShader, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getShaderCompiledOk() failed");
		goto finalize;
	}

	window->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(window->fragShader, 1, &jeWindow_fragShaderPtr, &jeWindow_fragShaderSize);
	glCompileShader(window->fragShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}
	if (!jeWindow_getShaderCompiledOk(window, window->fragShader, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getShaderCompiledOk() failed");
		goto finalize;
	}

	window->program = glCreateProgram();
	glAttachShader(window->program, window->vertShader);
	glAttachShader(window->program, window->fragShader);
	glBindAttribLocation(window->program, 0, "srcPos");
	glBindAttribLocation(window->program, 1, "srcCol");
	glBindAttribLocation(window->program, 2, "srcUv");
	glLinkProgram(window->program);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}
	if (!jeWindow_getProgramLinkedOk(window, window->program, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getProgramLinkedOk() failed");
		goto finalize;
	}

	glGenTextures(1, &window->texture);
	glBindTexture(GL_TEXTURE_2D, window->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->image.width, window->image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, window->image.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glGenBuffers(1, &window->vbo);

	glGenVertexArrays(1, &window->vao);

	glBindVertexArray(window->vao);
	glBindBuffer(GL_ARRAY_BUFFER, window->vbo);
	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)0);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(3 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(7 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glBindVertexArray(0);

	success = JE_TRUE;
	finalize: {
	}

	return success;
}
void jeWindow_step(jeWindow* window) {
	/*Wait for frame start time*/
	static const Uint32 frameTimeMs = 1000 / JE_WINDOW_FRAME_RATE;

	SDL_Event event;
	Uint32 timeMs = SDL_GetTicks();

	jeWindow_flushVertexBuffer(window);

	glDepthMask(GL_FALSE);
	jeWindow_flushSpriteDepthQueue(window);
	jeWindow_flushVertexBuffer(window);
	glDepthMask(GL_TRUE);

	SDL_GL_SwapWindow(window->window);

	if ((timeMs + 1) < window->nextFrameTimeMs) {
		/*If we end up with a nextFrameTime > wait time (e.g. via integer overflow), clamp*/
		if ((window->nextFrameTimeMs - timeMs) > frameTimeMs) {
			window->nextFrameTimeMs = timeMs + frameTimeMs;
		}

		SDL_Delay(frameTimeMs - 1);
		timeMs = SDL_GetTicks();
	}
	window->nextFrameTimeMs = timeMs + frameTimeMs;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				JE_LOG("jeWindow_step(): Quit event received");
				window->open = JE_FALSE;
				break;
			}
			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED: {
						JE_DEBUG(
							"jeWindow_step(): resizing window to width=%d, height=%d",
							jeWindow_getWidth(window),
							jeWindow_getHeight(window)
						);
						jeWindow_destroyGL(window);
						if (!jeWindow_initGL(window)) {
							JE_ERR("jeWindow_step(): jeWindow_initGL failed");
							window->open = JE_FALSE;
						}
						break;
					}
				}
				break;
			}
			case SDL_KEYUP: {
				if (event.key.repeat == 0) {
					JE_DEBUG("jeWindow_step(): SDL_KEYUP, key=%s", SDL_GetKeyName(event.key.keysym.sym));
				}

				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE: {
						JE_LOG("jeWindow_step(): Escape key released");
						window->open = JE_FALSE;
						break;
					}
					default: {
						break;
					}
				}

				break;
			}
			case SDL_KEYDOWN: {
				if (event.key.repeat == 0) {
					JE_DEBUG("jeWindow_step(): SDL_KEYDOWN, key=%s", SDL_GetKeyName(event.key.keysym.sym));
				}
				break;
			}
			case SDL_CONTROLLERDEVICEADDED: {
				JE_LOG("jeWindow_step(): SDL_CONTROLLERDEVICEADDED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED: {
				JE_LOG("jeWindow_step(): SDL_CONTROLLERDEVICEREMOVED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERDEVICEREMAPPED: {
				JE_DEBUG("jeWindow_step(): SDL_CONTROLLERDEVICEREMAPPED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERBUTTONDOWN: {
				JE_DEBUG("jeWindow_step(): SDL_CONTROLLERBUTTONDOWN, button=%s",
						 SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
				break;
			}
			case SDL_CONTROLLERBUTTONUP: {
				JE_DEBUG("jeWindow_step(): SDL_CONTROLLERBUTTONUP, button=%s",
						 SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
				break;
			}
			case SDL_CONTROLLERAXISMOTION: {
				/*Left commented out as this can be useful but is too verbose even for verbose logging...*/
				/*JE_DEBUG("jeWindow_step(): SDL_CONTROLLERAXISMOTION, axis=%s, value=%d",
						 SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis),
						 (int)event.caxis.value);*/
				break;
			}
			default: {
				break;
			}
		}
	}

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	window->keyState = SDL_GetKeyboardState(NULL);
}
void jeWindow_destroy(jeWindow* window) {
	window->open = JE_FALSE;

	SDL_Quit();

	jeWindow_destroyGL(window);

	jeSpriteDepthQueue_destroy(&window->spriteDepthQueue);

	jeController_destroy(&window->controller);

	jeImage_destroy(&window->image);

	if (window->window != NULL) {
		SDL_DestroyWindow(window->window);
		window->window = NULL;
	}

	memset((void*)window, 0, sizeof(*window));
}
jeBool jeWindow_create(jeWindow* window) {
	jeBool success = JE_FALSE;

	int controllerMappingsLoaded = -1;

	memset((void*)window, 0, sizeof(*window));

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		JE_ERR("jeWindow_create(): SDL_Init() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	/*define OpenGL 3.2 context for RenderDoc support when debugging*/
#if defined(JE_BUILD_DEBUG)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

	window->window = SDL_CreateWindow(
		JE_WINDOW_CAPTION,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		JE_WINDOW_WIDTH,
		JE_WINDOW_HEIGHT,
		/*flags*/ SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if (window->window == NULL) {
		JE_ERR("jeWindow_create(): SDL_CreateWindow() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	if (jeImage_createFromFile(&window->image, JE_WINDOW_SPRITE_FILENAME) == JE_FALSE) {
		JE_ERR("jeWindow_create(): jeImage_createFromFile() failed");
		goto finalize;
	}

	jeSpriteDepthQueue_create(&window->spriteDepthQueue);

	if (jeWindow_initGL(window) == JE_FALSE) {
		JE_ERR("jeWindow_create(): jeWindow_initGL() failed");
		goto finalize;
	}

	controllerMappingsLoaded = SDL_GameControllerAddMappingsFromFile(JE_WINDOW_CONTROLLER_DB_FILENAME);
	if (controllerMappingsLoaded == -1) {
		JE_ERR("jeWindow_create(): SDL_GameControllerAddMappingsFromFile() failed with error=%s", SDL_GetError());
	} else {
		JE_DEBUG("jeWindow_create(): SDL_GameControllerAddMappingsFromFile() controllerMappingsLoaded=%d", controllerMappingsLoaded);
	}

	jeController_create(&window->controller);
	window->keyState = SDL_GetKeyboardState(NULL);

	window->open = JE_TRUE;

	success = JE_TRUE;
	finalize: {
	}

	return success;
}
jeBool jeWindow_getInput(jeWindow* window, int inputId) {
	static const float axisMaxValue = 32767;

	jeBool pressed = JE_FALSE;

	SDL_Scancode key = window->controller.keys[inputId];
	SDL_Scancode altKey = window->controller.altKeys[inputId];
	SDL_GameControllerButton controllerButton = SDL_CONTROLLER_BUTTON_INVALID;
	SDL_GameControllerAxis controllerAxis = SDL_CONTROLLER_AXIS_INVALID;
	float controllerAxisDir = 0;
	float axisMinValue = axisMaxValue / window->controller.controllerAxisThreshold;

	if (window->controller.controller != NULL) {
		controllerButton = window->controller.controllerButtons[inputId];
		controllerAxis = window->controller.controllerAxis[inputId];
		controllerAxisDir = window->controller.controllerAxisDir[inputId];
	}

	if (key != SDL_SCANCODE_UNKNOWN) {
		pressed = pressed || window->keyState[key];
	}

	if (altKey != SDL_SCANCODE_UNKNOWN) {
		pressed = pressed || window->keyState[altKey];
	}

	if (controllerButton != SDL_CONTROLLER_BUTTON_INVALID) {
		pressed = pressed || SDL_GameControllerGetButton(window->controller.controller, controllerButton);
	}

	if (controllerAxis != SDL_CONTROLLER_AXIS_INVALID) {
		float axisValue = (float)SDL_GameControllerGetAxis(window->controller.controller, controllerAxis) * controllerAxisDir;
		pressed = pressed || (axisValue > axisMinValue);
	}

	return pressed;
}
int jeWindow_getFramesPerSecond(jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}