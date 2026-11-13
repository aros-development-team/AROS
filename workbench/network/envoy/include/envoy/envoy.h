#ifndef ENVOY_ENVOY_H
#define ENVOY_ENVOY_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy envoy.library tags (as published in the Envoy developer
          kit). The obsolete position and size tags are accepted and
          ignored.
*/

#include <utility/tagitem.h>
#include <envoy/nipc.h>

/* HostRequestA() */
#define HREQ_Dummy          (TAG_USER + 0xB1300)
#define HREQ_Buffer         (HREQ_Dummy + 1)    /* STRPTR : receives "host" or "realm:host"       */
#define HREQ_BuffSize       (HREQ_Dummy + 2)    /* ULONG  : size of that buffer                    */
#define HREQ_Left           (HREQ_Dummy + 3)    /* obsolete                                        */
#define HREQ_Top            (HREQ_Dummy + 4)    /* obsolete                                        */
#define HREQ_Width          (HREQ_Dummy + 5)    /* obsolete                                        */
#define HREQ_Height         (HREQ_Dummy + 6)    /* obsolete                                        */
#define HREQ_DefaultRealm   (HREQ_Dummy + 7)    /* STRPTR : realm to list first                    */
#define HREQ_NoRealms       (HREQ_Dummy + 8)    /* BOOL   : no Realms button                       */
#define HREQ_Screen         (HREQ_Dummy + 9)    /* struct Screen *                                 */
#define HREQ_Title          (HREQ_Dummy + 10)   /* STRPTR                                          */
#define HREQ_NoResizer      (HREQ_Dummy + 11)   /* obsolete                                        */
#define HREQ_NoDragBar      (HREQ_Dummy + 12)   /* obsolete                                        */
#define HREQ_Window         (HREQ_Dummy + 13)   /* struct Window * : blocked while the requester is up */
#define HREQ_OptimWindow    (HREQ_Dummy + 14)   /* struct Window * : the requester opens over it   */
#define HREQ_CallBack       (LREQ_Dummy + 15)   /* struct Hook * : called for messages on HREQ_MsgPort */
#define HREQ_MsgPort        (LREQ_Dummy + 16)   /* struct MsgPort * : a window's IDCMP port        */

/* LoginRequestA() */
#define LREQ_Dummy          (TAG_USER + 0xB1400)
#define LREQ_NameBuff       (LREQ_Dummy + 1)
#define LREQ_NameBuffLen    (LREQ_Dummy + 2)
#define LREQ_PassBuff       (LREQ_Dummy + 3)
#define LREQ_PassBuffLen    (LREQ_Dummy + 4)
#define LREQ_Left           (LREQ_Dummy + 5)    /* obsolete                                        */
#define LREQ_Top            (LREQ_Dummy + 6)    /* obsolete                                        */
#define LREQ_Width          (LREQ_Dummy + 7)    /* obsolete                                        */
#define LREQ_Height         (LREQ_Dummy + 8)    /* obsolete                                        */
#define LREQ_Screen         (LREQ_Dummy + 9)
#define LREQ_Title          (LREQ_Dummy + 10)
#define LREQ_NoDragBar      (LREQ_Dummy + 11)   /* obsolete                                        */
#define LREQ_Window         (LREQ_Dummy + 12)
#define LREQ_CallBack       (LREQ_Dummy + 13)
#define LREQ_MsgPort        (LREQ_Dummy + 14)
#define LREQ_NoSizeGadget   (LREQ_Dummy + 15)   /* obsolete                                        */
#define LREQ_OptimWindow    (LREQ_Dummy + 16)
#define LREQ_UserName       (LREQ_Dummy + 17)   /* STRPTR : preset name (default ENV:USERNAME)     */
#define LREQ_Password       (LREQ_Dummy + 18)   /* STRPTR : preset password                        */

/* UserRequestA() */
#define UGREQ_Dummy         (TAG_USER + 0xB1400)
#define UGREQ_UserBuff      (UGREQ_Dummy + 1)   /* STRPTR : receives the chosen user; users are listed */
#define UGREQ_UserBuffLen   (UGREQ_Dummy + 2)
#define UGREQ_GroupBuff     (UGREQ_Dummy + 3)   /* STRPTR : receives the chosen group; groups are listed */
#define UGREQ_GroupBuffLen  (UGREQ_Dummy + 4)
#define UGREQ_Left          (UGREQ_Dummy + 5)   /* obsolete                                        */
#define UGREQ_Top           (UGREQ_Dummy + 6)   /* obsolete                                        */
#define UGREQ_Width         (UGREQ_Dummy + 7)   /* obsolete                                        */
#define UGREQ_Height        (UGREQ_Dummy + 8)   /* obsolete                                        */
#define UGREQ_Screen        (UGREQ_Dummy + 9)
#define UGREQ_Title         (UGREQ_Dummy + 10)
#define UGREQ_NoDragBar     (UGREQ_Dummy + 11)  /* obsolete                                        */
#define UGREQ_Window        (UGREQ_Dummy + 12)
#define UGREQ_CallBack      (UGREQ_Dummy + 13)
#define UGREQ_MsgPort       (UGREQ_Dummy + 14)
#define UGREQ_NoSizeGadget  (UGREQ_Dummy + 15)  /* obsolete                                        */
#define UGREQ_UserList      (UGREQ_Dummy + 16)  /* struct List * : user names to show instead of accounts.library's */
#define UGREQ_GroupList     (UGREQ_Dummy + 17)  /* struct List * : group names likewise            */
#define UGREQ_OptimWindow   (UGREQ_Dummy + 18)

/* PasswordRequestA() */
#define PWREQ_Dummy         (TAG_USER + 0xB1400)
#define PWREQ_OldPassword   (PWREQ_Dummy + 1)   /* STRPTR : old password to verify, or buffer (see PWREQ_OldPWLen) */
#define PWREQ_NewPWBuff     (PWREQ_Dummy + 2)   /* STRPTR : receives the new password              */
#define PWREQ_NewPWBuffLen  (PWREQ_Dummy + 3)
#define PWREQ_PWRequired    (PWREQ_Dummy + 4)   /* BOOL : at least six characters, different from the old one */
#define PWREQ_Screen        (PWREQ_Dummy + 9)
#define PWREQ_Title         (PWREQ_Dummy + 10)
#define PWREQ_Window        (PWREQ_Dummy + 12)
#define PWREQ_CallBack      (PWREQ_Dummy + 13)
#define PWREQ_MsgPort       (PWREQ_Dummy + 14)
#define PWREQ_OptimWindow   (PWREQ_Dummy + 18)
#define PWREQ_OldPWLen      (PWREQ_Dummy + 19)  /* ULONG : PWREQ_OldPassword is a buffer of this size that receives the typed old password, unchecked */

#endif /* ENVOY_ENVOY_H */
