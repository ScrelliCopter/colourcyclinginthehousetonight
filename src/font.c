/* Auto-generated by 'vwfnt2c.py' */
#include "font.h"
#include <string.h>

int fontGetKerning(int c, int p)
{
	if (c == 'a')
	{
		if (p == 'F' || p == 'P' || p == 'T' || p == 'f') { return 0; }
	}
	else if (c == 'j')
	{
		if (p == 'F' || p == 'T' || p == 'f' || p == 'r' || p == 'v' || p == 'P' || p == 'Y') { return 0; }
	}
	else if (strchr("Acdegmnopqrsuvwxyz,.", c))
	{
		if (p == 'F' || p == 'T' || p == 'f') { return 0; }
	}
	else if (c == 'T')
	{
		if (strchr("AarbceghklmnopqstuvwxyzL", p)) { return 0; }
	}
	else if (c == 'Y')
	{
		if (p == 'L' || p == 'l') { return 0; }
	}
	else if (c == 'f')
	{
		if (p == 'f') { return 0; }
	}
	else if (c == ' ')
	{
		if      (p == 'f') { return 0; }
		else if (p == '!' || p == '?' || p == ',' || p == '.') { return 2; }
	}
	return 1;
}

const uint8_t fontGlyphBitmaps[385] =
{
/* 'a' */  0x60, 0x54, 0x54, 0x78,
/* 'b' */  0x7F, 0x44, 0x44, 0x38,
/* 'c' */  0x38, 0x44, 0x44,
/* 'd' */  0x38, 0x44, 0x44, 0x7F,
/* 'e' */  0x38, 0x54, 0x54, 0x58,
/* 'f' */  0x04, 0x7E, 0x05, 0x01,
/* 'g' */  0x18, 0xA4, 0xA4, 0x7C,
/* 'h' */  0x7F, 0x04, 0x04, 0x78,
/* 'i' */  0x7D,
/* 'j' */  0x40, 0x40, 0x3D,
/* 'k' */  0x7F, 0x10, 0x28, 0x44,
/* 'l' */  0x3F, 0x40, 0x40,
/* 'm' */  0x7C, 0x04, 0x04, 0x78, 0x04, 0x04, 0x78,
/* 'n' */  0x7C, 0x04, 0x04, 0x78,
/* 'o' */  0x38, 0x44, 0x44, 0x38,
/* 'p' */  0xFC, 0x44, 0x44, 0x38,
/* 'q' */  0x38, 0x44, 0x44, 0xFC,
/* 'r' */  0x7C, 0x08, 0x04,
/* 's' */  0x48, 0x54, 0x54, 0x24,
/* 't' */  0x04, 0x3F, 0x44, 0x44,
/* 'u' */  0x3C, 0x40, 0x40, 0x3C,
/* 'v' */  0x3C, 0x40, 0x20, 0x1C,
/* 'w' */  0x3C, 0x40, 0x30, 0x40, 0x3C,
/* 'x' */  0x44, 0x28, 0x10, 0x28, 0x44,
/* 'y' */  0x9C, 0xA0, 0xA0, 0x7C,
/* 'z' */  0x64, 0x54, 0x4C, 0x44,
/* 'A' */  0x78, 0x16, 0x11, 0x16, 0x78,
/* 'B' */  0x7F, 0x49, 0x49, 0x49, 0x36,
/* 'C' */  0x3E, 0x41, 0x41, 0x41, 0x22,
/* 'D' */  0x7F, 0x41, 0x41, 0x41, 0x3E,
/* 'E' */  0x7F, 0x49, 0x49, 0x49,
/* 'F' */  0x7F, 0x09, 0x09, 0x01,
/* 'G' */  0x3E, 0x41, 0x41, 0x49, 0x7A,
/* 'H' */  0x7F, 0x08, 0x08, 0x08, 0x7F,
/* 'I' */  0x41, 0x7F, 0x41,
/* 'J' */  0x41, 0x41, 0x41, 0x3F,
/* 'K' */  0x7F, 0x08, 0x14, 0x22, 0x41,
/* 'L' */  0x7F, 0x40, 0x40, 0x40,
/* 'M' */  0x7F, 0x03, 0x0C, 0x30, 0x0C, 0x03, 0x7F,
/* 'N' */  0x7F, 0x03, 0x0C, 0x30, 0x7F,
/* 'O' */  0x3E, 0x41, 0x41, 0x41, 0x3E,
/* 'P' */  0x7F, 0x09, 0x09, 0x09, 0x06,
/* 'Q' */  0x3E, 0x41, 0x41, 0x61, 0x5E,
/* 'R' */  0x7F, 0x09, 0x09, 0x09, 0x76,
/* 'S' */  0x26, 0x49, 0x49, 0x49, 0x32,
/* 'T' */  0x01, 0x01, 0x7F, 0x01, 0x01,
/* 'U' */  0x3F, 0x40, 0x40, 0x40, 0x3F,
/* 'V' */  0x1F, 0x20, 0x40, 0x20, 0x1F,
/* 'W' */  0x1F, 0x60, 0x18, 0x06, 0x18, 0x60, 0x1F,
/* 'X' */  0x63, 0x14, 0x08, 0x14, 0x63,
/* 'Y' */  0x07, 0x08, 0x70, 0x08, 0x07,
/* 'Z' */  0x61, 0x51, 0x49, 0x45, 0x43,
/* '0' */  0x3E, 0x45, 0x49, 0x51, 0x3E,
/* '1' */  0x42, 0x7F, 0x40,
/* '2' */  0x42, 0x61, 0x51, 0x49, 0x46,
/* '3' */  0x22, 0x41, 0x49, 0x49, 0x36,
/* '4' */  0x1C, 0x12, 0x11, 0x7F, 0x10,
/* '5' */  0x27, 0x45, 0x45, 0x45, 0x39,
/* '6' */  0x3E, 0x49, 0x49, 0x49, 0x32,
/* '7' */  0x01, 0x01, 0x79, 0x05, 0x03,
/* '8' */  0x36, 0x49, 0x49, 0x49, 0x36,
/* '9' */  0x26, 0x49, 0x49, 0x49, 0x3E,
/* ''' */  0x03,
/* '"' */  0x03, 0x00, 0x03,
/* ';' */  0x62,
/* ':' */  0x22,
/* ',' */  0xC0,
/* '.' */  0x40,
/* '!' */  0x5F,
/* '?' */  0x02, 0x01, 0x59, 0x09, 0x06,
/* '@' */  0x3E, 0x41, 0x5D, 0x55, 0x5E,
/* '#' */  0x22, 0x7F, 0x22, 0x22, 0x7F, 0x22,
/* '$' */  0x24, 0x2A, 0x7F, 0x2A, 0x12,
/* '%' */  0x42, 0x25, 0x12, 0x08, 0x24, 0x52, 0x21,
/* '&' */  0x38, 0x46, 0x49, 0x51, 0x22, 0x50,
/* '*' */  0x14, 0x08, 0x3E, 0x08, 0x14,
/* '+' */  0x08, 0x08, 0x3E, 0x08, 0x08,
/* '-' */  0x08, 0x08, 0x08, 0x08, 0x08,
/* '=' */  0x14, 0x14, 0x14, 0x14, 0x14,
/* '_' */  0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
/* '/' */  0x40, 0x30, 0x0C, 0x03,
/* '\' */  0x01, 0x06, 0x18, 0x60,
/* '|' */  0xFF,
/* '`' */  0x01, 0x02,
/* '~' */  0x08, 0x04, 0x08, 0x10, 0x08,
/* '(' */  0x3E, 0x41,
/* ')' */  0x41, 0x3E,
/* '{' */  0x08, 0x36, 0x41,
/* '}' */  0x41, 0x36, 0x08,
/* '[' */  0x7F, 0x41,
/* ']' */  0x41, 0x7F,
/* '<' */  0x08, 0x14, 0x22, 0x41,
/* '>' */  0x41, 0x22, 0x14, 0x08
};

