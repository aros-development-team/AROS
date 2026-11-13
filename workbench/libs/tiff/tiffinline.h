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

#ifndef _TIFFINLINE_
#define	_TIFFINLINE_

#include <tiffconf.h>
#include <tiff.h>

#include <stdarg.h>
#include <stdio.h>


/*
 * Return the value of a field in the
 * internal directory structure.
 */
static int
TIFFGetField(TIFF* tif, uint32_t tag, ...)
{
	int status;
	va_list ap;

	va_start(ap, tag);
	status = TIFFVGetField(tif, tag, ap);
	va_end(ap);
	return (status);
}

/*
 * Record the value of a field in the
 * internal directory structure.  The
 * field will be written to the file
 * when/if the directory structure is
 * updated.
 */
static int
TIFFSetField(TIFF* tif, uint32_t tag, ...)
{
	va_list ap;
	int status;

	va_start(ap, tag);
	status = TIFFVSetField(tif, tag, ap);
	va_end(ap);
	return (status);
}

/*
 * Like TIFFGetField, but return any default
 * value if the tag is not present in the directory.
 */
static int
TIFFGetFieldDefaulted(TIFF* tif, uint32_t tag, ...)
{
	int ok;
	va_list ap;

	va_start(ap, tag);
	ok =  TIFFVGetFieldDefaulted(tif, tag, ap);
	va_end(ap);
	return (ok);
}

#if (0)
void
TIFFError(const char* module, const char* fmt, ...)
{
	va_list ap;
	if (_TIFFerrorHandler) {
		va_start(ap, fmt);	
		(*_TIFFerrorHandler)(module, fmt, ap);
		va_end(ap);
	}
	if (_TIFFerrorHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFerrorHandlerExt)(0, module, fmt, ap);
		va_end(ap);
	}
}

void
TIFFErrorExt(thandle_t fd, const char* module, const char* fmt, ...)
{
	va_list ap;
	if (_TIFFerrorHandler) {
		va_start(ap, fmt);
		(*_TIFFerrorHandler)(module, fmt, ap);
		va_end(ap);
	}
	if (_TIFFerrorHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFerrorHandlerExt)(fd, module, fmt, ap);
		va_end(ap);
	}
}


void
TIFFWarning(const char* module, const char* fmt, ...)
{
	va_list ap;
	if (_TIFFwarningHandler) {
		va_start(ap, fmt);
		(*_TIFFwarningHandler)(module, fmt, ap);
		va_end(ap);
	}
	if (_TIFFwarningHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFwarningHandlerExt)(0, module, fmt, ap);
		va_end(ap);
	}
}

void
TIFFWarningExt(thandle_t fd, const char* module, const char* fmt, ...)
{
	va_list ap;
	if (_TIFFwarningHandler) {
		va_start(ap, fmt);	
		(*_TIFFwarningHandler)(module, fmt, ap);
		va_end(ap);
	}
	if (_TIFFwarningHandlerExt) {
		va_start(ap, fmt);
		(*_TIFFwarningHandlerExt)(fd, module, fmt, ap);
		va_end(ap);
	}
}
#endif

#endif /* _TIFFINLINE_ */
