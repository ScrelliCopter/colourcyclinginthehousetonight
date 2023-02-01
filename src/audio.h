#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

typedef struct SDL_Window SDL_Window;

int audioInit(SDL_Window* window);
int audioPlayFile(const char* oggPath, uint8_t volume);
int audioPlayMemory(const void* oggv, uint32_t oggvLen, uint8_t volume);
void audioClose(void);

#endif//AUDIO_H
