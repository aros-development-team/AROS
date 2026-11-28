/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Locating external images in image sets and themes.
*/

/*
    External images ("3:<name>" and "4:<name>" image specifications) are
    looked up the way MUI does it: an absolute name is used as it is,
    and a relative one is searched for in the image set directories.
    MUI keeps its image sets in

        MUI:Images/<set>/[<style>/]<size>pt/<Name>.<ext>

    Zune searches IMAGES:Zune/ (its equivalent of MUI:Images/) and then
    THEMES:, where every theme provides an image set of its own:

        THEMES:<theme>/[<style>/]Images/<size>pt/<Name>.<ext>

    with the same file naming as MUI:

        .mf0 .mf1   normal and selected image of a framed brush
        .mb0 .mb1   normal and selected image of a brush
        .mbr        single brush
        .mim        normal and selected image, side by side in one file
        .mi1        single image
        .image      BOOPSI image class

    The images of the theme in use are found through the THEMEIMAGES:
    assign, which lists the theme's image set of the selected size
    followed by its other sets. A name there can leave the type of the
    image open:

        THEMEIMAGES:<Name>

    is the image of that name in whichever form the theme provides it.
*/

#include <string.h>
#include <stdio.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/screens.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "mui.h"
#include "support.h"
#include "muimaster_intern.h"
#include "themeimage.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

/* the extensions that identify an external image; this is also the order
   in which they are tried for a name given without one */
static const char *const themeimage_exts[] =
{
    ".mf0", ".mb0", ".mim", ".mi1", ".mbr", ".image", NULL
};

/*
 * Looking for an image must never bring up a "Please insert volume"
 * requester, for instance when no theme is in use. These switch the
 * requesters of the calling process off and back on.
 */
static APTR requesters_off(void)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    APTR old = NULL;

    if (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS)
    {
        old = pr->pr_WindowPtr;
        pr->pr_WindowPtr = (APTR) - 1;
    }
    return old;
}

static void requesters_restore(APTR old)
{
    struct Process *pr = (struct Process *)FindTask(NULL);

    if (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS)
        pr->pr_WindowPtr = old;
}

ULONG zune_themeimage_parse_size(CONST_STRPTR name, ULONG len)
{
    ULONG size = 0;
    ULONG i = 0;

    while ((i < len) && (name[i] >= '0') && (name[i] <= '9'))
    {
        size = (size * 10) + (name[i] - '0');
        if (size > 1024)
            return 0;
        i++;
    }

    if ((i == 0) || (len - i != 2))
        return 0;
    if (((name[i] != 'p') && (name[i] != 'P'))
        || ((name[i + 1] != 't') && (name[i + 1] != 'T')))
        return 0;

    return size;
}

BOOL zune_themeimage_is_image(CONST_STRPTR name)
{
    ULONG len = strlen(name);
    int i;

    for (i = 0; themeimage_exts[i]; i++)
    {
        ULONG extlen = strlen(themeimage_exts[i]);

        if ((len > extlen)
            && !Stricmp(name + len - extlen, themeimage_exts[i]))
            return TRUE;
    }
    return FALSE;
}

static ULONG size_distance(ULONG size, ULONG wanted)
{
    return (size > wanted) ? (size - wanted) : (wanted - size);
}

