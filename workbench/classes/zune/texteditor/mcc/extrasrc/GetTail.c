/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by TextEditor.mcc Open Source Team

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

#include <SDI/SDI_compiler.h>

#include "Debug.h"

// GetTail()
// get the tail element of a list

struct Node *GetTail(struct List *list)
{
  struct Node *result = NULL;

  ENTER();

  if(list != NULL && IsListEmpty(list) == FALSE)
    result = list->lh_TailPred;

  RETURN(result);
  return result;
}
