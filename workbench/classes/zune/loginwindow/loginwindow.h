#ifndef ZUNE_PREFSWINDOW_H
#define ZUNE_PREFSWINDOW_H

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_LoginWindow                        "LoginWindow.mcc"

/*** Identifier base ********************************************************/
#define MUIB_LoginWindow                        (MUIB_AROS | 0x00000200)

/*** Public (Abstract) Methods **********************************************/
#define MUIM_LoginWindow_OK                     (MUIB_LoginWindow | 0x00000000)
#define MUIM_LoginWindow_Cancel                 (MUIB_LoginWindow | 0x00000001)

/*** Protected Attributes ***************************************************/
#define MUIA_LoginWindow_Title                  (MUIB_LoginWindow | 0x00000005)
#define MUIA_LoginWindow_LoginLogo              (MUIB_LoginWindow | 0x00000006)
#define MUIA_LoginWindow_LogoFrame              (MUIB_LoginWindow | 0x00000007)
#define MUIA_LoginWindow_DetailsFrame           (MUIB_LoginWindow | 0x00000008)
#define MUIA_LoginWindow_UserName               (MUIB_LoginWindow | 0x00000009)
#define MUIA_LoginWindow_UserName_Status        (MUIB_LoginWindow | 0x0000000a)
#   define  LWA_UNT_Input       0 /* default */
#   define  LWA_UNT_Read        1
#   define  LWA_UNT_Disabled    2
#   define  LWA_UNT_None        3
#define MUIA_LoginWindow_UserPass               (MUIB_LoginWindow | 0x0000000b)
#define MUIA_LoginWindow_UserPass_Disabled      (MUIB_LoginWindow | 0x0000000c)
#define MUIA_LoginWindow_Method                 (MUIB_LoginWindow | 0x0000000d)
#define MUIA_LoginWindow_Method_Status          (MUIB_LoginWindow | 0x0000000e)
#   define  LWA_METH_Input      0 /* default */
#   define  LWA_METH_Disabled   1
#   define  LWA_METH_None       2
#define MUIA_LoginWindow_LocalLogin_Disabled    (MUIB_LoginWindow | 0x0000000f)
#define MUIA_LoginWindow_Cancel_Disabled        (MUIB_LoginWindow | 0x00000010)

/*** Macros *****************************************************************/
#define LoginWindowObject MUIOBJMACRO_START(MUIC_LoginWindow)

#define LWA_RV_CANCEL                           MUIV_Application_ReturnID_Quit
#define LWA_RV_OK                               !LWA_RV_CANCEL

#define LWRV_CANCEL                             MUIV_Application_ReturnID_Quit
#define LWRV_OK                                 !LWRV_CANCEL


#endif /* ZUNE_PREFSWINDOW_H */
