/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include <exec/exec.h>
#include <intuition/intuition.h>
#include <intuition/icclass.h>
#include <dos/dos.h>
#include <libraries/gadtools.h>
#include <images/label.h>
#include <images/glyph.h>

#include <classes/window.h>

#include <gadgets/layout.h>
#include <gadgets/space.h>
#include <gadgets/button.h>
#include <gadgets/clicktab.h>
#include <gadgets/texteditor.h>
#include <gadgets/scroller.h>
#include <gadgets/checkbox.h>
#include <gadgets/listbrowser.h>
#include <gadgets/string.h>
#include <gadgets/getfile.h>
#include <gadgets/chooser.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <proto/window.h>
#include <proto/layout.h>
#include <proto/space.h>
#include <proto/button.h>
#include <proto/clicktab.h>
#include <proto/texteditor.h>
#include <proto/scroller.h>
#include <proto/checkbox.h>
#include <proto/listbrowser.h>
#include <proto/string.h>
#include <proto/getfile.h>
#include <proto/chooser.h>
#include <proto/label.h>

#include <reaction/reaction_macros.h>

#include <libraries/openurl.h>
#include <proto/openurl.h>

#include "gui_global.h"
#include "browsers.h"
#include "ftps.h"
#include "handlers.h"
#include "mailers.h"
#include "utility.h"
#include "macros.h"

#include "version.h"

static const char USED_VAR version[] = "$VER: OpenURL-Prefs " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT;

Object *win;
struct MsgPort *AppPort;
struct Hook idcmphook;

struct Window *window;

struct List list_Brow;
struct List list_Mail;
struct List list_FTPs;

struct Library * OpenURLBase = NULL;
struct OpenURLIFace *IOpenURL = NULL;

Object *Objects[OBJ_NUM];

static STRPTR PageLabels[] =
{
    (STRPTR)MSG_Win_Labels_Browsers,
    (STRPTR)MSG_Win_Labels_Mailers,
    (STRPTR)MSG_Win_Labels_FTPs,
    (STRPTR)MSG_Win_Labels_Misc,
    NULL
};

struct ColumnInfo BrowsColInfo[] =
{
    { 20, (STRPTR)"Use", CIF_WEIGHTED },
    { 20, (STRPTR)" Name", CIF_WEIGHTED },
    { 20, (STRPTR)" Path", CIF_WEIGHTED },
    { -1, NULL, -1 }
};

struct ColumnInfo MailColInfo[] =
{
    { 20, (STRPTR)"Use", CIF_WEIGHTED },
    { 20, (STRPTR)" Name", CIF_WEIGHTED },
    { 20, (STRPTR)" Path", CIF_WEIGHTED },
    { -1, NULL, -1 }
};

struct ColumnInfo FTPsColInfo[] =
{
    { 20, (STRPTR)"Use", CIF_WEIGHTED },
    { 20, (STRPTR)" Name", CIF_WEIGHTED },
    { 20, (STRPTR)" Path", CIF_WEIGHTED },
    { -1, NULL, -1 }
};

#define SPACE LAYOUT_AddChild, SpaceObject, End

uint16 pageData[13] =
{
    //      Plane 0
    0x3FF8,0x2008,0x2008,0x3FF8,0x2008,0x2008,0x2008,0x2008,
    0x2008,0x2008,0x2008,0x2008,0x3FF8
};

struct Image chooser_image =
{
    0, 0,           // LeftEdge, TopEdge
    16, 13, 8,      // Width, Height, Depth
    pageData,       // ImageData
    0x0001, 0x0000, // PlanePick, PlaneOnOff
    NULL            // NextImage
};

#define MyItem(i) Item((STRPTR)i,0,i)

/**
**/
struct TagItem	lst2btn[] = {
    {LISTBROWSER_SelectedNode, GA_ReadOnly },
    {TAG_END, 0 }
};

static struct NewMenu menu[] =
{
    Title((STRPTR)MSG_Menu_Project),
        MyItem(MSG_Menu_About),
        ItemBar,
        MyItem(MSG_Menu_Hide),
        ItemBar,
        MyItem(MSG_Menu_Quit),
    Title((STRPTR)MSG_Menu_Prefs),
        MyItem(MSG_Menu_Save),
        MyItem(MSG_Menu_Use),
        ItemBar,
        MyItem(MSG_Menu_LastSaved),
        MyItem(MSG_Menu_Restore),
        MyItem(MSG_Menu_Defaults),
    EndMenu
};

