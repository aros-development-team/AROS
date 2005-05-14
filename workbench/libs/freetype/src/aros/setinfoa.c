/*
 * Based on the code from the ft2.library from MorphOS, the ttf.library by
 * Richard Griffith and the type1.library by Amish S. Dave
 */
#include "ftglyphengine.h"
#include "glyph.h"

#include <aros/libcall.h>
#include <proto/utility.h>
#include <aros/debug.h>
#include <utility/tagitem.h>
#include <diskfont/oterrors.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>

#include LC_LIBDEFS_FILE

/* scantags, from SetInfoA, looking for the ones we care about
   just ignore anything we don't recognize for now              */

static void scantags(FT_GlyphEngine *ge, struct TagItem *tags)
{
    Tag   otagtag;
    ULONG otagdata;
    struct TagItem *tstate;
    struct TagItem *tag;
    int temp, temp2;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
	otagtag  = tag->ti_Tag;
	otagdata = tag->ti_Data;

	switch (otagtag)
	{
	    /* OT_Spec1 = the filename of the ft file */
	case OT_Spec1_FontFile:
	    D(bug("SetInfo: Tag=OT_Spec1 ft file=%ls\n",otagdata));
	    /*			if(OpenFace(ge,(char *)otagdata)) return; */
	    strcpy(ge->base_filename,(char *)otagdata);
	    /* clear all other names */
	    ge->afm_filename[0] = '\0';
	    break;

	case OT_Spec2_DefCodePage:	/* direct = use default */
	    D(bug("SetInfo: Tag=OT_Spec2 Default Code Page\n"));
	    set_default_codepage(ge);
	    break;

	case OT_Spec2_CodePage:
	    D(bug("SetInfo: Tag=OT_Spec2 Custom Code Page\n"));
	    CopyMem((char *)otagdata,ge->codepage,sizeof(ge->codepage));
	    break;

	case OT_Spec3_AFMFile:
	    strcpy(ge->afm_filename,(char *)otagdata);
	    break;

	    /* OT_Spec4 = metric source */
	case OT_Spec4_Metric:
	    D(bug("SetInfo: Tag=OT_Spec4 Metric Source=%lx\n",otagdata));
	    temp=(ULONG)otagdata;
	    if(ge->metric_source!=temp)
	    {
		ge->metric_source = temp;
		ge->instance_changed=TRUE;
	    }
	    break;

	    /* OT_Spec5 = custom metrics */
	case OT_Spec5_BBox:
	    D(bug("SetInfo: Tag=OT_Spec5 Custom Metrics=%lx\n",otagdata));
	    temp=(ULONG)otagdata;
	    if(ge->metric_custom!=temp)
	    {
		ge->metric_custom = temp;
		ge->instance_changed=TRUE;
	    }
	    break;

	case OT_Spec6_FaceNum:
	    D(bug("SetInfo: Tag=OT_Spec6_FaceNum\n"));
	    ge->face_num = otagdata;
	    break;

	case OT_Spec7_BMSize:
	    break;

	case OT_Spec9_Hinter:
	    break;

	case OT_SymbolSet: /* index to charmap encodings */
	    D(bug("SetInfo: Tag=OT_SymbolSet cmap=%lx\n",otagdata));
	    ge->requested_cmap = (ULONG)otagdata;
	    ge->cmap_index=NO_CMAP_SET;
	    break;

	    /* some programs specify point and DPI with every glyph
	     * request, so check if they really changed before setting
	     * instance_changed. (Note: OpenFace ALWAYS sets it.) */
	case OT_PointHeight:
	    D(bug("SetInfo: Tag=OT_PointHeight size=%lx\n",otagdata));
	    temp = ((ULONG)otagdata) >> 16;
	    if(ge->point_size != temp)
	    {
		ge->point_size = temp;
		ge->instance_changed=TRUE;
	    }
	    break;

	case OT_DeviceDPI:
	    temp = ((ULONG)otagdata) >> 16;
	    temp2 = ((ULONG)otagdata) & 0xffff;
	    D(bug("SetInfo: Tag=OT_DeviceDPI xres=%ld yres=%ld\n",temp,temp2));
	    if((temp!=ge->xres) || (temp2!=ge->yres))
	    {
		ge->xres = temp;
		ge->yres = temp2;
		ge->instance_changed=TRUE;
	    }
	    break;

	case OT_RotateSin:	/* by spec set SIN then COS */
	    D(bug("SetInfo: Tag=OT_RotateSin sin=%lx\n",otagdata));
	    ge->hold_sin = ((ULONG)otagdata);
	    break;

	case OT_RotateCos:	/* process both SIN and COS */
	    D(bug("SetInfo: Tag=OT_RotateCos cos=%lx\n",otagdata));
	    ge->hold_cos = ((ULONG)otagdata);
	    /* sin=0.0 cos=1.0 is no rotation */
	    if(ge->hold_sin==0 && ge->hold_cos==0x10000)
		ge->do_rotate=0;
	    else
	    {
		/* argh! counter clockwise rotation
		 * swapped .xy and .yx to match bullet.
		 * hmmm, shear is clockwise...arrggghhh! */
		ge->do_rotate=1;
		ge->rotate_matrix.xx = ge->hold_cos;
		ge->rotate_matrix.xy = -ge->hold_sin;
		ge->rotate_matrix.yx = ge->hold_sin;
		ge->rotate_matrix.yy = ge->hold_cos;
	    }
	    set_transform(ge);
	    break;

	case OT_ShearSin:	/* by spec set SIN then COS */
	    D(bug("SetInfo: Tag=OT_ShearSin sin=%lx\n",otagdata));
	    ge->hold_sin = ((ULONG)otagdata);
	    break;

	case OT_ShearCos:
	    D(bug("SetInfo: Tag=OT_ShearCos cos=%lx\n",otagdata));
	    ge->hold_cos = ((ULONG)otagdata);
	    /* sin=0.0 cos=1.0 is no shear */
	    if(ge->hold_sin==0 && ge->hold_cos==0x10000)
	    {
		ge->do_shear=0;
		//					ge->italic_sig=0;
	    }
	    else
	    {
		ge->do_shear=1;
		//					ge->italic_sig=1;
		if(ge->hold_cos)
		    temp=FT_MulDiv(ge->hold_sin,
				   0x10000,ge->hold_cos);
		else	temp=0;
		ge->shear_matrix.xx = 1<<16;
		ge->shear_matrix.xy = temp;
		ge->shear_matrix.yx = 0;
		ge->shear_matrix.yy = 1<<16;
	    }
	    set_transform(ge);
	    break;

#if 0
	case OT_EmboldenX:
	    D(bug("SetInfo: Tag=OT_EmboldenX data=%lx\n",otagdata));
	    ge->do_embold=0;
	    ge->bold_sig=0;
	    ge->embold_x = (ULONG)otagdata;
	    if(ge->embold_x || ge->embold_y)
	    {
		ge->do_embold=1;
		ge->bold_sig=2;
	    }
	    break;

	case OT_EmboldenY:
	    D(bug("SetInfo: Tag=OT_EmboldenY data=%lx\n",otagdata));
	    ge->do_embold=0;
	    ge->bold_sig=0;
	    ge->embold_y = (ULONG)otagdata;
	    if(ge->embold_x || ge->embold_y)
	    {
		ge->do_embold=1;
		ge->bold_sig=2;
	    }
	    break;
#endif
#if 0
	    Diskfont Bold signature
	    <Set: Tag=8000000E  Data=E75
	    <Set: Tag=8000000F  Data=99E
		
	    Diskfont Italic signature
	    <Set: Tag=8000000A  Data=4690
	    <Set: Tag=8000000B  Data=F615
#endif

	case OT_GlyphCode:
	    D(bug("SetInfo: Tag=OT_GlyphCode data=%lx\n",otagdata));
	    ge->request_char = (ULONG)otagdata;
	    break;

	case OT_GlyphCode2:
	    D(bug("SetInfo: Tag=OT_GlyphCode2 data=%lx\n",otagdata));
	    ge->request_char2 = (ULONG)otagdata;
	    break;

	case OT_OTagList: /* a tag of tags? */
	    D(bug("SetInfo: Tag=OT_OTagList data=%lx\n",otagdata));
	    if (otagdata)
		scantags(ge, (struct TagItem *)otagdata);
	    break;

	default:
	    D(bug("Ignored Set: Tag=%lx  Data=%lx\n",(LONG)otagtag, otagdata));
	    break;
	}
    }
}

/* SetInfoA, store request into our engine structure.         */
AROS_LH2(ULONG, SetInfoA,
	 AROS_LHA(struct GlyphEngine *, ge, A0),
	 AROS_LHA(struct TagItem *, tags, A1),
	 LIBBASETYPEPTR, LIBBASE, 7, FreeType2
)
{
    AROS_LIBFUNC_INIT

    FT_GlyphEngine *engine = (FT_GlyphEngine *)ge
    D(bug("LIB_SetInfoA libbase = 0x%lx engine = 0x%lx tags = 0x%lx\n",FTBase,engine,tags));

    set_last_error(engine,OTERR_Success);
    scantags(engine, tags);
    
    return (ULONG)(engine->last_error);
    
    AROS_LIBFUNC_EXIT
}
