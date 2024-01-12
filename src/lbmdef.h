#ifndef LBMDEF_H
#define LBMDEF_H

#include "util.h"

#define IFF_FORM FOURCC('F', 'O', 'R', 'M')
#define IFF_ILBM FOURCC('I', 'L', 'B', 'M')
#define IFF_PBM  FOURCC('P', 'B', 'M', ' ')
#define IFF_BMHD FOURCC('B', 'M', 'H', 'D')
#define IFF_CMAP FOURCC('C', 'M', 'A', 'P')
#define IFF_CAMG FOURCC('C', 'A', 'M', 'G')
#define IFF_CRNG FOURCC('C', 'R', 'N', 'G')
#define IFF_CCRT FOURCC('C', 'C', 'R', 'T')
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
enum { CMP_NONE, CMP_BYTE_RUN1, VERTICAL_RLE, NUM_CMP };

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

enum
{
	CAMG_GENLOCK =    0x2,  // GENLOCK_VIDEO
	CAMG_LACE    =    0x4,  // LACE
	CAMG_DBLSCAN =    0x8,  // DOUBLESCAN
	CAMG_SUPERHR =   0x20,  // SUPERHIRES
	CAMG_PFBA    =   0x40,  // PFBA
	CAMG_EHB     =   0x80,  // EXTRA_HALFBRITE
	CAMG_GLAUDIO =  0x100,  // CAMG_GLAUDIO
	CAMG_DUALPF  =  0x400,  // DUALPF
	CAMG_HAM     =  0x800,  // HAM
	CAMG_EXTMODE = 0x1000,  // EXTENDED_MODE
	CAMG_VP_HIDE = 0x2000,  // VP_HIDE
	CAMG_SPRITES = 0x4000,  // SPRITES
	CAMG_HIRES   = 0x8000   // HIRES
};
#define CAMG_SIZE 4

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

typedef int16_t CcrtDirection;
enum { DIR_NONE = 0, DIR_FORWARD = 1, DIR_BACKWARD = -1 };

typedef struct
{
	CcrtDirection direction;
	uint8_t start, end;
	int32_t seconds, microseconds;
	int16_t pad;
} LbmGraphicraftRange;
#define CCRT_SIZE 14

#endif//LBMDEF_H
