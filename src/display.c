#include "display.h"
#include "surface.h"
#include "util.h"
#include <SDL.h>
#include <stdbool.h>


struct Display
{
	SDL_Renderer* rend;

	Surface surf;
	SDL_Texture* surfTex;
	bool surfDamage;
	SDL_Rect surfRect;

	uint8_t rangeLow[LBM_MAX_CRNG];
	uint8_t rangeHigh[LBM_MAX_CRNG];
	int16_t rangeRate[LBM_MAX_CRNG];
	unsigned numRange;

	double srcAspect;
	int scrW, scrH;

	bool rangeTrigger[LBM_MAX_CRNG];
	float cycleTimers[LBM_MAX_CRNG];
	uint8_t cyclePos[LBM_MAX_CRNG];

	int cycleMethod;
	bool spanView, palView;
	bool repaint;
};

static void recalcDisplayRect(Display* d, int w, int h, double aspect);

Display* displayInit(SDL_Renderer* renderer, const Lbm* lbm, const void* precompSpans, uint32_t precompSpansLen)
{
	if (!renderer || !lbm)
		return NULL;

	Display* d = SDL_malloc(sizeof(Display));
	if (!d)
		return NULL;

	(*d) = (Display)
	{
		.rend = renderer,

		.surf       = SURFACE_CLEAR(),
		.surfTex    = NULL,
		.surfDamage = true,
		.numRange   = 0U,

		// Set by displayResize()
		.surfRect = { 0, 0, 0, 0 },
		.srcAspect = 0.0,
		.scrW = 0, .scrH = 0,

		.cycleMethod = 1,
		.spanView = false,
		.palView  = false,
		.repaint  = false
	};

	d->rend = renderer;
	if (displayReset(d, lbm, precompSpans, precompSpansLen))
	{
		displayFree(d);
		return NULL;
	}

	return d;
}

static void freeResources(Display* d)
{
	SDL_DestroyTexture(d->surfTex);
	surfaceFree(&d->surf);
}

void displayFree(Display* d)
{
	if (!d)
		return;
	freeResources(d);
	SDL_free(d);
}

int displayReset(Display* d, const Lbm* lbm, const void* precompSpans, uint32_t precompSpansLen)
{
	if (!d || !lbm)
		return -1;

	freeResources(d);

	// Create surface
	if (surfaceInit(&d->surf, lbm->w, lbm->h, lbm->pixels, lbm->palette))
		return -1;
	// copy ranges
	SDL_memcpy(d->rangeLow, lbm->rangeLow, sizeof(uint8_t) * LBM_MAX_CRNG);
	SDL_memcpy(d->rangeHigh, lbm->rangeHigh, sizeof(uint8_t) * LBM_MAX_CRNG);
	SDL_memcpy(d->rangeRate, lbm->rangeRate, sizeof(int16_t) * LBM_MAX_CRNG);
	d->numRange = lbm->numRange;
	if (precompSpans)
		surfaceLoadSpans(&d->surf, precompSpans, precompSpansLen);
	else if (displayHasAnimation(d))
		surfaceComputeSpans(&d->surf, d->rangeHigh, d->rangeLow, d->rangeRate, (int)d->numRange);
	surfaceCombine(&d->surf);

	// Create destination surface texure
	d->surfTex = SDL_CreateTexture(d->rend,
		SDL_PIXELFORMAT_BGRA32,
		SDL_TEXTUREACCESS_STREAMING,
		d->surf.w, d->surf.h);
	if (!d->surfTex)
		return -1;

	displayResize(d);

	// Reset cycle arrays
	for (unsigned i = 0; i < LBM_MAX_CRNG; ++i)
	{
		d->rangeTrigger[i] = false;
		d->cycleTimers[i]  = 0.0f;
		d->cyclePos[i]     = 0;
	}

	displayDamage(d);
	return 0;
}


int displayHasAnimation(const Display* d)
{
	if (!d || !d->numRange)
		return false;

	// Does the image contain any usable ranges?
	for (unsigned i = 0; i < d->numRange; ++i)
		if (d->rangeRate[i] && d->rangeHigh[i] > d->rangeLow[i])
			return true;
	return false;
}


static void drawPalette(Display* d, int size)
{
	for (unsigned i = 0; i < LBM_PAL_SIZE; ++i)
	{
		SDL_Rect dst = (SDL_Rect){
			(int)(i & 0xF) * size,
			(int)(i >> 4) * size,
			size, size };
		Colour c = d->surf.pal[i];
		SDL_SetRenderDrawColor(d->rend, COLOUR_R(c), COLOUR_G(c), COLOUR_B(c), COLOUR_A(c));
		SDL_RenderFillRect(d->rend, &dst);
	}
}

