/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: MUIMaster initialization code.
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <utility/utility.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/muimaster.h>

#include "muimaster_intern.h"
#include "libdefs.h"

#define LC_LIBHEADERTYPEPTR LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)     (((LIBBASETYPEPTR)(lib))->mmb_LibNode)
#define LC_SEGLIST_FIELD(lib) (((LIBBASETYPEPTR)(lib))->mmb_SegList)
#define LC_SYSBASE_FIELD(lib) (((LIBBASETYPEPTR)(lib))->mmb_SysBase)
#define LC_RESIDENTNAME  MUIMaster_resident
#define LC_RESIDENTPRI   0
#define LC_RESIDENTFLAGS RTF_AUTOINIT
#define LC_LIBBASESIZE   sizeof(LIBBASETYPE)

#define LC_NO_INITLIB
#define LC_STATIC_OPENLIB
#define LC_STATIC_CLOSELIB
#define LC_NO_EXPUNGELIB

#include <libcore/libheader.c>

#include <zunepriv.h>
#include <prefs.h>
#include <drawing.h>

struct ExecBase   *SysBase;
struct DosLibrary *DOSBase;
struct Library    *GfxBase;
struct Library    *LayersBase;
struct Library    *UtilityBase;
struct Library    *DiskfontBase;
struct Library    *IntuitionBase;
struct Library    *MUIMasterBase;

static void closeLibs(struct ExecBase *SysBase)
{
#define CLOSELIB(x) \
    (void)({if (x) { \
        CloseLibrary((struct Library *)(x)); \
        (x) = NULL; }})

    CLOSELIB(DOSBase);
    CLOSELIB(GfxBase);
    CLOSELIB(LayersBase);
    CLOSELIB(UtilityBase);
    CLOSELIB(DiskfontBase);
    CLOSELIB(IntuitionBase);
}

/* List of functions to call at exit */
static void (*atexitlist[15])(void);
static int    atexitcount = 0;

static ULONG L_OpenLib(LC_LIBHEADERTYPEPTR lh)
{
    SysBase = LC_SYSBASE_FIELD(lh);
    MUIMasterBase = (struct Library *)lh;

    if (LC_LIB_FIELD(lh).lib_OpenCnt == 0)
    {
        if ( !(DOSBase       = (struct DosLibrary *)OpenLibrary(DOSNAME, 36))
          || !(GfxBase       = OpenLibrary("graphics.library", 36))
          || !(LayersBase    = OpenLibrary("layers.library",   36))
          || !(UtilityBase   = OpenLibrary("utility.library",  36))
          || !(DiskfontBase  = OpenLibrary("diskfont.library", 36))
          || !(IntuitionBase = OpenLibrary(INTUITIONNAME,      36)) )
        {
            closeLibs(LC_SYSBASE_FIELD(lh));
            return FALSE;
        }

        //__zune_imspec_init();
        //__zune_images_init();

        /*
         * init prefs before loading from files - in case there's no file
         */
        __zune_prefs_init(&__zprefs);
    }

    return TRUE;
}

static void L_CloseLib(LC_LIBHEADERTYPEPTR lh)
{
    int i;

    if (LC_LIB_FIELD(lh).lib_OpenCnt == 0)
    {
        for (i = 0; i < atexitcount; i++)
            (*atexitlist[i])();

        /* destroy builtin classes */
        if (!destroy_classes())
            kprintf("*** destroy_classes() failed, doom is near\n");

        closeLibs(LC_SYSBASE_FIELD(lh));
    }
}

/*
 * Some utility functions (for lack of better place yet)
 */

STRPTR
g_strconcat(CONST_STRPTR string1, ...)
{
    const char *s;
    STRPTR buf;
    int len;
    va_list args;

    len = strlen(string1);
    va_start(args, string1);
    while ((s = va_arg(args, CONST_STRPTR)) != NULL)
      len += strlen(s);
    va_end(args);

    buf = (STRPTR)g_malloc(len+1);
    if (buf == NULL)
      return NULL;

    strcpy(buf, string1);
    va_start(args, string1);
    while ((s = va_arg(args, CONST_STRPTR)) != NULL)
      strcat(buf, s);
    va_end(args);

    return buf;
}

void
g_atexit(void (*f)(void))
{
    if (atexitcount < 15)
      atexitlist[atexitcount++] = f;
    else
      kprintf("ERROR: atexit list full\n");
}

/*** EOF ***/
