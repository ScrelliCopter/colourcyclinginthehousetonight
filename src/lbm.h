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

typedef size_t (*LbmIocbRead)(void* out, size_t size, void* user);
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

size_t lbmDefaultFileRead(void* out, size_t size, void* user);
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

typedef uint32_t Color;
#define COLOR_RSHIFT 16
#define COLOR_GSHIFT  8
#define COLOR_BSHIFT  0
#define COLOR_ASHIFT 24
#define COLOR_RMASK 0x00FF0000
#define COLOR_GMASK 0x0000FF00
#define COLOR_BMASK 0x000000FF
#define COLOR_AMASK 0xFF000000

#define MAKE_COLOR(R, G, B, A) ( \
	(COLOR_BMASK & ((Color)(B) << COLOR_BSHIFT)) | \
	(COLOR_GMASK & ((Color)(G) << COLOR_GSHIFT)) | \
	(COLOR_RMASK & ((Color)(R) << COLOR_RSHIFT)) | \
	(COLOR_AMASK & ((Color)(A) << COLOR_ASHIFT)))
#define MAKE_RGB(R, G, B) MAKE_COLOR((R), (G), (B), 0xFF)

#define COLOR_R(C) (((C) & COLOR_RMASK) >> COLOR_RSHIFT)
#define COLOR_G(C) (((C) & COLOR_GMASK) >> COLOR_GSHIFT)
#define COLOR_B(C) (((C) & COLOR_BMASK) >> COLOR_BSHIFT)
#define COLOR_A(C) (((C) & COLOR_AMASK) >> COLOR_ASHIFT)

#define LBM_PAL_SIZE 256
#define LBM_MAX_CRNG  16

typedef struct Lbm
{
	LbmIocb iocb;
	LbmCbCustomChunkSubscriber customSub;
	LbmCbCustomChunkHandler    customHndl;

	int w, h;
	uint8_t* pixels;
	Color palette[LBM_PAL_SIZE];
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

extern const Color lbmPbmDefaultPal[LBM_PAL_SIZE];
extern const Color lbmIlbmDosDefaultPal[LBM_PAL_SIZE];
extern const Color lbmIlbmDefaultPal[LBM_PAL_SIZE];

#endif//LBM_H
