#include "stdafx.h"
#include "window.h"
#include "debug.h"
#include "image.h"

#define JE_VERTEX_BUFFER_CAPACITY 16 * 1024

#define JE_CONTROLLER_DB_FILENAME "data/gamecontrollerdb.txt"

#define JE_WINDOW_FRAME_RATE 60
#define JE_WINDOW_START_SCALE 8
#define JE_WINDOW_START_WIDTH (JE_WINDOW_MIN_WIDTH * JE_WINDOW_START_SCALE)
#define JE_WINDOW_START_HEIGHT (JE_WINDOW_MIN_HEIGHT * JE_WINDOW_START_SCALE)
#define JE_WINDOW_START_CAPTION ""
#define JE_WINDOW_SPRITE_FILENAME "data/sprites.png"

/*https://www.khronos.org/registry/OpenGL/specs/gl/glspec21.pdf*/
/*https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.20.pdf*/
#define JE_WINDOW_VERT_SHADER \
	"#version 120\n" \
	\
	"uniform vec3 scaleXyz;" /*Transforms pos from world coords (+/- windowSize) to normalized device coords (-1.0 to 1.0)*/ \
	"uniform vec2 scaleUv;" /*Converts image coords to normalized texture coords (0.0 to 1.0)*/ \
	\
	"attribute vec3 srcPos;" \
	"attribute vec4 srcCol;" \
	"attribute vec2 srcUv;" \
	\
	"varying vec4 col;" \
	"varying vec2 uv;" \
	\
	"void main() {" \
		"gl_Position = vec4(srcPos * scaleXyz, 1);" \
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


bool jeGl_getOk(jeLoggerContext loggerContext) {
	bool ok = true;
	GLenum glError = GL_NO_ERROR;

	for (glError = glGetError(); glError != GL_NO_ERROR; glError = glGetError()) {
#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_log(loggerContext, JE_LOG_LABEL_ERR, "OpenGL error, glError=%d, message=%s", glError, gluErrorString(glError));
#endif
		ok = false;
	}

	JE_MAYBE_UNUSED(loggerContext);

	return ok;
}
bool jeGl_getShaderOk(GLuint shader, jeLoggerContext loggerContext) {
	bool ok = true;
	GLint compileStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;
	void* buffer = NULL;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msgMaxSize);
		buffer = malloc(msgMaxSize);

		glGetShaderInfoLog(shader, msgMaxSize, NULL, (GLchar*)buffer);

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_log(loggerContext, JE_LOG_LABEL_ERR, "OpenGL shader compilation failed, error=\n%s", (const char*)buffer);
#endif

		ok = false;
	}

	free(buffer);

	JE_MAYBE_UNUSED(loggerContext);

	return ok;
}
bool jeGl_getProgramOk(GLuint program, jeLoggerContext loggerContext) {
	bool ok = true;
	GLint linkStatus = GL_FALSE;
	GLsizei msgMaxSize = 0;
	void* buffer = NULL;

	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &msgMaxSize);
		buffer = malloc(msgMaxSize);

		glGetProgramInfoLog(program, msgMaxSize, NULL, (GLchar*)buffer);

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_log(loggerContext, JE_LOG_LABEL_ERR, "OpenGL program linking failed, error=\n%s", (const char*)buffer);
#endif

		ok = false;
	}

	free(buffer);

	JE_MAYBE_UNUSED(loggerContext);

	return ok;
}

typedef struct jeVertexBuffer jeVertexBuffer;
struct jeVertexBuffer {
	jeVertex vertex[JE_VERTEX_BUFFER_CAPACITY];
	GLuint count;
	jePrimitiveType primitiveType;

