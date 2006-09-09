/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>

#include "SDI_compiler.h"

#if defined(__amigaos4__)
extern struct Library *GfxBase;
#else
extern struct GfxBase *GfxBase;
#endif

struct BitMap * SAVEDS ASM MUIG_AllocBitMap(REG(d0, LONG width), REG(d1, LONG height), REG(d2, LONG depth), REG(d3, LONG flags), REG(a0, struct BitMap *friend))
{
  if(((struct Library *)GfxBase)->lib_Version >= 39)
  {
  #ifdef __AROS__
    BOOL CyberGFX = TRUE;
  #else
    BOOL CyberGFX = FindSemaphore("cybergraphics.library") ? TRUE : FALSE;
  #endif
    if(friend && !CyberGFX)
      friend = NULL;

    if(friend && (GetBitMapAttr(friend,BMA_FLAGS) & BMF_INTERLEAVED))
      friend = NULL;

    if(friend)
      flags |= BMF_MINPLANES;

    return AllocBitMap(width,height,depth,flags,friend);
  }
  else
  {
    struct BitMap *bm;
    int plsize = RASSIZE(width,height);
    int i;

    if((bm = AllocMem(sizeof(struct BitMap), MEMF_CLEAR)))
    {
      InitBitMap(bm,depth,width,height);

      if((bm->Planes[0] = AllocVec(plsize*depth,(flags & BMF_CLEAR) ? MEMF_CHIP|MEMF_CLEAR : MEMF_CHIP))) // !!!
      {
        for(i=1;i<depth;i++)
          bm->Planes[i] = (PLANEPTR)(((ULONG)bm->Planes[i-1]) + plsize);

        return (struct BitMap *)bm;
      }
    }

    return NULL;
  }
}


VOID SAVEDS ASM MUIG_FreeBitMap(REG(a0, struct BitMap *bm))
{
  WaitBlit();

  if(((struct Library *)GfxBase)->lib_Version >= 39)
  {
    FreeBitMap(bm);
  }
  else
  {
    FreeVec(bm->Planes[0]);
    FreeMem(bm,sizeof(struct BitMap));
  }
}