void IDCMPFunc(struct Hook *hook,Object *wobj,struct IntuiMessage *Msg);
ULONG loadPrefs(ULONG mode);
ULONG storePrefs(BOOL bStorePrefs);
void updateFTPWindow(struct URL_FTPNode  * pFTP);

Object *make_window(void)
{
    Object
        *page1 = NULL,
        *page2 = NULL,
        *page3 = NULL,
        *page4 = NULL;

    OBJ(OBJ_HIDDEN_CHOOSER) = ChooserObject,
        GA_ID,                  OBJ_HIDDEN_CHOOSER,
        GA_RelVerify,           TRUE,
  //      CHOOSER_Labels,         &chooserlist,
        CHOOSER_DropDown,       TRUE,
        CHOOSER_AutoFit,        TRUE,
        CHOOSER_Hidden,         TRUE,
        ICA_TARGET,             ICTARGET_IDCMP,
    End;  // Chooser

    page1 = VLayoutObject,
        LAYOUT_AddChild,    HLayoutObject,

            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                LAYOUT_AddChild, OBJ(OBJ_LBROWSER_BROW) = ListBrowserObject,
                    GA_ID,                      OBJ_LBROWSER_BROW,
           //         GA_Immediate,               TRUE,
                    GA_RelVerify,               TRUE,
                    LISTBROWSER_AutoFit,        TRUE,
                    LISTBROWSER_HorizontalProp, TRUE,
                    LISTBROWSER_ShowSelected,   TRUE,
                    LISTBROWSER_Labels,         &list_Brow,
                    LISTBROWSER_ColumnInfo,     &BrowsColInfo,
                    LISTBROWSER_ColumnTitles,   TRUE,
                End,  // ListBrowser
            End,   // VLayout

            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                SPACE,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Add),OBJ_ADD_BROW),
                CHILD_WeightedHeight,   0,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Edit),OBJ_EDIT_BROW),
                CHILD_WeightedHeight,   0,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Clone),OBJ_CLONE_BROW),
                CHILD_WeightedHeight,   0,

                LAYOUT_AddChild,     HLayoutObject,
                    LAYOUT_AddChild,    OBJ(OBJ_UP_BROW) = ButtonObject,
                        GA_ID,              OBJ_UP_BROW,
                        BUTTON_AutoButton,  BAG_UPARROW,
                    End,  // Button
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild,    OBJ(OBJ_DOWN_BROW) = ButtonObject,
                        GA_ID,              OBJ_DOWN_BROW,
                        BUTTON_AutoButton,  BAG_DNARROW,
                    End,  // Button
                    CHILD_WeightedWidth,   0,
                End,   // HLayout
                CHILD_WeightedHeight,   0,

                SPACE,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Delete),OBJ_DELETE_BROW),
                CHILD_WeightedHeight,   0,
                SPACE,
            End,   // VLayout
            CHILD_WeightedWidth,   0,

        End,   // HLayout
    End;  // VLayout         // *** end of page 1 ***


    page2 = VLayoutObject,
        LAYOUT_AddChild,    HLayoutObject,

            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                LAYOUT_AddChild, OBJ(OBJ_LBROWSER_MAIL) = ListBrowserObject,
                    GA_ID,                      OBJ_LBROWSER_MAIL,
           //         GA_Immediate,               TRUE,
                    GA_RelVerify,               TRUE,
                    LISTBROWSER_AutoFit,        TRUE,
                    LISTBROWSER_HorizontalProp, TRUE,
                    LISTBROWSER_ShowSelected,   TRUE,
                    LISTBROWSER_Labels,         &list_Mail,
                    LISTBROWSER_ColumnInfo,     &MailColInfo,
                    LISTBROWSER_ColumnTitles,   TRUE,
                End,  // ListBrowser
            End,   // VLayout

            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                SPACE,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Add),OBJ_ADD_MAIL),
                CHILD_WeightedHeight,   0,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Edit),OBJ_EDIT_MAIL),
                CHILD_WeightedHeight,   0,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Clone),OBJ_CLONE_MAIL),
                CHILD_WeightedHeight,   0,

                LAYOUT_AddChild,     HLayoutObject,
                    LAYOUT_AddChild,    OBJ(OBJ_UP_MAIL) = ButtonObject,
                        GA_ID,              OBJ_UP_MAIL,
                        BUTTON_AutoButton,  BAG_UPARROW,
                    End,  // Button
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild,    OBJ(OBJ_DOWN_MAIL) = ButtonObject,
                        GA_ID,              OBJ_DOWN_MAIL,
                        BUTTON_AutoButton,  BAG_DNARROW,
                    End,  // Button
                    CHILD_WeightedWidth,   0,
                End,   // HLayout
                CHILD_WeightedHeight,   0,

                SPACE,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Delete),OBJ_DELETE_MAIL),
                CHILD_WeightedHeight,   0,
                SPACE,
            End,   // VLayout
            CHILD_WeightedWidth,   0,

        End,   // HLayout
    End;  // VLayout         // *** end of page 2 ***

    page3 = VLayoutObject,
        LAYOUT_AddChild,    HLayoutObject,

            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                LAYOUT_AddChild, OBJ(OBJ_LBROWSER_FTP) = ListBrowserObject,
                    GA_ID,                      OBJ_LBROWSER_FTP,
                    GA_RelVerify,               TRUE,
                    LISTBROWSER_AutoFit,        TRUE,
                    LISTBROWSER_ShowSelected,   TRUE,
                    LISTBROWSER_Labels,         &list_FTPs,
                    LISTBROWSER_ColumnInfo,     &FTPsColInfo,
                    LISTBROWSER_ColumnTitles,   TRUE,
                End,  // ListBrowser
            End,   // VLayout

            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                SPACE,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Add),OBJ_ADD_FTP),
                CHILD_WeightedHeight,   0,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Edit),OBJ_EDIT_FTP),
                CHILD_WeightedHeight,   0,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Clone),OBJ_CLONE_FTP),
                CHILD_WeightedHeight,   0,

                LAYOUT_AddChild,     HLayoutObject,
                    LAYOUT_AddChild,    OBJ(OBJ_UP_FTP) = ButtonObject,
                        GA_ID,              OBJ_UP_FTP,
                        BUTTON_AutoButton,  BAG_UPARROW,
                    End,  // Button
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild,    OBJ(OBJ_DOWN_FTP) = ButtonObject,
                        GA_ID,              OBJ_DOWN_FTP,
                        BUTTON_AutoButton,  BAG_DNARROW,
                    End,  // Button
                    CHILD_WeightedWidth,   0,
                End,   // HLayout
                CHILD_WeightedHeight,   0,

                SPACE,
                LAYOUT_AddChild, Button(getString(MSG_AppList_Delete),OBJ_DELETE_FTP),
                CHILD_WeightedHeight,   0,
                SPACE,
            End,   // VLayout
            CHILD_WeightedWidth,   0,

        End,   // HLayout
    End;  // VLayout     // *** end of page 3 ***


    page4 = VLayoutObject,

        LAYOUT_AddChild,  VLayoutObject,
            LAYOUT_BevelStyle,  BVS_GROUP,
            LAYOUT_Label,       getString(MSG_Misc_Defaults),

            LAYOUT_AddChild, OBJ(OBJ_UNICONIFY) = CheckBoxObject,
                GA_ID,               OBJ_UNICONIFY,
                GA_RelVerify,        TRUE,
                GA_Selected,         TRUE,
                GA_Text,             getString(MSG_Misc_Show),
            End,  // CheckBox

            LAYOUT_AddChild, OBJ(OBJ_BRING) = CheckBoxObject,
                GA_ID,               OBJ_BRING,
                GA_RelVerify,        TRUE,
                GA_Selected,         TRUE,
                GA_Text,             getString(MSG_Misc_Bring),
            End,  // CheckBox

            LAYOUT_AddChild, OBJ(OBJ_OPEN) = CheckBoxObject,
                GA_ID,               OBJ_OPEN,
                GA_RelVerify,        TRUE,
                GA_Selected,         FALSE,
                GA_Text,             getString(MSG_Misc_Open),
            End,  // CheckBox

            LAYOUT_AddChild, OBJ(OBJ_LAUNCH) = CheckBoxObject,
                GA_ID,               OBJ_LAUNCH,
                GA_RelVerify,        TRUE,
                GA_Selected,         TRUE,
                GA_Text,             getString(MSG_Misc_Launch),
            End,  // CheckBox
        End,  // VLayout

        LAYOUT_AddChild,  VLayoutObject,
            LAYOUT_BevelStyle,  BVS_GROUP,
            LAYOUT_Label,       getString(MSG_Misc_Options),

            LAYOUT_AddChild, OBJ(OBJ_PREPEND) = CheckBoxObject,
                GA_ID,               OBJ_PREPEND,
                GA_RelVerify,        TRUE,
                GA_Selected,         TRUE,
                GA_Text,             getString(MSG_Misc_Prepend),
            End,  // CheckBox

            LAYOUT_AddChild, OBJ(OBJ_SEND_MAILTO) = CheckBoxObject,
                GA_ID,               OBJ_SEND_MAILTO,
                GA_RelVerify,        TRUE,
                GA_Selected,         TRUE,
                GA_Text,             getString(MSG_Misc_UseMailer),
            End,  // CheckBox

            LAYOUT_AddChild, OBJ(OBJ_SEND_FTP) = CheckBoxObject,
                GA_ID,               OBJ_SEND_FTP,
                GA_RelVerify,        TRUE,
                GA_Selected,         FALSE,
                GA_Text,             getString(MSG_Misc_UseFTP),
            End,  // CheckBox

        End,  // VLayout
    End;  // VLayout      // *** end of page 4 ***


    OBJ(OBJ_CLICKTAB) = ClickTabObject,
        GA_Text,            PageLabels,
        CLICKTAB_Current,   0,  // page to open with
        CLICKTAB_PageGroup, PageObject,
            PAGE_Add,       page1,
            PAGE_Add,       page2,
            PAGE_Add,       page3,
            PAGE_Add,       page4,
        PageEnd,
    ClickTabEnd;


    return WindowObject,
        WA_ScreenTitle,        getString(MSG_App_ScreenTitle),
        WA_Title,              getString(MSG_Win_WinTitle),
        WA_DragBar,            TRUE,
        WA_CloseGadget,        TRUE,
        WA_SizeGadget,         TRUE,
        WA_DepthGadget,        TRUE,
        WA_Activate,           TRUE,
        WINDOW_IconifyGadget,  TRUE,
        WINDOW_Icon,           IIcon->GetDiskObject("PROGDIR:OpenURL"),
        WINDOW_IconTitle,      getString(MSG_Win_WinTitle),
        WINDOW_AppPort,        AppPort,
        WINDOW_SharedPort,     AppPort,
        WINDOW_Position,       WPOS_CENTERSCREEN,
        WINDOW_NewMenu,        menu,
        WINDOW_Layout,         VLayoutObject,

            LAYOUT_AddChild,       OBJ(OBJ_CLICKTAB),

            LAYOUT_AddChild,        HLayoutObject,
                LAYOUT_BevelStyle,  BVS_SBAR_VERT,
            End,   // HLayout
            CHILD_WeightedHeight,   0,

            LAYOUT_AddChild,        HLayoutObject,
                LAYOUT_AddChild,    SpaceObject,
                    SPACE_MinWidth,    2,
                End,  // Space
                CHILD_WeightedWidth,   0,

                LAYOUT_AddChild,        HLayoutObject,
                    LAYOUT_EvenSize,    TRUE,
                    LAYOUT_AddChild, Button(getString(MSG_Win_Save),OBJ_SAVE),
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild, Button(getString(MSG_Win_Use),OBJ_USE),
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild, Button(getString(MSG_Win_Apply),OBJ_APPLY),
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild, Button(getString(MSG_Win_Cancel),OBJ_CANCEL),
                    CHILD_WeightedWidth,   0,
                End,   // HLayout
                CHILD_WeightedHeight,   0,

                LAYOUT_AddChild,    SpaceObject,
                    SPACE_MinWidth,    2,
                End,  // Space
                CHILD_WeightedWidth,   0,

            End,   // HLayout
            CHILD_MinWidth, 350,  // sets a more attractive size for the whole Layout
            CHILD_WeightedHeight,   0,

        End,   // VLayout
    WindowEnd;
}


