/* main.c - (C) 2023-2025 a dinosaur (zlib) */
#include "display.h"
#include "audio.h"
#include "util.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
# include <emscripten.h>
#endif


static SizedStr title     = STR_CLEAR();
static SizedStr audioPath = STR_CLEAR();

static SizedBuf precompSpans = BUF_CLEAR();
static SizedBuf oggv         = BUF_CLEAR();

static uint8_t volume = 0;

#define IFF_CUSTOM_SCENE_INFO FOURCC('S', 'N', 'F', 'O')
#define IFF_CUSTOM_OGG_VORBIS FOURCC('O', 'G', 'G', 'V')
#define IFF_CUSTOM_SPANS      FOURCC('S', 'P', 'A', 'N')

static int customSubscriber(IffFourCC fourcc)
{
	if (FOURCC_CMP(fourcc, IFF_CUSTOM_SCENE_INFO) ||
		FOURCC_CMP(fourcc, IFF_CUSTOM_SPANS) ||
		FOURCC_CMP(fourcc, IFF_CUSTOM_OGG_VORBIS)) return 1;
	return 0;
}

static int customHandler(IffFourCC fourcc, uint32_t size, uint8_t* chunk)
{
	if (FOURCC_CMP(fourcc, IFF_CUSTOM_SCENE_INFO))
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
MSVC_NOWARN(4295)
			#define AUDIO_PREFIX_LEN 6
			static const char audioPrefix[AUDIO_PREFIX_LEN] = "audio/";
			#define AUDIO_SUFFIX_LEN 4
			static const char audioSuffix[AUDIO_SUFFIX_LEN] = ".ogg";
MSVC_ENDNOWARN()

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
	else if (FOURCC_CMP(fourcc, IFF_CUSTOM_SPANS))
	{
		precompSpans = BUF_SIZED(chunk, size);
		return 1;
	}
	else if (FOURCC_CMP(fourcc, IFF_CUSTOM_OGG_VORBIS))
	{
		oggv = BUF_SIZED(chunk, size);
		return 1;
	}
	return 0;
}


static SDL_Window*   win  = NULL;
static SDL_Renderer* rend = NULL;

static Display* display = NULL;

static char displayText[2048];
static int  displayTextSplit;

static bool quit     = false;
static bool realtime = false;

#define TIMESCALE_NUM 15
static const float speedTimescales[TIMESCALE_NUM] =
{
	0.01f, 0.0375f, 0.09f,
	0.2f, 0.5f, 0.75f,
	1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 8.0f, 16.0f,
	50.0f, 100.0f
};

static int speed = 6;

static void updateInteractiveDisplayText(void);
static void setupDisplayText(const char* restrict lbmPath, const char* restrict displayTitle);
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
		const int winflg = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
		win = SDL_CreateWindow(wintitle, lbm.w, lbm.h, winflg);
#ifdef __APPLE__
		// Force Metal on Apple, SDL_Gpu is buggy :(
		const char* devName = "metal";
#else
		const char* devName = NULL;
#endif
		rend = SDL_CreateRenderer(win, devName);
		SDL_SetRenderVSync(rend, 1);
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
	displayContentScale(display, (double)SDL_GetWindowDisplayScale(win));
	lbmFree(&lbm);
	if (!display)
	{
		quit = true;
		return -1;
	}
	BUF_FREE(precompSpans);

	setupDisplayText(lbmPath, wintitle);

	playAudio();
	realtime = (!BUF_EMPTY(precompSpans) || displayHasAnimation(display)) ? true : false;
	return 0;
}

