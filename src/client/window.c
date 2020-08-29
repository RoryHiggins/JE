#include "core.h"
#include "image.h"
#include "window.h"

#define JE_WINDOW_CAPTION ""
#define JE_WINDOW_WIDTH 640
#define JE_WINDOW_HEIGHT 480
#define JE_WINDOW_SCALE 4
#define JE_WINDOW_FRAME_RATE 60
#define JE_WINDOW_SPRITE_FILENAME "data/sprites.png"
#define JE_WINDOW_VERTEX_BUFFER_CAPACITY 8192

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
		"col = srcCol;" \
		"uv = srcUv;" \
		"gl_Position = vec4(srcPos, 1.0);" \
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


struct jeWindow {
	bool open;
	Uint32 nextFrameTimeMs;
	jeImage image;
	SDL_Window* window;

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
bool jeWindow_getGlOk(jeWindow* window, const char* file, unsigned line, const char* function) {
	bool ok = true;
	GLenum glError = GL_NO_ERROR;

	for (glError = glGetError(); glError != GL_NO_ERROR; glError = glGetError()) {
		jeLog_logPrefixImpl(JE_LOG_ERR_LABEL, file, line);
		jeLog_logImpl("%s(): OpenGL error, glError=%d, message=%s", function, glError, gluErrorString(glError));
		ok = false;
	}

	JE_MAYBE_UNUSED(window);

	return ok;
}
bool jeWindow_getShaderCompiledOk(jeWindow* window, GLuint shader, const char* file, unsigned line, const char* function) {
	bool ok = true;
	GLint compileStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;
	void* buffer = NULL;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msgMaxSize);
		buffer = malloc(msgMaxSize);

		glGetShaderInfoLog(shader, msgMaxSize, NULL, (GLchar*)buffer);

		jeLog_logPrefixImpl(JE_LOG_ERR_LABEL, file, line);
		jeLog_logImpl("%s(): OpenGL shader compilation failed, error=\n%s", function, (const char*)buffer);

