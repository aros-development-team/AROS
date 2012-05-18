/* Copyright 2012 The AROS Development Team. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include "support.h"

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

IPTR aroscbase_offset;

int malloc_init(void);
void malloc_exit(void);

void bz_internal_error (int errcode)
{
    Alert(errcode);
}
       
static int InitFunc(LIBBASETYPEPTR LIBBASE)
{
    D(bug("Inside Init func of bz2.library\n"));

    aroscbase_offset = offsetof(LIBBASETYPE, _aroscbase);

    return TRUE;
}

static int OpenFunc(LIBBASETYPEPTR LIBBASE)
{
   D(bug("Opening bz2.library\n"));

   LIBBASE->_aroscbase = OpenLibrary("arosc.library", 0);

   D(bug("[bz2.library::OpenLib] aroscbase=%p\n", LIBBASE->_aroscbase));

   return LIBBASE->_aroscbase != NULL;
}

static int CloseFunc(LIBBASETYPEPTR LIBBASE)
{
   D(bug("Closing bz2.library\n"));

   CloseLibrary(LIBBASE->_aroscbase);

   return TRUE;
}

ADD2INITLIB(InitFunc, 0);
ADD2OPENLIB(OpenFunc, 0);
ADD2CLOSELIB(CloseFunc, 0);
