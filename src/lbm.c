#include "lbm.h"
#include "lbmdef.h"
#include <stdlib.h>
#include <string.h>


typedef enum { CHUNK_BMHD = 1, CHUNK_CMAP = 1 << 1, CHUNK_BODY = 1 << 2 } LbmChunkMask;

typedef struct
{
	const LbmIocb* iocb;
	LbmCbCustomChunkSubscriber customSub;
	LbmCbCustomChunkHandler    customHndl;
	uint8_t* custom;
	uint32_t customLen;

	IffChunkHeader  form;
	uint32_t        formatId;

	LbmChunkMask    chunkMask;
	LbmBitmapHeader bmhd;
	Colour          cmap[256];
	unsigned        numCmap;
	LbmColourRange  crng[LBM_MAX_CRNG];
	unsigned        numCrng;
	uint8_t*        body;
	unsigned        bodyLen;

} LbmReaderState;


#define IO_READ(V, L) s->iocb->read((V), (L), s->iocb->user)
#define IO_SEEK(OFF, WHENCE) s->iocb->seek((OFF), (WHENCE), s->iocb->user)
#define IO_TELL() s->iocb->tell(s->iocb->user)

#define IO_READ_FOURCC(V) IO_READ(&(V), sizeof(uint32_t))
#define IO_READ_ULONG(V)  IO_READ(&(V), sizeof(uint32_t)); (V) = SWAP_BE32(V)
#define IO_READ_UWORD(V)  IO_READ(&(V), sizeof(uint16_t)); (V) = SWAP_BE16(V)
#define IO_READ_UBYTE(V)  IO_READ(&(V), sizeof(uint8_t))
#define IO_READ_WORD(V)   IO_READ(&(V), sizeof(int16_t));  (V) = (int16_t)SWAP_BE16((uint16_t)(V))
#define IO_READ_BYTE(V)   IO_READ(&(V), sizeof(int8_t))

#define IO_CHUNK_SKIP(BYTES_READ) \
	if (chunk->realLen > (BYTES_READ)) \
		IO_SEEK(chunk->realLen - (BYTES_READ), LBMIO_SEEK_CUR)


static IffChunkHeader iffReadChunk(LbmReaderState* s)
{
	IffChunkHeader chunk;
	IO_READ_FOURCC(chunk.chunkId);
	IO_READ_ULONG( chunk.chunkLen);
	chunk.realLen = chunk.chunkLen + (chunk.chunkLen & 0x1);
	return chunk;
}

static uint32_t lbmReadFormatId(LbmReaderState* s)
{
	uint32_t formatId;
	IO_READ_FOURCC(formatId);
	return formatId;
}

static int lbmReadBitmapHeader(LbmReaderState* s, const IffChunkHeader* chunk)
{
	// Should always be first chunk
	if (s->chunkMask != 0)
		return -1;

	if (chunk->chunkLen < BMHD_SIZE)
		return -1;

	LbmBitmapHeader* bmhd = &s->bmhd;
	IO_READ_UWORD(bmhd->w);
	IO_READ_UWORD(bmhd->h);
	IO_READ_WORD( bmhd->x);
	IO_READ_WORD( bmhd->y);
	IO_READ_UBYTE(bmhd->numPlanes);
	IO_READ_UBYTE(bmhd->masking);
	IO_READ_UBYTE(bmhd->compression);
	IO_READ_UBYTE(bmhd->pad1);
	IO_READ_UWORD(bmhd->transparentColour);
	IO_READ_UBYTE(bmhd->xAspect);
	IO_READ_UBYTE(bmhd->yAspect);
	IO_READ_WORD( bmhd->pageWidth);
	IO_READ_WORD( bmhd->pageHeight);

	if (bmhd->masking >= NUM_MSK)
		return -1;
	if (bmhd->compression >= NUM_CMP)
		return -1;

	s->chunkMask |= CHUNK_BMHD;

	IO_CHUNK_SKIP(BMHD_SIZE);
	return 0;
}

