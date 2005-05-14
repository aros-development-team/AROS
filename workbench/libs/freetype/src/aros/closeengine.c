/*
 * Based on the code from the ft2.library from MorphOS, the ttf.library by
 * Richard Griffith and the type1.library by Amish S. Dave
 */

#include <aros/libcall.h>
#include <diskfont/glyph.h>

#include "ftglyphengine.h"

//#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

AROS_LH1(void, CloseEngine,
	 AROS_LHA(struct GlyphEngine *, ge, A0),
	 LIBBASETYPEPTR, LIBBASE, 6, FreeType2
)
{
    AROS_LIBFUNC_INIT

    D(bug("LIB_CloseEngine engine 0x%lx\n", ge));

    FreeGE((FT_GlyphEngine *)ge);

    return;
    
    AROS_LIBFUNC_EXIT
}
