/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

*/

#include <utility/utility.h> /* this must be before icon_intern.h */

#include <aros/symbolsets.h>

#ifndef __ICON_NOLIBBASE__
#define __ICON_NOLIBBASE__
#endif /* !__ICON_NOLIBBASE__ */

#include "icon_intern.h"
#include "identify.h"

#include LC_LIBDEFS_FILE

const LONG IFFParseBase_version = 39,
           GfxBase_version      = 39,
           CyberGfxBase_version = 41,
           DataTypesBase_version = 0;

/****************************************************************************************/

/* Offsets of the library bases in the IconBase, in the order they are closed.
 * The last four are the required libraries, opened last to first. */
#define IBOFF(field)    offsetof(struct IconBase, field)

static const UWORD libbases[] =
{
    IBOFF(ib_CyberGfxBase), IBOFF(ib_DataTypesBase), IBOFF(ib_IFFParseBase), IBOFF(ib_WorkbenchBase),
    IBOFF(ib_IntuitionBase), IBOFF(ib_GfxBase), IBOFF(ib_DOSBase), IBOFF(ib_UtilityBase)
};
#define NUM_LIBBASES    8
#define FIRST_REQUIRED  4

static const char * const libnames[] =
{
    "intuition.library", "graphics.library", "dos.library", "utility.library"
};

_Static_assert(sizeof(libbases) / sizeof(libbases[0]) == NUM_LIBBASES &&
               sizeof(libnames) / sizeof(libnames[0]) == NUM_LIBBASES - FIRST_REQUIRED,
               "one name per required library");

#define LIBPTR(base, n) (*(struct Library **)((UBYTE *)(base) + libbases[n]))

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR lh)
{
    LONG i;

    LB(lh)->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
    LB(lh)->dsh.h_Data  = lh;

    InitSemaphore(&LB(lh)->iconlistlock);
    for(i = 0; i < ICONLIST_HASHSIZE; i++)
    {
        NewList((struct List *)&LB(lh)->iconlists[i]);
    }
    
    /* Setup default global settings ---------------------------------------*/
    LB(lh)->ib_Screen               = NULL;
    LB(lh)->ib_Precision            = PRECISION_ICON;
    LB(lh)->ib_EmbossRectangle.MinX = -4;
    LB(lh)->ib_EmbossRectangle.MaxX = 4;
    LB(lh)->ib_EmbossRectangle.MinY = -4;
    LB(lh)->ib_EmbossRectangle.MaxY = 4;
    LB(lh)->ib_Frameless            = FALSE;
    LB(lh)->ib_IdentifyHook         = NULL;
    LB(lh)->ib_MaxNameLength        = 25;
    LB(lh)->ib_NewIconsSupport      = TRUE;
    LB(lh)->ib_ColorIconSupport     = TRUE;

    for (i = NUM_LIBBASES - 1; i >= FIRST_REQUIRED; i--)
    {
        if ((LIBPTR(lh, i) = OpenLibrary(libnames[i - FIRST_REQUIRED],
                (i == FIRST_REQUIRED + 1) ? GfxBase_version : 0)) == NULL)
        {
            /* Close the ones opened so far, latest first */
            while (++i < NUM_LIBBASES)
                CloseLibrary(LIBPTR(lh, i));
            return FALSE;
        }
    }

    /* Optional libraries are loaded dynamically if needed */
    return TRUE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    LONG i;

    /* Drop optional libraries, then the rest */
    for (i = 0; i < NUM_LIBBASES; i++)
    {
        if (LIBPTR(LIBBASE, i)) CloseLibrary(LIBPTR(LIBBASE, i));
    }
    
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0);
