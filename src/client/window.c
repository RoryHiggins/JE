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

typedef struct jeVertex jeVertex;

typedef struct jeController jeController;

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

struct jeController {
	SDL_Scancode keys[JE_INPUT_COUNT];
	SDL_Scancode altKeys[JE_INPUT_COUNT];
};

struct jeWindow {
	jeBool open;
	Uint32 nextFrameTimeMs;
	jeImage image;
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

void jeController_setBindings(jeController* controller) {
	memset((void*)controller, 0, sizeof(*controller));
	controller->keys[JE_INPUT_LEFT] = SDL_GetScancodeFromKey(SDLK_LEFT);
	controller->altKeys[JE_INPUT_LEFT] = SDL_GetScancodeFromKey(SDLK_a);

	controller->keys[JE_INPUT_RIGHT] = SDL_GetScancodeFromKey(SDLK_RIGHT);
	controller->altKeys[JE_INPUT_RIGHT] = SDL_GetScancodeFromKey(SDLK_d);

	controller->keys[JE_INPUT_UP] = SDL_GetScancodeFromKey(SDLK_UP);
	controller->altKeys[JE_INPUT_UP] = SDL_GetScancodeFromKey(SDLK_w);

	controller->keys[JE_INPUT_DOWN] = SDL_GetScancodeFromKey(SDLK_DOWN);
	controller->altKeys[JE_INPUT_DOWN] = SDL_GetScancodeFromKey(SDLK_s);

	controller->keys[JE_INPUT_A] = SDL_GetScancodeFromKey(SDLK_RETURN);
	controller->altKeys[JE_INPUT_A] = SDL_GetScancodeFromKey(SDLK_z);

	controller->keys[JE_INPUT_B] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
	controller->altKeys[JE_INPUT_B] = SDL_GetScancodeFromKey(SDLK_x);

	controller->keys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_c);
	controller->altKeys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_q);

	controller->keys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_v);
	controller->altKeys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_e);
}

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

	/*cleanup:*/ {
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

	/*cleanup:*/ {
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
		goto cleanup;
	}

	if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
		JE_ERR("jeWindow_create(): SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
		goto cleanup;
	}

	if (SDL_GL_SetSwapInterval(0) < 0) {
		JE_ERR("jeWindow_create(): SDL_GL_SetSwapInterval() failed to disable vsync, error=%s", SDL_GetError());
	}

	glewExperimental = JE_TRUE;
	if (glewInit() != GLEW_OK) {
		JE_ERR("jeWindow_create(): glewInit() failed");
		goto cleanup;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glDisable(GL_CULL_FACE);

	glViewport(0, 0, jeWindow_getWidth(window), jeWindow_getHeight(window));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}

	window->vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(window->vertShader, 1, &jeWindow_vertShaderPtr, &jeWindow_vertShaderSize);
	glCompileShader(window->vertShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}
	if (!jeWindow_getShaderCompiledOk(window, window->vertShader, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getShaderCompiledOk() failed");
		goto cleanup;
	}

	window->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(window->fragShader, 1, &jeWindow_fragShaderPtr, &jeWindow_fragShaderSize);
	glCompileShader(window->fragShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}
	if (!jeWindow_getShaderCompiledOk(window, window->fragShader, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getShaderCompiledOk() failed");
		goto cleanup;
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
		goto cleanup;
	}
	if (!jeWindow_getProgramLinkedOk(window, window->program, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getProgramLinkedOk() failed");
		goto cleanup;
	}

	glGenTextures(1, &window->texture);
	glBindTexture(GL_TEXTURE_2D, window->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->image.width, window->image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, window->image.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}

	glGenBuffers(1, &window->vbo);

	glGenVertexArrays(1, &window->vao);

	glBindVertexArray(window->vao);
	glBindBuffer(GL_ARRAY_BUFFER, window->vbo);
	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)0);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(3 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(7 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		JE_ERR("jeWindow_create(): jeWindow_getGlOk() failed");
		goto cleanup;
	}

	glBindVertexArray(0);

	success = JE_TRUE;
	cleanup: {
	}

	return success;
}
void jeWindow_step(jeWindow* window) {
	/*Wait for frame start time*/
	static const Uint32 frameTimeMs = 1000 / JE_WINDOW_FRAME_RATE;

	SDL_Event event;
	Uint32 timeMs = SDL_GetTicks();

	jeWindow_flushVertexBuffer(window);
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
				JE_DEBUG("jeWindow_step(): Quit event received");
				window->open = JE_FALSE;
				break;
			}
			case SDL_KEYUP: {
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE: {
						JE_DEBUG("jeWindow_step(): Escape key released");
						window->open = JE_FALSE;
						break;
					}
					default: break;
				}
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

	jeImage_destroy(&window->image);

	if (window->window != NULL) {
		SDL_DestroyWindow(window->window);
		window->window = NULL;
	}

	memset((void*)window, 0, sizeof(*window));
}
jeBool jeWindow_create(jeWindow* window) {
	jeBool success = JE_FALSE;

	memset((void*)window, 0, sizeof(*window));

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		JE_ERR("jeWindow_create(): SDL_Init() failed with error=%s", SDL_GetError());
		goto cleanup;
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
		goto cleanup;
	}

	if (jeImage_createFromFile(&window->image, JE_WINDOW_SPRITE_FILENAME) == JE_FALSE) {
		JE_ERR("jeWindow_create(): jeImage_createFromFile() failed");
		goto cleanup;
	}

	if (jeWindow_initGL(window) == JE_FALSE) {
		JE_ERR("jeWindow_create(): jeWindow_initGL() failed");
		goto cleanup;
	}

	jeController_setBindings(&window->controller);
	window->keyState = SDL_GetKeyboardState(NULL);

	window->open = JE_TRUE;

	success = JE_TRUE;
	cleanup: {
	}

	return success;
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

jeBool jeWindow_getInput(jeWindow* window, int inputId) {
	const Uint8* keyState = window->keyState;
	SDL_Scancode* keys = window->controller.keys;
	SDL_Scancode* altKeys = window->controller.altKeys;

	return (keyState[keys[inputId]] || keyState[altKeys[inputId]]);
}
int jeWindow_getFramesPerSecond(jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}
void jeWindow_drawSprite(jeWindow* window, float z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {

	static const GLuint spriteVertexCount = 6;
	float scaleX = (2.0f * (float)JE_WINDOW_SCALE) / (float)jeWindow_getWidth(window);
	float scaleY = (2.0f * (float)JE_WINDOW_SCALE) / (float)jeWindow_getHeight(window);

	if ((window->vboVertexCount + spriteVertexCount) >= JE_WINDOW_VERTEX_BUFFER_CAPACITY) {
		jeWindow_flushVertexBuffer(window);
	}

	/* Render sprites as two clockwise triangles:
		0 1   - 4
		2 -   3 5
	*/

	x1 = (x1 * scaleX);
	x2 = (x2 * scaleX);
	y1 = (-y1 * scaleY);
	y2 = (-y2 * scaleY);
	z = z / (float)(1 << 20);
	r = r / 256.0f;
	g = g / 256.0f;
	b = b / 256.0f;
	a = a / 256.0f;
	u1 = u1 / (float)window->image.width;
	u2 = u2 / (float)window->image.width;
	v1 = v1 / (float)window->image.height;
	v2 = v2 / (float)window->image.height;

	window->vboData[window->vboVertexCount].x = x1;
	window->vboData[window->vboVertexCount].y = y1;
	window->vboData[window->vboVertexCount].z = z;
	window->vboData[window->vboVertexCount].r = r;
	window->vboData[window->vboVertexCount].g = g;
	window->vboData[window->vboVertexCount].b = b;
	window->vboData[window->vboVertexCount].a = a;
	window->vboData[window->vboVertexCount].u = u1;
	window->vboData[window->vboVertexCount].v = v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = x2;
	window->vboData[window->vboVertexCount].y = y1;
	window->vboData[window->vboVertexCount].z = z;
	window->vboData[window->vboVertexCount].r = r;
	window->vboData[window->vboVertexCount].g = g;
	window->vboData[window->vboVertexCount].b = b;
	window->vboData[window->vboVertexCount].a = a;
	window->vboData[window->vboVertexCount].u = u2;
	window->vboData[window->vboVertexCount].v = v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = x1;
	window->vboData[window->vboVertexCount].y = y2;
	window->vboData[window->vboVertexCount].z = z;
	window->vboData[window->vboVertexCount].r = r;
	window->vboData[window->vboVertexCount].g = g;
	window->vboData[window->vboVertexCount].b = b;
	window->vboData[window->vboVertexCount].a = a;
	window->vboData[window->vboVertexCount].u = u1;
	window->vboData[window->vboVertexCount].v = v2;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = x1;
	window->vboData[window->vboVertexCount].y = y2;
	window->vboData[window->vboVertexCount].z = z;
	window->vboData[window->vboVertexCount].r = r;
	window->vboData[window->vboVertexCount].g = g;
	window->vboData[window->vboVertexCount].b = b;
	window->vboData[window->vboVertexCount].a = a;
	window->vboData[window->vboVertexCount].u = u1;
	window->vboData[window->vboVertexCount].v = v2;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = x2;
	window->vboData[window->vboVertexCount].y = y1;
	window->vboData[window->vboVertexCount].z = z;
	window->vboData[window->vboVertexCount].r = r;
	window->vboData[window->vboVertexCount].g = g;
	window->vboData[window->vboVertexCount].b = b;
	window->vboData[window->vboVertexCount].a = a;
	window->vboData[window->vboVertexCount].u = u2;
	window->vboData[window->vboVertexCount].v = v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = x2;
	window->vboData[window->vboVertexCount].y = y2;
	window->vboData[window->vboVertexCount].z = z;
	window->vboData[window->vboVertexCount].r = r;
	window->vboData[window->vboVertexCount].g = g;
	window->vboData[window->vboVertexCount].b = b;
	window->vboData[window->vboVertexCount].a = a;
	window->vboData[window->vboVertexCount].u = u2;
	window->vboData[window->vboVertexCount].v = v2;
	window->vboVertexCount++;
}
