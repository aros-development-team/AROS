#ifndef _MUI_CLASSES_SETTINGSGROUP_H
#define _MUI_CLASSES_SETTINGSGROUP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Settingsgroup                 "Settingsgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Settingsgroup                 (MUIB_ZUNE | 0x00003100)  

/*** Methods ****************************************************************/
#define MUIM_Settingsgroup_ConfigToGadgets (MUIB_MUI|0x00427043) /* MUI: V11 */
#define MUIM_Settingsgroup_GadgetsToConfig (MUIB_MUI|0x00425242) /* MUI: V11 */
struct MUIP_Settingsgroup_ConfigToGadgets  {STACKED ULONG MethodID; STACKED Object *configdata; };
struct MUIP_Settingsgroup_GadgetsToConfig  {STACKED ULONG MethodID; STACKED Object *configdata; };


extern const struct __MUIBuiltinClass _MUI_Settingsgroup_desc; /* PRIV */

#endif /* _MUI_CLASSES_SETTINGSGROUP_H */
