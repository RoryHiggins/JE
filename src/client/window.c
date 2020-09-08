#include "stdafx.h"
#include "window.h"
#include "debug.h"
#include "image.h"

#define JE_CONTROLLER_DB_FILENAME "data/gamecontrollerdb.txt"

#define JE_WINDOW_FRAME_RATE 60
#define JE_WINDOW_START_SCALE 8
#define JE_WINDOW_START_WIDTH (JE_WINDOW_MIN_WIDTH * JE_WINDOW_START_SCALE)
#define JE_WINDOW_START_HEIGHT (JE_WINDOW_MIN_HEIGHT * JE_WINDOW_START_SCALE)
#define JE_WINDOW_START_CAPTION ""
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

const char* jeRenderable_toDebugString(const jeRenderable* renderable) {
	static char buffer[1024];

	memset((void*)buffer, 0, sizeof(buffer));

	sprintf(
		buffer,
		"z=%f, primitiveType=%d, "
		"x1=%f, y1=%f, x2=%f, y2=%f, x3=%f, y3=%f, "
		"r=%f, g=%f, b=%f, a=%f, "
		"u1=%f, v1=%f, u2=%f, v2=%f, u3=%f, v3=%f",
		renderable->z, renderable->primitiveType,
		renderable->x1, renderable->y1, renderable->x2, renderable->y2, renderable->x3, renderable->y3,
		renderable->r, renderable->g, renderable->b, renderable->a,
		renderable->u1, renderable->v1, renderable->u2, renderable->v2, renderable->u3, renderable->v3
	);

	return buffer;
}
int jeRenderable_qsort_less(const void* aRaw, const void* bRaw) {
	const jeRenderable* a = (const jeRenderable*)aRaw;
	const jeRenderable* b = (const jeRenderable*)bRaw;

	if (a->z == b->z) {
		return a->primitiveType < b->primitiveType;
	}
	return (a->z < b->z);
}

