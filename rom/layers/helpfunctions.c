/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <graphics/clip.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>


struct ClipRect * GetClipRectStruct(struct Layer * L)
{
  struct ClipRect * CR;
  if (NULL == (CR = L->SuperSaveClipRects))
    return AllocMem(sizeof(struct ClipRect) ,MEMF_PUBLIC|MEMF_CLEAR);
  else
  {
    L->SuperSaveClipRects = CR->Next;
    CR->lobs = NULL;
    CR->BitMap = NULL;
  }
  return CR;  
}

struct Layer * internal_WhichLayer(struct Layer * l, WORD x, WORD y)
{
  for( ; l != NULL; l = l->back)
    if(x >= l->bounds.MinX && x <= l->bounds.MaxX &&
       y >= l->bounds.MinY && y <= l->bounds.MaxY)
	     return l;

  return NULL;
}
