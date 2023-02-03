#include "display.h"
#include "audio.h"
#include "util.h"
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>


static SizedStr title     = STR_CLEAR();
static SizedStr audioPath = STR_CLEAR();

static SizedBuf precompSpans = BUF_CLEAR();
static SizedBuf oggv         = BUF_CLEAR();

static uint8_t volume = 0;

#define IFF_CUSTOM_SCENE_INFO SDL_FOURCC('S', 'N', 'F', 'O')
#define IFF_CUSTOM_OGG_VORBIS SDL_FOURCC('O', 'G', 'G', 'V')
#define IFF_CUSTOM_SPANS      SDL_FOURCC('S', 'P', 'A', 'N')

static int customSubscriber(uint32_t fourcc)
{
	switch (fourcc)
	{
		case (IFF_CUSTOM_SCENE_INFO):
		case (IFF_CUSTOM_SPANS):
		case (IFF_CUSTOM_OGG_VORBIS):
			return 1;
		default:
			return 0;
	}
}

static int customHandler(uint32_t fourcc, uint32_t size, uint8_t* chunk)
{
	if (fourcc == IFF_CUSTOM_SCENE_INFO)
	{
		uint8_t titleLen, audioLen;

		// Read title
		SDL_memcpy(&titleLen, chunk, sizeof(uint8_t));
		if (titleLen > size - 3)
			return -1;
		++chunk;
		if (titleLen)
		{
			title = STR_ALLOC((size_t)titleLen);
			SDL_memcpy(title.ptr, chunk, titleLen);
			title.ptr[title.len] = '\0';
			chunk += titleLen;
		}

		// Read audio name
		SDL_memcpy(&audioLen, chunk, sizeof(uint8_t));
		if (audioLen > size - 3)
			return -1;
		++chunk;
		if (audioLen)
		{
			#define AUDIO_PREFIX_LEN 6
			static const char audioPrefix[AUDIO_PREFIX_LEN] = "audio/";
			#define AUDIO_SUFFIX_LEN 4
			static const char audioSuffix[AUDIO_SUFFIX_LEN] = ".ogg";

			audioPath = STR_ALLOC(AUDIO_PREFIX_LEN + (size_t)audioLen + AUDIO_SUFFIX_LEN);
			SDL_memcpy(audioPath.ptr, audioPrefix, AUDIO_PREFIX_LEN);
			SDL_memcpy(audioPath.ptr + AUDIO_PREFIX_LEN, chunk, audioLen);
			SDL_memcpy(audioPath.ptr + AUDIO_PREFIX_LEN + audioLen, audioSuffix, AUDIO_SUFFIX_LEN);
			audioPath.ptr[audioPath.len] = '\0';
			chunk += audioLen;
		}

		// Read volume
		SDL_memcpy(&volume, chunk, sizeof(uint8_t));
	}
	else if (fourcc == IFF_CUSTOM_SPANS)
	{
		precompSpans = BUF_SIZED(chunk, size);
		return 1;
	}
	else if (fourcc == IFF_CUSTOM_OGG_VORBIS)
	{
		oggv = BUF_SIZED(chunk, size);
		return 1;
	}
	return 0;
}


static SDL_Window*   win  = NULL;
static SDL_Renderer* rend = NULL;

static Display* display = NULL;

static bool quit     = false;
static bool realtime = false;

static int speed = 2;

static void playAudio(void);

