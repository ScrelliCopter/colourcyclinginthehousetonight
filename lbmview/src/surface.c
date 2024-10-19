/* surface.c - (C) 2023 a dinosaur (zlib) */
#include "surface.h"
#include "util.h"
#include "hsluv.h"
#include <SDL3/SDL_render.h>
#include <stdlib.h>
#include <stdbool.h>


int surfaceInit(Surface* surf,
	int w, int h,
	const uint8_t* pix, const Colour pal[])
{
	if (!surf || !pix || !pal || !w || !h)
		return -1;

	surf->srcPix = malloc(w * h);
	if (!surf->srcPix)
		return -1;

	surf->comb = malloc(w * h * sizeof(Colour));
	if (!surf->comb)
		return -1;

	SDL_memcpy(surf->srcPal, pal, sizeof(Colour) * LBM_PAL_SIZE);
	SDL_memcpy(surf->pal, pal, sizeof(Colour) * LBM_PAL_SIZE);
	SDL_memcpy(surf->srcPix, pix, w * h);
	surf->w = w;
	surf->h = h;
	return 0;
}

void surfaceFree(Surface* surf)
{
	if (!surf)
		return;
	if (surf->spans)
	{
		free(surf->spans);
		surf->spans = NULL;
		surf->spanBufLen = 0;
	}
	if (surf->comb)
	{
		free(surf->comb);
		surf->comb = NULL;
	}
	if (surf->srcPix)
	{
		free(surf->srcPix);
		surf->srcPix = NULL;
	}
}

void surfacePalShiftRight(Surface* surf, uint8_t hi, uint8_t low)
{
	if (!surf || low >= hi)
		return;

	Colour tmp = surf->pal[hi];
	for (unsigned j = hi; j > low; --j)
		surf->pal[j] = surf->pal[j - 1];
	surf->pal[low] = tmp;
}

void surfacePalShiftLeft(Surface* surf, uint8_t hi, uint8_t low)
{
	if (!surf || low >= hi)
		return;

	Colour tmp = surf->pal[low];
	SDL_memmove(&surf->pal[low], &surf->pal[low + 1], sizeof(Colour) * (hi - low));
	surf->pal[hi] = tmp;
}

void surfaceRange(Surface* surf, uint8_t hi, uint8_t low, int cycle)
{
	if (!surf || low >= hi)
		return;

	uint8_t range = ++hi - low;
	unsigned frame = (unsigned)(fmod(cycle, range));

	for (unsigned j = 0; j < range; ++j)
		surf->pal[low + j] = surf->srcPal[((j + frame) % range + low) & 0xFF];
}

void surfaceRangeSrgb(Surface* surf, uint8_t hi, uint8_t low, int cycle, double tween)
{
	if (!surf || low >= hi)
		return;

	uint8_t range = ++hi - low;
	double rateTime = efmod((double)cycle + tween, range);
	unsigned frame = (unsigned)rateTime;
	tween = rateTime - (double)frame;

	const Colour* src = surf->srcPal;
	Colour* dst = surf->pal;
	for (unsigned j = 0; j < range; ++j)
	{
		unsigned oldIdx = low + (j + frame) % range;
		unsigned newIdx = low + (j + frame + 1) % range;
		Colour old8 = src[oldIdx & 0xFF];
		Colour new8 = src[newIdx & 0xFF];

		dst[low + j] = MAKE_COLOUR(
			(uint8_t)LERP((double)COLOUR_R(old8), (double)COLOUR_R(new8), tween),
			(uint8_t)LERP((double)COLOUR_G(old8), (double)COLOUR_G(new8), tween),
			(uint8_t)LERP((double)COLOUR_B(old8), (double)COLOUR_B(new8), tween),
			(uint8_t)LERP((double)COLOUR_A(old8), (double)COLOUR_A(new8), tween));
	}
}

