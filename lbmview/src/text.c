#include "text.h"
#include "font.h"
#include <SDL.h>
#include <string.h>


const int textScale = 3;

void textCreateFontTexture(Font* font)
{
	if (!font || !font->r)
		return;

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, sizeof(fontGlyphBitmaps), 8, 8, SDL_PIXELFORMAT_RGBA32);
	if (!surf)
		return;

	for (unsigned i = 0; i < sizeof(fontGlyphBitmaps); ++i)
	{
		Uint32* pix = &((Uint32*)surf->pixels)[i];

		unsigned bits = fontGlyphBitmaps[i];
		for (unsigned j = 0; j < 8; ++j)
		{
			(*pix) = (bits & 0x1) ? 0xFFFFFFFF : 0x00000000;
			bits >>= 1;
			pix += sizeof(fontGlyphBitmaps);
		}
	}

	font->tex = SDL_CreateTextureFromSurface(font->r, surf);
	SDL_FreeSurface(surf);
	if (font->tex)
		SDL_SetTextureBlendMode(font->tex, SDL_BLENDMODE_BLEND);
}

static void handleControlChar(int c, int xorig, int* restrict x, int* restrict y)
{
	const int tabSize = FONT_SPACE_WIDTH * textScale * 8;
	switch (c)
	{
	case ' ':
		(*x) += (FONT_SPACE_WIDTH) * textScale;
		break;
	case '\t':
		(*x) = xorig + (((*x) - xorig + tabSize) / tabSize) * tabSize;
		break;
	case '\n':
	case '\r':
		(*x) = xorig;
		(*y) += 10 * textScale;
		break;
	default:
		break;
	}
}

void textComputeArea(int* restrict w, int* restrict h, const char* restrict str)
{
	const size_t len = strlen(str);

	int x = 0, y = 0, ww = 0;
	for (size_t i = 0; i < len; ++i)
	{
		int offset, size, c = (unsigned char)str[i];
		if (fontGetGlyph(c, &offset, &size))
			x += (size + fontGetKerning((unsigned char)str[i + 1], c)) * textScale;
		else
			handleControlChar(c, 0, &x, &y);
	}

	if (x > ww)
		ww = x;
	(*w) = ww;
	(*h) = y + 8 * textScale;
}

void textDraw(const Font* font, int x, int y, const char* restrict str)
{
	if (!font || !font->r || !font->tex || !str)
		return;

	const size_t len = strlen(str);
	const int xorig = x;

	for (size_t i = 0; i < len; ++i)
	{
		int offset, size, c = (unsigned char)str[i];
		if (fontGetGlyph(c, &offset, &size))
		{
			SDL_Rect src = { offset, 0, size, 8 };
			SDL_Rect dst = { x, y, size * textScale, 8 * textScale };
			SDL_RenderCopy(font->r, font->tex, &src, &dst);
			x += (size + fontGetKerning((unsigned char)str[i + 1], c)) * textScale;
		}
		else handleControlChar(c, xorig, &x, &y);
	}
}
