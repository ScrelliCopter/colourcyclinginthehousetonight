#ifndef SURFACE_H
#define SURFACE_H

#include "lbm.h"

typedef struct SurfSpan { int16_t l, r, inL, inR; } SurfSpan;

typedef struct
{
	int w, h;

	Color    srcPal[LBM_PAL_SIZE];
	Color    pal[LBM_PAL_SIZE];
	uint8_t*  srcPix;
	Color*   comb;
	SurfSpan* spans;
	int       spanBufLen;
	int       spanBeg;
	int       spanEnd;
} Surface;

#define SURFACE_CLEAR() (Surface){  \
	.w = 0, .h = 0,                 \
	.srcPix = NULL,                 \
	.comb = NULL,                   \
	.spans = NULL, .spanBufLen = 0, \
	.spanBeg = 0, .spanEnd = 0 }

int surfaceInit(Surface* surf,
	int w, int h,
	const uint8_t* pix,
	const Color pal[]);

void surfaceFree(Surface* surf);

void surfacePalShiftRight(Surface* surf, uint8_t hi, uint8_t low);
void surfacePalShiftLeft(Surface* surf, uint8_t hi, uint8_t low);

void surfaceRange(Surface* surf, uint8_t hi, uint8_t low, int cycle);
void surfaceRangeLinear(Surface* surf, uint8_t hi, uint8_t low, int cycle, double tween);
void surfaceRangeHsluv(Surface* surf, uint8_t hi, uint8_t low, int cycle, double tween);

int surfaceComputeSpans(Surface* surf,
	const uint8_t hi[], const uint8_t low[],
	const int16_t rate[], int numRanges);
int surfaceLoadSpans(Surface* surf, const void* chunk, size_t size);
void surfaceCombine(Surface* surf);
void surfaceCombinePartial(Surface* surf);

typedef struct SDL_Texture SDL_Texture;

void surfaceUpdate(Surface* surf, SDL_Texture* tex);

#endif //SURFACE_H
