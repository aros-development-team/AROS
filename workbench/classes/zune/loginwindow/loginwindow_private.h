#ifndef _PREFSWINDOW_PRIVATE_H_
#define _PREFSWINDOW_PRIVATE_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    
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
                    *lwd_CancelButton;
    Object          *lwd_LogonLogo,
                    *lwd_LogonHeader,
                    *lwd_UNInput,
                    *lwd_UPInput;
    /*- Public -------------------------------------------------------------*/
    STRPTR          lwd_Title,
                    lwd_UserName,
                    lwd_UserPass,
                    lwd_DoMethod;
    Object          *lwd_Method;
};

#endif /* _PREFSWINDOW_PRIVATE_H_ */
