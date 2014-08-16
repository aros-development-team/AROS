/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include "SDI_compiler.h"

#include "Debug.h"

// MoveList()
// append all nodes from one list to another

void MoveList(struct List *to, struct List *from)
{
  ENTER();

  if(to != NULL && from != NULL && IsListEmpty(from) == FALSE)
  {
    // connect tail node of "to" to the head node of "from"
    to->lh_TailPred->ln_Succ = from->lh_Head;
    to->lh_TailPred->ln_Succ->ln_Pred = to->lh_TailPred;

    // connect tail node of "from" to "to" as tail
    to->lh_TailPred = from->lh_TailPred;
    to->lh_TailPred->ln_Succ = (struct Node *)&to->lh_Tail;

    // clean "from" list
    NewList(from);
  }

  LEAVE();
}
