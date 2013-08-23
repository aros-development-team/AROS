/***************************************************************************

 NListtree.mcc - New Listtree MUI Custom Class
 Copyright (C) 1999-2001 by Carsten Scholling
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

/*
** QuickSort algorithm implementation
**
** based on the qsort() implementation of libnix 2.0
** changed and optimized to use a hook instead of a generic
** compare function
**
** Jens Langner <jens.langner@light-speed.de>, 2003
*/

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "NListtree.h"

#define SWAP(a,b)     temp=(a);(a)=(b);(b)=temp
#define COMPARE(a,b)  (LONG)DoMethod(data->Obj, MUIM_NListtree_Compare, (a), (b))

void qsort2(struct MUI_NListtree_TreeNode **table, ULONG entries, struct NListtree_Data *data)
{
  struct MUI_NListtree_TreeNode *temp;
  ULONG a,b,c;

  while(entries > 1)
  {
    a = 0;
    b = entries-1;
    c = (a+b)/2;    // Middle element

    for(;;)
    {
      while(COMPARE(table[c], table[a]) > 0) a++; // Look for one >= middle
      while(COMPARE(table[c], table[b]) < 0) b--; // Look for one <= middle

      if(a>=b) break; // We found no pair

      // swap them
      SWAP(table[a], table[b]);

      if(c==a)      c=b; // Keep track of middle element
      else if(c==b) c=a;

      a++; // These two are already sorted
      b--;
    }

    // a points to first element of right intervall now (b to last of left)
    b++;

    // do recursion on smaller intervall and iteration on larger one
    if(b < entries-b)
    {
      qsort2(table, b, data);
      table = &table[b];
      entries = entries-b;
    }
    else
    {
      qsort2(&table[b], entries-b, data);
      entries = b;
    }
  }
}