static const int asciiLut[256] =
{
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
           -1, 0,            -1, 0,            -1, 0,            -1, 0,
/* ' ' */  -1, 0, /* '!' */ 286, 1, /* '"' */ 279, 3, /* '#' */ 297, 6,
/* '$' */ 303, 5, /* '%' */ 308, 7, /* '&' */ 315, 6, /* ''' */ 278, 1,
/* '(' */ 363, 2, /* ')' */ 365, 2, /* '*' */ 321, 5, /* '+' */ 326, 5,
/* ',' */ 284, 1, /* '-' */ 331, 5, /* '.' */ 285, 1, /* '/' */ 347, 4,
/* '0' */ 230, 5, /* '1' */ 235, 3, /* '2' */ 238, 5, /* '3' */ 243, 5,
/* '4' */ 248, 5, /* '5' */ 253, 5, /* '6' */ 258, 5, /* '7' */ 263, 5,
/* '8' */ 268, 5, /* '9' */ 273, 5, /* ':' */ 283, 1, /* ';' */ 282, 1,
/* '<' */ 377, 4, /* '=' */ 336, 5, /* '>' */ 381, 4, /* '?' */ 287, 5,
/* '@' */ 292, 5, /* 'A' */ 102, 5, /* 'B' */ 107, 5, /* 'C' */ 112, 5,
/* 'D' */ 117, 5, /* 'E' */ 122, 4, /* 'F' */ 126, 4, /* 'G' */ 130, 5,
/* 'H' */ 135, 5, /* 'I' */ 140, 3, /* 'J' */ 143, 4, /* 'K' */ 147, 5,
/* 'L' */ 152, 4, /* 'M' */ 156, 7, /* 'N' */ 163, 5, /* 'O' */ 168, 5,
/* 'P' */ 173, 5, /* 'Q' */ 178, 5, /* 'R' */ 183, 5, /* 'S' */ 188, 5,
/* 'T' */ 193, 5, /* 'U' */ 198, 5, /* 'V' */ 203, 5, /* 'W' */ 208, 7,
/* 'X' */ 215, 5, /* 'Y' */ 220, 5, /* 'Z' */ 225, 5, /* '[' */ 373, 2,
/* '\' */ 351, 4, /* ']' */ 375, 2, /* '^' */  -1, 0, /* '_' */ 341, 6,
/* '`' */ 356, 2, /* 'a' */   0, 4, /* 'b' */   4, 4, /* 'c' */   8, 3,
/* 'd' */  11, 4, /* 'e' */  15, 4, /* 'f' */  19, 4, /* 'g' */  23, 4,
/* 'h' */  27, 4, /* 'i' */  31, 1, /* 'j' */  32, 3, /* 'k' */  35, 4,
/* 'l' */  39, 3, /* 'm' */  42, 7, /* 'n' */  49, 4, /* 'o' */  53, 4,
/* 'p' */  57, 4, /* 'q' */  61, 4, /* 'r' */  65, 3, /* 's' */  68, 4,
/* 't' */  72, 4, /* 'u' */  76, 4, /* 'v' */  80, 4, /* 'w' */  84, 5,
/* 'x' */  89, 5, /* 'y' */  94, 4, /* 'z' */  98, 4, /* '{' */ 367, 3,
/* '|' */ 355, 1, /* '}' */ 370, 3, /* '~' */ 358, 5,            -1, 0
};

int fontGetGlyph(int c, int* restrict offset, int* restrict size)
{
	if (c < 0x80)
	{
		int i = c * 2;
		if (asciiLut[i] < 0)
			return 0;
		(*offset) = asciiLut[i++];
		(*size)   = asciiLut[i];
		return 1;
	}
	return 0;
}
