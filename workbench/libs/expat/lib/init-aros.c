/* Copyright (c) 2012 The AROS Development Team. All rights reserved.
   See the file COPYING for copying permission.
*/

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

#define DEBUG 1
#include <aros/debug.h>

IPTR aroscbase_offset;


static int InitFunc(LIBBASETYPEPTR LIBBASE)
{
    D(bug("Inside Init func of expat.library\n"));

    aroscbase_offset = offsetof(LIBBASETYPE, _aroscbase);

    return TRUE;
}

static int OpenFunc(LIBBASETYPEPTR LIBBASE)
{
   D(bug("Opening expat.library\n"));

   LIBBASE->_aroscbase = OpenLibrary("arosc.library", 0);

   D(bug("[expat.library::OpenLib] aroscbase=%p\n", LIBBASE->_aroscbase));

   return LIBBASE->_aroscbase != NULL;
}

static int CloseFunc(LIBBASETYPEPTR LIBBASE)
{
   D(bug("Closing expat.library\n"));

   CloseLibrary(LIBBASE->_aroscbase);

   return TRUE;
}


ADD2INITLIB(InitFunc, 0);
ADD2OPENLIB(OpenFunc, 0);
ADD2CLOSELIB(CloseFunc, 0);
