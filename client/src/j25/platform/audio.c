#include <j25/platform/audio.h>

#include <j25/core/common.h>


#include <string.h>
// #include <ogg/ogg.h>
#include <SDL2/SDL.h>

#define JE_AUDIO_MIXER_DEVICES_COUNT 3

struct jeAudioDevice {
	SDL_AudioSpec spec;
	SDL_AudioDeviceID id;
};

struct jeAudio {
	SDL_AudioSpec spec;
	Uint8* buffer;
	Uint32 size;
};

struct jeAudioMixer {
	struct jeAudioDevice* musicDevice;
	struct jeAudioDevice* soundDevices[JE_AUDIO_MIXER_DEVICES_COUNT];
	uint32_t next_device;
	uint32_t num_devices;
	bool loop_music_device;
	const struct jeAudio* music_loop_audio; // NON-OWNING; MUST OUTLIVE MIXER
};

void jeAudioDevice_destroy(struct jeAudioDevice* device) {
	JE_TRACE("device=%p", (void*)device);

	if ((device != NULL) && (device->id != 0)) {
		SDL_CloseAudioDevice(device->id);
		device->id = 0;
	}

	if (device != NULL) {
		free(device);
		device = NULL;
	}
}
struct jeAudioDevice* jeAudioDevice_create(void) {
	bool ok = true;

	struct jeAudioDevice* device = NULL;

