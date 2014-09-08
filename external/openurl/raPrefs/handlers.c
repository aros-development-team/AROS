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

#include "handlers.h"
#include "browsers.h"
#include "ftps.h"
#include "mailers.h"
#include "gui_global.h"
#include "OpenURL.h"
#include "macros.h"
#include "utility.h"

#include <libraries/openurl.h>

#include <classes/window.h>

#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <gadgets/listbrowser.h>

#include <reaction/reaction_macros.h>

#include <proto/dos.h>
#include <proto/intuition.h>

extern struct Window *window;
extern struct Window *edit_brow_window;
extern struct Window *edit_mail_window;
extern struct Window *edit_ftp_window;

extern Object *win;
extern Object *edit_brow_win;
extern Object *edit_mail_win;
extern Object *edit_ftp_win;

CONST_STRPTR hidden_strings[] =
{
    "this popup",
    "is not yet",
    "functional",
    "sorry :-/",
     NULL
};

/// HandleInput_Main_Win
BOOL HandleInput_Main_Win(void)
{
    uint32 result      = 0;
    uint16 code        = 0;
    BOOL   done        = FALSE;

    while ((result = RA_HandleInput(win, &code)))
    {
        switch(result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                done = TRUE;
                break;
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case OBJ_LBROWSER_BROW:
                    {
                        uint32 retval = iget(OBJ(OBJ_LBROWSER_BROW), LISTBROWSER_RelEvent);

                        switch(retval)
                        {
                            case LBRE_CHECKED:
                            case LBRE_UNCHECKED:
                            {
                                struct URL_BrowserNode *bn;

                                bn = (struct URL_BrowserNode *)iget(OBJ(OBJ_LBROWSER_BROW), LISTBROWSER_SelectedNode);
                                if (retval == LBRE_UNCHECKED)
                                    SET_FLAG(bn->ubn_Flags,UNF_DISABLED);
                                else
                                    CLEAR_FLAG(bn->ubn_Flags,UNF_DISABLED);
                            }
                        }
                        if (retval != LBRE_DOUBLECLICK)break;
                        // we deliberately go on executing following case OBJ_EDIT_BROW
                        // because a double click mean the same as clicking edit button
                    }
                    case OBJ_EDIT_BROW:
                    {
                        struct URL_BrowserNode *bn;

                        bn = (struct URL_BrowserNode *)iget(OBJ(OBJ_LBROWSER_BROW), LISTBROWSER_SelectedNode);
                        updateBrowserWindow(bn);
                        edit_brow_window = RA_OpenWindow(edit_brow_win);
                        break;
                    }
                    case OBJ_LBROWSER_MAIL:
                    {
                        uint32 retval = iget(OBJ(OBJ_LBROWSER_MAIL), LISTBROWSER_RelEvent);

                        switch(retval)
                        {
                            case LBRE_CHECKED:
                            case LBRE_UNCHECKED:
                            {
                                struct URL_MailerNode *mn;

                                mn = (struct URL_MailerNode *)iget(OBJ(OBJ_LBROWSER_MAIL), LISTBROWSER_SelectedNode);
                                if (retval == LBRE_UNCHECKED)
                                    SET_FLAG(mn->umn_Flags,UNF_DISABLED);
                                else
                                    CLEAR_FLAG(mn->umn_Flags,UNF_DISABLED);
                            }
                        }
                        if (retval != LBRE_DOUBLECLICK) break;
                        // we deliberately go on executing following case OBJ_EDIT_MAIL
                        // because a double click mean the same as clicking edit button
                    }
                    case OBJ_EDIT_MAIL:
                    {
                        struct URL_MailerNode *mn;

                        mn = (struct URL_MailerNode *)iget(OBJ(OBJ_LBROWSER_MAIL), LISTBROWSER_SelectedNode);
                        updateMailerWindow(mn);
                        edit_mail_window = RA_OpenWindow(edit_mail_win);
                        break;
                    }
                    case OBJ_LBROWSER_FTP:
                    {
                        uint32 retval = iget(OBJ(OBJ_LBROWSER_FTP), LISTBROWSER_RelEvent);

                        switch(retval)
                        {
                            case LBRE_CHECKED:
                            case LBRE_UNCHECKED:
                            {
                                struct URL_FTPNode *fn;

                                fn = (struct URL_FTPNode *)iget(OBJ(OBJ_LBROWSER_FTP), LISTBROWSER_SelectedNode);
                                if (retval == LBRE_UNCHECKED)
                                    SET_FLAG(fn->ufn_Flags,UNF_DISABLED);
                                else
                                    CLEAR_FLAG(fn->ufn_Flags,UNF_DISABLED);
                            }
                        }
                        if (retval != LBRE_DOUBLECLICK) break;
                        // we deliberately go on executing following case OBJ_EDIT_FTP
                        //  because a double click mean the same as clicking edit button
                    }
                    case OBJ_EDIT_FTP:
                    {
                        struct URL_FTPNode *fn;

                        fn = (struct URL_FTPNode *)iget(OBJ(OBJ_LBROWSER_FTP), LISTBROWSER_SelectedNode);
                        updateFTPWindow(fn);
                        edit_ftp_window = RA_OpenWindow(edit_ftp_win);
                        break;
                    }
                    case OBJ_USE:
                    case OBJ_SAVE:
                    case OBJ_APPLY:
                        storePrefs((result & WMHI_GADGETMASK)==OBJ_SAVE);
                        if((result & WMHI_GADGETMASK)!=OBJ_APPLY)
                            done=TRUE;
                        break;
                    case OBJ_CANCEL:
                        done=TRUE;
                        break;
                }
                break;
            case WMHI_ICONIFY:
                if (RA_Iconify(win))
                {
                    window = NULL;
                    if (edit_brow_window)
                    {
                        RA_CloseWindow(edit_brow_win);
                        edit_brow_window = NULL;
                    }
                    if (edit_mail_window)
                    {
                        RA_CloseWindow(edit_mail_win);
                        edit_mail_window = NULL;
                    }
                    if (edit_ftp_window)
                    {
                        RA_CloseWindow(edit_ftp_win);
                        edit_ftp_window = NULL;
                    }
                }
                break;
            case WMHI_UNICONIFY:
                window = RA_OpenWindow(win);
                break;
        }
    }
    return (done);
}
///

