#ifndef _MUI_CLASSES_POPPEN_H
#define _MUI_CLASSES_POPPEN_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Poppen              "Poppen.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Poppen              (MUIB_ZUNE | 0x00002700)

/*** Methods ****************************************************************/
/* CHECKME: I used same values as in popimage.h!? */ /* PRIV */
#define MUIM_Poppen_OpenWindow   (MUIB_MUI|0x0042a548)     /* PRIV */
#define MUIM_Poppen_CloseWindow  (MUIB_MUI|0x0042a549)     /* PRIV */
struct MUIP_Poppen_OpenWindow    {ULONG MethodID;};           /* PRIV */
struct MUIP_Poppen_CloseWindow   {ULONG MethodID; LONG ok;};  /* PRIV */


extern const struct __MUIBuiltinClass _MUI_Poppen_desc; /* PRIV */

#endif /* _MUI_CLASSES_POPPEN_H */
