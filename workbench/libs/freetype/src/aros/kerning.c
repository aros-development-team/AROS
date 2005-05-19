/*
 * Based on the code from the ft2.library from MorphOS and the ttf.library by
 * Richard Griffith
 */
#include "kerning.h"
#include "glyph.h"

//#define DEBUG 1
#include <aros/debug.h>
#include <exec/memory.h>
#include <diskfont/oterrors.h>
#include <diskfont/glyph.h>

#include <proto/exec.h>
#include <proto/alib.h>

void FreeWidthList(FT_GlyphEngine *ge,struct MinList *list)
{
    struct GlyphWidthEntry *work_node, *next_node;

    //D(bug("FreeWidthList at %lx\n",list));
    if (list == NULL)
	return;

    work_node = (struct GlyphWidthEntry *)(list->mlh_Head);
    while ((next_node =
	   (struct GlyphWidthEntry *)(work_node->gwe_Node.mln_Succ)))
    {
	FreeMem(work_node, sizeof(struct GlyphWidthEntry));
	work_node = next_node;
    }
    FreeMem(list, sizeof(struct MinList));
}


struct MinList *GetWidthList(FT_GlyphEngine *ge)
{
    struct MinList *gw;
    struct GlyphWidthEntry *node;
    ULONG howwide;
    int hold_start, c_start, c_stop, i, j;
    ULONG units_per_em;
    FT_Glyph_Metrics *metrics;

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

    gw = (struct MinList *) AllocMem(sizeof(struct MinList), MEMF_CLEAR);

    if(gw==NULL)
    {
	D(bug("GetWidthList - AllocMem for list failed\n"));
	set_last_error(ge,OTERR_NoMemory);
	return NULL;
    }
    NewList((struct List *)gw);

#if 0
    units_per_em=ge->properties.header->Units_Per_EM;
    int yMin,yMax;

    yMin=ge->properties.header->yMin; if(yMin>0) yMin = 0-yMin;
    yMax=ge->properties.header->yMax;
    units_per_em=yMax-yMin;
#endif

    units_per_em = ge->corrected_upem;

    hold_start=ge->request_char;
    c_start=ge->request_char;
    c_stop=ge->request_char2;
    if(c_start>c_stop)
    {
	/* just in case */
	c_start=ge->request_char2;
	c_stop=ge->request_char;
    }
    D(bug("GetWidthList - from %ld to %ld\n",c_start,c_stop));

    for(i=c_start;i<=c_stop;i++)
    {
	ge->request_char=i;
	j=UnicodeToGlyphIndex(ge);

	if(j==0)
	{
	    D(bug("GetWidthList - no glyph %ld i=%ld p=%ld e=%ld\n"
		  ,i, ge->cmap_index,ge->platform, ge->encoding));
	    continue;
	}

	if(FT_Load_Glyph(ge->face,j,FT_LOAD_DEFAULT))
	{
	    D(bug("Error loading glyph %ld.\n",(LONG)j));
	    howwide=0;
	}
	else
	{
	    //FT_Get_Glyph_Metrics( ge->glyph, &ge->metrics );
	    metrics = &ge->face->glyph->metrics;

    	#if 0	    
	    howwide=((metrics->horiAdvance)<<16)/units_per_em;
	#else
    	    howwide = (metrics->horiAdvance << 10) /
	    	      (( ge->face->size->metrics.x_ppem * units_per_em) / ge->face->units_per_EM);
	#endif
	}

	node = AllocMem(sizeof(struct GlyphWidthEntry), MEMF_CLEAR);
	if(node==NULL)
	{
	    D(bug("GetWidthList - AllocMem for %ld failed\n",i));
	    FreeWidthList(ge,gw);
	    set_last_error(ge,OTERR_NoMemory);
	    ge->request_char=hold_start;
	    return NULL;
	}

	node->gwe_Code = i;	/* amiga char */
	/* character advance, as fraction of em width */
	node->gwe_Width = howwide;

	//		D(bug("GetWidthList - glyph %ld width %lx\n",
	//			node->gwe_Code,	node->gwe_Width	));

	AddTail( (struct List *)gw, (struct Node *)node );
    }

    ge->request_char=hold_start;
    return gw;
}


int get_kerning_dir(FT_GlyphEngine *ge)
{
    //FT_Kern_Subtable *k;
    //FT_Kern_0_Pair   *p;	/* a table of nPairs 'pairs' */
    
    //int i,kern_value;
    FT_Vector kerning;
    FT_UShort l,r;		/* left and right indexes */

    /* instance change may be irrelevant, but just in case */
    if (ge->instance_changed)
    {
	/* reset everything first */
	if(SetInstance(ge)!=OTERR_Success)
	    return NULL;
    }

    /* get left and right glyph indexes */
    l=char_to_glyph(ge,ge->request_char);
    r=char_to_glyph(ge,ge->request_char2);
    if(l==0 || r==0) return(0);	/* no chance */

    if (FT_Get_Kerning(ge->face, l, r, ft_kerning_unscaled, &kerning))
	return 0;

    return (-kerning.x << 16) / ge->corrected_upem;
}
