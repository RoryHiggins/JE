#include <j25/platform/window.h>

#include <j25/core/common.h>
#include <j25/core/container.h>
#include <j25/platform/image.h>
#include <j25/platform/rendering.h>

#define GLEW_STATIC
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <GL/glu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define JE_CONTROLLER_DB_FILENAME "client/data/gamecontrollerdb.txt"

#define JE_WINDOW_FRAME_RATE 60
#define JE_WINDOW_START_SCALE 4
#define JE_WINDOW_START_WIDTH (JE_WINDOW_MIN_WIDTH * JE_WINDOW_START_SCALE)
#define JE_WINDOW_START_HEIGHT (JE_WINDOW_MIN_HEIGHT * JE_WINDOW_START_SCALE)
#define JE_WINDOW_START_CAPTION "ld48"

#define JE_GL_MESSAGE_BUFFER_CAPACITY (4 * 1024)

/*https://www.khronos.org/registry/OpenGL/specs/gl/glspec21.pdf*/
/*https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.20.pdf*/
#define JE_WINDOW_VERT_SHADER \
	"#version 120\n" \
\
	"uniform vec3 scaleXyz;" /*Transforms from world (+/- windowSize) to normalized device coords (-1.0 \
								to 1.0)*/ \
	"uniform vec2 scaleUv;"  /*Converts to normalized texture coords (0.0 to 1.0)*/ \
\
	"attribute vec4 srcPos;" \
	"attribute vec4 srcCol;" \
	"attribute vec2 srcUv;" \
\
	"varying vec4 col;" \
	"varying vec2 uv;" \
\
	"void main() {" \
	"gl_Position = vec4(srcPos.xyz * scaleXyz, 1);" \
	"col = srcCol;" \
	"uv = srcUv * scaleUv;" \
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

struct jeSDL {
	bool intialized;
	uint32_t entryCount;
};

struct jeController {
	SDL_GameController* controller;

	SDL_Scancode keys[JE_INPUT_COUNT];
	SDL_Scancode altKeys[JE_INPUT_COUNT];
	SDL_GameControllerButton controllerButtons[JE_INPUT_COUNT];
	SDL_GameControllerAxis controllerAxis[JE_INPUT_COUNT];
	float controllerAxisDir[JE_INPUT_COUNT];

	float controllerAxisThreshold;
};

struct jeWindow {
	bool open;

	Uint64 frame;
	Uint32 fpsEstimate;
	Uint32 fpsLastSampleTimeMs;
	Uint32 nextFrameStartMs;

	struct jeVertexBuffer vertexBuffer;
	struct jeImage image;
	SDL_Window* window;

	struct jeController controller;
	const Uint8* keyState;

	SDL_GLContext context;
	GLuint texture;
	GLuint vertShader;
	GLuint fragShader;
	GLuint program;
	GLuint vbo;
	GLuint vao;
};

bool jeSDL_initReentrant();
void jeSDL_destroyReentrant();

bool jeGl_getOk(struct jeLogger logger);
bool jeGl_getShaderOk(GLuint shader, struct jeLogger logger);
bool jeGl_getProgramOk(GLuint program, struct jeLogger logger);

void jeController_destroy(struct jeController* controller);
void jeController_create(struct jeController* controller);

bool jeWindow_clear(struct jeWindow* window);
bool jeWindow_flushPrimitives(struct jeWindow* window);
void jeWindow_destroyGL(struct jeWindow* window);
bool jeWindow_initGL(struct jeWindow* window);

static struct jeSDL jeSDL_sdl = {false, 0};

static const GLchar* jeWindow_vertShaderPtr = JE_WINDOW_VERT_SHADER;
static const GLchar* jeWindow_fragShaderPtr = JE_WINDOW_FRAG_SHADER;
static const GLint jeWindow_vertShaderSize = sizeof(JE_WINDOW_VERT_SHADER);
static const GLint jeWindow_fragShaderSize = sizeof(JE_WINDOW_FRAG_SHADER);

static char jeGl_messageBuffer[JE_GL_MESSAGE_BUFFER_CAPACITY];

