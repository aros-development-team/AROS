/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_SETTINGSGROUP_H
#define _MUI_CLASSES_SETTINGSGROUP_H

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

#define MUIC_Settingsgroup  "Settingsgroup.mui"

/* Settingsgroups methods */

#define MUIM_Settingsgroup_ConfigToGadgets (METHOD_USER|0x00427043) /* MUI: V11 */
#define MUIM_Settingsgroup_GadgetsToConfig (METHOD_USER|0x00425242) /* MUI: V11 */
struct MUIP_Settingsgroup_ConfigToGadgets  {ULONG MethodID; Object *configdata; };
struct MUIP_Settingsgroup_GadgetsToConfig  {ULONG MethodID; Object *configdata; };

extern const struct __MUIBuiltinClass _MUI_Settingsgroup_desc; /* PRIV */

#endif
