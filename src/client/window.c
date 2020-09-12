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


jeBool jeGl_getOk(const char* file, int line, const char* function) {
	jeBool ok = JE_TRUE;
	GLenum glError = GL_NO_ERROR;

	for (glError = glGetError(); glError != GL_NO_ERROR; glError = glGetError()) {
#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
		jeLogger_logPrefixImpl(JE_LOG_LABEL_ERR, file, line);
		jeLogger_logImpl("%s(): OpenGL error, glError=%d, message=%s", function, glError, gluErrorString(glError));
#endif
		ok = JE_FALSE;
	}

	return ok;
}
jeBool jeGl_getShaderOk(GLuint shader, const char* file, int line, const char* function) {
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

	return ok;
}
jeBool jeGl_getProgramOk(GLuint program, const char* file, int line, const char* function) {
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

	return ok;
}

/* TODO move vertex buffer creation logic to a _create() function */
typedef struct jeVertexBuffer jeVertexBuffer;
struct jeVertexBuffer {
	jeVertex vertex[JE_VERTEX_BUFFER_CAPACITY];
	GLuint count;
	jePrimitiveType primitiveType;

	GLuint vao;  /* non-owning */
	GLuint program;  /* non-owning */
};
void jeVertexBuffer_reset(jeVertexBuffer* vertexBuffer) {
	vertexBuffer->count = 0;
	memset((void*)vertexBuffer->vertex, 0, JE_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex));
}
void jeVertexBuffer_destroy(jeVertexBuffer* vertexBuffer) {
	memset((void*)vertexBuffer, 0, sizeof(*vertexBuffer));
}
void jeVertexBuffer_create(jeVertexBuffer* vertexBuffer, GLuint program, GLuint vao) {
	memset((void*)vertexBuffer, 0, sizeof(*vertexBuffer));

	vertexBuffer->program = program;
	vertexBuffer->vao = vao;
}
void jeVertexBuffer_flush(jeVertexBuffer* vertexBuffer) {
	GLenum primitiveType = GL_TRIANGLES;

	if (vertexBuffer->count == 0) {
		goto finalize;
	}

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
			JE_ERROR("jeVertexBuffer_flush(): unknown primitive type, type=%d", vertexBuffer->primitiveType);
			goto finalize;
		}
	}

	glUseProgram(vertexBuffer->program);
	glBindVertexArray(vertexBuffer->vao);

	glBufferData(GL_ARRAY_BUFFER, JE_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)vertexBuffer->vertex, GL_DYNAMIC_DRAW);
	glDrawArrays(primitiveType, 0, vertexBuffer->count);

	glBindVertexArray(0);
	glUseProgram(0);

	jeGl_getOk(JE_LOG_CONTEXT, "jeVertexBuffer_flush");

	finalize: {
		jeVertexBuffer_reset(vertexBuffer);
	}
}
void jeVertexBuffer_append(jeVertexBuffer* vertexBuffer, const jeVertex* vertex, int vertexCount, jePrimitiveType primitiveType) {
	int i = 0;

	jeBool bufferCanFitRenderable = ((vertexBuffer->count + vertexCount) <= JE_VERTEX_BUFFER_CAPACITY);
	jeBool bufferPrimitiveIsCorrect = (vertexBuffer->primitiveType == primitiveType);

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
void jeVertexBuffer_appendSprite(jeVertexBuffer* vertexBuffer, jeRenderable* renderable) {
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

	jeVertexBuffer_append(vertexBuffer, vertex, JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT, JE_PRIMITIVE_TYPE_TRIANGLES);
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

	controller->keys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_F1);
	controller->altKeys[JE_INPUT_X] = SDL_GetScancodeFromKey(SDLK_c);
	controller->controllerButtons[JE_INPUT_X] = SDL_CONTROLLER_BUTTON_X;
	controller->controllerAxis[JE_INPUT_X] = SDL_CONTROLLER_AXIS_INVALID;
	controller->controllerAxisDir[JE_INPUT_X] = 0.0f;

	controller->keys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_F2);
	controller->altKeys[JE_INPUT_Y] = SDL_GetScancodeFromKey(SDLK_v);
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
		JE_INFO("jeController_create(): No compatible game controller found", i);
	}

	controller->controllerAxisThreshold = 0.1;
}