bool jeSDL_initReentrant() {
	bool ok = true;

	JE_TRACE("intialized=%s, entryCount=%u", jeSDL_sdl.intialized ? "true" : "false", jeSDL_sdl.entryCount);

	if (!jeSDL_sdl.intialized) {
		const Uint32 sdl_init_flags =
			(SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC |
			 SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);

		JE_TRACE("SDL_Init");
		if (SDL_Init(sdl_init_flags) != 0) {
			JE_ERROR("SDL_Init() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	jeSDL_sdl.intialized = true;
	jeSDL_sdl.entryCount++;

	return ok;
}
void jeSDL_destroyReentrant() {
	JE_TRACE("intialized=%s, entryCount=%u", jeSDL_sdl.intialized ? "true" : "false", jeSDL_sdl.entryCount);

	if (jeSDL_sdl.entryCount == 0) {
		JE_TRACE("SDL entryCount is already 0");
	} else {
		jeSDL_sdl.entryCount--;
	}

	if ((jeSDL_sdl.entryCount == 0) && jeSDL_sdl.intialized) {
		JE_TRACE("SDL_Quit");

		SDL_Quit();
		jeSDL_sdl.entryCount = 0;
		jeSDL_sdl.intialized = false;
	}
}

bool jeGl_getOk(struct jeLogger logger) {
	bool ok = true;
	GLenum glError = GL_NO_ERROR;

	for (glError = glGetError(); glError != GL_NO_ERROR; glError = glGetError()) {
#if JE_LOG_LEVEL_COMPILED <= JE_LOG_LEVEL_ERR
		jeLogger_log(
			logger, JE_LOG_LEVEL_ERR, "OpenGL error, glError=%u, message=%s", glError, gluErrorString(glError));
#endif
		ok = false;
	}

	JE_MAYBE_UNUSED(logger);

	return ok;
}
bool jeGl_getShaderOk(GLuint shader, struct jeLogger logger) {
	bool ok = true;
	GLint compileStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msgMaxSize);
		if (msgMaxSize > JE_GL_MESSAGE_BUFFER_CAPACITY) {
			msgMaxSize = JE_GL_MESSAGE_BUFFER_CAPACITY;
		}

		glGetShaderInfoLog(shader, msgMaxSize, NULL, (GLchar*)jeGl_messageBuffer);

#if JE_LOG_LEVEL_COMPILED <= JE_LOG_LEVEL_ERR
		jeLogger_log(
			logger, JE_LOG_LEVEL_ERR, "OpenGL shader compilation failed, error=\n%s", (const char*)jeGl_messageBuffer);
#endif

		ok = false;
	}

	JE_MAYBE_UNUSED(logger);

	return ok;
}
bool jeGl_getProgramOk(GLuint program, struct jeLogger logger) {
	bool ok = true;
	GLint linkStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;

	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &msgMaxSize);
		if (msgMaxSize > JE_GL_MESSAGE_BUFFER_CAPACITY) {
			msgMaxSize = JE_GL_MESSAGE_BUFFER_CAPACITY;
		}

		glGetProgramInfoLog(program, msgMaxSize, NULL, (GLchar*)jeGl_messageBuffer);

#if JE_LOG_LEVEL_COMPILED <= JE_LOG_LEVEL_ERR
		jeLogger_log(
			logger, JE_LOG_LEVEL_ERR, "OpenGL program linking failed, error=\n%s", (const char*)jeGl_messageBuffer);
#endif

		ok = false;
	}

	JE_MAYBE_UNUSED(logger);

	return ok;
}

