/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_RADIO_H
#define _MUI_CLASSES_RADIO_H

#define MUIC_Radio "Radio.mui"

/* Radio attributes */
#define MUIA_Radio_Active  (TAG_USER|0x00429b41) /* MUI:V4  isg LONG      */
#define MUIA_Radio_Entries (TAG_USER|0x0042b6a1) /* MUI:V4  i.. STRPTR *  */

extern const struct __MUIBuiltinClass _MUI_Radio_desc; /* PRIV */

#endif