	GLuint vao;  /* non-owning */
	GLuint program;  /* non-owning */
};
void jeVertexBuffer_create(jeVertexBuffer* vertexBuffer, GLuint program, GLuint vao) {
	memset((void*)vertexBuffer->vertex, 0, sizeof(jeVertex) * JE_VERTEX_BUFFER_CAPACITY);

	vertexBuffer->count = 0;
	vertexBuffer->primitiveType = JE_PRIMITIVE_TYPE_TRIANGLES;

	vertexBuffer->program = program;
	vertexBuffer->vao = vao;
}
void jeVertexBuffer_destroy(jeVertexBuffer* vertexBuffer) {
	memset((void*)vertexBuffer, 0, sizeof(*vertexBuffer));
}
void jeVertexBuffer_reset(jeVertexBuffer* vertexBuffer) {
	vertexBuffer->count = 0;
	memset((void*)vertexBuffer->vertex, 0, JE_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex));
}
void jeVertexBuffer_flush(jeVertexBuffer* vertexBuffer) {
	GLenum primitiveType = GL_TRIANGLES;

	switch (vertexBuffer->primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			primitiveType = GL_POINTS;
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			primitiveType = GL_LINES;
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			primitiveType = GL_TRIANGLES;
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			primitiveType = GL_TRIANGLES;
			break;
		}
		default: {
			primitiveType = GL_TRIANGLES;

			JE_ERROR("unknown primitive type, type=%d", vertexBuffer->primitiveType);
			break;
		}
	}

	if (vertexBuffer->count > 0) {
		glUseProgram(vertexBuffer->program);
		glBindVertexArray(vertexBuffer->vao);

		glBufferData(GL_ARRAY_BUFFER, JE_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)vertexBuffer->vertex, GL_DYNAMIC_DRAW);
		glDrawArrays(primitiveType, 0, vertexBuffer->count);

		glBindVertexArray(0);
		glUseProgram(0);

		jeGl_getOk(JE_LOG_CONTEXT);
	}

	jeVertexBuffer_reset(vertexBuffer);
}
void jeVertexBuffer_pushVertexBatch(jeVertexBuffer* vertexBuffer, const jeVertex* vertex, int vertexCount, jePrimitiveType primitiveType) {
	int i = 0;

	bool bufferCanFitRenderable = ((vertexBuffer->count + vertexCount) <= JE_VERTEX_BUFFER_CAPACITY);
	bool bufferPrimitiveIsCorrect = (vertexBuffer->primitiveType == primitiveType);

	if (!bufferCanFitRenderable || !bufferPrimitiveIsCorrect) {
		jeVertexBuffer_flush(vertexBuffer);

		if (!bufferPrimitiveIsCorrect) {
			vertexBuffer->primitiveType = primitiveType;
		}
	}

	for (i = 0; i < vertexCount; i++) {
		vertexBuffer->vertex[vertexBuffer->count] = vertex[i];
		vertexBuffer->count++;
	}
}
void jeVertexBuffer_pushSprite(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable) {
	int i = 0;
	jeVertex vertex[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT];

	/* Render sprite as two triangles represented by 6 clockwise vertices*/
	for (i = 0; i < JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT; i++) {
		vertex[i] = renderable->vertex[0];
	}
	vertex[1].x = renderable->vertex[1].x;
	vertex[1].u = renderable->vertex[1].u;

	vertex[2].y = renderable->vertex[1].y;
	vertex[2].v = renderable->vertex[1].v;

	vertex[3].y = renderable->vertex[1].y;
	vertex[3].v = renderable->vertex[1].v;

	vertex[4].x = renderable->vertex[1].x;
	vertex[4].u = renderable->vertex[1].u;

	vertex[5].x = renderable->vertex[1].x;
	vertex[5].y = renderable->vertex[1].y;
	vertex[5].u = renderable->vertex[1].u;
	vertex[5].v = renderable->vertex[1].v;

	jeVertexBuffer_pushVertexBatch(vertexBuffer, vertex, JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT, JE_PRIMITIVE_TYPE_TRIANGLES);
}
void jeVertexBuffer_pushRenderable(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable) {
	switch (renderable->primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			jeVertexBuffer_pushVertexBatch(vertexBuffer, renderable->vertex, JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT, JE_PRIMITIVE_TYPE_POINTS);
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			jeVertexBuffer_pushVertexBatch(vertexBuffer, renderable->vertex, JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT, JE_PRIMITIVE_TYPE_LINES);
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			jeVertexBuffer_pushVertexBatch(vertexBuffer, renderable->vertex, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT, JE_PRIMITIVE_TYPE_TRIANGLES);
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			jeVertexBuffer_pushSprite(vertexBuffer, renderable);
			break;
		}
		default: {
			JE_WARN("unrecognized type, primitive=%d", renderable->primitiveType);
			break;
		}
	}
}

