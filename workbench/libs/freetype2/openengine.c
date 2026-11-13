/*
 * Based on the code from the ft2.library from MorphOS, the ttf.library by
 * Richard Griffith and the type1.library by Amish S. Dave
 */
#include "ftglyphengine.h"

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include LC_LIBDEFS_FILE

static const UBYTE *EngineName = "freetype2";

AROS_LH0(struct GlyphEngine *, OpenEngine,
         LIBBASETYPEPTR, LIBBASE, 5, FreeType2
        )
{
    AROS_LIBFUNC_INIT

    FT_GlyphEngine *ge;

    D(bug("OpenEngine libbase = 0x%lx\n", LIBBASE));

    if ((ge = AllocGE()) != NULL) {
        ge->gle_Library = (struct Library *)LIBBASE;
        ge->gle_Name = (UBYTE *)EngineName;
    }

    D(bug(" returning 0x%p\n", ge));

    return (struct GlyphEngine *)ge;

    AROS_LIBFUNC_EXIT
}

