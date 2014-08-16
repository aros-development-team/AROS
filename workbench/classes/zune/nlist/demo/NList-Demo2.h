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

extern struct Hook DisplayLI_TextHook;
extern struct Hook CompareLI_TextHook;
extern struct Hook ConstructLI_TextHook;
extern struct Hook DestructLI_TextHook;

/* MUI STUFF */

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

/* *********************************************** */

#define SimpleButtonCycle(name) \
  TextObject, \
    ButtonFrame, \
    MUIA_CycleChain, 1, \
    MUIA_Font, MUIV_Font_Button, \
    MUIA_Text_Contents, name, \
    MUIA_Text_PreParse, "\33c", \
    MUIA_InputMode    , MUIV_InputMode_RelVerify, \
    MUIA_Background   , MUII_ButtonBack, \
  End

/* *********************************************** */

#define SimpleButtonTiny(name) \
  TextObject, \
    ButtonFrame, \
    MUIA_Font, MUIV_Font_Tiny, \
    MUIA_Text_Contents, name, \
    MUIA_Text_PreParse, "\33c", \
    MUIA_InputMode    , MUIV_InputMode_RelVerify, \
    MUIA_Background   , MUII_ButtonBack, \
  End

/* *********************************************** */

#define NFloattext(ftxt) \
    NListviewObject, \
      MUIA_Weight, 50, \
      MUIA_CycleChain, 1, \
      MUIA_NListview_Horiz_ScrollBar, MUIV_NListview_HSB_None, \
      MUIA_NListview_Vert_ScrollBar, MUIV_NListview_VSB_Always, \
      MUIA_NListview_NList,NFloattextObject, \
        MUIA_NList_DefaultObjectOnClick, TRUE, \
        MUIA_NFloattext_Text, ftxt, \
        MUIA_NFloattext_TabSize, 4, \
        MUIA_NFloattext_Justify, TRUE, \
      End, \
    End

/* *********************************************** */