static inline double srgbFromLinear(double x)
{
	return (x < 0.0031308)
		? x * 12.92
		: 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

static inline double linearFromSrgb(double x)
{
	return (x < 0.04045)
		? x / 12.92
		: pow((x + 0.055) / 1.055, 2.4);
}

void surfaceRangeLinear(Surface* surf, uint8_t hi, uint8_t low, int cycle, double tween)
{
	if (!surf || low >= hi)
		return;

	uint8_t range = ++hi - low;
	double rateTime = efmod((double)cycle + tween, range);
	unsigned frame = (unsigned)rateTime;
	tween = rateTime - (double)frame;

	const Colour* src = surf->srcPal;
	Colour* dst = surf->pal;
	for (unsigned j = 0; j < range; ++j)
	{
		unsigned oldIdx = low + (j + frame) % range;
		unsigned newIdx = low + (j + frame + 1) % range;
		Colour old8 = src[oldIdx & 0xFF];
		Colour new8 = src[newIdx & 0xFF];

		dst[low + j] = MAKE_COLOUR(
			(uint8_t)(srgbFromLinear(LERP(
				linearFromSrgb((double)COLOUR_R(old8) / 255.0),
				linearFromSrgb((double)COLOUR_R(new8) / 255.0), tween)) * 255.0),
			(uint8_t)(srgbFromLinear(LERP(
				linearFromSrgb((double)COLOUR_G(old8) / 255.0),
				linearFromSrgb((double)COLOUR_G(new8) / 255.0), tween)) * 255.0),
			(uint8_t)(srgbFromLinear(LERP(
				linearFromSrgb((double)COLOUR_B(old8) / 255.0),
				linearFromSrgb((double)COLOUR_B(new8) / 255.0), tween)) * 255.0),
			(uint8_t)(srgbFromLinear(LERP(
				linearFromSrgb((double)COLOUR_A(old8) / 255.0),
				linearFromSrgb((double)COLOUR_A(new8) / 255.0), tween)) * 255.0));
	}
}

void surfaceRangeHsluv(Surface* surf, uint8_t hi, uint8_t low, int cycle, double tween)
{
	if (!surf || low >= hi)
		return;

	uint8_t range = ++hi - low;
	double rateTime = efmod((double)cycle + tween, range);
	unsigned frame = (unsigned)rateTime;
	tween = rateTime - (double)frame;

	const Colour* src = surf->srcPal;
	Colour* dst = surf->pal;
	for (unsigned j = 0; j < range; ++j)
	{
		unsigned oldIdx = low + (j + frame) % range;
		unsigned newIdx = low + (j + frame + 1) % range;
		Colour old8 = src[oldIdx & 0xFF];
		Colour new8 = src[newIdx & 0xFF];

		double oldR = COLOUR_R(old8) / 255.0, oldG = COLOUR_G(old8) / 255.0, oldB = COLOUR_B(old8) / 255.0;
		double newR = COLOUR_R(new8) / 255.0, newG = COLOUR_G(new8) / 255.0, newB = COLOUR_B(new8) / 255.0;
		double oldH, oldS, oldL, newH, newS, newL;
		rgb2hsluv(oldR, oldG, oldB, &oldH, &oldS, &oldL);
		rgb2hsluv(newR, newG, newB, &newH, &newS, &newL);
		double h = DEGLERP(oldH, newH, tween), s = LERP(oldS, newS, tween), v = LERP(oldL, newL, tween);
		double r, g, b;
		hsluv2rgb(h, s, v, &r, &g, &b);
		uint8_t a = (uint8_t)LERP(COLOUR_A(old8), COLOUR_A(new8), tween);
		dst[low + j] = MAKE_COLOUR((uint8_t)(r * 255.0), (uint8_t)(g * 255.0), (uint8_t)(b * 255.0), a);
	}
}

#define LAB_WHITE_REF_X (095.047 / 100.0)
#define LAB_WHITE_REF_Y (100.000 / 100.0)
#define LAB_WHITE_REF_Z (108.883 / 100.0)

static void labFromRgb(double* oL, double* oA, double* oB, double r, double g, double b)
{
	r = linearFromSrgb(r), g = linearFromSrgb(g), b = linearFromSrgb(b);

#define GAMMA(V) (((V) > 0.008856) ? pow((V), 1.0 / 3.0) : (7.787 * (V)) + (16.0 / 116.0))
	double x = GAMMA((r * 0.4124 + g * 0.3576 + b * 0.1805) / LAB_WHITE_REF_X);
	double y = GAMMA((r * 0.2126 + g * 0.7152 + b * 0.0722) / LAB_WHITE_REF_Y);
	double z = GAMMA((r * 0.0193 + g * 0.1192 + b * 0.9505) / LAB_WHITE_REF_Z);

	*oL = (116.0 * y) - 16.0;
	*oA = 500.0 * (x - y);
	*oB = 200.0 * (y - z);
}

static void rgbFromLab(double* oR, double* oG, double* oB, double l, double a, double b)
{
	double y = (l + 16.0) / 116.0;
	double x = y + a / 500.0;
	double z = y - b / 200.0;

#define UNGAM(V) (((V) * (V) * (V) > 0.008856) ? (V) * (V) * (V) : ((V) - 16.0 / 116.0) / 7.787)
	x = UNGAM(x) * LAB_WHITE_REF_X, y = UNGAM(y) * LAB_WHITE_REF_Y, z = UNGAM(z) * LAB_WHITE_REF_Z;

	*oR = srgbFromLinear(x *  3.2406 + y * -1.5372 + z * -0.4986);
	*oG = srgbFromLinear(x * -0.9689 + y *  1.8758 + z *  0.0415);
	*oB = srgbFromLinear(x *  0.0557 + y * -0.2040 + z *  1.0570);
}

void surfaceRangeLab(Surface* surf, uint8_t hi, uint8_t low, int cycle, double tween)
{
	if (!surf || low >= hi)
		return;

	uint8_t range = ++hi - low;
	double rateTime = efmod((double)cycle + tween, range);
	unsigned frame = (unsigned)rateTime;
	tween = rateTime - (double)frame;

	const Colour* src = surf->srcPal;
	Colour* dst = surf->pal;
	for (unsigned j = 0; j < range; ++j)
	{
		unsigned oldIdx = low + (j + frame) % range;
		unsigned newIdx = low + (j + frame + 1) % range;
		Colour old8 = src[oldIdx & 0xFF];
		Colour new8 = src[newIdx & 0xFF];

		double oldL, oldA, oldB, newL, newA, newB;
		labFromRgb(&oldL, &oldA, &oldB, COLOUR_R(old8) / 255.0, COLOUR_G(old8) / 255.0, COLOUR_B(old8) / 255.0);
		labFromRgb(&newL, &newA, &newB, COLOUR_R(new8) / 255.0, COLOUR_G(new8) / 255.0, COLOUR_B(new8) / 255.0);
		double r, g, b;
		rgbFromLab(&r, &g, &b, LERP(oldL, newL, tween), LERP(oldA, newA, tween), LERP(oldB, newB, tween));
		uint8_t a = (uint8_t)LERP(COLOUR_A(old8), COLOUR_A(new8), tween);
		dst[low + j] = MAKE_COLOUR(
			(uint8_t)(SATURATE(r) * 255.0),
			(uint8_t)(SATURATE(g) * 255.0),
			(uint8_t)(SATURATE(b) * 255.0), a);
	}
}

static int resizeSpanBuffer(Surface* surf, int len)
{
	if (!surf->spans)
	{
		surf->spans = malloc(sizeof(SurfSpan) * len);
		surf->spanBufLen = len;
	}
	else if (len > surf->spanBufLen)
	{
		free(surf->spans);
		surf->spans = malloc(sizeof(SurfSpan) * len);
		surf->spanBufLen = len;
	}
	if (!surf->spans)
	{
		surf->spans = NULL;
		surf->spanBufLen = 0;
		return -1;
	}
	return 0;
}

int surfaceComputeSpans(Surface* surf,
	const uint8_t hi[], const uint8_t low[],
	const int16_t rate[], int numRanges)
{
	if (!surf || !hi || !low || numRanges <= 0)
		return -1;

	if (resizeSpanBuffer(surf, surf->h))
		return -1;

	surf->spanBeg = -1;
	surf->spanEnd = 0;

	const uint8_t* srcPix = surf->srcPix;
	for (int j = 0; j < surf->h; ++j)
	{
		SurfSpan cur = { 0, (int16_t)(surf->w - 1), -1, -1 };

		// Find outer bounds
		bool searchL = true, searchR = true;
		do
		{
			if (searchL)
			{
				uint8_t p = srcPix[cur.l];
				for (int i = 0; i < numRanges; ++i)
					if (rate[i] && hi[i] > low[i] && p >= low[i] && p <= hi[i])
					{
						searchL = false;
						break;
					}

				if (searchL)
				{
					++cur.l;
					if (cur.l >= cur.r)
						searchL = false;
				}
			}
			if (searchR)
			{
				uint8_t p = srcPix[cur.r];
				for (int i = 0; i < numRanges; ++i)
					if (rate[i] && hi[i] > low[i] && p >= low[i] && p <= hi[i])
					{
						searchR = false;
						break;
					}

				if (searchR)
				{
					--cur.r;
					if (cur.r <= cur.l)
						searchR = false;
				}
			}
		}
		while (searchL || searchR);

		if (cur.r - cur.l >= 0)
		{
			// Find inner hole
			if (cur.r - cur.l > 1)
			{
				int tmpL = cur.l + 1;
				bool inHole = false;
				for (int i = tmpL; i <= cur.r; ++i)
				{
					bool cycling = false;
					uint8_t p = srcPix[i];
					for (int k = 0; k < numRanges; ++k)
						if (rate[k] && hi[k] > low[k] && p >= low[k] && p <= hi[k])
						{
							cycling = true;
							break;
						}
					if (cycling)
					{
						int inner = (i - 1) - tmpL;
						if (inner > cur.inR - cur.inL)
							cur.inR = (cur.inL = (int16_t)tmpL) + (int16_t)inner;
						tmpL = i;
						inHole = false;
					}
					else if (!inHole)
					{
						tmpL = i;
						inHole = true;
					}
				}
			}

			if (surf->spanBeg < 0)
			{
				surf->spanBeg = j;
			}
			else
			{
				// Fill gaps
				for (int i = surf->spanEnd + 1; i < j; ++i)
					surf->spans[i - surf->spanBeg] = (SurfSpan){ -1, -1, -1, -1 };
			}
			surf->spans[j - surf->spanBeg] = cur;
			surf->spanEnd = j;
		}
		srcPix += surf->w;
	}

	return 0;
}

int surfaceLoadSpans(Surface* surf, const void* chunk, size_t size)
{
	if (!surf || !chunk || size < sizeof(int16_t) * 3)
		return -1;

	const int16_t* read = (const int16_t*)chunk;
	int startOfs = SWAP_BE16(*read++);
	int spanLen  = SWAP_BE16(*read++);
	if (startOfs < 0 || spanLen < 1)
		return -1;

	if (resizeSpanBuffer(surf, spanLen))
		return -1;

	const int16_t* end = (const int16_t*)chunk + (size / sizeof(int16_t));
	for (size_t i = 0; i < (size_t)spanLen; ++i)
	{
		SurfSpan* span = &surf->spans[i];
		*span = (SurfSpan){ -1, -1, -1, -1 };

		span->l = SWAP_BE16(*read++);
		if (read == end)
			break;
		if (span->l < 0)
			continue;
		span->r = SWAP_BE16(*read++);
		if (read == end)
			break;
		span->inL = SWAP_BE16(*read++);
		if (read == end)
			break;
		if (span->inL < 0)
			continue;
		span->inR = SWAP_BE16(*read++);
		if (read == end)
			break;
	}

	surf->spanBeg = startOfs;
	surf->spanEnd = startOfs + spanLen - 1;
	return 0;
}


void surfaceCombine(Surface* surf)
{
	if (!surf)
		return;

	const uint8_t* srcPix = surf->srcPix;
	Colour* dst = surf->comb;
	size_t dstLen = surf->w * (size_t)surf->h;

	for (size_t i = 0; i < dstLen; ++i)
	{
		uint8_t c = (*srcPix++);
		(*dst++) = surf->pal[c];
	}
}

void surfaceCombinePartial(Surface* surf)
{
	if (!surf || !surf->spans || surf->spanBeg < 0)
		return;

	const uint8_t* srcPix = surf->srcPix + surf->spanBeg * surf->w;
	Colour* dst = surf->comb + surf->spanBeg * surf->w;

	int numSpans = MIN(surf->h, surf->spanEnd + 1) - surf->spanBeg;
	for (int i = 0; i < numSpans; ++i)
	{
		SurfSpan span = surf->spans[i];
		if (span.l >= 0)
		{
			if (span.inL < 0)
			{
				for (int j = span.l; j <= span.r; ++j)
					dst[j] = surf->pal[srcPix[j]];
			}
			else
			{
				for (int j = span.l; j < span.inL; ++j)
					dst[j] = surf->pal[srcPix[j]];
				for (int j = span.inR + 1; j <= span.r; ++j)
					dst[j] = surf->pal[srcPix[j]];
			}
		}

		srcPix += surf->w;
		dst += surf->w;
	}
}

void surfaceUpdate(Surface* surf, SDL_Texture* tex)
{
	if (!surf || !tex)
		return;

	if (surf->spans)
		surfaceCombinePartial(surf);
	else
		surfaceCombine(surf);
	int pitch = surf->w * (int)sizeof(Colour);
	SDL_UpdateTexture(tex, NULL, surf->comb, pitch);
}
