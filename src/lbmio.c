/* lbmio.c - (C) 2023 a dinosaur (zlib) */
#include "lbm.h"
#include <stdio.h>


size_t lbmDefaultFileRead(void* out, size_t size, size_t numItems, void* user)
{
	return fread(out, size, numItems, (FILE*)user);
}

int lbmDefaultFileSeek(size_t offset, LbmIoWhence whence, void* user)
{
	int seek;
	switch (whence)
	{
	case LBMIO_SEEK_SET: seek = SEEK_SET; break;
	case LBMIO_SEEK_CUR: seek = SEEK_CUR; break;
	case LBMIO_SEEK_END: seek = SEEK_END; break;
	default: seek = -1;
	}

#ifdef HAVE_FSEEKO
	return fseeko((FILE*)user, (off_t)offset, seek);
#else
	return fseek((FILE*)user, (long)offset, seek);
#endif
}

size_t lbmDefaultFileTell(void* user)
{
#ifdef HAVE_FTELLO
	return (size_t)ftello((FILE*)user);
#else
	return ftell((FILE*)user);
#endif
}

void lbmDefaultFileClose(void* user)
{
	fclose((FILE*)user);
}
