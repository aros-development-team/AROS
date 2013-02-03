/***************************************************************************

 NListview.mcc - New Listview MUI Custom Class
 Registered MUI class, Serial Number: 1d51 (0x9d510020 to 0x9d51002F)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2009 by NList Open Source Team

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

#ifndef MUI_NListview_MCC_H
#define MUI_NListview_MCC_H

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#ifndef MUI_NList_MCC_H
#include <mui/NList_mcc.h>
#endif

#define MUIC_NListview "NListview.mcc"
#if defined(__AROS__) && !defined(NO_INLINE_STDARG)
#define NListviewObject MUIOBJMACRO_START(MUIC_NListview)
#else
#define NListviewObject MUI_NewObject(MUIC_NListview
#endif

/* Attributes */

#define MUIA_NListview_NList                0x9d510020UL /* GM  i.g Object *          */

#define MUIA_NListview_Vert_ScrollBar       0x9d510021UL /* GM  isg LONG              */
#define MUIA_NListview_Horiz_ScrollBar      0x9d510022UL /* GM  isg LONG              */
#define MUIA_NListview_VSB_Width            0x9d510023UL /* GM  ..g LONG              */
#define MUIA_NListview_HSB_Height           0x9d510024UL /* GM  ..g LONG              */

#define MUIV_Listview_ScrollerPos_Default 0
#define MUIV_Listview_ScrollerPos_Left    1
#define MUIV_Listview_ScrollerPos_Right   2
#define MUIV_Listview_ScrollerPos_None    3

#define MUIM_NListview_QueryBeginning     MUIM_NList_QueryBeginning /* obsolete */

#define MUIV_NListview_VSB_Always      1
#define MUIV_NListview_VSB_Auto        2
#define MUIV_NListview_VSB_FullAuto    3
#define MUIV_NListview_VSB_None        4
#define MUIV_NListview_VSB_Default     5
#define MUIV_NListview_VSB_Left        6

#define MUIV_NListview_HSB_Always      1
#define MUIV_NListview_HSB_Auto        2
#define MUIV_NListview_HSB_FullAuto    3
#define MUIV_NListview_HSB_None        4
#define MUIV_NListview_HSB_Default     5

#define MUIV_NListview_VSB_On          0x0030
#define MUIV_NListview_VSB_Off         0x0010

#define MUIV_NListview_HSB_On          0x0300
#define MUIV_NListview_HSB_Off         0x0100

#endif /* MUI_NListview_MCC_H */
