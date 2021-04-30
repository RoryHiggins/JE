#pragma once

#if !defined(JE_PLATFORM_AUDIO_H)
#define JE_PLATFORM_AUDIO_H

#include <j25/core/common.h>

#define JE_AUDIO_ID_INVALID (0)

// TODO hide these from header:
#include <SDL2/SDL.h>
struct jeAudioDevice {
	SDL_AudioSpec spec;
	SDL_AudioDeviceID id;
};

struct jeAudio {
	SDL_AudioSpec spec;
	Uint8* buffer;
	Uint32 size;
};
struct jeAudio;

struct jeAudioDriver;

typedef uint32_t jeAudioId;

// TODO hide these from header:
JE_API_PUBLIC void jeAudio_destroy(struct jeAudio* audio);
JE_API_PUBLIC bool jeAudio_createFromWavFile(struct jeAudio* audio, const struct jeAudioDevice* device, const char* filename);

JE_API_PUBLIC struct jeAudioDriver* jeAudioDriver_getInstance(void);
JE_API_PUBLIC bool jeAudioDriver_getAudioLoaded(struct jeAudioDriver* driver, jeAudioId audioId);
JE_API_PUBLIC jeAudioId jeAudioDriver_loadAudioFromWavFile(struct jeAudioDriver* driver, const char* filename);
JE_API_PUBLIC bool jeAudioDriver_unloadAudio(struct jeAudioDriver* driver, jeAudioId audioId);
JE_API_PUBLIC bool jeAudioDriver_playAudio(struct jeAudioDriver* driver, jeAudioId audioId);
JE_API_PUBLIC bool jeAudioDriver_clearAudio(struct jeAudioDriver* driver);
JE_API_PUBLIC bool jeAudioDriver_pump(struct jeAudioDriver* driver);

// TODO hide/remove from header:
JE_API_PUBLIC struct jeAudioDevice* jeAudioDriver_getMusicAudioDevice(struct jeAudioDriver* driver);
JE_API_PUBLIC bool jeAudioDriver_loopMusicRaw(struct jeAudioDriver* driver, const struct jeAudio* audio /* MUST OUTLIVE DRIVER*/);

JE_API_PUBLIC void jeAudio_runTests();

#endif
