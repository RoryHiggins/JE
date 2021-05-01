#pragma once

#include <j25/core/common.h>

#if !defined(JE_PLATFORM_AUDIO_H)
#define JE_PLATFORM_AUDIO_H

#define JE_AUDIO_ID_INVALID (0)

typedef uint32_t jeAudioId;

struct jeAudioDriver;

JE_API_PUBLIC struct jeAudioDriver* jeAudioDriver_getInstance(void);
JE_API_PUBLIC bool jeAudioDriver_getAudioLoaded(struct jeAudioDriver* driver, jeAudioId audioId);
JE_API_PUBLIC jeAudioId jeAudioDriver_loadAudioFromWavFile(struct jeAudioDriver* driver, const char* filename);
JE_API_PUBLIC bool jeAudioDriver_unloadAudio(struct jeAudioDriver* driver, jeAudioId audioId);
JE_API_PUBLIC bool jeAudioDriver_playAudio(struct jeAudioDriver* driver, jeAudioId audioId, bool shouldLoop);
JE_API_PUBLIC bool jeAudioDriver_stopAllAudio(struct jeAudioDriver* driver);
JE_API_PUBLIC bool jeAudioDriver_pump(struct jeAudioDriver* driver);

JE_API_PUBLIC void jeAudio_runTests();

#endif
