/*
 * Based on the code from the ft2.library from MorphOS, the ttf.library by
 * Richard Griffith and the type1.library by Amish S. Dave
 */

#include <diskfont/glyph.h>

#include "ftglyphengine.h"

//#define DEBUG 1
#include <aros/debug.h>

void CloseEngine(struct GlyphEngine *ge)
{
    D(bug("LIB_CloseEngine engine 0x%lx\n", ge));

    FreeGE((FT_GlyphEngine *)ge);

    return;
}