void jeController_destroy(struct jeController* controller) {
	JE_TRACE("controller=%p", (void*)controller);

	if (controller != NULL) {
		if (controller->controller != NULL) {
			SDL_GameControllerClose(controller->controller);
			controller->controller = NULL;
		}

		memset((void*)controller, 0, sizeof(*controller));
		controller = NULL;
	}
}
void jeController_create(struct jeController* controller) {
	JE_TRACE("controller=%p", (void*)controller);

	bool ok = true;

	if (controller == NULL) {
		JE_ERROR("controller=NULL");
		ok = false;
	}

	if (controller != NULL) {
		memset((void*)controller, 0, sizeof(*controller));
	}

	if (ok) {
		/*Set default input mappings*/

		controller->keys[JE_INPUT_LEFT] = SDL_GetScancodeFromKey(SDLK_LEFT);
		controller->keys[JE_INPUT_RIGHT] = SDL_GetScancodeFromKey(SDLK_RIGHT);
		controller->keys[JE_INPUT_UP] = SDL_GetScancodeFromKey(SDLK_UP);
		controller->keys[JE_INPUT_DOWN] = SDL_GetScancodeFromKey(SDLK_DOWN);
		controller->keys[JE_INPUT_A] = SDL_GetScancodeFromKey(SDLK_RETURN);
		controller->keys[JE_INPUT_B] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
		controller->keys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_F1);
		controller->keys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_F2);

		controller->altKeys[JE_INPUT_LEFT] = SDL_GetScancodeFromKey(SDLK_a);
		controller->altKeys[JE_INPUT_RIGHT] = SDL_GetScancodeFromKey(SDLK_d);
		controller->altKeys[JE_INPUT_UP] = SDL_GetScancodeFromKey(SDLK_w);
		controller->altKeys[JE_INPUT_DOWN] = SDL_GetScancodeFromKey(SDLK_s);
		controller->altKeys[JE_INPUT_A] = SDL_GetScancodeFromKey(SDLK_z);
		controller->altKeys[JE_INPUT_B] = SDL_GetScancodeFromKey(SDLK_x);
		controller->altKeys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_c);
		controller->altKeys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_v);

		controller->controllerButtons[JE_INPUT_LEFT] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
		controller->controllerButtons[JE_INPUT_RIGHT] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
		controller->controllerButtons[JE_INPUT_UP] = SDL_CONTROLLER_BUTTON_DPAD_UP;
		controller->controllerButtons[JE_INPUT_DOWN] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
		controller->controllerButtons[JE_INPUT_A] = SDL_CONTROLLER_BUTTON_A;
		controller->controllerButtons[JE_INPUT_B] = SDL_CONTROLLER_BUTTON_B;
		controller->controllerButtons[JE_INPUT_X] = SDL_CONTROLLER_BUTTON_X;
		controller->controllerButtons[JE_INPUT_Y] = SDL_CONTROLLER_BUTTON_Y;

		controller->controllerAxis[JE_INPUT_LEFT] = SDL_CONTROLLER_AXIS_LEFTX;
		controller->controllerAxis[JE_INPUT_RIGHT] = SDL_CONTROLLER_AXIS_LEFTX;
		controller->controllerAxis[JE_INPUT_UP] = SDL_CONTROLLER_AXIS_LEFTY;
		controller->controllerAxis[JE_INPUT_DOWN] = SDL_CONTROLLER_AXIS_LEFTY;
		controller->controllerAxis[JE_INPUT_A] = SDL_CONTROLLER_AXIS_INVALID;
		controller->controllerAxis[JE_INPUT_B] = SDL_CONTROLLER_AXIS_INVALID;
		controller->controllerAxis[JE_INPUT_X] = SDL_CONTROLLER_AXIS_INVALID;
		controller->controllerAxis[JE_INPUT_Y] = SDL_CONTROLLER_AXIS_INVALID;

		controller->controllerAxisDir[JE_INPUT_LEFT] = -1.0F;
		controller->controllerAxisDir[JE_INPUT_RIGHT] = 1.0F;
		controller->controllerAxisDir[JE_INPUT_UP] = -1.0F;
		controller->controllerAxisDir[JE_INPUT_DOWN] = 1.0F;
		controller->controllerAxisDir[JE_INPUT_A] = 0.0F;
		controller->controllerAxisDir[JE_INPUT_B] = 0.0F;
		controller->controllerAxisDir[JE_INPUT_X] = 0.0F;
		controller->controllerAxisDir[JE_INPUT_Y] = 0.0F;

		controller->controllerAxisThreshold = 0.1F;

		/*Find the first connected game controller and use that*/
		JE_DEBUG("numJoysticks=%d", SDL_NumJoysticks());
		for (int i = 0; i < SDL_NumJoysticks(); ++i) {
			if (SDL_IsGameController(i)) {
				JE_DEBUG("index=%d", i);
				controller->controller = SDL_GameControllerOpen(i);
				break;
			}
		}
		if (controller->controller == NULL) {
			JE_DEBUG("No compatible game controller found");
		}
	}
}