//FIXME: this has awful behaviour on load failure
static int reset(const char* lbmPath)
{
	// Open LBM
	Lbm lbm = LBM_CLEAR();
	FILE* file = fopen(lbmPath, "rb");
	if (!file)
		return 1;
	lbm.iocb = LBM_IO_DEFAULT(file);
	lbm.customSub = customSubscriber;
	lbm.customHndl = customHandler;

	if (audioIsOpen())
		audioClose();
	STR_FREE(audioPath);
	BUF_FREE(oggv);
	STR_FREE(title);

	if (lbmLoad(&lbm))
	{
		lbmFree(&lbm);
		return 1;
	}

	const char* wintitle = STR_EMPTY(title) ? "Untitled" : title.ptr;
	if (!win)
	{
		// Create window if it doesn't exist
		const int winpos = SDL_WINDOWPOS_UNDEFINED;
		const int winflg = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
		//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		win = SDL_CreateWindow(wintitle, winpos, winpos, lbm.w, lbm.h, winflg);
		rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
		if (!win || !rend)
		{
			lbmFree(&lbm);
			return -1;
		}
	}
	else
	{
		SDL_SetWindowTitle(win, wintitle);
		SDL_SetWindowSize(win, lbm.w, lbm.h);
	}

	// Setup display
	if (!display)
		display = displayInit(rend, &lbm, precompSpans.ptr, precompSpans.len);
	else
		displayReset(display, &lbm, precompSpans.ptr, precompSpans.len);
	lbmFree(&lbm);
	if (!display)
	{
		quit = true;
		return -1;
	}
	BUF_FREE(precompSpans);

	playAudio();
	realtime = (!BUF_EMPTY(precompSpans) || displayHasAnimation(display)) ? true : false;
	return 0;
}

void playAudio(void)
{
	// Play embedded or linked audio file
	if ((!BUF_EMPTY(oggv) || !STR_EMPTY(audioPath)) && !audioInit(win))
	{
		if (!BUF_EMPTY(oggv))
		{
			// Try playing embedded ogg if found
			if (audioPlayMemory(oggv.ptr, oggv.len, volume))
				BUF_FREE(oggv);
		}
		// Try playing linked audio
		if (BUF_EMPTY(oggv) && audioPath.ptr)
			audioPlayFile(audioPath.ptr, volume);
	}
	if (audioPath.ptr)
	{
		SDL_free(audioPath.ptr);
		audioPath.ptr = NULL;
	}
}

static void deinit(void)
{
	audioClose();
	STR_FREE(audioPath);
	BUF_FREE(oggv);
	STR_FREE(title);
	displayFree(display);
	BUF_FREE(precompSpans);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

static void handleEvent(const SDL_Event* event)
{
	if (event->type == SDL_QUIT)
		quit = true;
	else if (event->type == SDL_KEYDOWN)
	{
		if (event->key.keysym.scancode == SDL_SCANCODE_P)
			displayTogglePalette(display);
		else if (event->key.keysym.scancode == SDL_SCANCODE_S)
			displayToggleSpan(display);
		else if (event->key.keysym.scancode == SDL_SCANCODE_M)
			displayCycleBlendMethod(display);
		else if (event->key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
		{
			if (speed > 0)
				--speed;
		}
		else if (event->key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET)
		{
			if (speed < 4)
				++speed;
		}
	}
	else if (event->type == SDL_WINDOWEVENT)
	{
		if (event->window.event == SDL_WINDOWEVENT_RESIZED)
			displayResize(display);
		else if (event->window.event == SDL_WINDOWEVENT_EXPOSED && !realtime)
			displayDamage(display);
	}
	else if (event->type == SDL_DROPFILE)
	{
		char* file = event->drop.file;
		reset(file);
		SDL_free(file);
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
		return 1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	if (reset(argv[1]))
	{
		deinit();
		return 1;
	}

	const double perfScale = 1.0 / (double)SDL_GetPerformanceFrequency();
	Uint64 tick = SDL_GetPerformanceCounter();
	realtime = (!BUF_EMPTY(precompSpans) || displayHasAnimation(display)) ? true : false;
	while (!quit)
	{
		SDL_Event event;
		if (realtime)
			while (SDL_PollEvent(&event) > 0) handleEvent(&event);
		else if (SDL_WaitEvent(&event))
			handleEvent(&event);

		// Timing
		if (realtime)
		{
			const Uint64 lastTick = tick;
			tick = SDL_GetPerformanceCounter();
			const double dTick = perfScale * (double)(tick - lastTick);
			const double timeScale = (1 << speed) / 4.0;
			displayUpdateTimer(display, timeScale * dTick);
			displayDamage(display);
		}

		displayRepaint(display);
	}

	deinit();
	return 0;
}
