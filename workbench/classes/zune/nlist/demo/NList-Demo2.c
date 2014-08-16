/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

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

#if defined(__AROS__)
#include <clib/alib_protos.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <graphics/gfxmacros.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/asl.h>

extern struct Library *MUIMasterBase;

#include <mui/NListview_mcc.h>
#include <mui/NFloattext_mcc.h>

#include "NList-Demo2.h"

#include <proto/muimaster.h>

#include "SDI_hook.h"

/* *********************************************** */

struct LITD {
  LONG num;
  char str1[4];
  char *str2;
};

/* *********************************************** */

HOOKPROTONHNO(ConstructLI_TextFunc, APTR, struct NList_ConstructMessage *ncm)
{
  struct LITD *new_entry = (struct LITD *) AllocVec(sizeof(struct LITD),0);

  if (new_entry)
  {
    int i = 0, j = 0;
    new_entry->num = -1;
    new_entry->str2 = (char *) ncm->entry;
    while ((j < 3) && new_entry->str2[i])
    {
      if ((new_entry->str2[i] > 'A') && (new_entry->str2[i] < 'z'))
        new_entry->str1[j++] = new_entry->str2[i];
      if (new_entry->str2[i] == '\033')
        i++;
      i++;
    }
    new_entry->str1[j] = '\0';

    return (new_entry);
  }
  return (NULL);
}
MakeHook(ConstructLI_TextHook, ConstructLI_TextFunc);

/* *********************************************** */

HOOKPROTONHNO(DestructLI_TextFunc, void, struct NList_DestructMessage *ndm)
{
  if (ndm->entry)
    FreeVec((void *) ndm->entry);
}
MakeHook(DestructLI_TextHook, DestructLI_TextFunc);

/* *********************************************** */

static char buf[20];

HOOKPROTONHNO(DisplayLI_TextFunc, void, struct NList_DisplayMessage *ndm)
{
  struct LITD *entry = (struct LITD *) ndm->entry;

  if (entry)
  { if (entry->num < 0)
      entry->num = ndm->entry_pos;

    ndm->preparses[0]  = (STRPTR)"\033c";
    ndm->preparses[1]  = (STRPTR)"\033c";

    if      (entry->num % 20 == 3)
      ndm->strings[0] = (STRPTR)"\033o[0]";
    else if (entry->num % 20 == 13)
      ndm->strings[0] = (STRPTR)"\033o[1]";
    else
    {
      snprintf(buf, sizeof(buf), "%d", (unsigned int)entry->num);
      ndm->strings[0]  = buf;
    }

    ndm->strings[1]  = (char *) entry->str1;
    ndm->strings[2]  = (char *) entry->str2;
  }
  else
  {
    ndm->preparses[0] = (STRPTR)"\033c";
    ndm->preparses[1] = (STRPTR)"\033c";
    ndm->preparses[2] = (STRPTR)"\033c";
    ndm->strings[0] = (STRPTR)"Num";
    ndm->strings[1] = (STRPTR)"Short";
    ndm->strings[2] = (STRPTR)"This is the list title !\033n\033b   :-)";
  }
}
MakeHook(DisplayLI_TextHook, DisplayLI_TextFunc);


/* *********************************************** */

HOOKPROTONHNO(CompareLI_TextFunc, LONG, struct NList_CompareMessage *ncm)
{
  struct LITD *entry1 = (struct LITD *) ncm->entry1;
  struct LITD *entry2 = (struct LITD *) ncm->entry2;
  LONG col1 = ncm->sort_type & MUIV_NList_TitleMark_ColMask;
  LONG col2 = ncm->sort_type2 & MUIV_NList_TitleMark2_ColMask;
  LONG result = 0;

/*
  LONG st = ncm->sort_type & MUIV_NList_TitleMark_TypeMask;
kprintf("%lx|Compare() %lx / %lx / %lx\n",obj,ncm->sort_type,st,ncm->sort_type2);
*/

  if(ncm->sort_type == (LONG)MUIV_NList_SortType_None)
    return (0);

  if      (col1 == 0)
  { if (ncm->sort_type & MUIV_NList_TitleMark_TypeMask)
      result = entry2->num - entry1->num;
    else
      result = entry1->num - entry2->num;
  }
  else if (col1 == 1)
  { if (ncm->sort_type & MUIV_NList_TitleMark_TypeMask)
      result = (LONG) stricmp(entry2->str1,entry1->str1);
    else
      result = (LONG) stricmp(entry1->str1,entry2->str1);
  }
  else if (col1 == 2)
  { if (ncm->sort_type & MUIV_NList_TitleMark_TypeMask)
      result = (LONG) stricmp(entry2->str2,entry1->str2);
    else
      result = (LONG) stricmp(entry1->str2,entry2->str2);
  }

  if ((result != 0) || (col1 == col2))
    return (result);

  if      (col2 == 0)
  { if (ncm->sort_type2 & MUIV_NList_TitleMark2_TypeMask)
      result = entry2->num - entry1->num;
    else
      result = entry1->num - entry2->num;
  }
  else if (col2 == 1)
  { if (ncm->sort_type2 & MUIV_NList_TitleMark2_TypeMask)
      result = (LONG) stricmp(entry2->str1,entry1->str1);
    else
      result = (LONG) stricmp(entry1->str1,entry2->str1);
  }
  else if (col2 == 2)
  { if (ncm->sort_type2 & MUIV_NList_TitleMark2_TypeMask)
      result = (LONG) stricmp(entry2->str2,entry1->str2);
    else
      result = (LONG) stricmp(entry1->str2,entry2->str2);
  }

  return (result);
}
MakeHook(CompareLI_TextHook, CompareLI_TextFunc);

