/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_CONFIGDATA_H
#define _MUI_CLASSES_CONFIGDATA_H

#define MUIC_Configdata "Configdata.mui"

/* The config items for MUIM_GetConfigItem */
#define MUICFG_Font_Normal            0x1e
#define MUICFG_Font_Tiny              0x20
#define MUICFG_Font_Big		          0x23
#define MUICFG_PublicScreen           0x24
#define MUICFG_Background_Window      0x37
#define MUICFG_Background_Requester   0x38
#define MUICFG_Buttons_Background     0x39
#define MUICFG_Buttons_SelBackground  0x3e
#define MUICFG_Background_Register    0x54
#define MUICFG_Background_Framed      0x5a
#define MUICFG_Background_Page			  0x95

#define MUIA_Configdata_Application (TAG_USER|0x10203453) /* ZV1: PRIV i.g  Object * */
#define MUIA_Configdata_ZunePrefs   (TAG_USER|0x10203454) /* ZV1: PRIV .g.  struct ZunePrefsNew * */

extern const struct __MUIBuiltinClass _MUI_Configdata_desc; /* PRIV */

#endif
