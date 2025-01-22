/* display.c - (C) 2023-2025 a dinosaur (zlib) */
#include "display.h"
#include "surface.h"
#include "text.h"
#include "util.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdbool.h>


struct Display
{
	SDL_Renderer* rend;

	Surface surf;
	SDL_Texture* surfTex;
	bool surfDamage;
	SDL_FRect surfRect;

	uint8_t rangeLow[LBM_MAX_CRNG];
	uint8_t rangeHigh[LBM_MAX_CRNG];
	int16_t rangeRate[LBM_MAX_CRNG];
	unsigned numRange;

	double srcAspect;
	int scrW, scrH;
	int textScale;

	bool rangeTrigger[LBM_MAX_CRNG];
	float cycleTimers[LBM_MAX_CRNG];
	uint8_t cyclePos[LBM_MAX_CRNG];

	int cycleMethod;
	bool spanView, palView;
	bool repaint, hasAnim;

	Font font;
	const char* text;
	float textTimer;
};

#define TEXT_TIME_END  7.0f
#define TEXT_TIME_FADE 5.0f

static void recalcDisplayRect(Display* d, int w, int h, double aspect);

Display* displayInit(SDL_Renderer* renderer, const Lbm* lbm, const void* precompSpans, size_t precompSpansLen)
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
		.surfDamage = false,
		.numRange   = 0U,

		// Set by displayResize()
		.surfRect = { 0.f, 0.f, 0.f, 0.f },
		.srcAspect = 0.0,
		.scrW = 0, .scrH = 0,
		.textScale = 1,  // Set by displayContentScale()

		.cycleMethod = DISPLAY_CYCLEMETHOD_SRGB,
		.spanView = false,
		.palView  = false,
		.repaint  = false,
		.hasAnim  = false,

		.font = (Font)
		{
			.r = renderer,
			.tex = NULL
		},
		.text = NULL,
		.textTimer = 0.0f
	};

	d->rend = renderer;
	if (displayReset(d, lbm, precompSpans, precompSpansLen))
	{
		displayFree(d);
		return NULL;
	}

	textCreateFontTexture(&d->font);

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
	SDL_DestroyTexture(d->font.tex);
	SDL_free(d);
}

static bool hasAnimation(const Display* d)
{
	if (!d->numRange)
		return false;

	// Does the image contain any usable ranges?
	for (unsigned i = 0; i < d->numRange; ++i)
		if (d->rangeRate[i] && d->rangeHigh[i] > d->rangeLow[i])
			return true;
	return false;
}

int displayReset(Display* d, const Lbm* lbm, const void* precompSpans, size_t precompSpansLen)
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
	d->hasAnim  = hasAnimation(d);
	if (precompSpans)
		surfaceLoadSpans(&d->surf, precompSpans, precompSpansLen);
	else if (d->hasAnim)
		surfaceComputeSpans(&d->surf, d->rangeHigh, d->rangeLow, d->rangeRate, (int)d->numRange);
	surfaceCombine(&d->surf);

	// Create destination surface texure
	d->surfTex = SDL_CreateTexture(d->rend,
		SDL_PIXELFORMAT_BGRA32,
		SDL_TEXTUREACCESS_STREAMING,
		d->surf.w, d->surf.h);
	if (!d->surfTex)
		return -1;
	SDL_SetTextureBlendMode(d->surfTex, SDL_BLENDMODE_NONE);
	SDL_SetTextureScaleMode(d->surfTex, SDL_SCALEMODE_NEAREST);

	// Initial display resize
	int backBufferW, backBufferH;
	SDL_GetCurrentRenderOutputSize(d->rend, &backBufferW, &backBufferH);
	displayResize(d, backBufferW, backBufferH);

	// Reset cycle arrays
	for (unsigned i = 0; i < LBM_MAX_CRNG; ++i)
	{
		d->rangeTrigger[i] = false;
		d->cycleTimers[i]  = 0.0f;
		d->cyclePos[i]     = 0;
	}

	d->surfDamage = true;
	displayDamage(d);
	return 0;
}


bool displayHasAnimation(const Display* d)
{
	return d ? d->hasAnim : false;
}

bool displayIsTextShown(const Display* d)
{
	if (d && d->text && d->textTimer < TEXT_TIME_END)
		return true;
	return false;
}


static void drawPalette(Display* d, int size)
{
	for (unsigned i = 0; i < LBM_PAL_SIZE; ++i)
	{
		int x = (int)(i & 0xF) * size;
		int y = (int)(i >> 4) * size;

		Colour c = d->surf.pal[i];
		SDL_SetRenderDrawColor(d->rend, COLOUR_R(c), COLOUR_G(c), COLOUR_B(c), COLOUR_A(c));

		const SDL_FRect dst = { .x = (float)x, .y = (float)y, .w = (float)size, .h = (float)size };
		SDL_RenderFillRect(d->rend, &dst);
	}
}