int main()
{
    initStrings();

    localizeStrings(PageLabels);

    localizeNewMenu(menu);

    if(!(OpenURLBase = IExec->OpenLibrary(OPENURLNAME, OPENURLVER)))
         return -1;
    if(!(IOpenURL = (struct OpenURLIFace*)IExec->GetInterface(OpenURLBase, "main", 1L, NULL)))
        return -1;

    RA_SetUpHook(idcmphook, IDCMPFunc, NULL);

    if((AppPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE)) != NULL)
    {
        IExec->NewList(&list_Brow);
        IExec->NewList(&list_Mail);
        IExec->NewList(&list_FTPs);

        win = make_window();
        edit_brow_win = make_edit_brow_win();
        edit_mail_win = make_edit_mail_win();
        edit_ftp_win = make_edit_ftp_win();

        loadPrefs(URL_GetPrefs_Mode_InUse);

        // Set up inter-group label alignment

        iset(OBJ(OBJ_FTP_ALIGN1), LAYOUT_AlignLabels, OBJ(OBJ_FTP_ALIGN2));
        iset(OBJ(OBJ_MAIL_ALIGN1), LAYOUT_AlignLabels, OBJ(OBJ_MAIL_ALIGN2));
        iset(OBJ(OBJ_LBROWSER_BROW), ICA_TARGET, OBJ(OBJ_EDIT_BROW),
                                     ICA_MAP,    lst2btn);

        if((window = RA_OpenWindow(win)) != NULL)
        {
            uint32 sigmask;
            BOOL done = FALSE;

            sigmask = iget(win, WINDOW_SigMask);
            while (!done)
            {
                uint32 siggot;

                siggot = IExec->Wait(sigmask);
                if (siggot & sigmask)
                {
                    done = HandleInput_Main_Win();
                    HandleInput_Edit_Brow_Win();
                    HandleInput_Edit_Mail_Win();
                    HandleInput_Edit_FTP_Win();
                }
            }
        }
        IIntuition->DisposeObject(edit_ftp_win);
        IIntuition->DisposeObject(edit_mail_win);
        IIntuition->DisposeObject(edit_brow_win);
        IIntuition->DisposeObject(win);

        //  The hidden chooser isn't attached to anything,
        //  so we must dispose of it ourselves...

        IIntuition->DisposeObject(OBJ(OBJ_HIDDEN_CHOOSER));

        IListBrowser->FreeListBrowserList(&list_FTPs);
        IListBrowser->FreeListBrowserList(&list_Mail);
        IListBrowser->FreeListBrowserList(&list_Brow);
        IExec->FreeSysObject(ASOT_PORT, AppPort);
    }

    IExec->DropInterface((struct Interface*)IOpenURL);
    IExec->CloseLibrary(OpenURLBase);

    uninitStrings();

    return 0;
}