static void updateInteractiveDisplayText(void)
{
	if (displayTextSplit < 0)
		return;
	const char* methodName = NULL;
	switch (displayGetCycleMethod(display))
	{
	case DISPLAY_CYCLEMETHOD_STEP:   methodName = "Step"; break;
	case DISPLAY_CYCLEMETHOD_SRGB:   methodName = "sRGB"; break;
	case DISPLAY_CYCLEMETHOD_LINEAR: methodName = "Linear"; break;
	case DISPLAY_CYCLEMETHOD_HSLUV:  methodName = "HSLuv"; break;
	case DISPLAY_CYCLEMETHOD_LAB:    methodName = "CIELAB"; break;
	default:                         methodName = "?"; break;
	}

	char speedTimescaleBuf[8];
	snprintf(speedTimescaleBuf, sizeof(speedTimescaleBuf), "%.*f",
		(int)sizeof(speedTimescaleBuf) - 3, speedTimescales[speed]);
	int numTimescaleChars = 0;
	for (; speedTimescaleBuf[numTimescaleChars] != '\0' && speedTimescaleBuf[numTimescaleChars] != '.'; ++numTimescaleChars);
	numTimescaleChars += 2;
	if (speedTimescales[speed] < 1.0f)
	{
		int i = sizeof(speedTimescaleBuf) - 2;
		for (; i > numTimescaleChars && speedTimescaleBuf[i - 1] == '0'; --i);
		numTimescaleChars = i;
	}

	const char* yes = "YES", * no = "NO";
	snprintf(&displayText[displayTextSplit], sizeof(displayText) - displayTextSplit,
		"\nShow palette (P): %s\n"
		"Show spans (S): %s\n"
		"Cycle method (M): %s\n"
		"Speed -([), +(]): %.*sx",
		displayIsPaletteShown(display) ? yes : no,
		displayIsSpanShown(display)    ? yes : no,
		methodName,
		numTimescaleChars, speedTimescaleBuf);
	displayShowText(display, displayText);
}

static void setupDisplayText(const char* restrict lbmPath, const char* restrict displayTitle)
{
	const char* lbmName = strrchr(lbmPath, '/');
	displayTextSplit = snprintf(displayText, sizeof(displayText),
		"%s - \"%s\"\n", lbmName ? lbmName + 1 : lbmPath, displayTitle);
	if (displayTextSplit < 0)
		return;

	if (!BUF_EMPTY(oggv) || STR_EMPTY(audioPath))
		displayTextSplit += snprintf(&displayText[displayTextSplit], sizeof(displayText) - displayTextSplit,
			"Audio: %s", BUF_EMPTY(oggv) ? "NONE" : "EMBEDDED (OGGV)");
	else
		displayTextSplit += snprintf(&displayText[displayTextSplit], sizeof(displayText) - displayTextSplit,
			"Audio: EXTERNAL (%s)", audioPath.ptr);

	if (displayTextSplit >= 0 && (!STR_EMPTY(audioPath) || !BUF_EMPTY(oggv)))
		displayTextSplit += snprintf(&displayText[displayTextSplit], sizeof(displayText) - displayTextSplit,
			"  Volume: %.1f%%\n", (double)volume * (100.0 / 255.0));
	updateInteractiveDisplayText();
}

void playAudio(void)
{
	// Play embedded or linked audio file
	if ((!BUF_EMPTY(oggv) || !STR_EMPTY(audioPath)) && !audioInit(win))
	{
		if (!BUF_EMPTY(oggv))
		{
			// Try playing embedded ogg if found
			if (audioPlayMemory(oggv.ptr, (int)oggv.len, volume))
				BUF_FREE(oggv);
		}
		// Try playing linked audio
		if (BUF_EMPTY(oggv) && audioPath.ptr)
			audioPlayFile(audioPath.ptr, volume);
	}
	STR_FREE(audioPath);
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
	if (event->type == SDL_EVENT_QUIT)
		quit = true;
	else if (event->type == SDL_EVENT_KEY_DOWN)
	{
#ifdef EMSCRIPTEN
		// SDL bug: Workaround every keypress being immediately retriggered on the web for some reason
		if (event->key.repeat)
			return;
#endif
		if (event->key.scancode == SDL_SCANCODE_P)
		{
			displayToggleShowPalette(display);
			updateInteractiveDisplayText();
		}
		else if (event->key.scancode == SDL_SCANCODE_S)
		{
			displayToggleShowSpan(display);
			updateInteractiveDisplayText();
		}
		else if (event->key.scancode == SDL_SCANCODE_M)
		{
			displayCycleBlendMethod(display);
			updateInteractiveDisplayText();
		}
		else if (event->key.scancode == SDL_SCANCODE_LEFTBRACKET)
		{
			if (speed > 0)
				--speed;
			updateInteractiveDisplayText();
		}
		else if (event->key.scancode == SDL_SCANCODE_RIGHTBRACKET)
		{
			if (speed < TIMESCALE_NUM - 1)
				++speed;
			updateInteractiveDisplayText();
		}
	}
	else if (event->type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED)
	{
		displayContentScale(display, (double)SDL_GetWindowDisplayScale(win));
	}
	else if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		displayResize(display, event->display.data1, event->display.data2);
	}
	else if (event->type == SDL_EVENT_WINDOW_EXPOSED)
	{
		if (!realtime)
			displayDamage(display);
	}
	else if (event->type == SDL_EVENT_DROP_FILE)
	{
		reset(event->drop.data);
	}
}


