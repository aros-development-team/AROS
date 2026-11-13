/*
 * crypt.c --- password encryption
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tom Truscott.
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
 *        This product includes software developed by the University of
 *        California, Berkeley and its contributors.
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

#include <sys/errno.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

#include "base.h"

#include <proto/usergroup.h>

extern const unsigned char itoa64[];

/****** usergroup.library/ug_GetSalt ***************************************

    NAME
        ug_GetSalt - generate password encryption setting for an user

    SYNOPSIS
        setting = ug_GetSalt(passwd, buffer, size);
          D0                   A0      A1     D0

        UBYTE *ug_GetSalt(const struct passwd *, UBYTE *, ULONG);

    FUNCTION
        This function generates a setting parameter, which is used to
        configure the password encryption process.  If an old entry exists,
        the new salt depends on it.

        The particular salt format depends on the system security level.

    INPUTS
        passwd - the old passwd entry for the user, or NULL if there is none.

        buffer - address of character array, which the new salt is stored
                 in.

        size   - the number of bytes in the buffer. The buffer should be at
                 least 12 bytes long.

    RETURN VALUE
        Pointer to the buffer, or NULL if the buffer is too small.

    NOTE
        By default the Version 7 UNIX compatible setting is generated.

    SEE ALSO
       crypt()

****************************************************************************
*/

AROS_LH3I(char *, ug_GetSalt,
          AROS_LHA(const struct passwd *, pw, A0),
          AROS_LHA(char *, buffer, A1),
          AROS_LHA(ULONG, size, D0),
          struct Library *, UserGroupBase, 30, Usergroup)
{
    AROS_LIBFUNC_INIT

    LONG rv = LRandom();

    if (size < 12)
        return NULL;

    buffer[0] = itoa64[rv & 63];
    buffer[1] = itoa64[(rv >> 6) & 63];
    buffer[2] = '\0';

    return buffer;

    AROS_LIBFUNC_EXIT
}