static int lbmReadColourMap(LbmReaderState* s, const IffChunkHeader* chunk)
{
	// Needs to be after header, only allowed once
	if (s->chunkMask & CHUNK_CMAP || !(s->chunkMask & CHUNK_BMHD))
		return -1;

	s->numCmap = chunk->chunkLen / 3;
	if (!s->numCmap || s->numCmap > 256)
		return -1;

	for (size_t i = 0; i < s->numCmap; ++i)
	{
		uint8_t triplet[3];
		IO_READ(triplet, 3);
		s->cmap[i] = MAKE_COLOUR(triplet[0], triplet[1], triplet[2], 0xFF);
	}

	s->chunkMask |= CHUNK_CMAP;

	// Skip to end of chunk if there are leftover entries for some reason
	// (the spec told me to do it!!!)
	IO_CHUNK_SKIP(s->numCmap * 3);
	return 0;
}

static int lbmReadColourRange(LbmReaderState* s, const IffChunkHeader* chunk)
{
	// Must be after header
	if (!(s->chunkMask & CHUNK_BMHD))
		return -1;
	if (chunk->chunkLen < CRNG_SIZE)
		return -1;
	if (s->numCrng == LBM_MAX_CRNG)
		return -1;

	LbmColourRange crng;
	IO_READ_WORD( crng.pad1);
	IO_READ_WORD( crng.rate);
	IO_READ_WORD( crng.flags);
	IO_READ_UBYTE(crng.low);
	IO_READ_UBYTE(crng.high);
	if (crng.flags & RNG_REVERSE)
		crng.rate = (int16_t)-crng.rate;

	s->crng[s->numCrng++] = crng;
	IO_CHUNK_SKIP(CRNG_SIZE);
	return 0;
}

static size_t lbmReadPbm(LbmReaderState* s, uint8_t* pix, size_t pixLen, size_t chunkLen)
{
	if (s->bmhd.compression == CMP_NONE)
	{
		size_t len = MIN(pixLen, chunkLen);
		IO_READ(pix, len);
		if (len < pixLen)
			memset(&pix[len], 0, len - pixLen);
		return len;
	}
	else if (s->bmhd.compression == CMP_BYTE_RUN1)
	{
		size_t read = 0, pixRead = 0;
		size_t copyLen = 0, runLen = 0;
		do
		{
			if (copyLen)
			{
				IO_READ(&pix[pixRead], copyLen);
				pixRead += copyLen;
				read += copyLen;
				copyLen = 0;
			}
			else if (runLen)
			{
				uint8_t byte;
				IO_READ_UBYTE(byte);
				read += 1;
				memset(&pix[pixRead], byte, runLen);
				pixRead += runLen;
				runLen = 0;
			}
			else
			{
				int8_t byte;
				IO_READ_BYTE(byte);
				read += 1;
				if (byte >= 0)
				{
					copyLen = MIN((size_t)byte + 1 + pixRead, pixLen) - pixRead;
					copyLen = MIN(copyLen + read, chunkLen) - read;
				}
				else if (byte >= -127)
					runLen = MIN((size_t)-byte + 1 + pixRead, pixLen) - pixRead;
			}
		}
		while (pixRead < pixLen && read < chunkLen);
		if (pixRead < pixLen)
			memset(&pix[pixRead], 0, pixRead - pixLen);
		return read;
	}
	return SIZE_MAX;
}

static size_t lbmReadIlbm(LbmReaderState* s, uint8_t* pix, size_t pixLen, size_t chunkLen)
{
	if (s->bmhd.compression == CMP_NONE)
	{
		return SIZE_MAX;
	}
	else if (s->bmhd.compression == CMP_BYTE_RUN1)
	{
		return SIZE_MAX;
	}
	return SIZE_MAX;
}

static int lbmReadBody(LbmReaderState* s, const IffChunkHeader* chunk)
{
	// Must be after header
	if (!(s->chunkMask & CHUNK_BMHD))
		return -1;

	const size_t pixLen = s->bmhd.w * (size_t)s->bmhd.h;
	s->body = malloc(pixLen);
	if (!s->body)
		return -1;

	size_t len = chunk->chunkLen;
	if (s->formatId == IFF_PBM)
		len = lbmReadPbm(s, s->body, pixLen, len);
	else if (s->formatId == IFF_ILBM)
		len = lbmReadIlbm(s, s->body, pixLen, len);
	if (len == SIZE_MAX)
		return -1;

	s->chunkMask |= CHUNK_BODY;

	IO_CHUNK_SKIP(len);
	return 0;
}