typedef struct jeController jeController;
struct jeController {
	SDL_GameController* controller;

	SDL_Scancode keys[JE_INPUT_COUNT];
	SDL_Scancode altKeys[JE_INPUT_COUNT];
	SDL_GameControllerButton controllerButtons[JE_INPUT_COUNT];
	SDL_GameControllerAxis controllerAxis[JE_INPUT_COUNT];
	float controllerAxisDir[JE_INPUT_COUNT];

	float controllerAxisThreshold;
};
void jeController_destroy(jeController* controller) {
	if (controller->controller != NULL) {
		SDL_GameControllerClose(controller->controller);
		controller->controller = NULL;
	}
}
void jeController_create(jeController* controller) {
	int i = 0;

	memset((void*)controller, 0, sizeof(*controller));

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

	controller->controllerAxisDir[JE_INPUT_LEFT] = -1.0f;
	controller->controllerAxisDir[JE_INPUT_RIGHT] = 1.0f;
	controller->controllerAxisDir[JE_INPUT_UP] = -1.0f;
	controller->controllerAxisDir[JE_INPUT_DOWN] = 1.0f;
	controller->controllerAxisDir[JE_INPUT_A] = 0.0f;
	controller->controllerAxisDir[JE_INPUT_B] = 0.0f;
	controller->controllerAxisDir[JE_INPUT_X] = 0.0f;
	controller->controllerAxisDir[JE_INPUT_Y] = 0.0f;

	controller->controllerAxisThreshold = 0.1;

	/*Find the first connected game controller and use that*/
	JE_DEBUG("numJoysticks=%d", SDL_NumJoysticks());
	for (i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			JE_DEBUG("index=%d", i);
			controller->controller = SDL_GameControllerOpen(i);
			break;
		}
	}
	if (controller->controller == NULL) {
		JE_INFO("No compatible game controller found", i);
	}
}

struct jeWindow {
	bool open;

	Uint64 frame;
	Uint32 fpsEstimate;
	Uint32 fpsLastSampleTimeMs;
	Uint32 nextFrameTimeMs;

	jeImage image;
	jeRenderQueue renderQueue;
	SDL_Window* window;

	jeController controller;
	const Uint8* keyState;

