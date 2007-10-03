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
#define MUIM_Poppen_OpenWindow   (MUIB_Poppen | 0x00000000)     /* PRIV */
#define MUIM_Poppen_CloseWindow  (MUIB_Poppen | 0x00000001)     /* PRIV */
struct MUIP_Poppen_OpenWindow    {STACKED ULONG MethodID;};           /* PRIV */
struct MUIP_Poppen_CloseWindow   {STACKED ULONG MethodID; STACKED LONG ok;};  /* PRIV */


extern const struct __MUIBuiltinClass _MUI_Poppen_desc; /* PRIV */

#endif /* _MUI_CLASSES_POPPEN_H */
