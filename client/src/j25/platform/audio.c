#include <j25/platform/audio.h>

#include <j25/core/common.h>
#include <j25/core/container.h>


#include <string.h>
#include <SDL2/SDL.h>

#define JE_AUDIO_DRIVER_DEVICES_COUNT 4

struct jeAudioDevice {
	SDL_AudioSpec spec;
	SDL_AudioDeviceID id;
};

struct jeAudio {
	SDL_AudioSpec spec;
	Uint8* buffer;
	Uint32 size;
};
struct jeAudioDriver {
	struct jeArray audioAllocations;

	struct jeAudioDevice devices[JE_AUDIO_DRIVER_DEVICES_COUNT];
	uint32_t numDevices;

	// must outlive playback
	const struct jeAudio* deviceAudioLoops[JE_AUDIO_DRIVER_DEVICES_COUNT];
};

void jeAudioDevice_destroy(struct jeAudioDevice* device);
bool jeAudioDevice_create(struct jeAudioDevice* device);
bool jeAudioDevice_setPaused(struct jeAudioDevice* device, bool paused);
bool jeAudioDevice_queueAudio(struct jeAudioDevice* device, const struct jeAudio* audio);
bool jeAudioDevice_clearAudio(const struct jeAudioDevice* device);

void jeAudio_destroy(struct jeAudio* audio);
bool jeAudio_createFromWavFile(struct jeAudio* audio, const struct jeAudioDevice* device, const char* filename);
// TODO move format function to jeAudioDevice to break circular dependency
bool jeAudio_formatForDevice(struct jeAudio* audio, const struct jeAudioDevice* device);

void jeAudioDriver_destroy(struct jeAudioDriver* driver);
struct jeAudioDriver* jeAudioDriver_create(void);
struct jeAudioDriver* jeAudioDriver_getInstance(void);
struct jeAudio* jeAudioDriver_getAudioRaw(struct jeAudioDriver* driver, jeAudioId audioId);
bool jeAudioDriver_getAudioLoaded(struct jeAudioDriver* driver, jeAudioId audioId);
jeAudioId jeAudioDriver_loadAudioFromWavFile(struct jeAudioDriver* driver, const char* filename);
bool jeAudioDriver_unloadAudio(struct jeAudioDriver* driver, jeAudioId audioId);
bool jeAudioDriver_playAudioRaw(struct jeAudioDriver* driver, const struct jeAudio* audio /* must outlive playback */, bool shouldLoop);
bool jeAudioDriver_playAudio(struct jeAudioDriver* driver, jeAudioId audioId, bool shouldLoop);
bool jeAudioDriver_clearAudio(struct jeAudioDriver* driver);
bool jeAudioDriver_pump(struct jeAudioDriver* driver);

