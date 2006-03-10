/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: AllocBitMap.c,v 1.5 2005/06/24 12:47:14 gnikl Exp $

***************************************************************************/

#include <exec/memory.h>
#include <proto/graphics.h>
#include <proto/exec.h>

#include "SDI_compiler.h"
#include "Debug.h"

#if 0
#define USE_OS3 (1)
#else
#define USE_OS3 (((struct Library *)GfxBase)->lib_Version >= 39)
#endif

struct BitMap * SAVEDS ASM MUIG_AllocBitMap(REG(d0, LONG width), REG(d1, LONG height), REG(d2, LONG depth), REG(d3, LONG flags), REG(a0, struct BitMap *friend))
{
  ENTER();

  if(USE_OS3)
  {
    if(friend != NULL)
    {
    #ifndef __AROS__
      BOOL CyberGFX = FindSemaphore("cybergraphics.library") ? TRUE : FALSE;
    #else
      BOOL CyberGFX = TRUE;
    #endif
      if(CyberGFX
         && (GetBitMapAttr(friend,BMA_FLAGS) & BMF_INTERLEAVED) == 0)
        flags |= BMF_MINPLANES;
      else
        friend = NULL;
    }

    LEAVE();
    return AllocBitMap(width,height,depth,flags,friend);
  }
  else
  {
    struct BitMap *bm = AllocMem(sizeof(struct BitMap), MEMF_CLEAR);

    if(bm != NULL)
    {
      int i, plsize=RASSIZE(width,height);

      InitBitMap(bm,depth,width,height);

      if((bm->Planes[0] = AllocVec(plsize*depth,(flags & BMF_CLEAR) ? MEMF_CHIP|MEMF_CLEAR : MEMF_CHIP))) // !!!
      {
        for(i=1;i<depth;i++)
          bm->Planes[i] = (PLANEPTR)(((ULONG)bm->Planes[i-1]) + plsize);

        LEAVE();
        return bm;
      }
      else
      {
        FreeMem(bm,sizeof(struct BitMap));
      }
    }

    RETURN(NULL);
    return NULL;
  }
}


VOID SAVEDS ASM MUIG_FreeBitMap(REG(a0, struct BitMap *bm))
{
  ENTER();

  WaitBlit();

  if(USE_OS3)
  {
    FreeBitMap(bm);
  }
  else
  {
    FreeVec(bm->Planes[0]);
    FreeMem(bm,sizeof(struct BitMap));
  }

  LEAVE();
}
