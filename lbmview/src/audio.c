/* audio.c - (C) 2023 a dinosaur (zlib) */
#include "audio.h"
#include "util.h"
#ifdef USE_VORBISFILE
# include <vorbis/vorbisfile.h>
# include <stdbool.h>
# include <limits.h>
# include <errno.h>
#else
# include "stb_vorbis.h"
#endif
# include <stdlib.h>
#include <SDL3/SDL.h>


static const char* getVorbisError(int err)
{
	switch (err)
	{
#ifdef USE_VORBISFILE
	case OV_HOLE:       return "Encountered interruption while decoding";
	case OV_EREAD:      return "Error while reading";
	case OV_EFAULT:     return "Internal decoder failure";
	case OV_EIMPL:      return "Unimplmented mode or unknown request";
	case OV_EINVAL:     return "Invalid argument value";
	case OV_ENOTVORBIS: return "Bitstream is not a Vorbis file";
	case OV_EBADHEADER: return "Invalid Vorbis bitstream header";
	case OV_EVERSION:   return "Vorbis version mismatch";
	case OV_ENOTAUDIO:  return "Not an audio packet";
	case OV_EBADPACKET: return "Invalid packet";
	case OV_EBADLINK:   return "Invalid stream section";
	case OV_ENOSEEK:    return "Unseekable stream";
#else
# define STR(X) #X
	case VORBIS_outofmem:              return "System memory is exhausted";
	case VORBIS_feature_not_supported: return "floor 0 vorbis is not supported";
	case VORBIS_too_many_channels:     return "Exceeded max " STR(STB_VORBIS_MAX_CHANNELS) " Channels";
	case VORBIS_file_open_failure:     return "Failed to open file";
	case VORBIS_seek_without_length:   return "Unseekable stream";
	case VORBIS_unexpected_eof:        return "Unexpected EOF";
	case VORBIS_seek_invalid:          return "Tried to seek past EOF";
#endif
	default: return NULL;
	}
}

static void printError(const char* funcName, const char* errString, int err)
{
	fprintf(stderr, "%s(): %d %s\n",
		funcName ? funcName : "?",
		err, errString ? errString : "Unknown error");
}

static inline void printVorbisError(const char* funcName, int err) { printError(funcName, getVorbisError(err), err); }


#ifdef USE_VORBISFILE
static OggVorbis_File ov;
static bool ovOpen = false;
#else
static vorb* vorbin = NULL;
#endif
static SizedBuf decodeBuf = BUF_CLEAR();
static SDL_AudioStream* stream = NULL;
static SDL_AudioSpec spec;
static int volumeMul;

static inline void setVolume(uint8_t volume)
{
	volumeMul = (volume > 0) ? (int)(volume & 0xFF) + 1 : 0;
}

static void sdlAudioCallback(void* user, SDL_AudioStream* stream, int additional, int total) SDLCALL
{
	if (additional <= 0 || volumeMul == 0)
		return;

	int16_t* samples = (int16_t*)decodeBuf.ptr;
	if (samples == NULL)
		return;

	int remaining = additional;
	while (true)
	{
		const int bytes = MIN(remaining, (int)decodeBuf.len);

#ifdef USE_VORBISFILE
		int section, read = 0;
		while (read < bytes)
		{
			long res = ov_read(&ov, (char*)samples + read, bytes - read,
				SDL_BYTEORDER == SDL_BIG_ENDIAN ? 1 : 0, 2, 1, &section);
			if (res < 0)
				return;
			if (res == 0)
			{
				res = ov_time_seek_page(&ov, 0.0);  // Seek to beginning
				if (res != 0)
					return;
				continue;
			}
			read += res;
		}
		int vorbSamples = read / sizeof(int16_t);
#else
		int numSamples = additional / (int)sizeof(int16_t);
		int numChannels = spec.channels;

		// Decode vorbis samples
		int res = stb_vorbis_get_samples_short_interleaved(vorbin, numChannels, samples, numSamples);
		if (res < 0)
			return;
		int vorbSamples = res * numChannels;

		// If we read less than the buffer then we probably hit EOF
		if (vorbSamples < numSamples)
		{
			stb_vorbis_seek_start(vorbin);  // Seek to beginning
			res = stb_vorbis_get_samples_short_interleaved(vorbin, numChannels, &samples[vorbSamples], numSamples - vorbSamples);
			if (res < 0)
				return;
			vorbSamples += res * numChannels;
		}
#endif

		// Apply volume
		for (int i = 0; i < vorbSamples; ++i)
			samples[i] = (int16_t)(((int)samples[i] * volumeMul) >> 8);

		int vorbBytes = sizeof(int16_t) * vorbSamples;
		SDL_PutAudioStreamData(stream, decodeBuf.ptr, vorbBytes);

		if (remaining <= vorbBytes)
			return;
		remaining -= vorbBytes;
	}
}

