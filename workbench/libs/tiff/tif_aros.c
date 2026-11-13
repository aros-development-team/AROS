/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library UNIX-specific Routines. These are should also work with the
 * Windows Common RunTime Library.
 */

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "tif_config.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <errno.h>

#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#include "tiffiop.h"


#define TIFF_IO_MAX 2147483647U


typedef union bptr_as_handle_union
{
	BPTR doshandle;
	thandle_t h;
} bptr_as_handle_union_t;

TIFF *TIFFBPTROpenExt(BPTR fhandle, const char *name, const char *mode,
                    TIFFOpenOptions *opts);

static tmsize_t
_tiffReadProc(thandle_t fd, void* buf, tmsize_t size)
{
	bptr_as_handle_union_t fdh;
	const size_t bytes_total = (size_t) size;
        size_t bytes_read;
        tmsize_t count = -1;

    D(bug("[TIFF] %s(0x%p, 0x%p, %u)\n", __func__, fd, buf, size);)

	if ((tmsize_t) bytes_total != size)
	{
		return (tmsize_t) -1;
	}
	fdh.h = fd;
	for (bytes_read=0; bytes_read < bytes_total; bytes_read+=count)
	{
		char *buf_offset = (char *) buf+bytes_read;
		size_t io_size = bytes_total-bytes_read;
		if (io_size > TIFF_IO_MAX)
			io_size = TIFF_IO_MAX;
		count=Read(fdh.doshandle, buf_offset, (TIFFIOSize_t) io_size);
                if (count <= 0)
                        break;
        }
        if (count < 0)
                return (tmsize_t)-1;
        return (tmsize_t) bytes_read;
}

static tmsize_t
_tiffWriteProc(thandle_t fd, void* buf, tmsize_t size)
{
	bptr_as_handle_union_t fdh;
	const size_t bytes_total = (size_t) size;
        size_t bytes_written;
        tmsize_t count = -1;

    D(bug("[TIFF] %s(0x%p, 0x%p, %u)\n", __func__, fd, buf, size);)
	
#if (0)
	if ((tmsize_t) bytes_total != size)
	{
		errno=EINVAL;
		return (tmsize_t) -1;
	}
	fdh.h = fd;
        for (bytes_written=0; bytes_written < bytes_total; bytes_written+=count)
        {
                const char *buf_offset = (char *) buf+bytes_written;
                size_t io_size = bytes_total-bytes_written;
                if (io_size > TIFF_IO_MAX)
                        io_size = TIFF_IO_MAX;
                count=write(fdh.doshandle, buf_offset, (TIFFIOSize_t) io_size);
                if (count <= 0)
                        break;
        }
        if (count < 0)
                return (tmsize_t)-1;
#endif
        return (tmsize_t) bytes_written;
	/* return ((tmsize_t) write(fdh.doshandle, buf, bytes_total)); */
}

static uint64_t
_tiffSeekProc(thandle_t fd, uint64_t off, int whence)
{
	bptr_as_handle_union_t fdh;
	_TIFF_off_t off_io = (_TIFF_off_t) off;
	LONG mode, oldpos;

    D(bug("[TIFF] %s(0x%p, %u, %u)\n", __func__, fd, off, whence);)
	
	if ((uint64_t) off_io != off)
	{
		errno=EINVAL;
		return (uint64_t) -1; /* this is really gross */
	}
	fdh.h = fd;
	
	switch(whence)
	{
		case SEEK_SET:
			mode = OFFSET_BEGINNING;
			break;
		case SEEK_CUR:
			mode = OFFSET_CURRENT;
			break;
		case SEEK_END:
			mode = OFFSET_END;
			break;
		default:
			mode = OFFSET_BEGINNING;
			break;
	}
	
	oldpos = Seek(fdh.doshandle, (LONG)off_io, mode);
	if (oldpos != -1)
		return(off_io);
	else
		return -1;
}

static int
_tiffCloseProc(thandle_t fd)
{
	bptr_as_handle_union_t fdh;
	fdh.h = fd;

    D(bug("[TIFF] %s(0x%p)\n", __func__, fd);)

	return(Close(fdh.doshandle));
}

static uint64_t
_tiffSizeProc(thandle_t fd)
{
    struct FileInfoBlock *fib;
	bptr_as_handle_union_t fdh;
	uint64_t size = 0;

    D(bug("[TIFF] %s(0x%p)\n", __func__, fd);)

	fdh.h = fd;
    if ((fib = AllocDosObject(DOS_FIB, NULL))) {
		if (ExamineFH(fdh.doshandle, fib)) {
			size = (uint64_t)fib->fib_Size;
        }
        FreeDosObject(DOS_FIB, fib);
	}
	return (size);
}

