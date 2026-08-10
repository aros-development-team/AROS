#ifndef _POPINTERFACE_H_
#define _POPINTERFACE_H_

/*
    Copyright © 2026, The AROS Development Team. All rights reserved.

    PopInterface - a Popobject-derived "Add Interface" split button.

    Renders as an "Add Interface" button with a pop (dropdown) button at its
    side, in the style of Zune's Popstring/Popasl gadgets.  Pressing the main
    button requests a normal network interface; pressing the pop button drops
    down an "Add Tunnel" button which, when pressed, requests a 6in4 tunnel.

    The chosen action is reported through the notifiable MUIA_PopInterface_Chosen
    attribute (set to one of the MUIV_PopInterface_* values on each press).
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_PopInterface             (TAG_USER | 0x11000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *PopInterface_CLASS;

/*** Macros *****************************************************************/
#define PopInterfaceObject \
    BOOPSIOBJMACRO_START(PopInterface_CLASS->mcc_Class)

/*** Attributes *************************************************************/
/* ..g LONG - notifiable; the last chosen action (a MUIV_PopInterface_* value).
   Notify on MUIV_EveryTime with MUIV_TriggerValue to be told on every press. */
#define MUIA_PopInterface_Chosen      (MUIB_PopInterface | 0x0001)

/*** Special values for MUIA_PopInterface_Chosen *************************/
#define MUIV_PopInterface_None        (-1)
#define MUIV_PopInterface_DeviceInterface   (0)
#define MUIV_PopInterface_TunnelInterface      (1)

#endif /* _POPINTERFACE_H_ */
