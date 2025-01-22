#ifndef DISPLAY_H
#define DISPLAY_H

#include "lbm.h"
#include <stdbool.h>

typedef struct Display Display;

typedef struct SDL_Renderer SDL_Renderer;

enum DisplayCycleMethod
{
	DISPLAY_CYCLEMETHOD_STEP = 0,
	DISPLAY_CYCLEMETHOD_SRGB,
	DISPLAY_CYCLEMETHOD_LINEAR,
	DISPLAY_CYCLEMETHOD_HSLUV,
	DISPLAY_CYCLEMETHOD_LAB,

	DISPLAY_CYCLEMETHOD_NUM
};

Display* displayInit(SDL_Renderer* renderer, const Lbm* lbm, const void* precompSpans, size_t precompSpansLen);
void displayFree(Display* d);
int displayReset(Display* d, const Lbm* lbm, const void* precompSpans, size_t precompSpansLen);

bool displayHasAnimation(const Display* d);
bool displayIsTextShown(const Display* d);

void displayRepaint(Display* d);
void displayUpdateTimer(Display* d, double delta);
void displayUpdateTextDisplay(Display* d, double delta);

void displayToggleShowSpan(Display* d);
void displayToggleShowPalette(Display* d);
void displayCycleBlendMethod(Display* d);

bool displayIsSpanShown(const Display* d);
bool displayIsPaletteShown(const Display* d);
int displayGetCycleMethod(const Display* d);

void displayResize(Display* d, int w, int h);
void displayContentScale(Display* d, double scale);
void displayDamage(Display* d);

void displayShowText(Display* d, const char* text);

#endif//DISPLAY_H
