/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/layers.h>

#include <proto/datatypes.h>

#include "compilerspecific.h"
#include "debug.h"
#include "pictureclass.h"
#include "colorhandling.h"

unsigned int *MakeARGB(unsigned long *ColRegs, unsigned int Count)
{
 unsigned int *ARGB;
 register unsigned int i;

 ARGB=NULL;

 if(!(ColRegs && Count))
 {
  return(NULL);
 }

 ARGB=AllocVec(Count*sizeof(unsigned int), MEMF_ANY | MEMF_CLEAR);
 if(!ARGB)
 {
  return(NULL);
 }

 for(i=0; i<Count; i++)
 {
  ARGB[i]  = ((*(ColRegs++)) & 0xFF000000) >>  8;
  ARGB[i] |= ((*(ColRegs++)) & 0xFF000000) >> 16;
  ARGB[i] |= ((*(ColRegs++)) & 0xFF000000) >> 24;
 }

 return(ARGB);
}

unsigned int CountColors(unsigned int *ARGB, unsigned int Count)
{
 unsigned int NumColors;
 register unsigned int i, j;

 NumColors=0;

 if(!(ARGB && Count))
 {
  return(0);
 }

 for(i=0; i<Count; i++)
 {
  /*
   *  We assume that it is a new color.
   */

  NumColors++;

  for(j=0; j<i; j++)
  {
   if(ARGB[j]==ARGB[i])
   {
    /*
     *  Oops, it isn't a new color.
     */

    NumColors--;

    break;
   }
  }
 }

 return(NumColors);
}

int HistSort(const void *HistEntry1, const void *HistEntry2)
{
 struct HistEntry *HE1, *HE2;

 HE1=(struct HistEntry *) HistEntry1;
 HE2=(struct HistEntry *) HistEntry2;

 return((int) (HE2->Count - HE1->Count));
}
