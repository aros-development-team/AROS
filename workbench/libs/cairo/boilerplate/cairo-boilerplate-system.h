/*
 * Copyright © 2004 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#ifndef _XMALLOC_H_
#define _XMALLOC_H_

#include "cairo-boilerplate.h"

#define xmalloc cairo_boilerplate_xmalloc
void *
xmalloc (size_t size);

#define xcalloc cairo_boilerplate_xcalloc
void *
xcalloc (size_t nmemb,
	 size_t size);

#define xrealloc cairo_boilerplate_xrealloc
void *
xrealloc (void	 *buf,
	  size_t  size);

#define xasprintf cairo_boilerplate_xasprintf
void
xasprintf (char       **strp,
	   const char  *fmt,
			...) CAIRO_BOILERPLATE_PRINTF_FORMAT(2, 3);

#define xunlink cairo_boilerplate_xunlink
void
xunlink (const char *path);

#define xstrdup cairo_boilerplate_xstrdup
char *
xstrdup (const char *str);

#endif