static int lbmReadCustom(LbmReaderState* s, const IffChunkHeader* chunk)
{
	if (chunk->chunkLen > s->customLen)
	{
		if (s->custom)
			free(s->custom);
		s->custom = malloc(chunk->chunkLen);
		if (!s->custom)
			return -1;
		s->customLen = chunk->chunkLen;
	}
	IO_READ(s->custom, chunk->chunkLen);
	int res = s->customHndl(chunk->chunkId, chunk->chunkLen, s->custom);
	if (res < 0)
		return -1;
	if (res > 0)
	{
		s->custom = NULL;
		s->customLen = 0;
	}
	if (chunk->realLen > chunk->chunkLen)
		IO_SEEK(chunk->realLen - chunk->chunkLen, LBMIO_SEEK_CUR);
	return 0;
}

static int lbmReadSections(LbmReaderState* s)
{
	do
	{
		IffChunkHeader chunk = iffReadChunk(s);
		if (IO_TELL() + chunk.realLen >= s->form.chunkLen + sizeof(IffChunkHeader))
			return -1;

		switch (chunk.chunkId)
		{
		case (IFF_BMHD): if (lbmReadBitmapHeader(s, &chunk)) return -1; break;
		case (IFF_CMAP): if (lbmReadColourMap(s, &chunk)) return -1; break;
		case (IFF_CRNG): if (lbmReadColourRange(s, &chunk)) return -1; break;
		case (IFF_BODY): if (lbmReadBody(s, &chunk)) return -1; break;
		default:
			if (s->customSub && s->customHndl && s->customSub(chunk.chunkId))
			{
				if (lbmReadCustom(s, &chunk))
					return -1;
			}
			else IO_SEEK(chunk.realLen, LBMIO_SEEK_CUR);
			break;
		}

		// We don't support these right now
		if (chunk.chunkId == IFF_BMHD)
		{
			if (s->bmhd.masking != MSK_NONE)
				return -1;
			if (s->bmhd.numPlanes != 8)
				return -1;
		}
	}
	while (IO_TELL() < s->form.chunkLen - sizeof(IffChunkHeader));
	return 0;
}

int lbmLoad(Lbm* out)
{
	if (!out)
		return -1;
	const LbmIocb* iocb = &out->iocb;
	if (!iocb)
		return -1;
	if (!iocb->read || !iocb->seek || !iocb->tell)
		return -1;

	LbmReaderState s =
	{
		.custom = NULL, .customLen = 0,
		.iocb = iocb,
		.customSub = out->customSub,
		.customHndl = out->customHndl,
		.numCrng = 0,
		.numCmap = 0
	};
	int res = -1;

	// Read chunks
	s.form = iffReadChunk(&s);
	if (s.form.chunkId != IFF_FORM)
		goto cleanup;
	s.formatId = lbmReadFormatId(&s);
	if (s.formatId == IFF_ILBM) goto cleanup; // We don't support ILBM for now
	if (s.formatId != IFF_PBM && s.formatId != IFF_ILBM)
		goto cleanup;
	if (lbmReadSections(&s))
		goto cleanup;
	const LbmChunkMask requiredChunks = CHUNK_BMHD | CHUNK_BODY;
	if ((s.chunkMask & requiredChunks) != requiredChunks)
		goto cleanup;

	// Outputs
	out->w = s.bmhd.w;
	out->h = s.bmhd.h;
	out->pixels = s.body;

	// Copy palette
	if (s.numCmap > 0)
		memcpy(out->palette, s.cmap, s.numCmap * sizeof(Colour));

	// Default palette for missing or short palette
	if (s.numCmap < 256)
	{
		const Colour* defaultPal = lbmPbmDefaultPal;
		if (s.formatId == IFF_ILBM)
			defaultPal = lbmIlbmDefaultPal;

		unsigned copyNum = 256 - s.numCmap;
		memcpy(&out->palette[s.numCmap], &defaultPal[s.numCmap], copyNum * sizeof(Colour));
	}

	for (unsigned i = 0; i < s.numCrng; ++i)
	{
		const LbmColourRange* crng = &s.crng[i];
		out->rangeLow[i]  = crng->low;
		out->rangeHigh[i] = crng->high;
		out->rangeRate[i] = crng->rate;
	}
	out->numRange = s.numCrng;

	res = 0;
cleanup:
	if (s.custom)
		free(s.custom);
	if (iocb->close)
		iocb->close(out->iocb.user);
	return res;
}

void lbmFree(Lbm* out)
{
	if (out->pixels)
	{
		free(out->pixels);
		out->pixels = NULL;
	}
}