static int
_tiffMapProc(thandle_t fd, void** pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;

    D(bug("[TIFF] %s(0x%p)\n", __func__, fd);)

	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, void* base, toff_t size)
{
	(void) fd; (void) base; (void) size;

    D(bug("[TIFF] %s(0x%p)\n", __func__, fd);)
}

/*
 * Open a TIFF for read/writing.
 */
TIFF *TIFFBPTROpen(BPTR fhandle, const char *name, const char *mode)
{
    return TIFFBPTROpenExt(fhandle, name, mode, NULL);
}

TIFF *TIFFBPTROpenExt(BPTR fhandle, const char *name, const char *mode,
                    TIFFOpenOptions *opts)
{
	TIFF* tif = NULL;
	bptr_as_handle_union_t fdh;

    D(bug("[TIFF] %s(0x%p)\n", __func__, fhandle);)

	fdh.doshandle = fhandle;
    tif = TIFFClientOpenExt(name, mode, fdh.h, _tiffReadProc, _tiffWriteProc,
                            _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
                            _tiffMapProc, _tiffUnmapProc, opts);
#if (0)
	if (tif)
		tif->tif_fd = fhandle;
#endif
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF *TIFFOpen(const char *name, const char *mode)
{
    return TIFFOpenExt(name, mode, NULL);
}

TIFF *TIFFOpenExt(const char *name, const char *mode, TIFFOpenOptions *opts)
{
	static const char module[] = "TIFFOpen";
	BPTR fhandle;
	int m;
	TIFF* tif;

    D(bug("[TIFF] %s('%s')\n", __func__, name);)

    m = _TIFFgetMode(opts, NULL, mode, module);
	if (m == -1)
		return ((TIFF*)0);

	fhandle = Open(name, MODE_OLDFILE);
	if (!fhandle) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	tif = TIFFBPTROpenExt(fhandle, name, mode, opts);
	if(!tif)
		Close(fhandle);
	return tif;
}

void*
_TIFFmalloc(tmsize_t s)
{
    D(bug("[TIFF] %s(%u)\n", __func__, s);)

        if (s == 0)
                return ((void *) NULL);

	return (malloc((size_t) s));
}

void* _TIFFcalloc(tmsize_t nmemb, tmsize_t siz)
{
    D(bug("[TIFF] %s(0x%p, %u)\n", __func__, nmemb, siz);)

    if( nmemb == 0 || siz == 0 )
        return ((void *) NULL);

    return calloc((size_t) nmemb, (size_t)siz);
}

void
_TIFFfree(void* p)
{
    D(bug("[TIFF] %s(0x%p)\n", __func__, p);)

	free(p);
}

void*
_TIFFrealloc(void* p, tmsize_t s)
{
    D(bug("[TIFF] %s(0x%p, %u)\n", __func__, p, s);)

	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(void* p, int v, tmsize_t c)
{
    D(bug("[TIFF] %s(0x%p, %u, %u)\n", __func__, p, v, c);)

	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(void* d, const void* s, tmsize_t c)
{
    D(bug("[TIFF] %s(0x%p, 0x%p, %u)\n", __func__, d, s, c);)
	
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const void* p1, const void* p2, tmsize_t c)
{
    D(bug("[TIFF] %s(0x%p, 0x%p, %u)\n", __func__, p1, p2, c);)

	return (memcmp(p1, p2, (size_t) c));
}

static void
arosWarningHandler(const char* module, const char* fmt, va_list ap)
{
    D(bug("[TIFF] %s(%s, 0x%p)\n", __func__, module, fmt);)
	if (module != NULL)
		kprintf("%s: ", module);
	kprintf("Warning, ");
	vkprintf(fmt, ap);
	kprintf(".\n");
}
TIFFErrorHandler _TIFFwarningHandler = arosWarningHandler;

static void
arosErrorHandler(const char* module, const char* fmt, va_list ap)
{
    D(bug("[TIFF] %s(%s, 0x%p)\n", __func__, module, fmt);)

	if (module != NULL)
		kprintf("%s: ", module);
	vkprintf(fmt, ap);
	kprintf(".\n");
}
TIFFErrorHandler _TIFFerrorHandler = arosErrorHandler;