void IDCMPFunc(UNUSED struct Hook *hook, UNUSED Object *wobj, struct IntuiMessage *Msg)
{
    //struct Window *window = Msg->IDCMPWindow;
    uint32 active;

    if (Msg->Class == IDCMP_IDCMPUPDATE)
    {
        if (IUtility->GetTagData(GA_ID, 0, Msg->IAddress) == OBJ_HIDDEN_CHOOSER)
        {
            active = IUtility->GetTagData(CHOOSER_Active, -1, Msg->IAddress);
   //         printf("chooser picked = %d\n",active);

 //  find out which button was clicked, to replace OBJ_STRING with the correct String gad

   //         gadset(GAD(OBJ_STRING), window, NULL,
   //             GA_Text, hidden_strings[active], TAG_END);
        }
    }
}

ULONG loadPrefs(ULONG mode)
{
    struct URL_Prefs       *p;
    ULONG                  error = 0;

    /* get the openurl.library prefs */
/*    switch(mode)
    {
        case MUIV_Win_GetPrefs_InUse:      mode = URL_GetPrefs_Mode_InUse;   break;
        case MUIV_Win_GetPrefs_LastSaveds: mode = URL_GetPrefs_Mode_Envarc;  break;
        case MUIV_Win_GetPrefs_Restore:    mode = URL_GetPrefs_Mode_Env;     break;
        case MUIV_Win_GetPrefs_Defaults:   mode = URL_GetPrefs_Mode_Default; break;
        default: return FALSE;
    }
*/
    p = IOpenURL->URL_GetPrefs(URL_GetPrefs_Mode,mode,TAG_DONE);
    if (!p) error = MSG_Err_NoPrefs;
    else if (p->up_Version!=PREFS_VERSION) error = MSG_Err_BadPrefs;

    if (error)
    {
        RA_Request(NULL,0,getString(MSG_ErrReqTitle),
                          getString(MSG_ErrReqGadget),
                          getString(error),
                          p ? p->up_Version : 0);

        if (p) IOpenURL->URL_FreePrefsA(p,NULL);

        return FALSE;
    }

    /* Browsers */
	gadset(GAD(OBJ_LBROWSER_BROW), window, LISTBROWSER_Labels, ~0);
    updateBrowserList(&list_Brow, p->up_BrowserList);
	gadset(GAD(OBJ_LBROWSER_BROW), window, LISTBROWSER_Labels, &list_Brow, LISTBROWSER_AutoFit, TRUE);

    /* Mailers */
    gadset(GAD(OBJ_LBROWSER_MAIL), window, LISTBROWSER_Labels, ~0, TAG_DONE);
    updateMailerList(&list_Mail, p->up_MailerList);
    gadset(GAD(OBJ_LBROWSER_MAIL), window, LISTBROWSER_Labels, &list_Mail, LISTBROWSER_AutoFit, TRUE);

    /* FTPs */
	gadset(GAD(OBJ_LBROWSER_FTP), window, LISTBROWSER_Labels, ~0, TAG_DONE);
    updateFTPList(&list_FTPs, p->up_FTPList);
	gadset(GAD(OBJ_LBROWSER_FTP), window, LISTBROWSER_Labels, &list_FTPs, LISTBROWSER_AutoFit, TRUE);

    /* Miscellaneous */
    gadset(GAD(OBJ_PREPEND), window, GA_Selected, isFlagSet(p->up_Flags, UPF_PREPENDHTTP));
    gadset(GAD(OBJ_SEND_MAILTO), window, GA_Selected, isFlagSet(p->up_Flags, UPF_DOMAILTO));
    gadset(GAD(OBJ_SEND_FTP), window, GA_Selected, isFlagSet(p->up_Flags, UPF_DOFTP));

    gadset(GAD(OBJ_UNICONIFY), window, GA_Selected, p->up_DefShow);
    gadset(GAD(OBJ_BRING), window, GA_Selected, p->up_DefBringToFront);
    gadset(GAD(OBJ_OPEN), window, GA_Selected, p->up_DefNewWindow);
    gadset(GAD(OBJ_LAUNCH), window, GA_Selected, p->up_DefLaunch);

    /* free the preferences */
    IOpenURL->URL_FreePrefsA(p,NULL);

    return TRUE;
}