	if (ok) {
		device = malloc(sizeof(struct jeAudioDevice));
	}

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
		device = NULL;
	}

	return device;
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
bool jeAudioDevice_queue(struct jeAudioDevice* device, const struct jeAudio* audio) {
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

	ok = ok && jeAudioDevice_clear(device);

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
bool jeAudioDevice_clear(const struct jeAudioDevice* device) {
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

	if ((audio != NULL) && (audio->buffer != NULL)) {
		SDL_FreeWAV(audio->buffer);
		memset(&audio->spec, 0, sizeof(audio->spec));
		audio->buffer = NULL;
		audio->size = 0;
	}

	if (audio != NULL) {
		free(audio);
		audio = NULL;
	}
}
struct jeAudio* jeAudio_create(const struct jeAudioDevice* device) {
	JE_TRACE("device=%p", (void*)device);

	bool ok = true;

	if (device == NULL) {
		JE_ERROR("device=NULL");
		ok = false;
	}

	struct jeAudio* audio = NULL;
	if (ok) {
		audio = malloc(sizeof(struct jeAudio));
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

	return audio;
}
struct jeAudio* jeAudio_createFromWavFile(const struct jeAudioDevice* device, const char* filename) {
	JE_TRACE("device=%p, filename=%s", (void*)device, filename ? filename : "<NULL>");

	bool ok = true;

	struct jeAudio* audio = jeAudio_create(device);

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
		audio = NULL;
	}

	return audio;
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

	bool mustConvert = true;
	if (ok) {
		mustConvert = mustConvert && (audio->buffer != NULL);
	}

	SDL_AudioCVT converter;
	memset((void*)&converter, 0, sizeof(converter));
	converter.buf = NULL;

	if (ok && mustConvert) {
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

	if (ok && mustConvert) {
		converter.len = (int)audio->size;
		converter.buf = (Uint8*)malloc((size_t)(converter.len * converter.len_mult));

		if (converter.buf == NULL) {
			JE_ERROR("malloc() failed");
			ok = false;
		}
	}

	if (ok && mustConvert) {
		memcpy(converter.buf, audio->buffer, audio->size);

		if (SDL_ConvertAudio(&converter) < 0) {
			JE_ERROR("SDL_ConvertAudio() failed with error=%s", SDL_GetError());
			ok = false;
		}
	}

	if (ok && mustConvert) {
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

void jeAudioMixer_destroy(struct jeAudioMixer* mixer) {
	JE_TRACE("mixer=%p", (void*)mixer);

	if (mixer != NULL) {
		mixer->num_devices = 0;
		mixer->next_device = 0;

		jeAudioDevice_destroy(mixer->musicDevice);
		mixer->musicDevice = NULL;

		for (uint32_t i = 0; i < mixer->num_devices; i++) {
			jeAudioDevice_destroy(mixer->soundDevices[i]);
			mixer->soundDevices[i] = NULL;
		}

		free(mixer);
		mixer = NULL;
	}
}
struct jeAudioMixer* jeAudioMixer_create(void) {
	bool ok = true;

	struct jeAudioMixer* mixer = NULL;

	if (ok) {
		mixer = malloc(sizeof(struct jeAudioMixer));
	}

	if (mixer == NULL) {
		JE_ERROR("mixer=NULL");
		ok = false;
	}

	if (ok) {
		memset(mixer, 0, sizeof(*mixer));

		mixer->musicDevice = jeAudioDevice_create();
		if (mixer->musicDevice == NULL) {
			ok = false;
		}
	}

	if (ok) {
		for (uint32_t i = 0; i < JE_AUDIO_MIXER_DEVICES_COUNT; i++) {
			mixer->soundDevices[i] = jeAudioDevice_create();
			if (mixer->soundDevices[i] == NULL) {
				if (mixer->num_devices == 0) {
					ok = false;
				}
				break;
			}
			mixer->num_devices++;
		}

		mixer->next_device = 0;
	}

	if (!ok) {
		jeAudioMixer_destroy(mixer);
		mixer = NULL;
	}

	return mixer;
}
struct jeAudioDevice* jeAudioMixer_getMusicAudioDevice(struct jeAudioMixer* mixer) {
	bool ok = true;

	if (mixer == NULL) {
		JE_ERROR("mixer=NULL");
		ok = false;
	}

	struct jeAudioDevice* device = NULL;

	if (ok) {
		device = mixer->musicDevice;
	}

	return device;
}
bool jeAudioMixer_loopMusic(struct jeAudioMixer* mixer, const struct jeAudio* audio) {
	bool ok = true;

	if (mixer == NULL) {
		JE_ERROR("mixer=NULL");
		ok = false;
	}

	if (audio == NULL) {
		JE_ERROR("audio=NULL");
		ok = false;
	}

	struct jeAudioDevice* music_device = NULL;

	if (ok) {
		music_device = jeAudioMixer_getMusicAudioDevice(mixer);
		if (music_device == NULL) {
			ok = false;
		}
	}

	ok = ok && jeAudioDevice_clear(music_device);

	if (ok) {
		mixer->music_loop_audio = audio;
	}

	ok = ok && jeAudioDevice_queue(music_device, mixer->music_loop_audio);



	return ok;
}
struct jeAudioDevice* jeAudioMixer_playSound(struct jeAudioMixer* mixer, const struct jeAudio* audio) {
	bool ok = true;

	if (mixer == NULL) {
		JE_ERROR("mixer=NULL");
		ok = false;
	}

	struct jeAudioDevice* device = NULL;

	if (ok) {
		device = mixer->soundDevices[mixer->next_device];
		if (device == NULL) {
			JE_ERROR("device=NULL, index=%u", (unsigned)mixer->next_device);
			ok = false;
		}
	}

	if (ok) {
		mixer->next_device = (mixer->next_device + 1) % mixer->num_devices;
	}

	ok = ok && jeAudioDevice_clear(device);
	ok = ok && jeAudioDevice_queue(device, audio);

	return device;
}
bool jeAudioMixer_step(struct jeAudioMixer* mixer) {
	bool ok = true;

	if (mixer == NULL) {
		JE_ERROR("mixer=NULL");
		ok = false;
	}

	if (ok) {
		if (SDL_GetQueuedAudioSize(jeAudioMixer_getMusicAudioDevice(mixer)->id) == 0) {
			ok = ok && jeAudioDevice_queue(jeAudioMixer_getMusicAudioDevice(mixer), mixer->music_loop_audio);
		}
	}

	return ok;
}

void jeAudio_runTests() {
#if JE_DEBUGGING

	// struct jeAudioDevice* device = jeAudioDevice_create();
	// JE_ASSERT(device != NULL);

	// struct jeAudio* audioEmpty = jeAudio_create(device);
	// JE_ASSERT(audioEmpty != NULL);
	// JE_ASSERT(jeAudio_formatForDevice(audioEmpty, device));
	// jeAudio_destroy(audioEmpty);

	// struct jeAudioMixer* mixer = jeAudioMixer_create();
	// JE_ASSERT(mixer != NULL);

	// // JE_WARN("TODO REMOVE: temporary audio testing code");
	// // {
	// // 	// struct jeAudio* audio = jeAudio_createFromWavFile(device, "tmp\\test_sound.wav");
	// // 	struct jeAudio* audio = jeAudio_createFromWavFile(device, "apps/ld48/data/song1.wav");
	// // 	JE_ASSERT(audio != NULL);
	// // 	JE_ASSERT(jeAudio_formatForDevice(audio, device));
	// // 	JE_ASSERT(jeAudioDevice_queue(device, audio));
	// // 	JE_ASSERT(jeAudioDevice_clear(device));

	// // 	JE_ASSERT(jeAudioMixer_playSound(mixer, audio) != NULL);
	// // 	SDL_Delay(100);
	// // 	// JE_ASSERT(jeAudioMixer_playSound(mixer, audio) != NULL);
	// // 	// SDL_Delay(4000);

	// // 	// disgusting hack: allocate the music and never let it go for the program lifetime!
	// // 	// jeAudio_destroy(audio);
	// // }

	// jeAudioMixer_destroy(mixer);

	// jeAudioDevice_destroy(device);

#endif
}

