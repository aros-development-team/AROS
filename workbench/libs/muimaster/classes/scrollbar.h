/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_SCROLLBAR_H
#define _MUI_CLASSES_SCROLLBAR_H

/****************************************************************************/
/** Scrollbar                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scrollbar[];
#else
#define MUIC_Scrollbar "Scrollbar.mui"
#endif

/* Attributes */

#define MUIA_Scrollbar_Type                 0x8042fb6b /* V11 i.. LONG              */

#define MUIV_Scrollbar_Type_Default 0
#define MUIV_Scrollbar_Type_Bottom 1
#define MUIV_Scrollbar_Type_Top 2
#define MUIV_Scrollbar_Type_Sym 3

extern const struct __MUIBuiltinClass _MUI_Scrollbar_desc;

#endif
