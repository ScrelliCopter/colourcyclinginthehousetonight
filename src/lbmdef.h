#ifndef LBMDEF_H
#define LBMDEF_H

#include "util.h"

#define IFF_FORM FOURCC('F', 'O', 'R', 'M')
#define IFF_ILBM FOURCC('I', 'L', 'B', 'M')
#define IFF_PBM  FOURCC('P', 'B', 'M', ' ')
#define IFF_BMHD FOURCC('B', 'M', 'H', 'D')
#define IFF_CMAP FOURCC('C', 'M', 'A', 'P')
#define IFF_CRNG FOURCC('C', 'R', 'N', 'G')
#define IFF_BODY FOURCC('B', 'O', 'D', 'Y')

typedef struct
{
	uint32_t chunkId;
	uint32_t chunkLen;
	uint32_t realLen;
} IffChunkHeader;

typedef uint8_t LbmMask;
enum { MSK_NONE, MSK_HAS_MASK, MSK_HAS_TRANSPARENT_COLOUR, MSK_LASSO, NUM_MSK };

typedef uint8_t LbmCompression;
enum { CMP_NONE, CMP_BYTE_RUN1, NUM_CMP };

typedef struct
{
	uint16_t       w, h;
	int16_t        x, y;
	uint8_t        numPlanes;
	LbmMask        masking;
	LbmCompression compression;
	uint8_t        pad1;
	uint16_t       transparentColour;
	uint8_t        xAspect, yAspect;
	int16_t        pageWidth, pageHeight;
} LbmBitmapHeader;
#define BMHD_SIZE 20

typedef int16_t CrngFlags;
enum { RNG_ACTIVE  = 1, RNG_REVERSE = 2 };

typedef struct
{
	int16_t   pad1;
	int16_t   rate;
	CrngFlags flags;
	uint8_t   low, high;
} LbmColourRange;
#define CRNG_SIZE 8

#endif//LBMDEF_H
