#ifndef LIBRARIES_MIAMIPANEL_H
#define LIBRARIES_MIAMIPANEL_H

/*
**  $VER: lib.h 1.1 (21.01.2006)
**
**  miamipanel.library interface structures and defintions.
**
*/

/*****************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*****************************************************************************/
/*
** Callback types definitions
*/

typedef long (MiamiPanelCallBackType)(long code,long count,va_list args);

/*****************************************************************************/
/*
** Commands codes
*/

enum
{
    MIAMIPANELV_CallBack_Code_UnitOnline  = 112,
    MIAMIPANELV_CallBack_Code_UnitOffline = 113,

    MIAMIPANELV_CallBack_Code_ShowMainGUI = 56,
    MIAMIPANELV_CallBack_Code_HideMainGUI = 19,
    MIAMIPANELV_CallBack_Code_ClosePanel  = 110,
    MIAMIPANELV_CallBack_Code_QuitMiami   = 123,

    MIAMIPANELV_CallBack_Code_Localize    = 27,
};

/*****************************************************************************/
/*
** Interface states
*/

enum
{
    MIAMIPANELV_AddInterface_State_GoingOnline  = 1<<8,
    MIAMIPANELV_AddInterface_State_GoingOffline = 1<<9,
    MIAMIPANELV_AddInterface_State_Suspending   = 1<<10,
    MIAMIPANELV_AddInterface_State_Offline      = 1<<0,
    MIAMIPANELV_AddInterface_State_Online       = 1<<1,
    MIAMIPANELV_AddInterface_State_Suspended    = 1<<2,
};

/*****************************************************************************/
/*
** Flags defining the appearance of the control panel
*/

enum
{
    MIAMIPANELV_Init_Flags_ShowSpeed            = 1<<0,
    MIAMIPANELV_Init_Flags_ShowDataTransferRate = 1<<1,
    MIAMIPANELV_Init_Flags_ShowUpTime           = 1<<2,
    MIAMIPANELV_Init_Flags_ShowTotal            = 1<<3,
    MIAMIPANELV_Init_Flags_ShowStatusButton     = 1<<4,
    MIAMIPANELV_Init_Flags_Control              = 1<<5,
};

/*****************************************************************************/
/*
** String codes
*/

enum
{
    MIAMIPANELV_String_Status_GoingOnline  = 5000, /* ">On"  */
    MIAMIPANELV_String_Status_GoingOffline = 5001, /* ">Of"  */
    MIAMIPANELV_String_Status_Suspending   = 5002, /* ">Su"  */
    MIAMIPANELV_String_Status_Online       = 5003, /* "Onl"  */
    MIAMIPANELV_String_Status_Offline      = 5004, /* "Off"  */
    MIAMIPANELV_String_Status_Suspended    = 5005, /* "Sus"  */

    MIAMIPANELV_String_Button_Show         = 5006, /* "Show" */
    MIAMIPANELV_String_Button_Hide         = 5007, /* "Hide" */
    MIAMIPANELV_String_Button_Quit         = 5008, /* "Quit" */
    MIAMIPANELV_String_Button_Online       = 5009, /* "Onl"  */
    MIAMIPANELV_String_Button_Offline      = 5010, /* "Off"  */
};

/*****************************************************************************/

#endif /* LIBRARIES_MIAMIPANEL_H */