ULONG storePrefs(BOOL bStorePrefs)
{
    struct URL_Prefs up;

    /* Copy settings from gadgets into structure */
    up.up_Version = PREFS_VERSION;

    /* Browsers */
    IExec->CopyMem(&list_Brow, &up.up_BrowserList, sizeof(struct MinList));

    /* Mailers */
    IExec->CopyMem(&list_Mail, &up.up_MailerList, sizeof(struct MinList));

    /* FTPs */
    IExec->CopyMem(&list_FTPs, &up.up_FTPList, sizeof(struct MinList));

    /* Miscellaneous */
    if(iget(OBJ(OBJ_PREPEND), GA_Selected))
        SET_FLAG(up.up_Flags, UPF_PREPENDHTTP);
    else
        CLEAR_FLAG(up.up_Flags, UPF_PREPENDHTTP);

    if(iget(OBJ(OBJ_SEND_MAILTO), GA_Selected))
        SET_FLAG(up.up_Flags, UPF_DOMAILTO);
    else
        CLEAR_FLAG(up.up_Flags, UPF_DOMAILTO);

    if(iget(OBJ(OBJ_SEND_FTP), GA_Selected))
        SET_FLAG(up.up_Flags, UPF_DOFTP);
    else
        CLEAR_FLAG(up.up_Flags, UPF_DOFTP);

    if(iget(OBJ(OBJ_SEND_MAILTO), GA_Selected))
        SET_FLAG(up.up_Flags, UPF_DOMAILTO);
    else
        CLEAR_FLAG(up.up_Flags, UPF_DOMAILTO);

    if(iget(OBJ(OBJ_SEND_FTP), GA_Selected))
        SET_FLAG(up.up_Flags, UPF_DOFTP);
    else
        CLEAR_FLAG(up.up_Flags, UPF_DOFTP);

    up.up_DefShow = iget(OBJ(OBJ_UNICONIFY), GA_Selected);
    up.up_DefBringToFront = iget(OBJ(OBJ_BRING), GA_Selected);
    up.up_DefNewWindow = iget(OBJ(OBJ_OPEN), GA_Selected);
    up.up_DefLaunch = iget(OBJ(OBJ_LAUNCH), GA_Selected);

    /* Save to disk */
    if (!IOpenURL->URL_SetPrefs(&up,URL_SetPrefs_Save,bStorePrefs,TAG_DONE))
        RA_Request((Object *)window,getString(MSG_ErrReqTitle),getString(MSG_ErrReqGadget),getString(MSG_Err_FailedSave),NULL);

    return TRUE;
}