/// HandleInput_Edit_Brow_Win
void HandleInput_Edit_Brow_Win()
{
    uint32 result      = 0;
    uint16 code        = 0;

    while ((result = RA_HandleInput(edit_brow_win, &code)))
    {
        switch(result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                RA_CloseWindow(edit_brow_win);
                edit_brow_window = NULL;
                break;
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case OBJ_BROW_USE:
                        gadset(GAD(OBJ_LBROWSER_BROW), window, LISTBROWSER_Labels, ~0);
                        updateBrowserNode();
                        gadset(GAD(OBJ_LBROWSER_BROW), window,  LISTBROWSER_Labels, &list_Brow,
                                                                LISTBROWSER_AutoFit, TRUE);
                    case OBJ_BROW_CANCEL:
                        RA_CloseWindow(edit_brow_win);
                        edit_brow_window = NULL;
                        break;
                    case OBJ_BROW_PATH_GET:
                        if (gfRequestFile(OBJ(OBJ_BROW_PATH_GET), edit_brow_window))
                        {
                        }
                        break;
                    case OBJ_BROW_PATH_CHOOSE:  // set Attrs according to the button clicked on.
                    case OBJ_BROW_OPEN_CHOOSE:
                    case OBJ_BROW_NEW_CHOOSE:
                        iset(OBJ(OBJ_HIDDEN_CHOOSER), CHOOSER_LabelArray, hidden_strings);
                        IIntuition->ActivateGadget(GAD(OBJ_HIDDEN_CHOOSER),
                                                   edit_brow_window, NULL);
                        break;
                    case OBJ_BROW_AREXX_CHOOSE:
                        iset(OBJ(OBJ_HIDDEN_CHOOSER), CHOOSER_LabelArray, hidden_strings);
                        IIntuition->ActivateGadget(GAD(OBJ_HIDDEN_CHOOSER),
                                                   edit_brow_window, NULL);
                        break;
                }
        }
    }
}
///

