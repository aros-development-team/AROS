/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Code for miscellaneous operations needed bz graphics
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <graphics/text.h>
#include "graphics_intern.h"


ULONG CalcHashIndex(ULONG n)
{
  UBYTE Index = (n        & 0xff) +
               ((n >>  8) & 0xff) +
               ((n >> 16) & 0xff) +
               ((n >> 24) & 0xff);
  Index &=0x07;
  return Index; 
}


/* Functions for solving the TextFontExtensionProblem using hashes */

#define GFBI(x) ((struct GfxBase_intern *)x)

static ULONG tfe_calchashidx(APTR ptr)
{
    ULONG val = (ULONG)ptr;

    ULONG idx =  (val	     & 0xff) +
    		((val >> 8)  & 0xff) +
    		((val >> 16) & 0xff) +
    		((val >> 24) & 0xff); 

    idx &= (TFE_HASHTABSIZE - 1);
    
    return idx;
}

struct TextFontExtension *tfe_hashlookup(struct TextFont *tf, struct GfxBase *GfxBase)
{
   
    ULONG idx = tfe_calchashidx(tf);
    struct tfe_hashnode *n;

    ObtainSemaphoreShared( &GFBI(GfxBase)->tfe_hashtab_sema );
    
    for ( n = GFBI(GfxBase)->tfe_hashtab[idx]; n; n = n->next)
    {
        if (n->back == tf)
	    return n->ext;
    }

    ReleaseSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
    
    return NULL;
}

BOOL tfe_hashadd(struct TextFontExtension * etf
		, struct TextFont *tf
		, struct GfxBase *GfxBase)

{
    ULONG idx = tfe_calchashidx(tf);
    struct tfe_hashnode *n;
    
    n = AllocMem( sizeof (struct tfe_hashnode), MEMF_ANY);
    if (n)
    {
    	n->back = tf;
	n->ext  = etf;
	
	ObtainSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
	
	n->next = GFBI(GfxBase)->tfe_hashtab[idx];
	GFBI(GfxBase)->tfe_hashtab[idx] = n;
	
	ReleaseSemaphore( &GFBI(GfxBase)->tfe_hashtab_sema );
	
	return TRUE;

    }
    return FALSE;
}

VOID tfe_hashdelete(struct TextFont *tf, struct GfxBase *GfxBase)
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