static void drawSpans(Display* d)
{
	if (!d->surf.spans)
		return;

	// Save current renderer state
	SDL_Colour oldColour;
	SDL_BlendMode oldMode;
	SDL_GetRenderDrawColor(d->rend, &oldColour.r, &oldColour.g, &oldColour.b, &oldColour.a);
	SDL_GetRenderDrawBlendMode(d->rend, &oldMode);

	// Alpha blend spans over scene
	SDL_SetRenderDrawBlendMode(d->rend, SDL_BLENDMODE_BLEND);
	const Uint8 alpha = 0x3F;

	// Compute scale factors
	float sw = (float)d->surfRect.w / (float)d->surf.w;
	float sh = (float)d->surfRect.h / (float)d->surf.h;

	const SurfSpan* spans = d->surf.spans;
	SDL_FRect fdst = { 0, 0, 0, sh };
	for (int i = d->surf.spanBeg; i <= d->surf.spanEnd; ++i)
	{
		SurfSpan span = (*spans++);
		if (span.l < 0) // Skip empties
			continue;

		fdst.y = (float)d->surfRect.y + (float)i * sh;
		if (span.inL < 0)
		{
			// Fill contiguous spans in red
			SDL_SetRenderDrawColor(d->rend, 0xFF, 0x00, 0x6E, alpha);
			fdst.x = (float)d->surfRect.x + (float)span.l * sw;
			fdst.w = (1.0f + (float)span.r - (float)span.l) * sw;
			SDL_RenderFillRectF(d->rend, &fdst);
		}
		else
		{
			// Fill split span (left portion in blue)
			SDL_SetRenderDrawColor(d->rend, 0x00, 0x66, 0xFF, alpha);
			fdst.x = (float)d->surfRect.x + (float)span.l * sw;
			fdst.w = ((float)span.inL - (float)span.l) * sw;
			SDL_RenderFillRectF(d->rend, &fdst);
			// (Right portion in green)
			SDL_SetRenderDrawColor(d->rend, 0x00, 0xFF, 0x06, alpha);
			fdst.x = (float)d->surfRect.x + (1.0f + (float)span.inR) * sw;
			fdst.w = ((float)span.r - (float)span.inR) * sw;
			SDL_RenderFillRectF(d->rend, &fdst);
		}
	}

	// Restore previous render state
	SDL_SetRenderDrawColor(d->rend, oldColour.r, oldColour.g, oldColour.b, oldColour.a);
	SDL_SetRenderDrawBlendMode(d->rend, oldMode);
}


void recalcDisplayRect(Display* d, int w, int h, double aspect)
{
	SDL_Rect dst;

	double dstAspect = (double)w / (double)h;
	if (d->srcAspect < dstAspect)
	{
		dst.w = (int)round((double)h * aspect);
		dst.h = h;
		dst.x = (w - dst.w) / 2;
		dst.y = 0;
	}
	else if (d->srcAspect > dstAspect)
	{
		dst.w = w;
		dst.h = (int)round((double)w / aspect);
		dst.x = 0;
		dst.y = (h - dst.h) / 2;
	}
	else
	{
		dst = (SDL_Rect){ 0, 0, w, h };
	}

	d->surfRect = dst;
}


static const uint16_t cycleMod = 0x4000;
static const double rateScale = (1.0 / (double)cycleMod);

void displayUpdateTimer(Display* d, double delta)
{
	for (unsigned i = 0; i < d->numRange; ++i)
	{
		d->rangeTrigger[i] = false;
		if (!d->rangeRate[i])
			continue;

		uint16_t rate = (uint16_t)abs(d->rangeRate[i]);
		uint8_t range = d->rangeHigh[i] + 1 - d->rangeLow[i];

		d->cycleTimers[i] += (float)(rate * 60.0 * delta);
		if (d->cycleTimers[i] >= (float)cycleMod)
		{
			d->cycleTimers[i] = efmodf(d->cycleTimers[i], cycleMod);
			bool dir = d->rangeRate[i] == (int16_t)rate;
			d->cyclePos[i] = (uint8_t)emod(d->cyclePos[i] + (dir ? -1 : 1), range);
			d->rangeTrigger[i] = true;
		}
	}
}

void displayRepaint(Display* d)
{
	if (!d || !d->repaint)
		return;

	// Update palette
	if (d->cycleMethod == 0)
	{
		for (unsigned i = 0; i < d->numRange; ++i)
			if (d->rangeTrigger[i])
			{
				if (d->rangeRate[i] > 0)
					surfacePalShiftRight(&d->surf, d->rangeHigh[i], d->rangeLow[i]);
				else
					surfacePalShiftLeft(&d->surf, d->rangeHigh[i], d->rangeLow[i]);
				d->surfDamage = true;
			}
	}
	else if (d->cycleMethod == 1)
	{
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRangeLinear(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i],
				copysign(d->cycleTimers[i] * rateScale, -d->rangeRate[i]));
		d->surfDamage = true;
	}
	else if (d->cycleMethod == 2)
	{
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRangeHsluv(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i],
				copysign(d->cycleTimers[i] * rateScale, -d->rangeRate[i]));
		d->surfDamage = true;
	}

	// Animate image with palette
	if (d->surfDamage)
	{
		surfaceUpdate(&d->surf, d->surfTex);
		d->surfDamage = false;
	}

	// Render everthing
	SDL_SetRenderDrawColor(d->rend, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(d->rend);
	SDL_RenderCopy(d->rend, d->surfTex, NULL, &d->surfRect);

	if (d->spanView)
		drawSpans(d);
	if (d->palView)
		drawPalette(d, MAX(7, (1024 + d->scrW + d->scrH) >> 8));

	SDL_RenderPresent(d->rend);
	d->repaint = false;
}


void displayToggleSpan(Display* d)
{
	if (!d)
		return;
	d->spanView = ! d->spanView;
	d->repaint = true;
}

void displayTogglePalette(Display* d)
{
	if (!d)
		return;
	d->palView = ! d->palView;
	d->repaint = true;
}

void displayCycleBlendMethod(Display* d)
{
	if (!d)
		return;
	d->cycleMethod = (d->cycleMethod + 1) % 3;
	if (d->cycleMethod == 0)
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRange(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i]);
	d->surfDamage = true;
	d->repaint = true;
}

void displayResize(Display* d)
{
	SDL_GetRendererOutputSize(d->rend, &d->scrW, &d->scrH);
	d->srcAspect = (double)d->surf.w / (double)d->surf.h;
	recalcDisplayRect(d, d->scrW, d->scrH, d->srcAspect);
	d->repaint = true;
}

void displayDamage(Display* d)
{
	if (!d)
		return;
	d->repaint = true;
}
