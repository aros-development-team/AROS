#ifndef _MUI_CLASSES_POPIMAGE_H
#define _MUI_CLASSES_POPIMAGE_H

/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popimage              "Popimage.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popimage              (MUIB_ZUNE | 0x00002300)

/*** Methods ****************************************************************/
#define MUIM_Popimage_OpenWindow   (MUIB_Popimage | 0x00000000) /* PRIV */
#define MUIM_Popimage_CloseWindow  (MUIB_Popimage | 0x00000001) /* PRIV */

struct MUIP_Popimage_OpenWindow /* PRIV */
{                               /* PRIV */
    STACKED ULONG MethodID;     /* PRIV */
};                              /* PRIV */

struct MUIP_Popimage_CloseWindow /* PRIV */
{                                /* PRIV */
    STACKED ULONG MethodID;      /* PRIV */
    STACKED LONG ok;             /* PRIV */
};                               /* PRIV */


extern const struct __MUIBuiltinClass _MUI_Popimage_desc;       /* PRIV */

#endif /* _MUI_CLASSES_POPIMAGE_H */
