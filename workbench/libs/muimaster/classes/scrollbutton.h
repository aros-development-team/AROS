/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef _MUI_CLASSES_SCROLLBUTTON_H
#define _MUI_CLASSES_SCROLLBUTTON_H

#define MUIC_Scrollbutton "Scrollbutton.mui"

#define ScrollbuttonObject MUI_NewObject(MUIC_Scrollbutton

#define MUIA_Scrollbutton_NewPosition		(TAG_USER+0x182a000) /* ..G ULONG (packed two WORDs)*/
#define MUIA_Scrollbutton_Horiz					(TAG_USER+0x182a001) /*     WORD */
#define MUIA_Scrollbutton_Vert						(TAG_USER+0x182a002) /*     WORD */
#define MUIA_Scrollbutton_HorizProp       (TAG_USER+0x182a003) /* ISG Object * */
#define MUIA_Scrollbutton_VertProp        (TAG_USER+0x182a004) /* ISG Object * */

extern const struct __MUIBuiltinClass _MUI_Scrollbutton_desc;

#endif  /* FC__SCROLLBUTTONCLASS_H */
