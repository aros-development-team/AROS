#ifndef _ICONWINDOW_H_
#define _ICONWINDOW_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

struct IconWindow_ActionMsg
{
    int type;
    Object *iconlist;
    int isroot;
    struct IconList_Click *click;
    /* to be continued...*/
};

/*** Identifier Base ********************************************************/
#define MUIB_IconWindow               (TAG_USER | 0x10000000)

/*** Public Attributes ******************************************************/
#define MUIA_IconWindow_IsRoot        (MUIB_IconWindow | 0x00000000) /* I-G */
#define MUIA_IconWindow_Drawer        (MUIB_IconWindow | 0x00000001) /* I-G */
#define MUIA_IconWindow_ActionHook    (MUIB_IconWindow | 0x00000002) /* I-- */ /* Hook to call when some action happens */
#define MUIA_IconWindow_IsBackdrop    (MUIB_IconWindow | 0x00000003) /* ISG */ /* is Backdrop window ? */
#define MUIA_IconWindow_IconList      (MUIB_IconWindow | 0x00000004) /* --G */
#define MUIA_IconWindow_Background    (MUIB_IconWindow | 0x00000005) /* ISG */

/*** Public Methods *********************************************************/
#define MUIM_IconWindow_Open          (MUIB_IconWindow | 0x00000000)
#define MUIM_IconWindow_UnselectAll   (MUIB_IconWindow | 0x00000001)

/*** Private Methods ********************************************************/
#define MUIM_IconWindow_DoubleClicked (MUIB_IconWindow | 0x00000002)
#define MUIM_IconWindow_IconsDropped  (MUIB_IconWindow | 0x00000003)
#define MUIM_IconWindow_Clicked       (MUIB_IconWindow | 0x00000004)

#define ICONWINDOW_ACTION_OPEN 1
#define ICONWINDOW_ACTION_CLICK 2
#define ICONWINDOW_ACTION_ICONDROP 3

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IconWindow_CLASS;

/*** Macros *****************************************************************/
#define IconWindowObject BOOPSIOBJMACRO_START(IconWindow_CLASS->mcc_Class)

#endif /* _ICONWINDOW_H_ */