ULONG zune_themeimage_sizes(CONST_STRPTR imagedir, ULONG wanted,
    ULONG *sizes, ULONG maxsizes)
{
    struct FileInfoBlock *fib;
    ULONG count = 0;
    ULONG i, j;
    BPTR lock;
    APTR oldwin = requesters_off();

    lock = Lock(imagedir, ACCESS_READ);
    requesters_restore(oldwin);
    if (!lock)
        return 0;

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib)
    {
        if (Examine(lock, fib))
        {
            while ((count < maxsizes) && ExNext(lock, fib))
            {
                ULONG size;

                if (fib->fib_DirEntryType <= 0)
                    continue;
                size = zune_themeimage_parse_size(fib->fib_FileName,
                    strlen(fib->fib_FileName));
                if (size)
                    sizes[count++] = size;
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);

    /* closest to the wanted size first; a larger image wins a tie */
    for (i = 1; i < count; i++)
    {
        ULONG cur = sizes[i];

        for (j = i; j > 0; j--)
        {
            ULONG dprev = size_distance(sizes[j - 1], wanted);
            ULONG dcur = size_distance(cur, wanted);

            if ((dprev < dcur) || ((dprev == dcur) && (sizes[j - 1] > cur)))
                break;
            sizes[j] = sizes[j - 1];
        }
        sizes[j] = cur;
    }

    return count;
}

static BOOL file_exists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, ACCESS_READ);

    if (!lock)
        return FALSE;
    UnLock(lock);
    return TRUE;
}

static STRPTR join_path(CONST_STRPTR prefix, CONST_STRPTR name)
{
    ULONG size = strlen(prefix) + strlen(name) + 1;
    STRPTR path = AllocVec(size, MEMF_ANY);

    if (path)
    {
        strcpy(path, prefix);
        strcat(path, name);
    }
    return path;
}

static STRPTR existing_path(CONST_STRPTR prefix, CONST_STRPTR name)
{
    STRPTR path = join_path(prefix, name);

    if (path && !file_exists(path))
    {
        FreeVec(path);
        path = NULL;
    }
    return path;
}

/*
    Locates an image of the theme in use. 'name' is what follows
    THEMEIMAGES: and names the image without its type. The assign takes
    care of finding it in the theme's image sets.
*/
static STRPTR locate_in_theme(CONST_STRPTR name)
{
    char *buf;
    ULONG buflen;
    int e;

    /* only plain image names; one with a type is a file name, and has
       been looked for already */
    if (!*name || strchr(name, '/') || strchr(name, ':')
        || zune_themeimage_is_image(name))
        return NULL;

    buflen = strlen(THEMEIMAGE_PREFIX) + strlen(name) + 8;
    buf = AllocVec(buflen, MEMF_ANY);
    if (!buf)
        return NULL;

    for (e = 0; themeimage_exts[e]; e++)
    {
        snprintf(buf, buflen, "%s%s%s", THEMEIMAGE_PREFIX, name,
            themeimage_exts[e]);
        if (file_exists(buf))
            return buf;
    }
    FreeVec(buf);

    return NULL;
}

static STRPTR locate_image(CONST_STRPTR name, struct Screen *scr)
{
    STRPTR path;
    BPTR lock;

    /* the name of an existing file is used as given */
    lock = Lock(name, ACCESS_READ);
    if (lock)
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        BOOL isfile = TRUE;

        if (fib)
        {
            if (Examine(lock, fib))
                isfile = (fib->fib_DirEntryType < 0);
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(lock);

        if (isfile)
            return StrDup(name);
    }

    if (!strchr(name, ':'))
    {
        /* relative names belong to an image set */
        if ((path = existing_path(IMSPEC_EXTERNAL_PREFIX, name)) != NULL)
            return path;
#ifdef __AROS__
        if ((path = existing_path(THEMEIMAGE_SETS_PREFIX, name)) != NULL)
            return path;
#endif
        return NULL;
    }

#ifdef __AROS__
    if (!Strnicmp(name, THEMEIMAGE_PREFIX, strlen(THEMEIMAGE_PREFIX)))
        return locate_in_theme(name + strlen(THEMEIMAGE_PREFIX));
#endif

    return NULL;
}

STRPTR zune_themeimage_locate(CONST_STRPTR name, struct Screen *scr)
{
    STRPTR path;
    APTR oldwin;

    if (!name || !*name)
        return NULL;

    D(bug("zune_themeimage_locate('%s')\n", name));

    oldwin = requesters_off();
    path = locate_image(name, scr);
    requesters_restore(oldwin);

    return path;
}