int audioInit(SDL_Window* window)
{
	if (!window)
		return -1;
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
		return -1;
	return 0;
}

#ifdef USE_VORBISFILE
static int checkVobisInfo(const vorbis_info* vorbNfo)
{
	if (vorbNfo->rate > INT32_MAX)
		return -1;
	if (vorbNfo->channels > UINT8_MAX)
		return -1;
	if (vorbNfo->channels < 0)
		return -1;
	return 0;
}
#else
static int checkVobisInfo(const stb_vorbis_info* vorbNfo)
{
	if (vorbNfo->sample_rate > INT32_MAX)
		return -1;
#if STB_VORBIS_MAX_CHANNELS > UINT8_MAX
	if (vorbNfo->channels > UINT8_MAX)
		return -1;
#endif
	if (vorbNfo->channels < 0)
		return -1;
	return 0;
}
#endif

#ifdef USE_VORBISFILE
static int openDevice(const vorbis_info* vorbNfo)
#else
static int openDevice(const stb_vorbis_info* vorbNfo)
#endif
{
	spec = (SDL_AudioSpec)
	{
		.format   = SDL_AUDIO_S16,
		.channels = (Uint8)vorbNfo->channels,
#ifdef USE_VORBISFILE
		.freq = (int)vorbNfo->rate
#else
		.freq = (int)vorbNfo->sample_rate
#endif
	};

	// Open output stream
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, sdlAudioCallback, NULL);
	if (!stream)
	{
		printError("audioInit", SDL_GetError(), (int)SDL_GetAudioStreamDevice(stream));
		return -1;
	}

	// Allocate work buffer
	SDL_AudioSpec devSpec;
	int frames = 0;
	SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &devSpec, &frames);
	decodeBuf = BUF_ALLOC(SDL_AUDIO_FRAMESIZE(spec) * MAX((size_t)frames, 1024U << 3U));
	if (!decodeBuf.ptr)
		return -1;
	return 0;
}

int audioPlayFile(const char* oggPath, uint8_t volume)
{
	if (!oggPath)
		return -1;

#ifdef USE_VORBISFILE
	const int err = ov_fopen(oggPath, &ov);
	if (err != 0)
#else
	int err = VORBIS__no_error;
	vorbin = stb_vorbis_open_filename(oggPath, &err, NULL);
	if (err != VORBIS__no_error)
#endif
	{
		printVorbisError("audioInit", err);
		return -1;
	}

#ifdef USE_VORBISFILE
	vorbis_info* vorbNfo = ov_info(&ov, -1);
	if (!vorbNfo || checkVobisInfo(vorbNfo) || openDevice(vorbNfo))
	{
		ov_clear(&ov);
		return -1;
	}
	ovOpen = true;
#else
	stb_vorbis_info vorbNfo = stb_vorbis_get_info(vorbin);
	if (checkVobisInfo(&vorbNfo))
		return -1;
	if (openDevice(&vorbNfo))
		return -1;
#endif

	setVolume(volume);
	SDL_ResumeAudioStreamDevice(stream);
	return 0;
}

