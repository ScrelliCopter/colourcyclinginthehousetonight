#ifndef LBM_H
#define LBM_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
	LBMIO_SEEK_SET = 0, // From start of file
	LBMIO_SEEK_CUR = 1, // From current position in file
	LBMIO_SEEK_END = 2  // From EOF
} LbmIoWhence;

typedef size_t (*LbmIocbRead)(void* out, size_t size, size_t numItems, void* user);
typedef int    (*LbmIocbSeek)(size_t offset, LbmIoWhence whence, void* user);
typedef size_t (*LbmIocbTell)(void* user);
typedef void   (*LbmIocbClose)(void* user);

typedef struct
{
	LbmIocbRead  read;
	LbmIocbSeek  seek;
	LbmIocbTell  tell;
	LbmIocbClose close;
	void* user;

} LbmIocb;

size_t lbmDefaultFileRead(void* out, size_t size, size_t numItems, void* user);
int    lbmDefaultFileSeek(size_t offset, LbmIoWhence whence, void* user);
size_t lbmDefaultFileTell(void* user);
void   lbmDefaultFileClose(void* user);

#define LBM_IO_CLEAR() (LbmIocb) { \
	.read = NULL,                  \
	.seek = NULL,                  \
	.tell = NULL,                  \
	.close = NULL,                 \
	.user = NULL }                 \

#define LBM_IO_DEFAULT(FILE) (LbmIocb){ \
	.read = &lbmDefaultFileRead,        \
	.seek = &lbmDefaultFileSeek,        \
	.tell = &lbmDefaultFileTell,        \
	.close = &lbmDefaultFileClose,      \
	.user = FILE }

typedef int (*LbmCbCustomChunkSubscriber)(uint32_t fourcc);
typedef int (*LbmCbCustomChunkHandler)(uint32_t fourcc, uint32_t size, uint8_t* chunk);

typedef uint32_t Colour;
#define COLOUR_RSHIFT 16
#define COLOUR_GSHIFT  8
#define COLOUR_BSHIFT  0
#define COLOUR_ASHIFT 24
#define COLOUR_RMASK 0x00FF0000
#define COLOUR_GMASK 0x0000FF00
#define COLOUR_BMASK 0x000000FF
#define COLOUR_AMASK 0xFF000000

#define MAKE_COLOUR(R, G, B, A) ( \
	(COLOUR_BMASK & ((Colour)(B) << COLOUR_BSHIFT)) | \
	(COLOUR_GMASK & ((Colour)(G) << COLOUR_GSHIFT)) | \
	(COLOUR_RMASK & ((Colour)(R) << COLOUR_RSHIFT)) | \
	(COLOUR_AMASK & ((Colour)(A) << COLOUR_ASHIFT)))
#define MAKE_RGB(R, G, B) MAKE_COLOUR((R), (G), (B), 0xFF)

#define COLOUR_R(C) ((uint8_t)(((C) & COLOUR_RMASK) >> COLOUR_RSHIFT))
#define COLOUR_G(C) ((uint8_t)(((C) & COLOUR_GMASK) >> COLOUR_GSHIFT))
#define COLOUR_B(C) ((uint8_t)(((C) & COLOUR_BMASK) >> COLOUR_BSHIFT))
#define COLOUR_A(C) ((uint8_t)(((C) & COLOUR_AMASK) >> COLOUR_ASHIFT))

#define LBM_PAL_SIZE 256
#define LBM_MAX_CRNG  16
#define LBM_MAX_CCRT LBM_MAX_CRNG

typedef struct Lbm
{
	LbmIocb iocb;
	LbmCbCustomChunkSubscriber customSub;
	LbmCbCustomChunkHandler    customHndl;

	int w, h;
	uint8_t* pixels;
	Colour palette[LBM_PAL_SIZE];
	uint8_t rangeLow[LBM_MAX_CRNG];
	uint8_t rangeHigh[LBM_MAX_CRNG];
	int16_t rangeRate[LBM_MAX_CRNG];
	unsigned numRange;

} Lbm;

#define LBM_CLEAR() (Lbm){  \
    .iocb = LBM_IO_CLEAR(), \
	.customSub = NULL,      \
	.customHndl = NULL,     \
	.w = 0, .h = 0,         \
	.pixels = NULL }

int lbmLoad(Lbm* out);
void lbmFree(Lbm* out);

extern const Colour lbmPbmDefaultPal[LBM_PAL_SIZE];
extern const Colour lbmIlbmDosDefaultPal[LBM_PAL_SIZE];
extern const Colour lbmIlbmDefaultPal[LBM_PAL_SIZE];

#endif//LBM_H
