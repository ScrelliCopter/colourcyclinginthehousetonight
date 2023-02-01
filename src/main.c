#include "surface.h"
#include "audio.h"
#include "util.h"
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>


#define AUDIO_PREFIX_LEN 6
const char audioPrefix[AUDIO_PREFIX_LEN] = "audio/";
#define AUDIO_SUFFIX_LEN 4
const char audioSuffix[AUDIO_SUFFIX_LEN] = ".ogg";

char* title = NULL;
char* audioPath = NULL;
uint8_t volume = 0;
void* precompSpans = NULL;
uint32_t precompSpansLen = 0;
void* oggv = NULL;
uint32_t oggvLen = 0;

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
			title = SDL_malloc((size_t)titleLen + 1);
			SDL_memcpy(title, chunk, titleLen);
			title[titleLen] = '\0';
			chunk += titleLen;
		}

		// Read audio name
		SDL_memcpy(&audioLen, chunk, sizeof(uint8_t));
		if (audioLen > size - 3)
			return -1;
		++chunk;
		if (audioLen)
		{
			size_t autoPathLen = AUDIO_PREFIX_LEN + (size_t)audioLen + AUDIO_SUFFIX_LEN;
			audioPath = SDL_malloc(autoPathLen + 1);
			SDL_memcpy(audioPath, audioPrefix, AUDIO_PREFIX_LEN);
			SDL_memcpy(audioPath + AUDIO_PREFIX_LEN, chunk, audioLen);
			SDL_memcpy(audioPath + AUDIO_PREFIX_LEN + audioLen, audioSuffix, AUDIO_SUFFIX_LEN);
			audioPath[autoPathLen] = '\0';
			chunk += audioLen;
		}

		// Read volume
		SDL_memcpy(&volume, chunk, sizeof(uint8_t));
	}
	else if (fourcc == IFF_CUSTOM_SPANS)
	{
		precompSpans = (void*)chunk;
		precompSpansLen = size;
		return 1;
	}
	else if (fourcc == IFF_CUSTOM_OGG_VORBIS)
	{
		oggv = (void*)chunk;
		oggvLen = size;
		return 1;
	}
	return 0;
}

void drawPalette(SDL_Renderer* rend, const Colour pal[], int size)
{
	for (unsigned i = 0; i < 256; ++i)
	{
		SDL_Rect dst = (SDL_Rect){
			(int)(i & 0xF) * size,
			(int)(i >> 4) * size,
			size, size };
		Colour c = pal[i];
		SDL_SetRenderDrawColor(rend, COLOUR_R(c), COLOUR_G(c), COLOUR_B(c), COLOUR_A(c));
		SDL_RenderFillRect(rend, &dst);
	}
}

