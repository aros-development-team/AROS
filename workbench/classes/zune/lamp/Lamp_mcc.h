#ifndef LAMP_MCC_H
#define LAMP_MCC_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Lamp  "Lamp.mcc"

/*** Identifier base ********************************************************/

/*** Public (Abstract) Methods **********************************************/
#define MUIM_Lamp_SetRGB                0x85b90008ul
struct MUIP_Lamp_SetRGB                 {STACKED ULONG methodid; STACKED ULONG red; STACKED ULONG green; STACKED ULONG blue; };

/*** Protected Attributes ***************************************************/
#define MUIA_Lamp_Type                  0x85b90001ul /* [ISG]  ULONG                */
#define MUIA_Lamp_Color                 0x85b90002ul /* [ISG]  ULONG *              */
#define MUIA_Lamp_ColorType             0x85b90003ul /* [..G]  ULONG                */
#define MUIA_Lamp_Red                   0x85b90004ul /* [ISG]  ULONG                */
#define MUIA_Lamp_Green                 0x85b90005ul /* [ISG]  ULONG                */
#define MUIA_Lamp_Blue                  0x85b90006ul /* [ISG]  ULONG                */
#define MUIA_Lamp_PenSpec               0x85b90007ul /* [ISG]  struct MUI_PenSpec * */

/*** Macros *****************************************************************/
#define LampObject                      MUIOBJMACRO_START(MUIC_Lamp)

#define MUIV_Lamp_Type_Tiny             0
#define MUIV_Lamp_Type_Small            1
#define MUIV_Lamp_Type_Medium           2
#define MUIV_Lamp_Type_Big              3
#define MUIV_Lamp_Type_Huge             4

#define MUIV_Lamp_ColorType_UserDefined 0
#define MUIV_Lamp_ColorType_Color       1
#define MUIV_Lamp_ColorType_PenSpec     2

#define MUIV_Lamp_Color_Off             0
#define MUIV_Lamp_Color_Ok              1
#define MUIV_Lamp_Color_Warning         2
#define MUIV_Lamp_Color_Error           3
#define MUIV_Lamp_Color_FatalError      4
#define MUIV_Lamp_Color_Processing      5
#define MUIV_Lamp_Color_LookingUp       6
#define MUIV_Lamp_Color_Connecting      7
#define MUIV_Lamp_Color_SendingData     8
#define MUIV_Lamp_Color_ReceivingData   9
#define MUIV_Lamp_Color_LoadingData     10
#define MUIV_Lamp_Color_SavingData      11

#endif /* LAMP_MCC_H */
