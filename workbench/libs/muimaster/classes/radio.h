#ifndef _MUI_CLASSES_RADIO_H
#define _MUI_CLASSES_RADIO_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Radio         "Radio.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Radio         (MUIB_ZUNE | 0x00002a00)  

/*** Attributes *************************************************************/
#define MUIA_Radio_Active  (MUIB_MUI|0x00429b41) /* MUI:V4  isg LONG      */
#define MUIA_Radio_Entries (MUIB_MUI|0x0042b6a1) /* MUI:V4  i.. STRPTR *  */


extern const struct __MUIBuiltinClass _MUI_Radio_desc; /* PRIV */

#endif /* _MUI_CLASSES_RADIO_H */
