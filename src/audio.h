#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

typedef struct SDL_Window SDL_Window;

int audioInit(SDL_Window* window);
int audioPlayFile(const char* oggPath, uint8_t volume);
int audioPlayMemory(const void* oggv, int oggvLen, uint8_t volume);
void audioClose(void);

int audioIsOpen(void);
int audioIsPlaying(void);

#endif//AUDIO_H
