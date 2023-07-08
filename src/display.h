#ifndef DISPLAY_H
#define DISPLAY_H

#include "lbm.h"
#include <stdint.h>

typedef struct Display Display;

typedef struct SDL_Renderer SDL_Renderer;

Display* displayInit(SDL_Renderer* renderer, const Lbm* lbm, const void* precompSpans, size_t precompSpansLen);
void displayFree(Display* d);
int displayReset(Display* d, const Lbm* lbm, const void* precompSpans, size_t precompSpansLen);

int displayHasAnimation(const Display* d);

void displayRepaint(Display* d);
void displayUpdateTimer(Display* d, double delta);

void displayToggleSpan(Display* d);
void displayTogglePalette(Display* d);
void displayCycleBlendMethod(Display* d);
void displayResize(Display* d);
void displayDamage(Display* d);

#endif//DISPLAY_H