bool jeWindow_getIsOpen(const struct jeWindow* window) {
	bool ok = true;
	bool open = false;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		open = window->open;
	}

	JE_TRACE("window=%p, open=%u", (void*)window, (uint32_t)open);

	return open;
}
uint32_t jeWindow_getWidth(const struct jeWindow* window) {
	bool ok = true;
	int width = 0;
	int height = 0;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		SDL_GetWindowSize(window->window, &width, &height);
	}

	JE_TRACE("window=%p, width=%d", (void*)window, width);

	return (uint32_t)width;
}
uint32_t jeWindow_getHeight(const struct jeWindow* window) {
	bool ok = true;
	int width = 0;
	int height = 0;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		SDL_GetWindowSize(window->window, &width, &height);
	}

	JE_TRACE("window=%p, height=%d", (void*)window, height);

	return (uint32_t)height;
}
bool jeWindow_getIsValid(struct jeWindow* window) {
	return jeWindow_getIsOpen(window);
}
bool jeWindow_clear(struct jeWindow* window) {
	bool ok = true;

	JE_TRACE("window=%p", (void*)window);

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
			JE_ERROR("SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		glClearColor(1.0F, 1.0F, 1.0F, 1.0F);
		glClear(GL_COLOR_BUFFER_BIT);

		glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	return ok;
}
void jeWindow_resetPrimitives(struct jeWindow* window) {
	JE_TRACE("window=%p", (void*)window);

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		jeVertexBuffer_reset(&window->vertexBuffer);
	}
}
void jeWindow_pushPrimitive(struct jeWindow* window, const struct jeVertex* vertices, uint32_t primitiveType) {
	JE_TRACE("window=%p, primitiveType=%u", (void*)window, primitiveType);

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (vertices == NULL) {
		JE_ERROR("vertices=NULL");
		ok = false;
	}

	if (jePrimitiveType_getValid(primitiveType) == false) {
		JE_ERROR("primitiveType is not valid");
		ok = false;
	}

	if (ok) {
		jeVertexBuffer_pushPrimitive(&window->vertexBuffer, vertices, primitiveType);
	}
}
bool jeWindow_flushPrimitives(struct jeWindow* window) {
	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		if (window->window == NULL) {
			JE_ERROR("window->window=NULL");
			ok = false;
		}
	}

	if (ok) {
		if (window->context == NULL) {
			JE_ERROR("window->context=NULL");
			ok = false;
		}
	}

	uint32_t vertexCount = 0;
	if (ok) {
		vertexCount = window->vertexBuffer.vertices.count;

		ok = jeVertexBuffer_sort(&window->vertexBuffer, JE_PRIMITIVE_TYPE_TRIANGLES);
	}

	const GLvoid* vertexData = NULL;
	if (ok) {
		glUseProgram(window->program);
		glBindVertexArray(window->vao);

		vertexData = (const GLvoid*)window->vertexBuffer.vertices.data;
		if (vertexData == NULL) {
			JE_ERROR("vertexData=NULL");
			ok = false;
		}
	}

	JE_TRACE("window=%p, vertexCount=%u, vertexData=%p", (void*)window, vertexCount, (void*)vertexData);

	if (ok) {
		if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
			JE_ERROR("SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertexCount * sizeof(struct jeVertex)), vertexData, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

		glBindVertexArray(0);
		glUseProgram(0);

		jeGl_getOk(JE_LOG_CONTEXT);

		jeWindow_resetPrimitives(window);
	}

	return ok;
}
void jeWindow_destroyGL(struct jeWindow* window) {
	bool hasGLContext =
		((window != NULL) && (window->window != NULL) && (window->context != NULL) &&
		 (SDL_GL_MakeCurrent(window->window, window->context) == 0));

	JE_DEBUG("window=%p, hasGLContext=%u", (void*)window, (uint32_t)hasGLContext);

	if (hasGLContext) {
		if (window->vao != 0) {
			JE_TRACE("deleting vao, vao=%u", window->vao);

			glBindVertexArray(window->vao);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDeleteVertexArrays(1, &window->vao);
			window->vao = 0;
		}

		glBindVertexArray(0);

		if (window->vbo != 0) {
			JE_TRACE("deleting vbo, vbo=%u", window->vbo);

			glDeleteBuffers(1, &window->vbo);
			window->vbo = 0;
		}

		if (window->texture != 0) {
			JE_TRACE("deleting texture, texture=%u", window->texture);

			glDeleteTextures(1, &window->texture);
			window->texture = 0;
		}

		if (window->program != 0) {
			JE_TRACE(
				"deleting program, program=%u, vertShader=%u, fragShader=%u",
				window->program,
				window->vertShader,
				window->fragShader);

			glDetachShader(window->program, window->fragShader);
			glDetachShader(window->program, window->vertShader);
			glDeleteProgram(window->program);
			glDeleteShader(window->fragShader);
			glDeleteShader(window->vertShader);
			window->program = 0;
			window->fragShader = 0;
			window->vertShader = 0;
		}
	}

	if ((window != NULL) && (window->context != NULL)) {
		JE_TRACE("deleting context, context=%p", (void*)window->context);

		SDL_GL_DeleteContext(window->context);
		window->context = NULL;
	}
}
bool jeWindow_initGL(struct jeWindow* window) {
	JE_DEBUG("window=%p", (void*)window);

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		if (window->window == NULL) {
			JE_ERROR("window->window=NULL");
			ok = false;
		}
	}

	if (ok) {
		window->context = SDL_GL_CreateContext(window->window);
		if (window->context == NULL) {
			JE_ERROR("SDL_GL_CreateContext() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
			JE_ERROR("SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		if (SDL_GL_SetSwapInterval(1) < 0) {
			JE_INFO("SDL_GL_SetSwapInterval() failed to enable vsync, error=%s", SDL_GetError());
			/*Lack of vsync support is not an error.  We still have a frame timer to limit framerate*/
		}
	}

	if (ok) {
		glewExperimental = true;
		if (glewInit() != GLEW_OK) {
			JE_ERROR("glewInit() failed");
			ok = false;
		}
	}

	if (ok) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);

		glDisable(GL_CULL_FACE);

		glViewport(0, 0, (GLsizei)jeWindow_getWidth(window), (GLsizei)jeWindow_getHeight(window));

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() error");
			ok = false;
		}
	}

	if (ok) {
		window->vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(window->vertShader, 1, &jeWindow_vertShaderPtr, &jeWindow_vertShaderSize);
		glCompileShader(window->vertShader);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() error");
			ok = false;
		}
		if (jeGl_getShaderOk(window->vertShader, JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getShaderOk() error");
			ok = false;
		}
	}

	if (ok) {
		window->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(window->fragShader, 1, &jeWindow_fragShaderPtr, &jeWindow_fragShaderSize);
		glCompileShader(window->fragShader);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() error");
			ok = false;
		}
		if (jeGl_getShaderOk(window->fragShader, JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getShaderOk() error");
			ok = false;
		}
	}

	if (ok) {
		window->program = glCreateProgram();
		glAttachShader(window->program, window->vertShader);
		glAttachShader(window->program, window->fragShader);

		glBindAttribLocation(window->program, 0, "srcPos");
		glBindAttribLocation(window->program, 1, "srcCol");
		glBindAttribLocation(window->program, 2, "srcUv");

		glLinkProgram(window->program);
		glUseProgram(window->program);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() error");
			ok = false;
		}
		if (jeGl_getProgramOk(window->program, JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getProgramOk() error");
			ok = false;
		}
	}

	if (ok) {
		glGenTextures(1, &window->texture);
		glBindTexture(GL_TEXTURE_2D, window->texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			(GLsizei)window->image.width,
			(GLsizei)window->image.height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			window->image.buffer.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glGenBuffers(1, &window->vbo);

		glGenVertexArrays(1, &window->vao);

		glBindVertexArray(window->vao);
		glBindBuffer(GL_ARRAY_BUFFER, window->vbo);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() failed");
			ok = false;
		}
	}

	if (ok) {
		GLfloat scaleXyz[3];
		GLfloat scaleUv[2];

		/*Transforms pos from world coords (+/- windowSize) to normalized device coords (-1.0 to 1.0)*/
		scaleXyz[0] = 2.0F / JE_WINDOW_MIN_WIDTH;
		scaleXyz[1] = -2.0F / JE_WINDOW_MIN_HEIGHT;

		/*Normalize z to between -1.0 and 1.0.  Supports depths +/- 2^20.  Near the precise uint32_t limit for float32*/
		scaleXyz[2] = 1.0F / (float)(1 << 20);

		/*Converts image coords to normalized texture coords (0.0 to 1.0)*/
		scaleUv[0] = 1.0F / (float)(window->image.width ? window->image.width : 1);
		scaleUv[1] = 1.0F / (float)(window->image.height ? window->image.height : 1);

		GLint scaleXyzLocation = glGetUniformLocation(window->program, "scaleXyz");
		GLint scaleUvLocation = glGetUniformLocation(window->program, "scaleUv");

		glUniform3f(scaleXyzLocation, scaleXyz[0], scaleXyz[1], scaleXyz[2]);
		glUniform2f(scaleUvLocation, scaleUv[0], scaleUv[1]);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() failed");
			ok = false;
		}
	}

	if (ok) {
		GLuint srcPosLocation = (GLuint)glGetAttribLocation(window->program, "srcPos");
		GLuint srcColLocation = (GLuint)glGetAttribLocation(window->program, "srcCol");
		GLuint srcUvLocation = (GLuint)glGetAttribLocation(window->program, "srcUv");

		glEnableVertexAttribArray(srcPosLocation);
		glEnableVertexAttribArray(srcColLocation);
		glEnableVertexAttribArray(srcUvLocation);

		glVertexAttribPointer(srcPosLocation, 4, GL_FLOAT, GL_FALSE, sizeof(struct jeVertex), (const GLvoid*)0);
		glVertexAttribPointer(
			srcColLocation, 4, GL_FLOAT, GL_FALSE, sizeof(struct jeVertex), (const GLvoid*)(4 * sizeof(GLfloat)));
		glVertexAttribPointer(
			srcUvLocation, 2, GL_FLOAT, GL_FALSE, sizeof(struct jeVertex), (const GLvoid*)(8 * sizeof(GLfloat)));

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() error");
			ok = false;
		}
	}

	if (ok) {
		glUseProgram(0);
		glBindVertexArray(0);
	}

	if (!ok) {
		jeWindow_destroyGL(window);
		window = NULL;
	}

	return ok;
}
void jeWindow_show(struct jeWindow* window) {
	JE_TRACE("window=%p", (void*)window);

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		if (window->window == NULL) {
			JE_ERROR("window->window=NULL");
			ok = false;
		}
	}

	if (ok) {
		SDL_ShowWindow(window->window);
	}
}
bool jeWindow_step(struct jeWindow* window) {
	JE_TRACE("window=%p", (void*)window);

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		if (window->window == NULL) {
			JE_ERROR("window->window=NULL");
			ok = false;
		}
	}

	SDL_Event event;
	while (ok && SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				JE_DEBUG("Quit event received");
				window->open = false;
				break;
			}
			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED: {
						JE_DEBUG(
							"resizing window to width=%u, height=%u",
							jeWindow_getWidth(window),
							jeWindow_getHeight(window));
						jeWindow_destroyGL(window);
						if (!jeWindow_initGL(window)) {
							JE_ERROR("jeWindow_initGL failed");
							ok = false;
						}
						break;
					}
				}
				break;
			}
			case SDL_KEYUP: {
				if ((event.key.repeat == 0) || (JE_LOG_LEVEL_COMPILED <= JE_LOG_LEVEL_TRACE)) {
					JE_DEBUG("SDL_KEYUP, key=%s", SDL_GetKeyName(event.key.keysym.sym));
				}

				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE: {
						JE_DEBUG("Escape key released");
						window->open = false;
						break;
					}
					default: {
						break;
					}
				}

				break;
			}
			case SDL_KEYDOWN: {
				if ((event.key.repeat == 0) || (JE_LOG_LEVEL_COMPILED <= JE_LOG_LEVEL_TRACE)) {
					JE_DEBUG("SDL_KEYDOWN, key=%s", SDL_GetKeyName(event.key.keysym.sym));
				}
				break;
			}
			case SDL_CONTROLLERDEVICEADDED: {
				JE_DEBUG("SDL_CONTROLLERDEVICEADDED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED: {
				JE_DEBUG("SDL_CONTROLLERDEVICEREMOVED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERDEVICEREMAPPED: {
				JE_DEBUG("SDL_CONTROLLERDEVICEREMAPPED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERBUTTONDOWN: {
				JE_DEBUG(
					"SDL_CONTROLLERBUTTONDOWN, button=%s",
					SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
				break;
			}
			case SDL_CONTROLLERBUTTONUP: {
				JE_DEBUG(
					"SDL_CONTROLLERBUTTONUP, button=%s",
					SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
				break;
			}
			case SDL_CONTROLLERAXISMOTION: {
				JE_TRACE(
					"SDL_CONTROLLERAXISMOTION, axis=%s, value=%d",
					SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis),
					(int)event.caxis.value);
				break;
			}
			case SDL_MOUSEMOTION: {

			}
			default: {
				JE_TRACE("unhandled event, event.type=%u", event.type);
				break;
			}
		}
	}

	ok = ok && jeWindow_clear(window);
	ok = ok && jeWindow_flushPrimitives(window);

	if (ok) {
		SDL_GL_SwapWindow(window->window);
	}

	if (ok) {
		window->keyState = SDL_GetKeyboardState(NULL);

		window->frame++;

		/*Sample the framerate every JE_WINDOW_FRAME_RATE frames, i.e. every second*/
		if ((window->frame % JE_WINDOW_FRAME_RATE) == 0) {
			Uint32 timeMs = SDL_GetTicks();

			float sampleDuration = ((float)timeMs - (float)window->fpsLastSampleTimeMs) / 1000.0F;
			float sampleFrameDuration = sampleDuration / (float)JE_WINDOW_FRAME_RATE;
			window->fpsEstimate = (uint32_t)(1.0F / sampleFrameDuration);
			window->fpsLastSampleTimeMs = timeMs;
		}

		static const Uint32 frameDurationMs = 1000 / JE_WINDOW_FRAME_RATE;
		Uint32 timeMs = SDL_GetTicks();
		Uint32 waitMs = window->nextFrameStartMs - timeMs;

		/*guard against waitMs underflow*/
		if (timeMs > window->nextFrameStartMs) {
			waitMs = 0;
			window->nextFrameStartMs = timeMs;
		}

		/*guard against waiting more than a full frame duration*/
		if (waitMs > frameDurationMs) {
			window->nextFrameStartMs = timeMs + frameDurationMs;
			waitMs = frameDurationMs;
		}

		if (waitMs > 0) {
			SDL_Delay(waitMs);
		}
		window->nextFrameStartMs += frameDurationMs;
	}

	if (!ok && (window != NULL)) {
		window->open = false;
	}

	return ok;
}
void jeWindow_destroy(struct jeWindow* window) {
	JE_DEBUG("window=%p", (void*)window);

	if (window != NULL) {
		window->open = false;

		jeWindow_destroyGL(window);

		jeController_destroy(&window->controller);

		jeImage_destroy(&window->image);

		jeVertexBuffer_destroy(&window->vertexBuffer);

		if (window->window != NULL) {
			SDL_DestroyWindow(window->window);
			window->window = NULL;
		}

		jeSDL_destroyReentrant();

		memset((void*)window, 0, sizeof(struct jeWindow));

		free(window);
		window = NULL;
	}
}
struct jeWindow* jeWindow_create(bool startVisible, const char* optSpritesFilename) {
	JE_DEBUG(" ");

	bool ok = true;

	struct jeWindow* window = (struct jeWindow*)malloc(sizeof(struct jeWindow));

	if (window == NULL) {
		JE_ERROR("malloc() failed");
		ok = false;
	}

	if (window != NULL) {
		memset((void*)window, 0, sizeof(struct jeWindow));
	}

	ok = ok && jeSDL_initReentrant();

	if (ok) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

#if defined(JE_BUILD_OPENGL_FORWARD_COMPATIBLE)
		/*Replace with OpenGL 3.2 context for RenderDoc support*/
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

		window->window = SDL_CreateWindow(
			JE_WINDOW_START_CAPTION,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			JE_WINDOW_START_WIDTH,
			JE_WINDOW_START_HEIGHT,
			(startVisible ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN) | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

		if (window->window == NULL) {
			JE_ERROR("SDL_CreateWindow() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		SDL_SetWindowMinimumSize(window->window, JE_WINDOW_MIN_WIDTH, JE_WINDOW_MIN_HEIGHT);

		if ((optSpritesFilename == NULL) || !jeImage_createFromPNGFile(&window->image, optSpritesFilename)) {
			/*
			 * As a fallback, create a gray texture big enough to allow mapping of color.
			 * Gray is chosen to have it be visible against the white fill color.
			 */
			const struct jeColorRGBA32 grey = {0x80, 0x80, 0x80, 0xFF};
			const struct jeColorRGBA32 white = {0xFF, 0xFF, 0xFF, 0xFF};

			jeImage_destroy(&window->image);
			jeImage_create(&window->image, 2048, 2048, grey);

			/*Topleft texel is used for rendering without texture and must be white*/
			((struct jeColorRGBA32*)window->image.buffer.data)[0] = white;
		}
	}

	ok = ok && jeVertexBuffer_create(&window->vertexBuffer);

	ok = ok && jeWindow_initGL(window);

	if (ok) {
		int controllerMappingsLoaded = SDL_GameControllerAddMappingsFromFile(JE_CONTROLLER_DB_FILENAME);

		if (controllerMappingsLoaded == -1) {
			JE_WARN("SDL_GameControllerAddMappingsFromFile() failed with error=%s", SDL_GetError());
			/*Lack of controller mappings is not fatal*/
		} else {
			JE_DEBUG("SDL_GameControllerAddMappingsFromFile() controllerMappingsLoaded=%d", controllerMappingsLoaded);
		}

		jeController_create(&window->controller);
		window->keyState = SDL_GetKeyboardState(NULL);

		jeWindow_clear(window);

		window->open = true;

		window->fpsLastSampleTimeMs = SDL_GetTicks();
		window->nextFrameStartMs = SDL_GetTicks();
	}

	if (!ok && (window != NULL)) {
		jeWindow_destroy(window);
		window = NULL;
	}

	return window;
}
bool jeWindow_getInput(const struct jeWindow* window, uint32_t inputId) {
	static const float axisMaxValue = 32767;

	JE_TRACE("window=%p, inputId=%u", (void*)window, inputId);

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (inputId >= JE_INPUT_COUNT) {
		JE_ERROR("inputId out of bounds, inputId=%u", inputId);
		ok = false;
	}

	bool pressed = false;

	if (ok) {
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
			float axisValue =
				(float)SDL_GameControllerGetAxis(window->controller.controller, controllerAxis) * controllerAxisDir;
			pressed = pressed || (axisValue > axisMinValue);
		}
	}

	return pressed;
}
bool jeWindow_getMousePos(const struct jeWindow* window, int32_t *outX, int32_t* outY) {
	int x = 0;
	int y = 0;

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		SDL_GetMouseState(&x, &y);

		x /= (jeWindow_getWidth(window) / JE_WINDOW_MIN_WIDTH);
		y /= (jeWindow_getHeight(window) / JE_WINDOW_MIN_HEIGHT);
	}

	if (outX != NULL) {
		*outX = (int32_t)x;
	}

	if (outY != NULL) {
		*outY = (int32_t)y;
	}

	JE_TRACE("window=%p, x=%d, y=%d", (void*)window, x, y);

	return ok;
}
bool jeWindow_getMouseButton(const struct jeWindow* window, uint32_t button) {
	bool state = false;
	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	Uint32 sdlButtonMask = 0;
	if (ok) {
		switch (button) {
			case JE_MOUSE_BUTTON_LEFT: {
				sdlButtonMask = SDL_BUTTON(SDL_BUTTON_LEFT);
				break;
			}
			case JE_MOUSE_BUTTON_MIDDLE: {
				sdlButtonMask = SDL_BUTTON(SDL_BUTTON_MIDDLE);
				break;
			}
			case JE_MOUSE_BUTTON_RIGHT: {
				sdlButtonMask = SDL_BUTTON(SDL_BUTTON_RIGHT);
				break;
			}
			default: {
				JE_ERROR("unrecognized button, button=%u", button);
				ok = false;
			}
		}
	}

	if (ok) {
		state = (SDL_GetMouseState(NULL, NULL) & sdlButtonMask) != 0;
	}

	return state;
}
uint32_t jeWindow_getFps(const struct jeWindow* window) {
	uint32_t fpsEstimate = 0;

	bool ok = true;

	if (window == NULL) {
		JE_ERROR("window=NULL");
		ok = false;
	}

	if (ok) {
		fpsEstimate = window->fpsEstimate;
	}

	JE_TRACE("window=%p, fpsEstimate=%u", (void*)window, fpsEstimate);

	return fpsEstimate;
}

void jeWindow_runTests() {
#if JE_DEBUGGING
	JE_TRACE(" ");

	struct jeController controller;
	jeController_create(&controller);
	jeController_destroy(&controller);

	JE_ASSERT(jeSDL_initReentrant());

	struct jeWindow* window = jeWindow_create(/*startVisible*/ false, /*optSpritesFilename*/ NULL);
	JE_ASSERT(window != NULL);

	JE_ASSERT(jeWindow_getIsOpen(window));
	JE_ASSERT(jeWindow_getWidth(window) == JE_WINDOW_START_WIDTH);
	JE_ASSERT(jeWindow_getHeight(window) == JE_WINDOW_START_HEIGHT);
	for (uint32_t i = JE_INPUT_FIRST; i < JE_INPUT_COUNT; i++) {
		jeWindow_getInput(window, i);
	}

	for (uint32_t i = JE_MOUSE_BUTTON_FIRST; i < JE_MOUSE_BUTTON_COUNT; i++) {
		jeWindow_getMouseButton(window, i);
	}

	int32_t mouseX = 0;
	int32_t mouseY = 0;
	JE_ASSERT(jeWindow_getMousePos(window, &mouseX, &mouseY));

	int32_t oldMouseX = mouseX;
	mouseX++;
	JE_ASSERT(jeWindow_getMousePos(window, &mouseX, &mouseY));
	JE_ASSERT(mouseX == oldMouseX);

	jeWindow_getFps(window);

	JE_ASSERT(jeWindow_clear(window));

	jeWindow_destroyGL(window);

	JE_ASSERT(jeWindow_initGL(window));

	struct jeVertex triangleVertices[JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT];
	jeWindow_pushPrimitive(window, triangleVertices, JE_PRIMITIVE_TYPE_TRIANGLES);

	/*Create a second window before displaying to ensure they do not clobber each other with opengl state*/
	struct jeWindow* window2 = jeWindow_create(/*startVisible*/ false, /*optSpritesFilename*/ NULL);
	JE_ASSERT(window2 != NULL);
	jeWindow_destroy(window2);

	JE_ASSERT(jeWindow_step(window));

	jeWindow_show(window);

	jeWindow_destroy(window);
	window = NULL;

	jeSDL_destroyReentrant();
#endif
}
