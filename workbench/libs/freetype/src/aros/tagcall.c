#define NO_INLINE_STDARG
#include <proto/freetype2.h>
#include <diskfont/glyph.h>

ULONG SetInfo (struct GlyphEngine *glyphEngine, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = SetInfoA(glyphEngine, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
}

ULONG ReleaseInfo (struct GlyphEngine *glyphEngine, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = ReleaseInfoA(glyphEngine, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
}

ULONG ObtainInfo (struct GlyphEngine *glyphEngine, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
	
    retval = ObtainInfoA(glyphEngine, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
}
