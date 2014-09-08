/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef GUI_GLOBAL_H
#define GUI_GLOBAL_H

#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>

#define CATCOMP_NUMBERS
#include "locale.h"
#undef CATCOMP_NUMBERS

struct Gadget;

enum
{
    // ***** edit browser *****
    OBJ_BROW_ALIGN,

    OBJ_BROW_NAME_STR,
    OBJ_BROW_PATH_GET,
    OBJ_BROW_PATH_CHOOSE,
    OBJ_BROW_AREXX_STR,
    OBJ_BROW_AREXX_CHOOSE,

    OBJ_BROW_SHOW_STR,
    OBJ_BROW_FRONT_STR,
    OBJ_BROW_OPEN_STR,
    OBJ_BROW_OPEN_CHOOSE,
    OBJ_BROW_NEW_STR,
    OBJ_BROW_NEW_CHOOSE,

    OBJ_BROW_USE,
    OBJ_BROW_CANCEL,

    // ***** edit mailer *****
    OBJ_MAIL_ALIGN1,

    OBJ_MAIL_NAME_STR,
    OBJ_MAIL_PATH_GET,
    OBJ_MAIL_PATH_CHOOSE,
    OBJ_MAIL_AREXX_STR,
    OBJ_MAIL_AREXX_CHOOSE,

    OBJ_MAIL_ALIGN2,

    OBJ_MAIL_SHOW_STR,
    OBJ_MAIL_FRONT_STR,
    OBJ_MAIL_WRITE_STR,
    OBJ_MAIL_WRITE_CHOOSE,

    OBJ_MAIL_USE,
    OBJ_MAIL_CANCEL,

    // ***** edit FTP client *****
    OBJ_FTP_ALIGN1,

    OBJ_FTP_NAME_STR,
    OBJ_FTP_PATH_GET,
    OBJ_FTP_PATH_CHOOSE,
    OBJ_FTP_AREXX_STR,
    OBJ_FTP_AREXX_CHOOSE,
    OBJ_FTP_REMOVE,

    OBJ_FTP_ALIGN2,

    OBJ_FTP_SHOW_STR,
    OBJ_FTP_FRONT_STR,
    OBJ_FTP_OPEN_STR,
    OBJ_FTP_OPEN_CHOOSE,
    OBJ_FTP_NEW_STR,
    OBJ_FTP_NEW_CHOOSE,

    OBJ_FTP_USE,
    OBJ_FTP_CANCEL,

    // ***** page 1 *****
    OBJ_LBROWSER_BROW,
    OBJ_ADD_BROW,
    OBJ_EDIT_BROW,
    OBJ_CLONE_BROW,
    OBJ_UP_BROW,
    OBJ_DOWN_BROW,
    OBJ_DELETE_BROW,

    // ***** page 2 *****
    OBJ_LBROWSER_MAIL,
    OBJ_ADD_MAIL,
    OBJ_EDIT_MAIL,
    OBJ_CLONE_MAIL,
    OBJ_UP_MAIL,
    OBJ_DOWN_MAIL,
    OBJ_DELETE_MAIL,


    // ***** page 3 *****
    OBJ_LBROWSER_FTP,
    OBJ_ADD_FTP,
    OBJ_EDIT_FTP,
    OBJ_CLONE_FTP,
    OBJ_UP_FTP,
    OBJ_DOWN_FTP,
    OBJ_DELETE_FTP,

    // ***** page 4 ****
    OBJ_UNICONIFY,
    OBJ_BRING,
    OBJ_OPEN,
    OBJ_LAUNCH,

    OBJ_PREPEND,
    OBJ_SEND_MAILTO,
    OBJ_SEND_FTP,

    // ***** other *****
    OBJ_CLICKTAB,
    OBJ_SAVE,
    OBJ_USE,
    OBJ_APPLY,
    OBJ_CANCEL,

    OBJ_HIDDEN_CHOOSER,

    OBJ_NUM
};

#define OBJ(x) Objects[x]
#define GAD(x) (struct Gadget *)Objects[x]

extern Object *Objects[OBJ_NUM];

struct Image chooser_image;

extern struct List list_Brow;
extern struct List list_Mail;
extern struct List list_FTPs;

extern struct MsgPort *AppPort;
extern struct Hook idcmphook;

extern Object *edit_brow_win;
extern Object *edit_mail_win;
extern Object *edit_ftp_win;

#endif // GUI_GLOBAL_H
