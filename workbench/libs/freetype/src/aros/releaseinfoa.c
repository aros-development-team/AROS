/*
 * Based on the code from the ft2.library from MorphOS, the ttf.library by
 * Richard Griffith and the type1.library by Amish S. Dave
 */
#include "ftglyphengine.h"
#include "kerning.h"

#include <proto/utility.h>
#include <aros/debug.h>
#include <utility/tagitem.h>
#include <diskfont/glyph.h>
#include <diskfont/diskfonttag.h>

/* ReleaseInfoA, release allocated memory from ObtainInfo */
ULONG ReleaseInfoA(struct GlyphEngine *ge, struct TagItem *tags)
{
    FT_GlyphEngine *engine = (FT_GlyphEngine *)ge;
    Tag   otagtag;
    ULONG otagdata;

    struct TagItem *tstate;
    struct TagItem *tag;
    struct GlyphMap *GMap;

    D(bug("ReleaseInfoA engine = 0x%lx tags = 0x%lx\n",engine,tags));

    tstate = tags;
    while (tag = NextTagItem(&tstate))
    {
	otagtag  = tag->ti_Tag;
	otagdata = tag->ti_Data;

	switch (otagtag)
	{
	case OT_GlyphMap:
	case OT_GlyphMap8Bits:
	case OT_GlyphMap8Bit_Old:
	    //D(bug("Release: OT_GlyphMap  Data=%lx\n", otagdata));
	    GMap=(struct GlyphMap *)otagdata;
	    if(GMap->glm_BitMap) FreeVec(GMap->glm_BitMap);
	    FreeVec(GMap);
	    break;

	case OT_WidthList:
	    //D(bug("Release: OT_WidthList  Data=%lx\n", otagdata));
	    FreeWidthList(engine,(struct MinList  *)otagdata);
	    break;

	default: /* ??? */
	    D(bug("Unknown Release: Tag=%lx  Data=%lx\n",(LONG)otagtag, otagdata));
	    break;
	}
    }
    
    return 0;
}
