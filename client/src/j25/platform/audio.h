#pragma once

#if !defined(JE_PLATFORM_AUDIO_H)
#define JE_PLATFORM_AUDIO_H

#include <j25/core/common.h>

struct jeAudioDevice;
struct jeAudio;
struct jeAudioMixer;

JE_API_PUBLIC void jeAudioDevice_destroy(struct jeAudioDevice* device);
JE_API_PUBLIC struct jeAudioDevice* jeAudioDevice_create(void);
JE_API_PUBLIC bool jeAudioDevice_setPaused(struct jeAudioDevice* device, bool paused);
JE_API_PUBLIC bool jeAudioDevice_queue(struct jeAudioDevice* device, const struct jeAudio* audio);
JE_API_PUBLIC bool jeAudioDevice_clear(const struct jeAudioDevice* device);

JE_API_PUBLIC void jeAudio_destroy(struct jeAudio* audio);
JE_API_PUBLIC struct jeAudio* jeAudio_create(const struct jeAudioDevice* device);
JE_API_PUBLIC struct jeAudio* jeAudio_createFromWavFile(const struct jeAudioDevice* device, const char* filename);
JE_API_PUBLIC bool jeAudio_formatForDevice(struct jeAudio* audio, const struct jeAudioDevice* device);

JE_API_PUBLIC void jeAudioMixer_destroy(struct jeAudioMixer* mixer);
JE_API_PUBLIC struct jeAudioMixer* jeAudioMixer_create(void);
JE_API_PUBLIC struct jeAudioDevice* jeAudioMixer_getMusicAudioDevice(struct jeAudioMixer* mixer);
JE_API_PUBLIC struct jeAudioDevice* jeAudioMixer_playSound(struct jeAudioMixer* mixer, const struct jeAudio* audio);

JE_API_PUBLIC void jeAudio_runTests();

#endif
