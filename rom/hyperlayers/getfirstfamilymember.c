/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/tagitem.h>
#include "basicfuncs.h"

/*
 * Get the first member of a layer family. If the layer
 * has no children at all this function returns the
 * pointer to the same layer as given.
 * If the layer has children which again have children etc.
 * this function returns the frontmost child.
 */

struct Layer *GetFirstFamilyMember(struct Layer *l)
{
  struct Layer * lastgood = l, *_l = l->front;
  
  while ((NULL != _l) && (_l->nesting > l->nesting))
  {
    lastgood = _l;
    _l = _l->front;
  }
  return lastgood;
} /* GetFirstFamilyMember */
