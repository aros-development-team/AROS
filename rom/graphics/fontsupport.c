/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Misc font help funcs
    Lang: english
*/

#include <proto/exec.h>

#include <exec/memory.h>
#include <graphics/text.h>

#include "graphics_intern.h"
#include "fontsupport.h"

ULONG CalcHashIndex(ULONG n)
{
  UBYTE Index = (n        & 0xff) +
               ((n >>  8) & 0xff) +
               ((n >> 16) & 0xff) +
               ((n >> 24) & 0xff);
  Index &=0x07;
  return Index; 
}


/* Functions for solving the TextFontExtension problem using hashes */

#define GFBI(x) ((struct GfxBase_intern *)x)

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



struct tfe_hashnode *tfe_hashnode_create(struct GfxBase *GfxBase)
{
    struct tfe_hashnode *n;
    
    n = AllocMem( sizeof (struct tfe_hashnode), MEMF_ANY|MEMF_CLEAR);
    return n;
}

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