/// HandleInput_Edit_Mail_Win
void HandleInput_Edit_Mail_Win()
{
    uint32 result      = 0;
    uint16 code        = 0;

    while ((result = RA_HandleInput(edit_mail_win, &code)))
    {
        switch(result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                RA_CloseWindow(edit_mail_win);
                edit_mail_window = NULL;
                break;
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case OBJ_MAIL_USE:
                        gadset(GAD(OBJ_LBROWSER_MAIL), window, LISTBROWSER_Labels, ~0);
                        updateMailerNode();
                        gadset(GAD(OBJ_LBROWSER_MAIL), window, LISTBROWSER_Labels, &list_Mail,
                                                               LISTBROWSER_AutoFit, TRUE);
                    case OBJ_MAIL_CANCEL:
                        RA_CloseWindow(edit_mail_win);
                        edit_mail_window = NULL;
                        break;
                    case OBJ_MAIL_PATH_GET:
                        if (gfRequestFile(OBJ(OBJ_MAIL_PATH_GET), edit_mail_window))
                        {
                        }
                        break;
                    case OBJ_MAIL_PATH_CHOOSE:  // set Attrs according to the button clicked on.
                    case OBJ_MAIL_WRITE_CHOOSE:
                        iset(OBJ(OBJ_HIDDEN_CHOOSER), CHOOSER_LabelArray, hidden_strings);
                        IIntuition->ActivateGadget(GAD(OBJ_HIDDEN_CHOOSER),
                                                   edit_mail_window, NULL);
                        break;
                    case OBJ_MAIL_AREXX_CHOOSE:
                        iset(OBJ(OBJ_HIDDEN_CHOOSER), CHOOSER_LabelArray, hidden_strings);
                        IIntuition->ActivateGadget(GAD(OBJ_HIDDEN_CHOOSER),
                                                   edit_mail_window, NULL);
                        break;
                }
        }
    }
}
///

/// HandleInput_Edit_FTP_Win
void HandleInput_Edit_FTP_Win()
{
    uint32 result      = 0;
    uint16 code        = 0;

    while ((result = RA_HandleInput(edit_ftp_win, &code)))
    {
        switch(result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                RA_CloseWindow(edit_ftp_win);
                edit_ftp_window = NULL;
                break;
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case OBJ_FTP_USE:
                        gadset(GAD(OBJ_LBROWSER_BROW), window, LISTBROWSER_Labels, ~0);
                        updateFTPNode();
                        gadset(GAD(OBJ_LBROWSER_BROW), window, LISTBROWSER_Labels, &list_FTPs,
                                                               LISTBROWSER_AutoFit, TRUE);
                    case OBJ_FTP_CANCEL:
                        RA_CloseWindow(edit_ftp_win);
                        edit_brow_window = NULL;
                        break;
                    case OBJ_FTP_PATH_GET:
                        if (gfRequestFile(OBJ(OBJ_FTP_PATH_GET), edit_ftp_window))
                        {
                        }
                        break;
                    case OBJ_FTP_PATH_CHOOSE:  // set Attrs according to the button clicked on.
                    case OBJ_FTP_OPEN_CHOOSE:
                    case OBJ_FTP_NEW_CHOOSE:
                        iset( OBJ(OBJ_HIDDEN_CHOOSER), CHOOSER_LabelArray, hidden_strings);
                        IIntuition->ActivateGadget(GAD(OBJ_HIDDEN_CHOOSER),
                                                   edit_ftp_window, NULL);
                        break;
                    case OBJ_FTP_AREXX_CHOOSE:
                        iset( OBJ(OBJ_HIDDEN_CHOOSER), CHOOSER_LabelArray, hidden_strings);
                        IIntuition->ActivateGadget(GAD(OBJ_HIDDEN_CHOOSER),
                                                   edit_ftp_window, NULL);
                        break;
                }
        }
    }
}
///