void recalcDisplayRect(SDL_Rect* dst, int* scrW, int* scrH, double srcAspect, SDL_Renderer* rend)
{
	SDL_GetRendererOutputSize(rend, scrW, scrH);
	double dstAspect = (double)*scrW / (double)*scrH;
	if (srcAspect < dstAspect)
	{
		dst->w = (int)round((double)*scrH * srcAspect);
		dst->h = *scrH;
		dst->x = (*scrW - dst->w) / 2;
		dst->y = 0;
	}
	else if (srcAspect > dstAspect)
	{
		dst->w = *scrW;
		dst->h = (int)round((double)*scrW / srcAspect);
		dst->x = 0;
		dst->y = (*scrH - dst->h) / 2;
	}
	else
	{
		*dst = (SDL_Rect){ 0, 0, *scrW, *scrH };
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
		return 1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	SDL_Window*   win  = NULL;
	SDL_Renderer* rend = NULL;
	SDL_Texture*  tex  = NULL;
	Surface       surf = SURFACE_CLEAR();
	Lbm           lbm  = LBM_CLEAR();

	FILE* file = fopen(argv[1], "rb");
	if (!file)
		return 1;
	lbm.iocb = LBM_IO_DEFAULT(file);
	lbm.customSub = customSubscriber;
	lbm.customHndl = customHandler;
	if (!file || lbmLoad(&lbm))
		return 1;

	int ret = 1;
	const int winpos = SDL_WINDOWPOS_UNDEFINED;
	const int winflg = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
	const char* wintitle = title ? title : "Untitled";
	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	win = SDL_CreateWindow(wintitle, winpos, winpos, lbm.w, lbm.h, winflg);
	if (!win)
		goto cleanup;
	rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
	if (!rend)
		goto cleanup;

	tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, lbm.w, lbm.h);
	if (!tex)
		goto cleanup;

	if (surfaceInit(&surf, lbm.w, lbm.h, lbm.pixels, lbm.palette))
		goto cleanup;
	if (precompSpans)
	{
		surfaceLoadSpans(&surf, precompSpans, precompSpansLen);
		free(precompSpans);
		precompSpans = NULL;
	}
	else
		surfaceComputeSpans(&surf, lbm.rangeHigh, lbm.rangeLow, lbm.rangeRate, (int)lbm.numRange);
	surfaceCombine(&surf);

	const uint16_t cycleMod = 0x4000;
	const double rateScale = (1.0 / (double)cycleMod);

	float cycleTimers[LBM_MAX_CRNG];
	uint8_t cyclePos[LBM_MAX_CRNG];
	for (unsigned i = 0; i < LBM_MAX_CRNG; ++i)
	{
		cycleTimers[i] = 0.0f;
		cyclePos[i] = 0;
	}

	const double srcAspect = (double)lbm.w / (double)lbm.h;
	SDL_Rect dstRect;
	int scrW, scrH;
	recalcDisplayRect(&dstRect, &scrW, &scrH, srcAspect, rend);

	// Play embedded or linked audio file
	if ((oggv || audioPath) && !audioInit(win))
	{
		if (oggv)
		{
			// Try playing embedded ogg if found
			if (audioPlayMemory(oggv, oggvLen, volume))
			{
				free(oggv);
				oggv = NULL;
			}
		}
		// Try playing linked audio
		if (!oggv && audioPath)
			audioPlayFile(audioPath, volume);
	}
	if (audioPath)
	{
		free(audioPath);
		audioPath = NULL;
	}

	bool palView = false;
	int method = 1;
	int speed = 2;

	const double perfScale = 1.0 / (double)SDL_GetPerformanceFrequency();
	Uint64 tick = SDL_GetPerformanceCounter();
	bool quit = false;
	while (!quit)
	{
		bool damage = false;
		SDL_Event event;
		while (SDL_PollEvent(&event) > 0)
		{
			if (event.type == SDL_QUIT)
				quit = true;
			else if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.scancode == SDL_SCANCODE_P)
					palView = !palView;
				else if (event.key.keysym.scancode == SDL_SCANCODE_M)
				{
					method = (method + 1) % 3;
					if (method == 0)
						for (unsigned i = 0; i < lbm.numRange; ++i)
							surfaceRange(&surf, lbm.rangeHigh[i], lbm.rangeLow[i], cyclePos[i]);
					damage = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
				{
					if (speed > 0)
						--speed;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET)
				{
					if (speed < 4)
						++speed;
				}
			}
			else if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					recalcDisplayRect(&dstRect, &scrW, &scrH, srcAspect, rend);
			}
		}

		// Timing
		const Uint64 lastTick = tick;
		tick = SDL_GetPerformanceCounter();
		const double dTick = perfScale * (double)(tick - lastTick);
		const double timeScale = (1 << speed) / 4.0;

		bool rangeTrigger[LBM_MAX_CRNG];
		for (unsigned i = 0; i < lbm.numRange; ++i)
		{
			rangeTrigger[i] = false;
			if (!lbm.rangeRate[i])
				continue;

			uint16_t rate = (uint16_t)abs(lbm.rangeRate[i]);
			uint8_t range = lbm.rangeHigh[i] + 1 - lbm.rangeLow[i];

			cycleTimers[i] += (float)(rate * timeScale * 60.0 * dTick);
			if (cycleTimers[i] >= (float)cycleMod)
			{
				cycleTimers[i] = efmodf(cycleTimers[i], cycleMod);
				bool dir = lbm.rangeRate[i] == (int16_t)rate;
				cyclePos[i] = (uint8_t)emod(cyclePos[i] + (dir ? -1 : 1), range);
				rangeTrigger[i] = true;
			}
		}

		// Update palette
		if (method == 0)
		{
			for (unsigned i = 0; i < lbm.numRange; ++i)
				if (rangeTrigger[i])
				{
					if (lbm.rangeRate[i] > 0)
						surfacePalShiftRight(&surf, lbm.rangeHigh[i], lbm.rangeLow[i]);
					else
						surfacePalShiftLeft(&surf, lbm.rangeHigh[i], lbm.rangeLow[i]);
					damage = true;
				}
		}
		else if (method == 1)
		{
			for (unsigned i = 0; i < lbm.numRange; ++i)
				surfaceRangeLinear(&surf, lbm.rangeHigh[i], lbm.rangeLow[i], cyclePos[i],
					copysign(cycleTimers[i] * rateScale, -lbm.rangeRate[i]));
			damage = true;
		}
		else if (method == 2)
		{
			for (unsigned i = 0; i < lbm.numRange; ++i)
				surfaceRangeHsluv(&surf, lbm.rangeHigh[i], lbm.rangeLow[i], cyclePos[i],
					copysign(cycleTimers[i] * rateScale, -lbm.rangeRate[i]));
			damage = true;
		}

		// Animate image with palette
		if (damage)
			surfaceUpdate(&surf, tex);

		// Render everthing
		SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(rend);
		SDL_RenderCopy(rend, tex, NULL, &dstRect);

		if (palView)
			drawPalette(rend, surf.pal, MAX(7, (1024 + scrW + scrH) >> 8));

		SDL_RenderPresent(rend);
	}

	ret = 0;
cleanup:
	audioClose();
	if (audioPath)
		free(audioPath);
	if (oggv)
		free(oggv);
	if (title)
		free(title);
	surfaceFree(&surf);
	if (precompSpans)
		free(precompSpans);
	lbmFree(&lbm);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return ret;
}