	SDL_GLContext context;
	GLuint texture;
	GLuint vertShader;
	GLuint fragShader;
	GLuint program;
	GLuint vbo;
	GLuint vao;
	jeVertexBuffer vertexBuffer;
};
static const GLchar *jeWindow_vertShaderPtr = JE_WINDOW_VERT_SHADER;
static const GLchar *jeWindow_fragShaderPtr = JE_WINDOW_FRAG_SHADER;
static const GLint jeWindow_vertShaderSize = sizeof(JE_WINDOW_VERT_SHADER);
static const GLint jeWindow_fragShaderSize = sizeof(JE_WINDOW_FRAG_SHADER);
bool jeWindow_getIsOpen(jeWindow* window) {
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
void jeWindow_clear(jeWindow* window) {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	JE_MAYBE_UNUSED(window);
}
void jeWindow_drawRenderable(jeWindow* window, jeRenderable* renderable) {
	jeRenderQueue_push(&window->renderQueue, renderable);
}
void jeWindow_flushRenderables(jeWindow* window) {
	int i = 0;
	const jeRenderable* renderable = NULL;

	JE_TRACE("sorting");
	jeRenderQueue_sort(&window->renderQueue);

	JE_TRACE("start renderables loop");
	for (i = 0; i < window->renderQueue.renderables.count; i++) {
		renderable = jeRenderQueue_get(&window->renderQueue, i);
		if (renderable == NULL) {
			JE_ERROR("jeRenderQueue_get() failed");
			break;
		}

		jeVertexBuffer_pushRenderable(&window->vertexBuffer, renderable);
	}
	JE_TRACE("end renderables loop");

	jeVertexBuffer_flush(&window->vertexBuffer);
	JE_TRACE("flush vertex buffer");

	jeRenderQueue_setCount(&window->renderQueue, 0);
	JE_TRACE("clear renderables");
}
void jeWindow_destroyGL(jeWindow* window) {
	jeVertexBuffer_destroy(&window->vertexBuffer);

	if (window->vao != 0) {
		glBindVertexArray(window->vao);
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
bool jeWindow_initGL(jeWindow* window) {
	bool ok = true;

	GLfloat scaleXyz[3];
	GLfloat scaleUv[2];
	GLint scaleXyzLocation = 0;
	GLint scaleUvLocation = 0;
	GLint srcPosLocation = 0;
	GLint srcColLocation = 0;
	GLint srcUvLocation = 0;

	if (ok) {
		window->context = SDL_GL_CreateContext(window->window);
		if (window->context == 0) {
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
			JE_WARN("SDL_GL_SetSwapInterval() failed to enable vsync, error=%s", SDL_GetError());
			/*Lack of vsync support is not fatal; we still have a frame timer to limit framerate*/
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
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

		glDisable(GL_CULL_FACE);

		glViewport(0, 0, jeWindow_getWidth(window), jeWindow_getHeight(window));

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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->image.width, window->image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, window->image.buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glGenBuffers(1, &window->vbo);

		glGenVertexArrays(1, &window->vao);

		jeVertexBuffer_create(&window->vertexBuffer, window->program, window->vao);

		glBindVertexArray(window->vao);
		glBindBuffer(GL_ARRAY_BUFFER, window->vbo);
		glBufferData(GL_ARRAY_BUFFER, JE_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vertexBuffer.vertex, GL_DYNAMIC_DRAW);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() failed");
			ok = false;
		}
	}

	if (ok) {
		/*Transforms pos from world coords (+/- windowSize) to normalized device coords (-1.0 to 1.0)*/
		scaleXyz[0] = 2.0f / JE_WINDOW_MIN_WIDTH;
		scaleXyz[1] = -2.0f / JE_WINDOW_MIN_HEIGHT;

		/*Normalize z to between -1.0 and 1.0.  Supports depths +/- 2^20.  Near the precise int limit for float32*/
		scaleXyz[2] = 1.0f / (float)(1 << 20);

		/*Converts image coords to normalized texture coords (0.0 to 1.0)*/
		scaleUv[0] = 1.0f / (float)window->image.width;
		scaleUv[1] = 1.0f / (float)window->image.height;

		scaleXyzLocation = glGetUniformLocation(window->program, "scaleXyz");
		scaleUvLocation = glGetUniformLocation(window->program, "scaleUv");

		glUniform3f(scaleXyzLocation, scaleXyz[0], scaleXyz[1], scaleXyz[2]);
		glUniform2f(scaleUvLocation, scaleUv[0], scaleUv[1]);

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() failed");
			ok = false;
		}
	}

	if (ok) {
		srcPosLocation = glGetAttribLocation(window->program, "srcPos");
		srcColLocation = glGetAttribLocation(window->program, "srcCol");
		srcUvLocation = glGetAttribLocation(window->program, "srcUv");

		glEnableVertexAttribArray(srcPosLocation);
		glEnableVertexAttribArray(srcColLocation);
		glEnableVertexAttribArray(srcUvLocation);

		glVertexAttribPointer(srcPosLocation, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)0);
		glVertexAttribPointer(srcColLocation, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(4 * sizeof(GLfloat)));
		glVertexAttribPointer(srcUvLocation, 2, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(8 * sizeof(GLfloat)));

		if (jeGl_getOk(JE_LOG_CONTEXT) == false) {
			JE_ERROR("jeGl_getOk() error");
			ok = false;
		}
	}

	if (ok) {
		glUseProgram(0);
		glBindVertexArray(0);
	}

	return ok;
}
void jeWindow_step(jeWindow* window) {
	/*Wait for frame start time*/
	static const Uint32 frameTimeMs = 1000 / JE_WINDOW_FRAME_RATE;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
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
							"resizing window to width=%d, height=%d",
							jeWindow_getWidth(window),
							jeWindow_getHeight(window)
						);
						jeWindow_destroyGL(window);
						if (!jeWindow_initGL(window)) {
							JE_ERROR("jeWindow_initGL failed");
							window->open = false;
						}
						break;
					}
				}
				break;
			}
			case SDL_KEYUP: {
				if ((event.key.repeat == 0) || (JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE)) {
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
				if ((event.key.repeat == 0) || (JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE)) {
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
				JE_DEBUG("SDL_CONTROLLERBUTTONDOWN, button=%s",
						 SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
				break;
			}
			case SDL_CONTROLLERBUTTONUP: {
				JE_DEBUG("SDL_CONTROLLERBUTTONUP, button=%s",
						 SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
				break;
			}
			case SDL_CONTROLLERAXISMOTION: {
				JE_TRACE("SDL_CONTROLLERAXISMOTION, axis=%s, value=%d",
						 SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis),
						 (int)event.caxis.value);
				break;
			}
			default: {
				break;
			}
		}
	}

	jeWindow_clear(window);

	window->keyState = SDL_GetKeyboardState(NULL);

	window->frame++;

	/*Sample the framerate every JE_WINDOW_FRAME_RATE frames, i.e. every second*/
	if ((window->frame % JE_WINDOW_FRAME_RATE) == 0) {
		Uint32 timeMs = SDL_GetTicks();

		float sampleDuration = ((float)timeMs - (float)window->fpsLastSampleTimeMs) / 1000.0f;
		float sampleFrameDuration = sampleDuration / (float)JE_WINDOW_FRAME_RATE;
		window->fpsEstimate = (int)(1.0f / sampleFrameDuration);
		window->fpsLastSampleTimeMs = timeMs;
	}

	Uint32 timeToWaitMs = window->nextFrameTimeMs - SDL_GetTicks();
	if (timeToWaitMs > frameTimeMs) {
		timeToWaitMs = frameTimeMs;
		window->nextFrameTimeMs = SDL_GetTicks() + frameTimeMs;
	}

	if (timeToWaitMs > 0) {
		SDL_Delay(timeToWaitMs);
	}
	window->nextFrameTimeMs += frameTimeMs;

	jeWindow_flushRenderables(window);

	SDL_GL_SwapWindow(window->window);
}
void jeWindow_destroy(jeWindow* window) {
	JE_INFO("");

	if (window != NULL) {
		window->open = false;

		jeWindow_destroyGL(window);

		jeRenderQueue_destroy(&window->renderQueue);

		jeController_destroy(&window->controller);

		jeImage_destroy(&window->image);

		if (window->window != NULL) {
			SDL_DestroyWindow(window->window);
			window->window = NULL;
		}

		SDL_Quit();

		free(window);
	}
}
jeWindow* jeWindow_create() {
	bool ok = true;

	int controllerMappingsLoaded = -1;

	jeWindow* window = (jeWindow*)malloc(sizeof(jeWindow));

	JE_INFO("");

	memset((void*)window, 0, sizeof(*window));

	if (ok) {
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			JE_ERROR("SDL_Init() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

#if defined(JE_BUILD_DEBUG)
		/*replace with OpenGL 3.2 context for RenderDoc support when debugging*/
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	}

	if (ok) {
		window->window = SDL_CreateWindow(
			JE_WINDOW_START_CAPTION,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			JE_WINDOW_START_WIDTH,
			JE_WINDOW_START_HEIGHT,
			/*flags*/ SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
		);
		if (window->window == NULL) {
			JE_ERROR("SDL_CreateWindow() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	ok = ok && jeImage_create(&window->image, JE_WINDOW_SPRITE_FILENAME);

	ok = ok && jeRenderQueue_create(&window->renderQueue);

	if (ok) {
		if (jeWindow_initGL(window) == false) {
			JE_ERROR("jeWindow_initGL() failed");
			ok = false;
		}
	}

	if (ok) {
		controllerMappingsLoaded = SDL_GameControllerAddMappingsFromFile(JE_CONTROLLER_DB_FILENAME);

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
		window->nextFrameTimeMs = SDL_GetTicks();
	}

	if (!ok) {
		jeWindow_destroy(window);
	}

	return window;
}
bool jeWindow_getInput(jeWindow* window, int inputId) {
	static const float axisMaxValue = 32767;

	bool pressed = false;

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
int jeWindow_getFps(jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return window->fpsEstimate;
}
