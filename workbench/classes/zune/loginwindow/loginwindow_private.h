#ifndef _PREFSWINDOW_PRIVATE_H_
#define _PREFSWINDOW_PRIVATE_H_

/*
    Copyright © 2003-2026, The AROS Development Team. All rights reserved.
    
    $Id$
*/

/*** Instance data **********************************************************/
struct LoginWindow_DATA
{
    /*- Private ------------------------------------------------------------*/

    struct Catalog  *lwd_Catalog;
    APTR            lwd_Pool;
    APTR            lwd_MethodList;
    
    /*- Protected ----------------------------------------------------------*/

    Object          *lwd_OKButton,
                    *lwd_CancelButton,
                    *lwd_ShutdownButton,
                    *lwd_RebootButton;
    BOOL             lwd_SystemMode;
    Object          *lwd_LogonLogo,
                    *lwd_LogonHeader,
                    *lwd_UNInput,
                    *lwd_UPInput;
    /*- Public -------------------------------------------------------------*/
    STRPTR          lwd_Title,
                    lwd_UserName,
                    lwd_UserPass,
                    lwd_DoMethod;
    Object          *lwd_Method,
                    *lwd_MethodString;   /* the popstring's string object */
    ULONG            lwd_NameType;
};

#endif /* _PREFSWINDOW_PRIVATE_H_ */
