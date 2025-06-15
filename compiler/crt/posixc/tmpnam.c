/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)tmpnam.c	8.3 (Berkeley) 3/28/94";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <stdio.h>
#include <unistd.h>

#ifdef __AROS__
#   include <stdlib.h>
#   define _mktemp mktemp
#else
__warn_references(tmpnam,
    "warning: tmpnam() possibly used unsafely; consider using mkstemp()");

extern char *_mktemp(char *);
#endif

/*****************************************************************************

    NAME */

	char *tmpnam(

/*  SYNOPSIS */
	char *s)

/*  FUNCTION
        Generates a string that is a valid and unique filename for a temporary
        file. The filename is not associated with an open file and is not
        guaranteed to remain unique if used multiple times without creating
        the file immediately.

    INPUTS
        s - Optional pointer to a user-provided buffer. If NULL, a static internal
            buffer is used. The buffer must be at least L_tmpnam bytes in size.

    RESULT
        Returns a pointer to the generated filename (either `s` or internal static buffer),
        or NULL if an error occurs.

    NOTES
        - The generated name is not guaranteed to be safe against race conditions;
          use `tmpfile()` or `mkstemp()` for safer temporary file handling.
        - If called multiple times with NULL, the static buffer is reused and
          may be overwritten by subsequent calls.
        - On AROS, the filename is formed using the `P_tmpdir` macro and a counter.
        - `_mktemp()` is used to finalize the name in-place by replacing `XXXXXX`.

    EXAMPLE
        char name[L_tmpnam];
        if (tmpnam(name) != NULL) {
            printf("Temporary file name: %s\n", name);
        }

    BUGS
        - Not thread-safe when `s == NULL`, due to use of a static buffer.
        - The `_mktemp()` function is inherently unsafe; it does not create the file,
          which can lead to race conditions if multiple processes or threads are involved.
        - May return the same name if not followed by immediate file creation.

    SEE ALSO
        tmpfile(), mkstemp(), mktemp()

    INTERNALS
        - Uses a static counter (`tmpcount`) to generate unique suffixes.
        - Constructs the name using snprintf() and `P_tmpdir` (typically "T:" on AROS).
        - Calls `_mktemp()` to transform the `XXXXXX` suffix into a unique string.

******************************************************************************/
{
	static unsigned int tmpcount;
	static char buf[L_tmpnam];

	if (s == NULL)
		s = buf;
	(void)snprintf(s, L_tmpnam, "%stmp.%u.XXXXXX", P_tmpdir, tmpcount);
	++tmpcount;
	return (_mktemp(s));
}
