/* audio.c - (C) 2023 a dinosaur (zlib) */
#include "audio.h"
#include "util.h"
#include "stb_vorbis.h"
#include <SDL.h>


static const char* getVorbisError(int err)
{
	switch (err)
	{
	case (VORBIS_outofmem): return "System memory is exhausted";
	case (VORBIS_feature_not_supported): return "floor 0 vorbis is not supported";
	case (VORBIS_too_many_channels): return "Exceeded max " STR(STB_VORBIS_MAX_CHANNELS) " Channels";
	case (VORBIS_file_open_failure): return "Failed to open file";
	case (VORBIS_seek_without_length): return "Unseekable stream";
	case (VORBIS_unexpected_eof): return "Unexpected EOF";
	case (VORBIS_seek_invalid): return "Tried to seek past EOF";
	default: return NULL;
	}
}

static void printError(const char* funcName, const char* errString, int err)
{
	if (!errString) errString = "Unknown error";
	if (!funcName) funcName = "?";
	fprintf(stderr, "%s(): %d %s\n", funcName, err, errString);
}

static inline void printVorbisError(const char* funcName, int err)
{
	printError(funcName, getVorbisError(err), err);
}


static vorb* vorbin = NULL;
static SDL_AudioDeviceID dev = 0;
static SDL_AudioSpec spec;
static int volumeMul;

static inline void setVolume(uint8_t volume)
{
	volumeMul = (volume > 0) ? (int)(volume & 0xFF) + 1 : 0;
}

static void sdlAudioCallback(void* user, Uint8* stream, int len)
{
	if (volumeMul > 0)
	{
		short* samples = (short*)stream;
		int numSamples = len / (int)sizeof(short);
		int numChannels = spec.channels;

		// Decode vorbis samples
		int res = stb_vorbis_get_samples_short_interleaved(vorbin, numChannels, samples, numSamples);
		if (res < 0)
			goto audioPanic;
		int vorbSamples = res * numChannels;

		// If we read less than the buffer then we probably hit EOF
		if (vorbSamples < numSamples)
		{
			stb_vorbis_seek_start(vorbin); // Seek to beginning
			res = stb_vorbis_get_samples_short_interleaved(vorbin, numChannels, &samples[vorbSamples], numSamples - vorbSamples);
			if (res < 0)
				goto audioPanic;
			vorbSamples += res * numChannels;
		}

		// Apply volume
		for (int i = 0; i < vorbSamples; ++i)
			samples[i] = (short)(((int)samples[i] * volumeMul) >> 8);

		// If both reads read less than buffer size then zero the rest to be safe
		if (vorbSamples < numSamples)
			SDL_memset(&samples[vorbSamples], 0, (numSamples - vorbSamples) * sizeof(short));
		return;
	}

audioPanic:
	SDL_memset(stream, 0, len);
}

int audioInit(SDL_Window* window)
{
	if (!window)
		return -1;
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		return -1;
	return 0;
}

static int checkVobisInfo(const stb_vorbis_info* vorbNfo)
{
	if (vorbNfo->sample_rate > INT32_MAX)
		return -1;
#if STB_VORBIS_MAX_CHANNELS > UINT8_MAX
	if (vorbNfo.channels > UINT8_MAX)
		return -1;
#endif
	if (vorbNfo->channels < 0)
		return -1;
	return 0;
}

static int openDevice(const stb_vorbis_info* vorbNfo)
{
	SDL_AudioSpec wantSpec =
	{
		.callback = sdlAudioCallback,
		.samples  = (1024U << 3U),
		.format   = AUDIO_S16,

		.freq     = (int)vorbNfo->sample_rate,
		.channels = vorbNfo->channels
	};
	dev = SDL_OpenAudioDevice(NULL, 0, &wantSpec, &spec, 1);
	if (dev == 0)
	{
		printError("audioInit", SDL_GetError(), (int)dev);
		return -1;
	}
	return 0;
}

int audioPlayFile(const char* oggPath, uint8_t volume)
{
	if (!oggPath)
		return -1;

	int err = VORBIS__no_error;
	vorbin = stb_vorbis_open_filename(oggPath, &err, NULL);
	if (err != VORBIS__no_error)
	{
		printVorbisError("audioInit", err);
		return -1;
	}

	stb_vorbis_info vorbNfo = stb_vorbis_get_info(vorbin);
	if (checkVobisInfo(&vorbNfo))
		return -1;
	if (openDevice(&vorbNfo))
		return -1;

	setVolume(volume);
	SDL_PauseAudioDevice(dev, 0);
	return 0;
}

int audioPlayMemory(const void* oggv, uint32_t oggvLen, uint8_t volume)
{
	if (!oggv)
		return -1;

	int err = VORBIS__no_error;
	vorbin = stb_vorbis_open_memory(oggv, (int)oggvLen, &err, NULL);
	if (err != VORBIS__no_error)
	{
		printVorbisError("audioInit", err);
		return -1;
	}

	stb_vorbis_info vorbNfo = stb_vorbis_get_info(vorbin);
	if (checkVobisInfo(&vorbNfo))
		return -1;
	if (openDevice(&vorbNfo))
		return -1;

	setVolume(volume);
	SDL_PauseAudioDevice(dev, 0);
	return 0;
}

void audioClose(void)
{
	SDL_PauseAudioDevice(dev, 0);
	SDL_CloseAudioDevice(dev);
	dev = 0;
	if (vorbin)
	{
		stb_vorbis_close(vorbin);
		vorbin = NULL;
	}
}


int audioIsOpen(void)
{
	if (dev > 0 && vorbin)
		return 1;
	return 0;
}

int audioIsPlaying(void)
{
	if (audioIsOpen() && SDL_GetAudioDeviceStatus(dev) == SDL_AUDIO_PLAYING)
		return 1;
	return 0;
}