static Uint64 tick;

static void mainLoop(void)
{
	bool realtimeNew = !BUF_EMPTY(precompSpans) || displayHasAnimation(display) || displayIsTextShown(display);
	if (!realtime == realtimeNew)
#if 0
		tick = SDL_GetPerformanceCounter();
#else
		tick = SDL_GetTicksNS();
#endif
	realtime = realtimeNew;

	SDL_Event event;
#ifndef EMSCRIPTEN
	if (realtime)
		while (SDL_PollEvent(&event) > 0)
			handleEvent(&event);
	else if (SDL_WaitEvent(&event))
		handleEvent(&event);
#else
	while (SDL_PollEvent(&event) > 0)
		handleEvent(&event);
#endif

	if (realtime)
	{
		const Uint64 lastTick = tick;
#if 0
		const double divisor = (double)SDL_GetPerformanceFrequency();
		tick = SDL_GetPerformanceCounter();
#else
		const double divisor = 1000000000.0;
		tick = SDL_GetTicksNS();
#endif
		const double dTick = (double)(tick - lastTick) / divisor;
		displayUpdateTimer(display, (double)speedTimescales[speed] * dTick);
		displayUpdateTextDisplay(display, dTick);
		displayDamage(display);
	}

	displayRepaint(display);
}

#ifndef EMSCRIPTEN
typedef struct { SDL_Semaphore* sem; char* filepath; } OpenFileDialogueState;

static void openFileDialogue(void* user, const char* const* list, int filter)
{
	if (list && list[0] != NULL)
		((OpenFileDialogueState*)user)->filepath = SDL_strdup(list[0]);
	else if (!list)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ShowOpenFileDialog: %s", SDL_GetError());
	SDL_SignalSemaphore(((OpenFileDialogueState*)user)->sem);
}
#endif

int main(int argc, char* argv[])
{
#ifndef EMSCRIPTEN
	// Open file picker when no arguments are provided
	if (argc == 1)
	{
		// MacOS appears to remember the last directory by default
		const char* location = NULL; //SDL_GetUserFolder(SDL_FOLDER_HOME);

		SDL_Semaphore* sem = SDL_CreateSemaphore(0);
		if (!sem)
			return 1;
		if (!SDL_Init(SDL_INIT_VIDEO))  //SDL bug: misbehaves with just INIT_EVENTS, needs INIT_VIDEO
			return 1;

		// Prompt the user to select an LBM
		OpenFileDialogueState state = { .sem = sem, .filepath = NULL };
		static const SDL_DialogFileFilter filter[1] =
		{
			{ .name = "IFF ILBM/PBM files", .pattern = "lbm;iff;ilb;ilbm;frm;lores;aga" }
		};
		SDL_ShowOpenFileDialog(openFileDialogue, (void*)&state, NULL, filter, SDL_arraysize(filter), location, false);

		// Wait for dialogue selection
		SDL_WaitSemaphore(sem);
		SDL_DestroySemaphore(sem);

		if (state.filepath != NULL)
		{
			int err = reset(state.filepath);
			SDL_free(state.filepath);
			if (err)
			{
				deinit();
				return 1;
			}
			goto SkipCommandLineInit;
		}
	}
#endif

	if (argc != 2)
		return 1;

	if (!SDL_Init(SDL_INIT_VIDEO))
		return 1;

	if (reset(argv[1]))
	{
		deinit();
		return 1;
	}

SkipCommandLineInit:
#if 0
	tick = SDL_GetPerformanceCounter();
#else
	tick = SDL_GetTicksNS();
#endif

#ifdef EMSCRIPTEN
	emscripten_set_main_loop(mainLoop, 0, 1);
#else
	while (!quit)
		mainLoop();
#endif

	deinit();
	return 0;
}