void jeAudioDevice_destroy(struct jeAudioDevice* device) {
	JE_TRACE("device=%p", (void*)device);

	if (device != NULL) {
		if (device->id != 0) {
			SDL_CloseAudioDevice(device->id);
			device->id = 0;
		}

		memset((void*)device, 0, sizeof(*device));
	}
}
bool jeAudioDevice_create(struct jeAudioDevice* device) {
	bool ok = true;

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	if (ok) {
		memset(device, 0, sizeof(*device));
		device->id = 0;
	}

	SDL_AudioSpec desiredSpec;
	memset(&desiredSpec, 0, sizeof(desiredSpec));
	desiredSpec.freq = 48000;
	desiredSpec.format = AUDIO_F32;
	desiredSpec.channels = 2;
	desiredSpec.samples = 4096;
	desiredSpec.callback = NULL;

	if (ok) {
		device->id = SDL_OpenAudioDevice(
			/*deviceName*/ NULL,
			/*isCapture*/ 0,
			&desiredSpec,
			&device->spec,
			/*allowed_changes*/ 0
		);

		if (device->id == 0) {
			JE_ERROR("SDL_OpenAudioDevice() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	ok = ok && jeAudioDevice_setPaused(device, false);

	if (!ok) {
		jeAudioDevice_destroy(device);
	}

	return ok;
}
bool jeAudioDevice_setPaused(struct jeAudioDevice* device, bool paused) {
	JE_TRACE("device=%p, paused=%u", (void*)device, (unsigned)paused);

	bool ok = true;

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	if (ok) {
		SDL_PauseAudioDevice(device->id, (int)paused);
	}

	return ok;
}
bool jeAudioDevice_queueAudio(struct jeAudioDevice* device, const struct jeAudio* audio) {
	JE_TRACE("device=%p, audio=%p", (void*)device, (void*)audio);

	bool ok = true;

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	if (audio == NULL) {
		JE_ERROR("audio=NULL");
		ok = false;
	}

	ok = ok && jeAudioDevice_clearAudio(device);

	bool mustQueue = true;

	if (ok) {
		mustQueue = mustQueue && (audio->buffer != NULL);
	}

	if (ok && mustQueue) {
		if (SDL_QueueAudio(device->id, audio->buffer, audio->size) < 0) {
			JE_ERROR("SDL_QueueAudio() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	return ok;
}
bool jeAudioDevice_clearAudio(const struct jeAudioDevice* device) {
	JE_TRACE("device=%p", (void*)device);

	bool ok = true;

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	if (ok) {
		SDL_ClearQueuedAudio(device->id);
	}

	return ok;
}

void jeAudio_destroy(struct jeAudio* audio) {
	JE_TRACE("audio=%p", (void*)audio);

	if (audio != NULL) {
		if (audio->buffer != NULL) {
			SDL_FreeWAV(audio->buffer);
			audio->buffer = NULL;
			audio->size = 0;
		}

		memset(&audio, 0, sizeof(*audio));
	}
}
bool jeAudio_createFromWavFile(struct jeAudio* audio, const struct jeAudioDevice* device, const char* filename) {
	JE_TRACE("device=%p, filename=%s", (void*)device, filename ? filename : "<NULL>");

	bool ok = true;

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	if (audio == NULL) {
		JE_ERROR("audio=NULL");
		ok = false;
	}

	if (ok) {
		memset((void*)audio, 0, sizeof(*audio));
		audio->buffer = NULL;
		audio->size = 0;
	}

	if (!ok) {
		jeAudio_destroy(audio);
		audio = NULL;
	}

	if (audio == NULL) {
		JE_ERROR("audio=NULL");
		ok = false;
	}

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	if (filename == NULL) {
		JE_ERROR("filename=NULL");
		ok = false;
	}

	if (ok) {
		if (SDL_LoadWAV(filename, &audio->spec, &audio->buffer, &audio->size) == NULL) {
			JE_ERROR("SDL_LoadWAV() failed with error=%s", SDL_GetError());
			ok = false;
		}

		JE_DEBUG(
			"completed, filename=%s, size=%u, frequency=%u, channels=%u",
			filename,
			(Uint32)audio->size,
			(Uint32)audio->spec.freq,
			(Uint32)audio->spec.channels
		);
	}

	ok = ok && jeAudio_formatForDevice(audio, device);

	if (!ok) {
		jeAudio_destroy(audio);
	}

	return ok;
}
bool jeAudio_formatForDevice(struct jeAudio* audio, const struct jeAudioDevice* device) {
	JE_TRACE("audio=%p, device=%p", (void*)audio, (void*)device);

	bool ok = true;

	if (audio == NULL) {
		JE_ERROR("audio=NULL");
		ok = false;
	}

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	ok = ok && (audio->buffer != NULL);

	SDL_AudioCVT converter;
	memset((void*)&converter, 0, sizeof(converter));
	converter.buf = NULL;

	if (ok) {
		int result = SDL_BuildAudioCVT(&converter,
			audio->spec.format,
			audio->spec.channels,
			audio->spec.freq,
			device->spec.format,
			device->spec.channels,
			device->spec.freq
		);

		if (result < 0) {
			JE_ERROR("SDL_BuildAudioCVT() failed with error=%s", SDL_GetError());
			ok = false;
		}

		if (converter.buf != NULL) {
			JE_ERROR("SDL_BuildAudioCVT() allocated a buffer, this is unexpected and may leak");
			ok = false;
		}
	}

	if (ok) {
		converter.len = (int)audio->size;
		converter.buf = (Uint8*)malloc((size_t)(converter.len * converter.len_mult));

		if (converter.buf == NULL) {
			JE_ERROR("malloc() failed");
			ok = false;
		}
	}

	if (ok) {
		memcpy(converter.buf,
			audio->buffer,
			audio->size);

		if (SDL_ConvertAudio(&converter) < 0) {
			JE_ERROR("SDL_ConvertAudio() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok) {
		SDL_FreeWAV(audio->buffer);
		audio->size = (Uint32)converter.len;
		audio->buffer = converter.buf;
		audio->spec.format = device->spec.format;
		audio->spec.channels = device->spec.channels;
		audio->spec.freq = device->spec.freq;
	}

	if (!ok && (converter.buf != NULL)) {
		free((void*)converter.buf);
	}

	return ok;
}

void jeAudioDriver_destroy(struct jeAudioDriver* driver) {
	JE_TRACE("driver=%p", (void*)driver);

	if (driver != NULL) {
		for (uint32_t i = 0; i < jeArray_getCount(&driver->audioAllocations); i++) {
			struct jeAudio* audio = (struct jeAudio*)jeArray_get(&driver->audioAllocations, i);
			if ((audio != NULL) && (audio->buffer != NULL)) {
				jeAudio_destroy(audio);
			}
		}
		jeArray_destroy(&driver->audioAllocations);

		driver->numDevices = 0;

		for (uint32_t i = 0; i < driver->numDevices; i++) {
			jeAudioDevice_destroy(&driver->devices[i]);
		}

		free(driver);
		driver = NULL;
	}
}
struct jeAudioDriver* jeAudioDriver_create(void) {
	bool ok = true;

	struct jeAudioDriver* driver = NULL;

	if (ok) {
		driver = malloc(sizeof(struct jeAudioDriver));
	}

	if (driver == NULL) {
		JE_ERROR("malloc failed");
		ok = false;
	}

	if (ok) {
		memset(driver, 0, sizeof(*driver));
	}

	if (ok) {
		for (uint32_t i = 0; i < JE_AUDIO_DRIVER_DEVICES_COUNT; i++) {
			if (!jeAudioDevice_create(&driver->devices[i])) {
				if (driver->numDevices == 0) {
					ok = false;
				}
				break;
			}

			driver->numDevices++;
		}
	}

	ok = ok && jeArray_create(&driver->audioAllocations, sizeof(struct jeAudio));

	if (!ok) {
		jeAudioDriver_destroy(driver);
		driver = NULL;
	}

	return driver;
}
struct jeAudioDriver* jeAudioDriver_getInstance(void) {
	static struct jeAudioDriver* driver = {0};

	if (driver == NULL) {
		driver = jeAudioDriver_create();
	}

	return driver;
}
struct jeAudio* jeAudioDriver_getAudioRaw(struct jeAudioDriver* driver, jeAudioId audioId) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	if (audioId == JE_AUDIO_ID_INVALID) {
		JE_ERROR("audioId=JE_AUDIO_ID_INVALID");
		ok = false;
	}

	if (ok) {
		uint32_t audioCount = jeArray_getCount(&driver->audioAllocations);

		if (((uint32_t)audioId >= jeArray_getCount(&driver->audioAllocations))) {
			JE_ERROR("audioId out of bounds, audioId=%u, count=%u", audioId, audioCount);
			ok = false;
		}
	}

	struct jeAudio* audio = NULL;
	if (ok) {
		audio = jeArray_get(&driver->audioAllocations, (uint32_t)audioId);
		if (audio == NULL) {
			JE_ERROR("audio=NULL");
			ok = false;
		}
	}

	if (ok) {
		// audio has been unloaded
		if (audio->buffer == NULL) {
			audio = NULL;
		}
	}

	return audio;
}
bool jeAudioDriver_getAudioLoaded(struct jeAudioDriver* driver, jeAudioId audioId) {
	return (jeAudioDriver_getAudioRaw(driver, audioId) != NULL);
}
jeAudioId jeAudioDriver_loadAudioFromWavFile(struct jeAudioDriver* driver, const char* filename) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	const struct jeAudioDevice* referenceDevice = NULL;

	if (ok) {
		referenceDevice = &driver->devices[0];
		if (referenceDevice == NULL) {
			JE_ERROR("referenceDevice=NULL");
			ok = false;
		}
	}

	struct jeAudio audio;
	memset(&audio, 0, sizeof(audio));

	ok = ok && jeAudio_createFromWavFile(&audio, referenceDevice, filename);

	if (ok) {
		uint32_t audioCount = jeArray_getCount(&driver->audioAllocations);
		if (audioCount == 0) {
			if (!jeArray_setCount(&driver->audioAllocations, 1)) {
				JE_ERROR("jeArray_setCount(1) failed");
				ok = false;
			}
		}
	}

	jeAudioId audioId = JE_AUDIO_ID_INVALID;
	if (ok) {
		audioId = jeArray_getCount(&driver->audioAllocations);
	}
	ok = ok && jeArray_push(&driver->audioAllocations, (void*)&audio, 1);

	if (!ok) {
		jeAudio_destroy(&audio);
		audioId = JE_AUDIO_ID_INVALID;
	}

	return audioId;
}
bool jeAudioDriver_unloadAudio(struct jeAudioDriver* driver, jeAudioId audioId) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	if (audioId == JE_AUDIO_ID_INVALID) {
		JE_ERROR("audioId=JE_AUDIO_ID_INVALID");
		ok = false;
	}

	if (ok) {
		uint32_t audioCount = jeArray_getCount(&driver->audioAllocations);

		if (((uint32_t)audioId >= jeArray_getCount(&driver->audioAllocations))) {
			JE_ERROR("audioId out of bounds, audioId=%u, count=%u", (uint32_t)audioId, audioCount);
			ok = false;
		}
	}

	struct jeAudio* audio = NULL;
	if (ok) {
		audio = jeArray_get(&driver->audioAllocations, (uint32_t)audioId);
		if (audio == NULL) {
			JE_ERROR("audio=NULL");
			ok = false;
		}
	}

	if (ok) {
		if (audio->buffer == NULL) {
			JE_ERROR("audio->buffer=NULL");
			ok = false;
		}
	}

	if (ok) {
		jeAudio_destroy(audio);
	}

	return ok;
}
struct jeAudioDevice* jeAudioDriver_allocateBestDevice(struct jeAudioDriver* driver, uint32_t* outDeviceIndex) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	struct jeAudioDevice* device = NULL;
	uint32_t deviceIndex = 0;
	if (ok) {
		for (uint32_t i = 0; i < driver->numDevices; i++) {
			if (driver->devices[i].id == 0) {
				JE_ERROR("driver->devices[i].id");
				ok = false;
				continue;
			}

			device = &driver->devices[i];
			deviceIndex = i;

			// stop searching after we find a device that's not in use (the ideal case)
			if (SDL_GetQueuedAudioSize(device->id) == 0) {
				break;
			}
		}
	}

	if (ok) {
		driver->deviceAudioLoops[deviceIndex] = NULL;

		if ((device != NULL) && !jeAudioDevice_clearAudio(device)) {
			ok = false;
			device = NULL;
			deviceIndex = 0;
		}
	}

	*outDeviceIndex = deviceIndex;
	return device;
}
bool jeAudioDriver_playAudioRaw(struct jeAudioDriver* driver, const struct jeAudio* audio /* must outlive playback*/, bool shouldLoop) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	struct jeAudioDevice* device = NULL;
	uint32_t deviceIndex = 0;

	if (ok) {
		device = jeAudioDriver_allocateBestDevice(driver, &deviceIndex);
		if (device == NULL) {
			JE_ERROR("jeAudioDriver_allocateBestDevice() failed");
			ok = false;
		}
	}
	if (ok) {
		if (deviceIndex >= driver->numDevices) {
			JE_ERROR("deviceIndex out of bounds");
			ok = false;
		}
	}

	ok = ok && jeAudioDevice_clearAudio(device);
	ok = ok && jeAudioDevice_queueAudio(device, audio);

	if (ok) {
		if (shouldLoop) {
			driver->deviceAudioLoops[deviceIndex] = audio;
		}
	}

	return ok;
}
bool jeAudioDriver_playAudio(struct jeAudioDriver* driver, jeAudioId audioId, bool shouldLoop) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	if (audioId == JE_AUDIO_ID_INVALID) {
		JE_ERROR("audioId=JE_AUDIO_ID_INVALID");
		ok = false;
	}

	struct jeAudio* audio = NULL;
	if (ok) {
		audio = jeAudioDriver_getAudioRaw(driver, audioId);
		if (audio == NULL) {
			JE_ERROR("audio=NULL");
			ok = false;
		}
	}

	ok = ok && jeAudioDriver_playAudioRaw(driver, audio, shouldLoop);

	return ok;
}
bool jeAudioDriver_clearAudio(struct jeAudioDriver* driver) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	if (ok) {
		for (uint32_t i = 0; i < driver->numDevices; i++) {
			struct jeAudioDevice* device = &driver->devices[i];
			if (device->id == 0) {
				JE_ERROR("device->id=0");
				ok = false;
				continue;
			}

			jeAudioDevice_clearAudio(device);
			driver->deviceAudioLoops[i] = NULL;
		}
	}

	return ok;
}
bool jeAudioDriver_pump(struct jeAudioDriver* driver) {
	bool ok = true;

	if (driver == NULL) {
		JE_ERROR("driver=NULL");
		ok = false;
	}

	if (ok) {
		for (uint32_t i = 0; i < driver->numDevices; i++) {
			struct jeAudioDevice* device = &driver->devices[i];
			if (device->id == 0) {
				JE_ERROR("device->id=0");
				ok = false;
				continue;
			}

			const struct jeAudio* audioLoop = driver->deviceAudioLoops[i];
			if (audioLoop == NULL) {
				continue;
			}

			if (SDL_GetQueuedAudioSize(device->id) == 0) {
				ok = ok && jeAudioDevice_queueAudio(device, audioLoop);
			}
		}
	}

	return ok;
}

void jeAudio_runTests() {
#if JE_DEBUGGING

	const char* emptyAudioFilename = "client\\data\\audio_empty.wav";

	{
		struct jeAudioDevice device;
		JE_ASSERT(jeAudioDevice_create(&device));

		struct jeAudio audio;
		JE_ASSERT(jeAudio_createFromWavFile(&audio, &device, emptyAudioFilename));
		JE_ASSERT(jeAudioDevice_setPaused(&device, false));
		JE_ASSERT(jeAudioDevice_queueAudio(&device, &audio));
		JE_ASSERT(jeAudioDevice_setPaused(&device, true));
		JE_ASSERT(jeAudioDevice_clearAudio(&device));

		jeAudio_destroy(&audio);
		jeAudioDevice_destroy(&device);
	}

	{
		struct jeAudioDriver* driver = jeAudioDriver_create();
		JE_ASSERT(driver != NULL);
		jeAudioId audioId = jeAudioDriver_loadAudioFromWavFile(driver, emptyAudioFilename);
		JE_ASSERT(audioId != JE_AUDIO_ID_INVALID);
		JE_ASSERT(jeAudioDriver_getAudioLoaded(driver, audioId));

		JE_ASSERT(jeAudioDriver_playAudio(driver, audioId, /*shouldLoop*/ false));
		JE_ASSERT(jeAudioDriver_pump(driver));
		JE_ASSERT(jeAudioDriver_clearAudio(driver));

		JE_ASSERT(jeAudioDriver_playAudio(driver, audioId, /*shouldLoop*/ true));
		JE_ASSERT(jeAudioDriver_clearAudio(driver));
		JE_ASSERT(jeAudioDriver_pump(driver));

		JE_ASSERT(jeAudioDriver_unloadAudio(driver, audioId));

		jeAudioDriver_destroy(driver);
	}

	JE_ASSERT(jeAudioDriver_getInstance() != NULL);

#endif
}

