/*
 * Based on the code from the ft2.library from MorphOS, the ttf.library by
 * Richard Griffith and the type1.library by Amish S. Dave
 */
#include "ftglyphengine.h"
#include "glyph.h"
#include "kerning.h"

#include <proto/utility.h>
#include <aros/debug.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <diskfont/oterrors.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>

#include LC_LIBDEFS_FILE

/* given an Amiga character, return a filled in GlyphMap */
static struct GlyphMap *GetGlyph(FT_GlyphEngine *ge, int glyph_8bits)
{
    if (ge->instance_changed)
    {
	/* reset everything first */
	if(SetInstance(ge)!=OTERR_Success)
	    return NULL;
    }

    /*if(ge->cmap_index==NO_CMAP_SET)
     {
     //if(ChooseEncoding(ge)!=0)
     return(NULL);
     }*/

    UnicodeToGlyphIndex(ge);
    if(ge->glyph_code)
    {	/* has code, try to render */
	/* first, get a GlyphMap structure to fill in */
	ge->GMap=AllocVec((ULONG)sizeof(struct GlyphMap),
			  MEMF_PUBLIC | MEMF_CLEAR);
	if(ge->GMap)
	    RenderGlyph(ge, glyph_8bits);
    }
    else
    {
	set_last_error(ge,OTERR_UnknownGlyph);
	return NULL;
    }

    /* odd ball glyph (includes SPACE, arrrgghhh) mimic bullet */
    if (ge->GMap->glm_Width == 0)
    {
	set_last_error(ge,OTERR_UnknownGlyph);
	FreeVec(ge->GMap);
	ge->GMap = NULL;
	return NULL;
    }

    return ge->GMap;
}

/**
 * ObtainInfoA
 **/
AROS_LH2(ULONG, ObtainInfoA,
	 AROS_LHA(struct GlyphEngine *, ge, A0),
	 AROS_LHA(struct TagItem *, tags, A1),
	 LIBBASETYPEPTR, LIBBASE, 8, FreeType2
)
{
    AROS_LIBFUNC_INIT

    FT_GlyphEngine *engine=(FT_GlyphEngine *)ge;
    struct GlyphMap **gm_p;
    struct MinList **gw_p;
    Tag   otagtag;
    IPTR otagdata;
    struct TagItem *tstate;
    struct TagItem *tag;

    ULONG rc = 0;

    D(bug("ObtainInfoA engine = 0x%lx tags = 0x%lx\n",engine,tags));

    /* establish the correct face */
    switch_family(engine);

    /* can't do anything without a face */
    if (engine->face_established==FALSE)
	return OTERR_Failure;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
	otagtag  = tag->ti_Tag;
	otagdata = tag->ti_Data;

	switch (otagtag)
	{
	case OT_GlyphMap:
	    D(bug("Obtain: OT_GlyphMap  Data=%lx\n", otagdata));

	    gm_p = (struct GlyphMap **)otagdata;

	    if((*gm_p = GetGlyph(engine, FALSE))==NULL)
	    {
		D(bug("Could not obtain GlyphMap\n"));
		rc = (ULONG) engine->last_error;
	    }
	    break;

	case OT_GlyphMap8Bits:
	case OT_GlyphMap8Bit_Old:
	    D(bug("Obtain: OT_GlyphMap8Bit  Data=%lx\n", otagdata));

	    gm_p = (struct GlyphMap **)otagdata;

	    if((*gm_p = GetGlyph(engine, TRUE))==NULL)
            {
                D(bug("Could not obtain GlyphMap8Bit\n"));
		rc = (ULONG) engine->last_error;
            }
	    break;

	case OT_WidthList:
	    D(bug("Obtain: OT_WidthList  Data=%lx\n", otagdata));

	    gw_p = (struct MinList  **)otagdata;

	    if((*gw_p = GetWidthList(engine))==NULL)
		rc = (ULONG) engine->last_error;
	    break;

	case OT_TextKernPair:
	case OT_DesignKernPair:
	    D(bug("Obtain: KernPair Data=%lx\n", otagdata));
	    *((ULONG *)otagdata)=get_kerning_dir(engine);
	    break;

	default:
	    D(bug("Unanswered Obtain: Tag=%lx  Data=%lx\n",(LONG)otagtag, otagdata));
	    rc=OTERR_UnknownTag;
	    break;
	}
    }

    return rc;
    
    AROS_LIBFUNC_EXIT
}