static void drawSpans(Display* d)
{
	if (!d->surf.spans)
		return;

	// Save current renderer state
	SDL_Color oldColour;
	SDL_BlendMode oldMode;
	SDL_GetRenderDrawColor(d->rend, &oldColour.r, &oldColour.g, &oldColour.b, &oldColour.a);
	SDL_GetRenderDrawBlendMode(d->rend, &oldMode);

	// Alpha blend spans over scene
	SDL_SetRenderDrawBlendMode(d->rend, SDL_BLENDMODE_BLEND);
	const Uint8 alpha = 0x3F;

	// Compute scale factors
	float sw = d->surfRect.w / (float)d->surf.w;
	float sh = d->surfRect.h / (float)d->surf.h;

	const SurfSpan* spans = d->surf.spans;
	SDL_FRect fdst = { 0, 0, 0, sh };
	for (int i = d->surf.spanBeg; i <= d->surf.spanEnd; ++i)
	{
		SurfSpan span = (*spans++);
		if (span.l < 0) // Skip empties
			continue;

		fdst.y = d->surfRect.y + (float)i * sh;
		if (span.inL < 0)
		{
			// Fill contiguous spans in red
			SDL_SetRenderDrawColor(d->rend, 0xFF, 0x00, 0x6E, alpha);
			fdst.x = d->surfRect.x + (float)span.l * sw;
			fdst.w = (1.0f + (float)span.r - (float)span.l) * sw;
			SDL_RenderFillRect(d->rend, &fdst);
		}
		else
		{
			// Fill split span (left portion in blue)
			SDL_SetRenderDrawColor(d->rend, 0x00, 0x66, 0xFF, alpha);
			fdst.x = d->surfRect.x + (float)span.l * sw;
			fdst.w = ((float)span.inL - (float)span.l) * sw;
			SDL_RenderFillRect(d->rend, &fdst);
			// (Right portion in green)
			SDL_SetRenderDrawColor(d->rend, 0x00, 0xFF, 0x06, alpha);
			fdst.x = d->surfRect.x + (1.0f + (float)span.inR) * sw;
			fdst.w = ((float)span.r - (float)span.inR) * sw;
			SDL_RenderFillRect(d->rend, &fdst);
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
		dst.w = (int)(round((double)h * aspect));
		dst.h = h;
		dst.x = (w - dst.w) / 2;
		dst.y = 0;
	}
	else if (d->srcAspect > dstAspect)
	{
		dst.w = w;
		dst.h = (int)(round((double)w / aspect));
		dst.x = 0;
		dst.y = (h - dst.h) / 2;
	}
	else
	{
		dst = (SDL_Rect){ 0, 0, w, h };
	}

	d->surfRect = (SDL_FRect){ (int)dst.x, (int)dst.y, (int)dst.w, (int)dst.h };
}


#define CYCLE_MOD 0x4000
static const double rateScale = (1.0 / (double)CYCLE_MOD);

void displayUpdateTimer(Display* d, double delta)
{
	if (!d)
		return;
	for (unsigned i = 0; i < d->numRange; ++i)
	{
		d->rangeTrigger[i] = false;
		if (!d->rangeRate[i])
			continue;

		uint16_t rate = (uint16_t)abs(d->rangeRate[i]);
		uint8_t range = d->rangeHigh[i] + 1 - d->rangeLow[i];

		d->cycleTimers[i] += (float)(rate * 60.0 * delta);
		if (d->cycleTimers[i] >= (float)CYCLE_MOD)
		{
			d->cycleTimers[i] = efmodf(d->cycleTimers[i], CYCLE_MOD);
			bool dir = d->rangeRate[i] == (int16_t)rate;
			d->cyclePos[i] = (uint8_t)emod(d->cyclePos[i] + (dir ? -1 : 1), range);
			d->rangeTrigger[i] = true;
		}
	}
}

void displayUpdateTextDisplay(Display* d, double delta)
{
	if (d && d->text && d->textTimer < TEXT_TIME_END)
		d->textTimer += (float)delta;
}

static void updatePalette(Display* d)
{
	switch (d->cycleMethod)
	{
	case DISPLAY_CYCLEMETHOD_STEP:
		for (unsigned i = 0; i < d->numRange; ++i)
			if (d->rangeTrigger[i])
			{
				if (d->rangeRate[i] > 0)
					surfacePalShiftRight(&d->surf, d->rangeHigh[i], d->rangeLow[i]);
				else
					surfacePalShiftLeft(&d->surf, d->rangeHigh[i], d->rangeLow[i]);
				d->surfDamage = true;
			}
		break;
	case DISPLAY_CYCLEMETHOD_SRGB:
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRangeSrgb(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i],
				copysign((double)d->cycleTimers[i] * rateScale, -d->rangeRate[i]));
		d->surfDamage = true;
		break;
	case DISPLAY_CYCLEMETHOD_LINEAR:
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRangeLinear(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i],
				copysign((double)d->cycleTimers[i] * rateScale, -d->rangeRate[i]));
		d->surfDamage = true;
		break;
	case DISPLAY_CYCLEMETHOD_HSLUV:
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRangeHsluv(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i],
				copysign((double)d->cycleTimers[i] * rateScale, -d->rangeRate[i]));
		d->surfDamage = true;
		break;
	case DISPLAY_CYCLEMETHOD_LAB:
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRangeLab(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i],
				copysign((double)d->cycleTimers[i] * rateScale, -d->rangeRate[i]));
		d->surfDamage = true;
		break;
	default: break;
	}
}