#ifdef USE_VORBISFILE
static size_t ovMemOfs, ovMemLen;

static size_t ovMemRead(void* ptr, size_t size, size_t nmemb, void* source)
{
	const Uint8* oggv = source;
	size_t readLen = size * nmemb;
	if (ovMemOfs >= ovMemLen)
	{
		errno = ERANGE;
		return 0U;
	}
	if (ovMemOfs + readLen > ovMemLen)
		readLen -= ovMemOfs + readLen - ovMemLen;
	SDL_memcpy(ptr, oggv + ovMemOfs, readLen);
	ovMemOfs += readLen;
	return readLen;
}

static int ovMemSeek(void* _, ogg_int64_t ofs, int whence)
{
	switch (whence)
	{
	case SEEK_SET:
		if (ofs < 0 || (size_t)ofs > ovMemLen)
			return -1;
		ovMemOfs = ofs;
		return 0;
	case SEEK_CUR:
		if ((ogg_int64_t)ovMemOfs < ofs || ovMemOfs + ofs > ovMemLen)
			return -1;
		ovMemOfs += ofs;
		return 0;
	case SEEK_END:
		if (ofs > 0 || -ofs > (ogg_int64_t)ovMemLen)
			return -1;
		ovMemOfs = ovMemLen + ofs;
		return 0;
	default:
		return -1;
	}
}

static long ovMemTell(void* _)
{
	if (ovMemLen > LONG_MAX)
		return -1;
	return (long)ovMemOfs;
}
#endif

int audioPlayMemory(const void* oggv, int oggvLen, uint8_t volume)
{
	if (!oggv && oggvLen <= 0)
		return -1;

#ifdef USE_VORBISFILE
	ovMemOfs = 0, ovMemLen = oggvLen;
	const int err = ov_open_callbacks((void*)oggv, &ov, NULL, 0, (ov_callbacks)
	{
		.read_func  = ovMemRead,
		.seek_func  = ovMemSeek,
		.close_func = NULL,
		.tell_func  = ovMemTell
	});
#else
	int err = VORBIS__no_error;
	vorbin = stb_vorbis_open_memory(oggv, oggvLen, &err, NULL);
#endif
#ifdef USE_VORBISFILE
	if (err != 0)
	{
		ov_clear(&ov);
#else
	if (err != VORBIS__no_error)
	{
#endif
		printVorbisError("audioInit", err);
		return -1;
	}

#ifdef USE_VORBISFILE
	vorbis_info* vorbNfo = ov_info(&ov, -1);
	if (checkVobisInfo(vorbNfo) || openDevice(vorbNfo))
	{
		ov_clear(&ov);
		return -1;
	}
	ovOpen = true;
#else
	stb_vorbis_info vorbNfo = stb_vorbis_get_info(vorbin);
	if (checkVobisInfo(&vorbNfo))
		return -1;
	if (openDevice(&vorbNfo))
		return -1;
#endif

	setVolume(volume);
	SDL_ResumeAudioStreamDevice(stream);
	return 0;
}

void audioClose(void)
{
	SDL_PauseAudioStreamDevice(stream);
	BUF_FREE(decodeBuf);
	SDL_DestroyAudioStream(stream);
	stream = NULL;
#ifdef USE_VORBISFILE
	ov_clear(&ov);
	ovOpen = false;
#else
	if (vorbin)
	{
		stb_vorbis_close(vorbin);
		vorbin = NULL;
	}
#endif
}


int audioIsOpen(void)
{
	if (!stream)
		return 0;
#ifdef USE_VORBISFILE
	if (ovOpen)
		return 1;
#else
	if (vorbin)
		return 1;
#endif
	return 0;
}

int audioIsPlaying(void)
{
	if (!audioIsOpen())
		return 0;
	SDL_AudioDeviceID dev = SDL_GetAudioStreamDevice(stream);
	return dev != 0 && !SDL_AudioDevicePaused(dev);
}