/*Z-sorted queue of sprites*/
typedef struct jeDepthQueue jeDepthQueue;
struct jeDepthQueue {
	jeRenderable* renderables;
	int capacity;
	int count;
};
void jeDepthQueue_destroy(jeDepthQueue* depthQueue) {
	depthQueue->count = 0;
	depthQueue->capacity = 0;

	if (depthQueue->renderables != NULL) {
		free(depthQueue->renderables);
		depthQueue->renderables = NULL;
	}
}
void jeDepthQueue_create(jeDepthQueue* depthQueue) {
	depthQueue->renderables = NULL;
	depthQueue->capacity = 0;
	depthQueue->count = 0;
}
void jeDepthQueue_setCapacity(jeDepthQueue* depthQueue, int capacity) {
	JE_DEBUG("jeDepthQueue_setCapacity(): newCapacity=%d, currentCapacity=%d", capacity, depthQueue->capacity);

	if (capacity == depthQueue->capacity) {
		goto finalize;
	}

	if (capacity == 0) {
		jeDepthQueue_destroy(depthQueue);
		goto finalize;
	}

	if (depthQueue->renderables == NULL) {
		depthQueue->renderables = (jeRenderable*)malloc(sizeof(jeRenderable) * capacity);
	} else {
		depthQueue->renderables = (jeRenderable*)realloc(depthQueue->renderables, sizeof(jeRenderable) * capacity);
	}

	depthQueue->capacity = capacity;

	if (depthQueue->count > capacity) {
		depthQueue->count = capacity;
	}

	finalize: {
	}
}
void jeDepthQueue_insert(jeDepthQueue* depthQueue, jeRenderable renderable) {
	static const int startCapacity = 32;

	int newCapacity = 0;

	if (depthQueue->count >= depthQueue->capacity) {
		newCapacity = startCapacity;
		if (depthQueue->capacity >= startCapacity) {
			newCapacity = depthQueue->capacity * 4;
		}
		jeDepthQueue_setCapacity(depthQueue, newCapacity);
	}

	depthQueue->renderables[depthQueue->count] = renderable;
	depthQueue->count++;
}
void jeDepthQueue_sort(jeDepthQueue* depthQueue) {
	qsort(depthQueue->renderables, depthQueue->count, sizeof(jeRenderable), jeRenderable_qsort_less);
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
	jeDepthQueue depthQueue;
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
	int vboPrimitiveType;
};
static const GLchar *jeWindow_vertShaderPtr = JE_WINDOW_VERT_SHADER;
static const GLchar *jeWindow_fragShaderPtr = JE_WINDOW_FRAG_SHADER;
static const GLint jeWindow_vertShaderSize = sizeof(JE_WINDOW_VERT_SHADER);
static const GLint jeWindow_fragShaderSize = sizeof(JE_WINDOW_FRAG_SHADER);
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
jeBool jeWindow_getShaderOk(jeWindow* window, GLuint shader, const char* file, int line, const char* function) {
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
jeBool jeWindow_getProgramOk(jeWindow* window, GLuint program, const char* file, int line, const char* function) {
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
void jeWindow_flushVertexBuffer(jeWindow* window) {
	GLenum primitiveType;
	switch (window->vboPrimitiveType) {
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
			JE_ERROR("jeWindow_flushVertexBuffer(): unknown primitive type, type=%d", window->vboPrimitiveType);
			goto finalize;
		}
	}

	glUseProgram(window->program);
	glBindVertexArray(window->vao);

	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	glDrawArrays(primitiveType, 0, window->vboVertexCount);

	glBindVertexArray(0);
	glUseProgram(0);

	jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_flushVertexBuffer()");

	finalize: {
		window->vboVertexCount = 0;
		memset((void*)window->vboData, 0, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex));
	}
}
void jeWindow_drawPoint(jeWindow* window, jeRenderable point) {
	static const GLuint vertexCount = 1;

	float worldScaleX = 0.0f;
	float worldScaleY = 0.0f;

	jeBool bufferCanFitRenderable = ((window->vboVertexCount + vertexCount) <= JE_WINDOW_VERTEX_BUFFER_CAPACITY);
	jeBool bufferPrimitiveIsCorrect = (window->vboPrimitiveType == JE_PRIMITIVE_TYPE_POINTS);

	if (!bufferCanFitRenderable || !bufferPrimitiveIsCorrect) {
		jeWindow_flushVertexBuffer(window);

		if (!bufferPrimitiveIsCorrect) {
			window->vboPrimitiveType = JE_PRIMITIVE_TYPE_POINTS;
		}
	}

	/*Transform pos from world coords (+/- windowSize) to screen coords (-1.0 to 1.0)*/
	worldScaleX = 2.0f / JE_WINDOW_MIN_WIDTH;
	worldScaleY = 2.0f / JE_WINDOW_MIN_HEIGHT;
	point.x1 = (point.x1 * worldScaleX);
	point.y1 = (-point.y1 * worldScaleY);
	point.z = point.z / (float)(1 << 20);  /*Support depths +/- 2^20.  Close to limit of integer representation for float32*/

	/*Normalize pixel uvs to between 0.0 and 1.0*/
	point.u1 = point.u1 / (float)window->image.width;
	point.v1 = point.v1 / (float)window->image.height;

	window->vboData[window->vboVertexCount].x = point.x1;
	window->vboData[window->vboVertexCount].y = point.y1;
	window->vboData[window->vboVertexCount].z = point.z;
	window->vboData[window->vboVertexCount].r = point.r;
	window->vboData[window->vboVertexCount].g = point.g;
	window->vboData[window->vboVertexCount].b = point.b;
	window->vboData[window->vboVertexCount].a = point.a;
	window->vboData[window->vboVertexCount].u = point.u1;
	window->vboData[window->vboVertexCount].v = point.v1;
	window->vboVertexCount++;
}
void jeWindow_drawLine(jeWindow* window, jeRenderable line) {
	static const GLuint vertexCount = 6;

	float worldScaleX = 0.0f;
	float worldScaleY = 0.0f;

	jeBool bufferCanFitRenderable = ((window->vboVertexCount + vertexCount) <= JE_WINDOW_VERTEX_BUFFER_CAPACITY);
	jeBool bufferPrimitiveIsCorrect = (window->vboPrimitiveType == JE_PRIMITIVE_TYPE_LINES);

	if (!bufferCanFitRenderable || !bufferPrimitiveIsCorrect) {
		jeWindow_flushVertexBuffer(window);

		if (!bufferPrimitiveIsCorrect) {
			window->vboPrimitiveType = JE_PRIMITIVE_TYPE_LINES;
		}
	}

	/*Transform pos from world coords (+/- windowSize) to screen coords (-1.0 to 1.0)*/
	worldScaleX = 2.0f / JE_WINDOW_MIN_WIDTH;
	worldScaleY = 2.0f / JE_WINDOW_MIN_HEIGHT;
	line.x1 = (line.x1 * worldScaleX);
	line.x2 = (line.x2 * worldScaleX);
	line.y1 = (-line.y1 * worldScaleY);
	line.y2 = (-line.y2 * worldScaleY);
	line.z = line.z / (float)(1 << 20);  /*Support depths +/- 2^20.  Close to limit of integer representation for float32*/

	/*Normalize pixel uvs to between 0.0 and 1.0*/
	line.u1 = line.u1 / (float)window->image.width;
	line.u2 = line.u2 / (float)window->image.width;
	line.v1 = line.v1 / (float)window->image.height;
	line.v2 = line.v2 / (float)window->image.height;

	window->vboData[window->vboVertexCount].x = line.x1;
	window->vboData[window->vboVertexCount].y = line.y1;
	window->vboData[window->vboVertexCount].z = line.z;
	window->vboData[window->vboVertexCount].r = line.r;
	window->vboData[window->vboVertexCount].g = line.g;
	window->vboData[window->vboVertexCount].b = line.b;
	window->vboData[window->vboVertexCount].a = line.a;
	window->vboData[window->vboVertexCount].u = line.u1;
	window->vboData[window->vboVertexCount].v = line.v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = line.x2;
	window->vboData[window->vboVertexCount].y = line.y2;
	window->vboData[window->vboVertexCount].z = line.z;
	window->vboData[window->vboVertexCount].r = line.r;
	window->vboData[window->vboVertexCount].g = line.g;
	window->vboData[window->vboVertexCount].b = line.b;
	window->vboData[window->vboVertexCount].a = line.a;
	window->vboData[window->vboVertexCount].u = line.u2;
	window->vboData[window->vboVertexCount].v = line.v2;
	window->vboVertexCount++;
}
void jeWindow_drawTriangle(jeWindow* window, jeRenderable triangle) {
	static const GLuint vertexCount = 3;

	float worldScaleX = 0.0f;
	float worldScaleY = 0.0f;

	jeBool bufferCanFitRenderable = ((window->vboVertexCount + vertexCount) <= JE_WINDOW_VERTEX_BUFFER_CAPACITY);
	jeBool bufferPrimitiveIsCorrect = (window->vboPrimitiveType == JE_PRIMITIVE_TYPE_TRIANGLES);

	if (!bufferCanFitRenderable || !bufferPrimitiveIsCorrect) {
		jeWindow_flushVertexBuffer(window);

		if (!bufferPrimitiveIsCorrect) {
			window->vboPrimitiveType = JE_PRIMITIVE_TYPE_TRIANGLES;
		}
	}

	/*Transform pos from world coords (+/- windowSize) to screen coords (-1.0 to 1.0)*/
	worldScaleX = 2.0f / JE_WINDOW_MIN_WIDTH;
	worldScaleY = 2.0f / JE_WINDOW_MIN_HEIGHT;
	triangle.x1 = (triangle.x1 * worldScaleX);
	triangle.x2 = (triangle.x2 * worldScaleX);
	triangle.x3 = (triangle.x2 * worldScaleX);
	triangle.y1 = (-triangle.y1 * worldScaleY);
	triangle.y2 = (-triangle.y2 * worldScaleY);
	triangle.y3 = (-triangle.y2 * worldScaleY);
	triangle.z = triangle.z / (float)(1 << 20);  /*Support depths +/- 2^20.  Close to limit of integer representation for float32*/

	/*Normalize pixel uvs to between 0.0 and 1.0*/
	triangle.u1 = triangle.u1 / (float)window->image.width;
	triangle.u2 = triangle.u2 / (float)window->image.width;
	triangle.u3 = triangle.u3 / (float)window->image.width;
	triangle.v1 = triangle.v1 / (float)window->image.height;
	triangle.v2 = triangle.v2 / (float)window->image.height;
	triangle.v3 = triangle.v3 / (float)window->image.height;

	window->vboData[window->vboVertexCount].x = triangle.x1;
	window->vboData[window->vboVertexCount].y = triangle.y1;
	window->vboData[window->vboVertexCount].z = triangle.z;
	window->vboData[window->vboVertexCount].r = triangle.r;
	window->vboData[window->vboVertexCount].g = triangle.g;
	window->vboData[window->vboVertexCount].b = triangle.b;
	window->vboData[window->vboVertexCount].a = triangle.a;
	window->vboData[window->vboVertexCount].u = triangle.u1;
	window->vboData[window->vboVertexCount].v = triangle.v1;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = triangle.x2;
	window->vboData[window->vboVertexCount].y = triangle.y2;
	window->vboData[window->vboVertexCount].z = triangle.z;
	window->vboData[window->vboVertexCount].r = triangle.r;
	window->vboData[window->vboVertexCount].g = triangle.g;
	window->vboData[window->vboVertexCount].b = triangle.b;
	window->vboData[window->vboVertexCount].a = triangle.a;
	window->vboData[window->vboVertexCount].u = triangle.u2;
	window->vboData[window->vboVertexCount].v = triangle.v2;
	window->vboVertexCount++;

	window->vboData[window->vboVertexCount].x = triangle.x3;
	window->vboData[window->vboVertexCount].y = triangle.y3;
	window->vboData[window->vboVertexCount].z = triangle.z;
	window->vboData[window->vboVertexCount].r = triangle.r;
	window->vboData[window->vboVertexCount].g = triangle.g;
	window->vboData[window->vboVertexCount].b = triangle.b;
	window->vboData[window->vboVertexCount].a = triangle.a;
	window->vboData[window->vboVertexCount].u = triangle.u3;
	window->vboData[window->vboVertexCount].v = triangle.v3;
	window->vboVertexCount++;
}
void jeWindow_drawSprite(jeWindow* window, jeRenderable sprite) {
	static const GLuint vertexCount = 6;

	float worldScaleX = 0.0f;
	float worldScaleY = 0.0f;

	jeBool bufferCanFitRenderable = ((window->vboVertexCount + vertexCount) <= JE_WINDOW_VERTEX_BUFFER_CAPACITY);
	jeBool bufferPrimitiveIsCorrect = (window->vboPrimitiveType == JE_PRIMITIVE_TYPE_SPRITES);

	if (!bufferCanFitRenderable || !bufferPrimitiveIsCorrect) {
		jeWindow_flushVertexBuffer(window);

		if (!bufferPrimitiveIsCorrect) {
			window->vboPrimitiveType = JE_PRIMITIVE_TYPE_SPRITES;
		}
	}

	/*Transform pos from world coords (+/- windowSize) to screen coords (-1.0 to 1.0)*/
	worldScaleX = 2.0f / JE_WINDOW_MIN_WIDTH;
	worldScaleY = 2.0f / JE_WINDOW_MIN_HEIGHT;
	sprite.x1 = (sprite.x1 * worldScaleX);
	sprite.x2 = (sprite.x2 * worldScaleX);
	sprite.y1 = (-sprite.y1 * worldScaleY);
	sprite.y2 = (-sprite.y2 * worldScaleY);
	sprite.z = sprite.z / (float)(1 << 20);  /*Support depths +/- 2^20.  Close to limit of integer representation for float32*/

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
void jeWindow_drawRenderable(jeWindow* window, jeRenderable sprite) {
	jeDepthQueue_insert(&window->depthQueue, sprite);
}
void jeWindow_flushDepthQueue(jeWindow* window) {
	int i = 0;

	jeRenderable renderable;

	memset((void*)&renderable, 0, sizeof(renderable));

	jeDepthQueue_sort(&window->depthQueue);

	for (i = 0; i < window->depthQueue.count; i++) {
		renderable = window->depthQueue.renderables[i];
		switch (renderable.primitiveType) {
			case JE_PRIMITIVE_TYPE_POINTS: {
				jeWindow_drawPoint(window, renderable);
				break;
			}
			case JE_PRIMITIVE_TYPE_LINES: {
				jeWindow_drawLine(window, renderable);
				break;
			}
			case JE_PRIMITIVE_TYPE_TRIANGLES: {
				jeWindow_drawTriangle(window, renderable);
				break;
			}
			case JE_PRIMITIVE_TYPE_SPRITES: {
				jeWindow_drawSprite(window, renderable);
				break;
			}
			default: {
				JE_WARN("jeWindow_flushDepthQueue(): unrecognized type, primitive=%d, index=%d, count=%d",
						renderable.primitiveType, i, window->depthQueue.count);
				break;
			}
		}
	}
	window->depthQueue.count = 0;
}
void jeWindow_clear(jeWindow* window) {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	JE_MAYBE_UNUSED(window);
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

	GLint lineWidthRange[2];
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
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	window->vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(window->vertShader, 1, &jeWindow_vertShaderPtr, &jeWindow_vertShaderSize);
	glCompileShader(window->vertShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}
	if (!jeWindow_getShaderOk(window, window->vertShader, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getShaderOk() failed");
		goto finalize;
	}

	window->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(window->fragShader, 1, &jeWindow_fragShaderPtr, &jeWindow_fragShaderSize);
	glCompileShader(window->fragShader);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}
	if (!jeWindow_getShaderOk(window, window->fragShader, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getShaderOk() failed");
		goto finalize;
	}

	window->program = glCreateProgram();
	glAttachShader(window->program, window->vertShader);
	glAttachShader(window->program, window->fragShader);
	glBindAttribLocation(window->program, 0, "srcPos");
	glBindAttribLocation(window->program, 1, "srcCol");
	glBindAttribLocation(window->program, 2, "srcUv");
	glLinkProgram(window->program);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}
	if (!jeWindow_getProgramOk(window, window->program, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getProgramOk() failed");
		goto finalize;
	}

	glGenTextures(1, &window->texture);
	glBindTexture(GL_TEXTURE_2D, window->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->image.width, window->image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, window->image.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glGenBuffers(1, &window->vbo);

	glGenVertexArrays(1, &window->vao);

	glBindVertexArray(window->vao);
	glBindBuffer(GL_ARRAY_BUFFER, window->vbo);
	glBufferData(GL_ARRAY_BUFFER, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex), (const GLvoid*)window->vboData, GL_DYNAMIC_DRAW);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)0);
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(3 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(jeVertex), (const GLvoid*)(7 * sizeof(GLfloat)));
	if (!jeWindow_getGlOk(window, JE_LOG_CONTEXT, "jeWindow_initGL()")) {
		JE_ERROR("jeWindow_initGL(): jeWindow_getGlOk() failed");
		goto finalize;
	}

	glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
	if ((scale < lineWidthRange[0]) || (scale > lineWidthRange[1])) {
		JE_WARN("jeWindow_initGL(): scale not in supported lineWidthRange, scale=%d, min=%d, max=%d",
				scale, lineWidthRange[0], lineWidthRange[1]);
	}
	glLineWidth(scale);

	glBindVertexArray(0);

	window->vboPrimitiveType = JE_PRIMITIVE_TYPE_TRIANGLES;
	window->vboVertexCount = 0;
	memset((void*)window->vboData, 0, JE_WINDOW_VERTEX_BUFFER_CAPACITY * sizeof(jeVertex));

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
	jeWindow_flushDepthQueue(window);
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

	window->open = JE_FALSE;

	jeWindow_destroyGL(window);

	jeDepthQueue_destroy(&window->depthQueue);

	jeController_destroy(&window->controller);

	jeImage_destroy(&window->image);

	if (window->window != NULL) {
		SDL_DestroyWindow(window->window);
		window->window = NULL;
	}

	SDL_Quit();

	free(window);
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

	if (jeImage_createFromFile(&window->image, JE_WINDOW_SPRITE_FILENAME) == JE_FALSE) {
		JE_ERROR("jeWindow_create(): jeImage_createFromFile() failed");
		goto finalize;
	}

	jeDepthQueue_create(&window->depthQueue);

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
jeBool jeWindow_getInputState(jeWindow* window, int inputId) {
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