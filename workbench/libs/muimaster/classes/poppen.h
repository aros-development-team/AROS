/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_POPPEN_H
#define _MUI_CLASSES_POPPEN_H

#define MUIC_Poppen "Poppen.mui"

/* CHECKME: I used same values as in popimage.h!? */ /* PRIV */

/* Poppen methods */
#define MUIM_Poppen_OpenWindow   (METHOD_USER|0x0042a548)     /* PRIV */
#define MUIM_Poppen_CloseWindow  (METHOD_USER|0x0042a549)     /* PRIV */
struct MUIP_Poppen_OpenWindow    {ULONG MethodID;};           /* PRIV */
struct MUIP_Poppen_CloseWindow   {ULONG MethodID; LONG ok;};  /* PRIV */

extern const struct __MUIBuiltinClass _MUI_Poppen_desc; /* PRIV */

#endif
