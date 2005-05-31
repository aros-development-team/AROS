/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Misc font help funcs
    Lang: english
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/graphics.h>

#include <exec/memory.h>
#include <graphics/text.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "fontsupport.h"

/****************************************************************************************/

ULONG CalcHashIndex(ULONG n)
{
    UBYTE Index = (n        & 0xff) +
        	 ((n >>  8) & 0xff) +
        	 ((n >> 16) & 0xff) +
        	 ((n >> 24) & 0xff);
    Index &= 0x07;
    
    return Index; 
}
/****************************************************************************************/

/* Functions for solving the TextFontExtension problem using hashes */

#define GFBI(x) ((struct GfxBase_intern *)x)

/****************************************************************************************/

static inline ULONG tfe_calchashidx(APTR ptr)
{
    ULONG val = (ULONG)ptr;

    ULONG idx =  (val	     & 0xff) +
    		((val >> 8)  & 0xff) +
    		((val >> 16) & 0xff) +
    		((val >> 24) & 0xff); 

    idx &= (TFE_HASHTABSIZE - 1);
    
    return idx;
}

/****************************************************************************************/

struct tfe_hashnode *tfe_hashlookup(struct TextFont *tf, struct GfxBase *GfxBase)
{
   
    ULONG idx = tfe_calchashidx(tf);
    struct tfe_hashnode *n = NULL;
    

    ObtainSemaphoreShared( &GFBI(GfxBase)->tfe_hashtab_sema );
    
    for ( n = GFBI(GfxBase)->tfe_hashtab[idx]; n; n = n->next)
    {
        if (n->back == tf)
	{
	    break;
	}
    }

    ReleaseSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
    
    return n;
}

/****************************************************************************************/

struct tfe_hashnode *tfe_hashnode_create(struct GfxBase *GfxBase)
{
    struct tfe_hashnode *n;
    
    n = AllocMem( sizeof (struct tfe_hashnode), MEMF_ANY|MEMF_CLEAR);
    
    return n;
}

/****************************************************************************************/

void tfe_hashadd(struct tfe_hashnode *hn
		, struct TextFont *tf
		, struct TextFontExtension 	*etf
		, struct GfxBase *GfxBase)

{
    ULONG idx = tfe_calchashidx(tf);
    
    hn->back = tf;
    hn->ext  = etf;
	
    ObtainSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
	
    hn->next = GFBI(GfxBase)->tfe_hashtab[idx];
    GFBI(GfxBase)->tfe_hashtab[idx] = hn;
	
    ReleaseSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
	
    return;
}

/****************************************************************************************/

void tfe_hashdelete(struct TextFont *tf, struct GfxBase *GfxBase)
{
    ULONG idx = tfe_calchashidx(tf);
    struct tfe_hashnode *n, *last = NULL;
    
    ObtainSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
    
    for (n = GFBI(GfxBase)->tfe_hashtab[idx]; n; n = n->next)
    {
        if (n->back == tf)
	{
	    if (last)
	    	last->next = n->next;
	    else
	    	GFBI(GfxBase)->tfe_hashtab[idx] = n->next;
		
	    FreeMem(n, sizeof (struct tfe_hashnode));
	    break;
	}
	last = n;
    }

    ReleaseSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
    
    return;
    
}

/****************************************************************************************/

OOP_Object *fontbm_to_hiddbm(struct TextFont *font, struct GfxBase *GfxBase)
{
    ULONG width, height;
    OOP_Object *bm_obj;
    OOP_Object *tmp_gc;
    
    /* Caclulate sizes for the font bitmap */
    struct TagItem bm_tags[] =
    {
	{ aHidd_BitMap_Width	, 0	    	    	},
	{ aHidd_BitMap_Height	, 0	    	    	},
	{ aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Plane	},
	{ TAG_DONE  	    	    	    	    	}
    };
    
    tmp_gc = obtain_cache_object(SDD(GfxBase)->gc_cache, GfxBase);
    if (NULL == tmp_gc)
    	return NULL;

    width  = font->tf_Modulo * 8;
    height = font->tf_YSize;
    
    bm_tags[0].ti_Data = width;
    bm_tags[1].ti_Data = height;
	    
    #warning Handle color textfonts
    
    bm_obj = HIDD_Gfx_NewBitMap(SDD(GfxBase)->gfxhidd, bm_tags);
    if (NULL != bm_obj)
    {
	struct TagItem gc_tags[] =
	{
	    { aHidd_GC_DrawMode     	    , vHidd_GC_DrawMode_Copy	},
	    { aHidd_GC_Foreground   	    , 1	    	    	    	},
	    { aHidd_GC_Background   	    , 0 	    	    	},
	    { aHidd_GC_ColorExpansionMode   , vHidd_GC_ColExp_Opaque    },
	    { TAG_DONE	    	    	    	    	    	    	}
	};

	/* Copy the character data into the bitmap */
	OOP_SetAttrs(tmp_gc, gc_tags);
	
	HIDD_BM_PutTemplate(bm_obj, tmp_gc, font->tf_CharData, font->tf_Modulo,
	    	    	    0, 0, 0, width, height, FALSE);

    }
    
    release_cache_object(SDD(GfxBase)->gc_cache, tmp_gc, GfxBase);
    
    return bm_obj;
}

/****************************************************************************************/

UBYTE *colorfontbm_to_chunkybuffer(struct TextFont *font, struct GfxBase *GfxBase)
{
    ULONG  width, height;
    UBYTE *chunky;
    
    width  = font->tf_Modulo * 8;
    height = font->tf_YSize;
    
    chunky = AllocVec(width * height, MEMF_CLEAR);
    if (chunky)
    {
    	struct BitMap bm;
	struct RastPort rp;
	UBYTE d, shift = 1, plane = 0;
	
	InitBitMap(&bm, CTF(font)->ctf_Depth, width, height);
	
	for(d = 0; d < CTF(font)->ctf_Depth; d++, shift <<= 1)
	{
	    if (CTF(font)->ctf_PlanePick & shift)
	    {
	    	bm.Planes[d] = (PLANEPTR)(CTF(font)->ctf_CharData[plane++]);
	    }
	    else
	    {
	    	bm.Planes[d] = (CTF(font)->ctf_PlaneOnOff & shift) ? (PLANEPTR)-1 : NULL;
	    }
	}
	
	InitRastPort(&rp);
	rp.BitMap = &bm;
	
	ReadPixelArray8(&rp, 0, 0, width - 1, height - 1, chunky, NULL); 
	DeinitRastPort(&rp);
    }
       
    return chunky;
}

/****************************************************************************************/