		ok = false;
	}

	/*cleanup:*/ {
		if (buffer != NULL) {
			free(buffer);
		}
	}

	JE_MAYBE_UNUSED(window);

	return ok;
}
bool jeWindow_getProgramLinkedOk(jeWindow* window, GLuint program, const char* file, unsigned line, const char* function) {
	bool ok = true;
	GLint linkStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;
	void* buffer = NULL;

	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &msgMaxSize);
		buffer = malloc(msgMaxSize);

		glGetProgramInfoLog(program, msgMaxSize, NULL, (GLchar*)buffer);

		jeLog_logPrefixImpl(JE_LOG_ERR_LABEL, file, line);
		jeLog_logImpl("%s(): OpenGL program linking failed, error=\n%s", function, (const char*)buffer);

		ok = false;
	}

	/*cleanup:*/ {
		if (buffer != NULL) {
			free(buffer);
		}
	}

	JE_MAYBE_UNUSED(window);

	return ok;
}
bool jeWindow_isOpen(jeWindow* window) {
	return window->open;
}
void jeWindow_destroy(jeWindow* window) {
	JE_MAYBE_UNUSED(window);

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

	if (window->window != NULL) {
		SDL_DestroyWindow(window->window);
		window->window = NULL;
	}

	SDL_Quit();

	jeImage_destroy(&window->image);

	window->open = false;

	memset(window, 0, sizeof(*window));
}
bool jeWindow_create(jeWindow* window) {
	bool success = false;

	memset(window, 0, sizeof(*window));

	if (!jeImage_createFromFile(&window->image, JE_WINDOW_SPRITE_FILENAME)) {
		goto cleanup;
	}

	if (SDL_Init(SDL_INIT_VIDEO)) {
		JE_ERR("jeWindow_create(): SDL_Init() failed with error=%s", SDL_GetError());
		goto cleanup;
	}

	/*SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);*/

	/*FOR RENDERDOC TESTING ONLY*/
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	window->window = SDL_CreateWindow(
		JE_WINDOW_CAPTION,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		JE_WINDOW_WIDTH,
		JE_WINDOW_HEIGHT,
		/*flags*/ SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
	);
	if (window->window == NULL) {
		JE_ERR("jeWindow_create(): SDL_CreateWindow() failed with error=%s", SDL_GetError());
		goto cleanup;
	}

	window->context = SDL_GL_CreateContext(window->window);
	if (window->context == 0) {
		JE_ERR("jeWindow_create(): SDL_GL_CreateContext() failed with error=%s", SDL_GetError());
		goto cleanup;
	}

	if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
		JE_ERR("jeWindow_create(): SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
		goto cleanup;
	}

	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		JE_ERR("jeWindow_create(): glewInit() failed");
		goto cleanup;
	}

	if (SDL_GL_SetSwapInterval(1) < 0) {
		JE_ERR("jeWindow_create(): SDL_GL_SetSwapInterval() failed to enable vsync, error=%s", SDL_GetError());
	}

	glDisable(GL_DEPTH_TEST);
	/*glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);*/
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, JE_WINDOW_WIDTH, JE_WINDOW_HEIGHT);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	window->vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(window->vertShader, 1, &jeWindow_vertShaderPtr, &jeWindow_vertShaderSize);
	glCompileShader(window->vertShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")
		|| !jeWindow_getShaderCompiledOk(window, window->vertShader, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	window->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(window->fragShader, 1, &jeWindow_fragShaderPtr, &jeWindow_fragShaderSize);
	glCompileShader(window->fragShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")
		|| !jeWindow_getShaderCompiledOk(window, window->fragShader, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	window->program = glCreateProgram();
	glAttachShader(window->program, window->vertShader);
	glAttachShader(window->program, window->fragShader);
	glBindAttribLocation(window->program, 0, "srcPos");
	glBindAttribLocation(window->program, 1, "srcCol");
	glBindAttribLocation(window->program, 2, "srcUv");
	glLinkProgram(window->program);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")
		|| !jeWindow_getProgramLinkedOk(window, window->program, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	glGenTextures(1, &window->texture);
	glBindTexture(GL_TEXTURE_2D, window->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->image.width, window->image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, window->image.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	glGenBuffers(1, &window->vbo);

	glGenVertexArrays(1, &window->vao);

	glBindVertexArray(window->vao);
	glBindBuffer(GL_ARRAY_BUFFER, window->vbo);
	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)0);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(3 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(7 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_create()")) {
		goto cleanup;
	}

	glBindVertexArray(0);

	window->open = true;

	success = true;
	cleanup: {
	}

	return success;
}

bool jeWindow_getInput(jeWindow* window, int inputId) {
	const Uint8* keyState = SDL_GetKeyboardState(NULL);

	switch (inputId) {
		case JE_INPUT_LEFT: {
			return keyState[SDL_GetScancodeFromKey(SDLK_LEFT)] || keyState[SDL_GetScancodeFromKey(SDLK_a)];
		}
		case JE_INPUT_RIGHT: {
			return keyState[SDL_GetScancodeFromKey(SDLK_RIGHT)] || keyState[SDL_GetScancodeFromKey(SDLK_d)];
		}
		case JE_INPUT_UP: {
			return keyState[SDL_GetScancodeFromKey(SDLK_UP)] || keyState[SDL_GetScancodeFromKey(SDLK_w)];
		}
		case JE_INPUT_DOWN: {
			return keyState[SDL_GetScancodeFromKey(SDLK_DOWN)] || keyState[SDL_GetScancodeFromKey(SDLK_s)];
		}
		case JE_INPUT_A: {
			return keyState[SDL_GetScancodeFromKey(SDLK_RETURN)] || keyState[SDL_GetScancodeFromKey(SDLK_z)];
		}
		case JE_INPUT_B: {
			return keyState[SDL_GetScancodeFromKey(SDLK_BACKSPACE)] || keyState[SDL_GetScancodeFromKey(SDLK_x)];
		}
		case JE_INPUT_X: {
			return keyState[SDL_GetScancodeFromKey(SDLK_c)];
		}
		case JE_INPUT_Y: {
			return keyState[SDL_GetScancodeFromKey(SDLK_v)];
		}
	}

	JE_MAYBE_UNUSED(window);

	return false;
}
unsigned jeWindow_getFramesPerSecond(jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}
void jeWindow_flushVertexBuffer(jeWindow* window) {
	/*TODO remove temp testing*/
	/*window->vboVertexCount = 0;
	window->vboData[window->vboVertexCount].x = -1.0f;
	window->vboData[window->vboVertexCount].y = -1.0f;
	window->vboData[window->vboVertexCount].z = 0.0f;
	window->vboData[window->vboVertexCount].r = 1.0f;
	window->vboData[window->vboVertexCount].g = 1.0f;
	window->vboData[window->vboVertexCount].b = 1.0f;
	window->vboData[window->vboVertexCount].a = 1.0f;
	window->vboData[window->vboVertexCount].u = 0.0f;
	window->vboData[window->vboVertexCount].v = 0.0f;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = 1.0f;
	window->vboData[window->vboVertexCount].y = -1.0f;
	window->vboData[window->vboVertexCount].z = 0.0f;
	window->vboData[window->vboVertexCount].r = 1.0f;
	window->vboData[window->vboVertexCount].g = 1.0f;
	window->vboData[window->vboVertexCount].b = 1.0f;
	window->vboData[window->vboVertexCount].a = 1.0f;
	window->vboData[window->vboVertexCount].u = 0.0f;
	window->vboData[window->vboVertexCount].v = 0.0f;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = -1.0f;
	window->vboData[window->vboVertexCount].y = 1.0f;
	window->vboData[window->vboVertexCount].z = 0.0f;
	window->vboData[window->vboVertexCount].r = 1.0f;
	window->vboData[window->vboVertexCount].g = 1.0f;
	window->vboData[window->vboVertexCount].b = 1.0f;
	window->vboData[window->vboVertexCount].a = 1.0f;
	window->vboData[window->vboVertexCount].u = 0.0f;
	window->vboData[window->vboVertexCount].v = 0.0f;
	window->vboVertexCount++;*/

	glUseProgram(window->program);
	glBindVertexArray(window->vao);

	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, window->vboVertexCount);

	glBindVertexArray(0);
	glUseProgram(0);

	jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_flushVertexBuffer()");

	window->vboVertexCount = 0;
	memset(window->vboData, 0, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex));
}
void jeWindow_drawSprite(jeWindow* window, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {

	static const GLuint spriteVertexCount = 6;
	static const float scaleX = (2.0f * (float)JE_WINDOW_SCALE) / (float)JE_WINDOW_WIDTH;
	static const float scaleY = (2.0f * (float)JE_WINDOW_SCALE) / (float)JE_WINDOW_HEIGHT;

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
				JE_LOG("jeWindow_step(): Quit event received");
				window->open = false;
				break;
			}
			case SDL_KEYUP: {
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE: {
						JE_LOG("jeWindow_step(): Escape key released");
						window->open = false;
						break;
					}
					default: break;
				}
				break;
			}
		}
	}

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