struct jeWindow {
	jeBool open;
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
jeBool jeWindow_getIsOpen(jeWindow* window) {
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
	jeRenderQueue_insert(&window->renderQueue, renderable);
}
void jeWindow_flushRenderQueue(jeWindow* window) {
	int i = 0;

	jeRenderable renderable;

	memset((void*)&renderable, 0, sizeof(renderable));

	jeRenderQueue_sort(&window->renderQueue);

	for (i = 0; i < window->renderQueue.count; i++) {
		renderable = window->renderQueue.renderables[i];
		switch (renderable.primitiveType) {
			case JE_PRIMITIVE_TYPE_POINTS: {
				jeVertexBuffer_append(&window->vertexBuffer, renderable.vertex, JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT, JE_PRIMITIVE_TYPE_POINTS);
				break;
			}
			case JE_PRIMITIVE_TYPE_LINES: {
				jeVertexBuffer_append(&window->vertexBuffer, renderable.vertex, JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT, JE_PRIMITIVE_TYPE_LINES);
				break;
			}
			case JE_PRIMITIVE_TYPE_TRIANGLES: {
				jeVertexBuffer_append(&window->vertexBuffer, renderable.vertex, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT, JE_PRIMITIVE_TYPE_TRIANGLES);
				break;
			}
			case JE_PRIMITIVE_TYPE_SPRITES: {
				jeVertexBuffer_appendSprite(&window->vertexBuffer, &renderable);
				break;
			}
			default: {
				JE_WARN("jeWindow_flushRenderQueue(): unrecognized type, primitive=%d, index=%d, count=%d",
						renderable.primitiveType, i, window->renderQueue.count);
				break;
			}
		}
	}
	window->renderQueue.count = 0;
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
jeBool jeWindow_initGL(jeWindow* window) {
	jeBool success = JE_FALSE;

	GLfloat scaleXyz[3];
	GLfloat scaleUv[2];
	GLint scaleXyzLocation = 0;
	GLint scaleUvLocation = 0;
	GLint srcPosLocation = 0;
	GLint srcColLocation = 0;
	GLint srcUvLocation = 0;

	GLint lineWidthRange[2] = {0, 0};
	GLfloat scale = (float)(jeWindow_getWidth(window) / JE_WINDOW_MIN_WIDTH);;

	window->context = SDL_GL_CreateContext(window->window);
	if (window->context == 0) {
		JE_ERROR("jeWindow_initGL(): SDL_GL_CreateContext() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	if (SDL_GL_MakeCurrent(window->window, window->context) != 0) {
		JE_ERROR("jeWindow_initGL(): SDL_GL_MakeCurrent() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	if (SDL_GL_SetSwapInterval(1) < 0) {
		JE_ERROR("jeWindow_initGL(): SDL_GL_SetSwapInterval() failed to enable vsync, error=%s", SDL_GetError());
	}

	glewExperimental = JE_TRUE;
	if (glewInit() != GLEW_OK) {
		JE_ERROR("jeWindow_initGL(): glewInit() failed");
		goto finalize;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glDisable(GL_CULL_FACE);

	glViewport(0, 0, jeWindow_getWidth(window), jeWindow_getHeight(window));
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	window->vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(window->vertShader, 1, &jeWindow_vertShaderPtr, &jeWindow_vertShaderSize);
	glCompileShader(window->vertShader);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}
	if (jeGl_getShaderOk(window->vertShader, JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getShaderOk() failed");
		goto finalize;
	}

	window->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(window->fragShader, 1, &jeWindow_fragShaderPtr, &jeWindow_fragShaderSize);
	glCompileShader(window->fragShader);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}
	if (jeGl_getShaderOk(window->fragShader, JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getShaderOk() failed");
		goto finalize;
	}

	window->program = glCreateProgram();
	glAttachShader(window->program, window->vertShader);
	glAttachShader(window->program, window->fragShader);
	glBindAttribLocation(window->program, 0, "srcPos");
	glBindAttribLocation(window->program, 1, "srcCol");
	glBindAttribLocation(window->program, 2, "srcUv");
	glLinkProgram(window->program);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}
	if (jeGl_getProgramOk(window->program, JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getProgramOk() failed");
		goto finalize;
	}

	glUseProgram(window->program);
	if (jeGl_getProgramOk(window->program, JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): glUseProgram() failed");
		goto finalize;
	}

	glGenTextures(1, &window->texture);
	glBindTexture(GL_TEXTURE_2D, window->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->image.width, window->image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, window->image.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	glGenBuffers(1, &window->vbo);

	glGenVertexArrays(1, &window->vao);

	jeVertexBuffer_create(&window->vertexBuffer, window->program, window->vao);

	glBindVertexArray(window->vao);
	glBindBuffer(GL_ARRAY_BUFFER, window->vbo);
	glBufferData(GL_ARRAY_BUFFER, JE_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vertexBuffer.vertex, GL_DYNAMIC_DRAW);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	/*Transforms pos from world coords (+/- windowSize) to normalized device coords (-1.0 to 1.0)*/
	scaleXyz[0] = 2.0f / JE_WINDOW_MIN_WIDTH;
	scaleXyz[1] = -2.0f / JE_WINDOW_MIN_HEIGHT;

	/*Normalize z to between -1.0 and 1.0.  Supports depths +/- 2^20.  Near the precise int limit for float32*/
	scaleXyz[2] = 1.0f / (float)(1 << 20);

	/*Converts image coords to normalized texture coords (0.0 to 1.0)*/
	scaleUv[0] = 1.0f / (float)window->image.width;
	scaleUv[1] = 1.0f / (float)window->image.height;

	scaleXyzLocation = glGetUniformLocation(window->program, "scaleXyz");
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	glUniform3f(scaleXyzLocation, scaleXyz[0], scaleXyz[1], scaleXyz[2]);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	scaleUvLocation = glGetUniformLocation(window->program, "scaleUv");
	glUniform2f(scaleUvLocation, scaleUv[0], scaleUv[1]);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	srcPosLocation = glGetAttribLocation(window->program, "srcPos");
	glEnableVertexAttribArray(srcPosLocation);
	glVertexAttribPointer(srcPosLocation, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)0);
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	srcColLocation = glGetAttribLocation(window->program, "srcCol");
	glEnableVertexAttribArray(srcColLocation);
	glVertexAttribPointer(srcColLocation, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(4 * sizeof(GLfloat)));
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	srcUvLocation = glGetAttribLocation(window->program, "srcUv");
	glEnableVertexAttribArray(srcUvLocation);
	glVertexAttribPointer(srcUvLocation, 2, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(8 * sizeof(GLfloat)));
	if (jeGl_getOk(JE_LOG_CONTEXT, "jeWindow_initGL") == JE_FALSE) {
		JE_ERROR("jeWindow_initGL(): jeGl_getOk() failed");
		goto finalize;
	}

	glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
	if ((scale < lineWidthRange[0]) || (scale > lineWidthRange[1])) {
		JE_WARN("jeWindow_initGL(): scale not in supported lineWidthRange, scale=%d, min=%d, max=%d",
				scale, lineWidthRange[0], lineWidthRange[1]);
	}

	glUseProgram(0);

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

	jeVertexBuffer_flush(&window->vertexBuffer);

	glDepthMask(GL_FALSE);
	jeWindow_flushRenderQueue(window);
	jeVertexBuffer_flush(&window->vertexBuffer);
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
				JE_DEBUG("jeWindow_step(): Quit event received");
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
							JE_ERROR("jeWindow_step(): jeWindow_initGL failed");
							window->open = JE_FALSE;
						}
						break;
					}
				}
				break;
			}
			case SDL_KEYUP: {
				if ((event.key.repeat == 0) || (JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE)) {
					JE_DEBUG("jeWindow_step(): SDL_KEYUP, key=%s", SDL_GetKeyName(event.key.keysym.sym));
				}

				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE: {
						JE_DEBUG("jeWindow_step(): Escape key released");
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
				if ((event.key.repeat == 0) || (JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE)) {
					JE_DEBUG("jeWindow_step(): SDL_KEYDOWN, key=%s", SDL_GetKeyName(event.key.keysym.sym));
				}
				break;
			}
			case SDL_CONTROLLERDEVICEADDED: {
				JE_DEBUG("jeWindow_step(): SDL_CONTROLLERDEVICEADDED, index=%d", event.cdevice.which);
				jeController_destroy(&window->controller);
				jeController_create(&window->controller);
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED: {
				JE_DEBUG("jeWindow_step(): SDL_CONTROLLERDEVICEREMOVED, index=%d", event.cdevice.which);
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
				JE_TRACE("jeWindow_step(): SDL_CONTROLLERAXISMOTION, axis=%s, value=%d",
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
}
void jeWindow_destroy(jeWindow* window) {
	JE_INFO("jeWindow_destroy()");

	if (window == NULL) {
		goto finalize;
	}

	window->open = JE_FALSE;

	jeWindow_destroyGL(window);

	jeRenderQueue_destroy(&window->renderQueue);

	jeController_destroy(&window->controller);

	jeImage_destroy(&window->image);

	if (window->window != NULL) {
		SDL_DestroyWindow(window->window);
		window->window = NULL;
	}

	free(window);

	finalize: {
		SDL_Quit();
	}
}
jeWindow* jeWindow_create() {
	jeBool success = JE_FALSE;

	int controllerMappingsLoaded = -1;

	jeWindow* window = (jeWindow*)malloc(sizeof(jeWindow));

	JE_INFO("jeWindow_create()");

	memset((void*)window, 0, sizeof(*window));

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		JE_ERROR("jeWindow_create(): SDL_Init() failed with error=%s", SDL_GetError());
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
		JE_WINDOW_START_CAPTION,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		JE_WINDOW_START_WIDTH,
		JE_WINDOW_START_HEIGHT,
		/*flags*/ SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if (window->window == NULL) {
		JE_ERROR("jeWindow_create(): SDL_CreateWindow() failed with error=%s", SDL_GetError());
		goto finalize;
	}

	if (jeImage_create(&window->image, JE_WINDOW_SPRITE_FILENAME) == JE_FALSE) {
		JE_ERROR("jeWindow_create(): jeImage_create() failed");
		goto finalize;
	}

	jeRenderQueue_create(&window->renderQueue);

	if (jeWindow_initGL(window) == JE_FALSE) {
		JE_ERROR("jeWindow_create(): jeWindow_initGL() failed");
		goto finalize;
	}

	controllerMappingsLoaded = SDL_GameControllerAddMappingsFromFile(JE_CONTROLLER_DB_FILENAME);
	if (controllerMappingsLoaded == -1) {
		JE_ERROR("jeWindow_create(): SDL_GameControllerAddMappingsFromFile() failed with error=%s", SDL_GetError());
	} else {
		JE_DEBUG("jeWindow_create(): SDL_GameControllerAddMappingsFromFile() controllerMappingsLoaded=%d", controllerMappingsLoaded);
	}

	jeController_create(&window->controller);
	window->keyState = SDL_GetKeyboardState(NULL);

	jeWindow_clear(window);

	window->open = JE_TRUE;

	success = JE_TRUE;
	finalize: {
		if ((success == JE_FALSE) && (window != NULL)) {
			free(window);
			window = NULL;
		}
	}

	return window;
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
int jeWindow_getFPS(jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}
