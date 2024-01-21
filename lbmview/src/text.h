#ifndef TEXT_H
#define TEXT_H

typedef struct SDL_Surface SDL_Surface;

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

typedef struct Font
{
	SDL_Renderer* r;
	SDL_Texture* tex;

} Font;

void textCreateFontTexture(Font* font);
void textComputeArea(int* restrict w, int* restrict h, const char* restrict str);
void textDraw(const Font* font, int x, int y, const char* restrict str);

#endif//TEXT_H