void displayRepaint(Display* d)
{
	if (!d || !d->repaint)
		return;

	// Update palette
	if (d->hasAnim)
		updatePalette(d);

	// Animate image with palette
	if (d->surfDamage)
	{
		surfaceUpdate(&d->surf, d->surfTex);
		d->surfDamage = false;
	}

	// Render everthing
	SDL_SetRenderDrawColor(d->rend, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(d->rend);
	SDL_RenderTexture(d->rend, d->surfTex, NULL, &d->surfRect);

	if (d->spanView)
		drawSpans(d);
	if (d->palView)
		drawPalette(d, MAX(7, (1024 + d->scrW + d->scrH) >> 8));

	if (d->text && d->textTimer < TEXT_TIME_END)
	{
		SDL_SetRenderDrawBlendMode(d->rend, SDL_BLENDMODE_BLEND);
		float interp = d->textTimer > TEXT_TIME_FADE
			? (d->textTimer - TEXT_TIME_FADE) / (TEXT_TIME_END - TEXT_TIME_FADE) : 0.0f;
		Uint8 alpha = (Uint8)((1.0f - interp) * 0xFF);
		SDL_SetRenderDrawColor(d->rend, 0x00, 0x00, 0x00, alpha >> 1);
		SDL_SetTextureAlphaMod(d->font.tex, alpha);
		int w, h;
		textComputeArea(d->textScale, &w, &h, d->text);
		const int margin = 1 + 5 * d->textScale;
		h += margin;
		SDL_RenderFillRect(d->rend, &(SDL_FRect){0.f, (float)(d->scrH - h), (float)d->scrW, (float)h});
		SDL_SetRenderDrawBlendMode(d->rend, SDL_BLENDMODE_NONE);
		textDraw(&d->font, d->textScale, margin, d->scrH - h + (margin >> 1), d->text);
	}

	SDL_RenderPresent(d->rend);
	d->repaint = false;
}


void displayToggleShowSpan(Display* d)
{
	if (!d)
		return;
	d->spanView = ! d->spanView;
	d->repaint = true;
}

void displayToggleShowPalette(Display* d)
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
	d->cycleMethod = (d->cycleMethod + 1) % DISPLAY_CYCLEMETHOD_NUM;
	if (d->cycleMethod == 0)
		for (unsigned i = 0; i < d->numRange; ++i)
			surfaceRange(&d->surf, d->rangeHigh[i], d->rangeLow[i], d->cyclePos[i]);
	d->surfDamage = true;
	d->repaint = true;
}


bool displayIsSpanShown(const Display* d)
{
	return d ? d->spanView : false;
}

bool displayIsPaletteShown(const Display* d)
{
	return d ? d->palView : false;
}

int displayGetCycleMethod(const Display* d)
{
	return d ? d->cycleMethod : -1;
}


void displayResize(Display* d, int w, int h)
{
	if (!d)
		return;
	d->srcAspect = (double)d->surf.w / (double)d->surf.h;
	recalcDisplayRect(d, w, h, d->srcAspect);
	d->repaint = true;

	d->scrW = w, d->scrH = h;
}

void displayContentScale(Display* d, double scale)
{
	if (!d)
		return;
	d->textScale = MAX(1, 1 + (int)(scale + 0.5));
}

void displayDamage(Display* d)
{
	if (d)
		d->repaint = true;
}


void displayShowText(Display* d, const char* text)
{
	if (!d || !text)
		return;
	d->text = text;
	d->textTimer = 0;
}
