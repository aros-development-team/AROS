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

#include "ftps.h"

#include "gui_global.h"
#include "utility.h"
#include "macros.h"

#include <classes/window.h>
#include <libraries/openurl.h>

#include <reaction/reaction_macros.h>

#include <images/label.h>
#include <gadgets/space.h>

#include <gadgets/getfile.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/label.h>
#include <proto/space.h>
#include <proto/layout.h>
#include <proto/window.h>
#include <proto/string.h>
#include <proto/getfile.h>
#include <proto/checkbox.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/listbrowser.h>

Object *edit_ftp_win;
struct Window *edit_ftp_window;

Object * make_edit_ftp_win(void)
{
    return WindowObject,
        WA_ScreenTitle,        getString(MSG_App_ScreenTitle),
        WA_Title,              getString(MSG_FTP_WinTitle),
        WA_DragBar,            TRUE,
        WA_CloseGadget,        TRUE,
        WA_SizeGadget,         TRUE,
        WA_DepthGadget,        TRUE,
        WA_Activate,           TRUE,
   //     WINDOW_AppPort,        AppPort,
        WINDOW_SharedPort,     AppPort,
        WINDOW_Position,       WPOS_CENTERSCREEN,
        WINDOW_LockHeight,     TRUE,
        WINDOW_Layout,         VLayoutObject,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_AddChild,  OBJ(OBJ_FTP_ALIGN1) = VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                LAYOUT_BevelStyle,  BVS_GROUP,
                LAYOUT_Label,       getString(MSG_Edit_Definitions),

                LAYOUT_AddChild,   OBJ(OBJ_FTP_NAME_STR) = StringObject,
                    GA_ID,               OBJ_FTP_NAME_STR,
                    GA_RelVerify,        TRUE,
                    GA_TabCycle,         TRUE,
             //           STRINGA_Buffer,        buffer,
                End,  // String
                Label(getString(MSG_Edit_Name)),

                LAYOUT_AddChild,  HLayoutObject,
                    LAYOUT_SpaceInner,     FALSE,
                    LAYOUT_AddChild,       OBJ(OBJ_FTP_PATH_GET) = GetFileObject,
                        GA_ID,                 OBJ_FTP_PATH_GET,
                        GA_RelVerify,          TRUE,
          //              GETFILE_TitleText,     "Select Path To Browser",
                    End,  // GetFile
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_PATH_CHOOSE) = ButtonObject,
                        GA_ID,              OBJ_FTP_PATH_CHOOSE,
                        GA_RelVerify,       TRUE,
                        GA_Image,           &chooser_image,
                    End,  // Button
                    CHILD_WeightedWidth, 0,
                End,   // HLayout
                Label(getString(MSG_Edit_Path)),

                LAYOUT_AddChild,  HLayoutObject,
                    LAYOUT_SpaceInner,  FALSE,
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_AREXX_STR) = StringObject,
                        GA_ID,               OBJ_FTP_AREXX_STR,
                        GA_RelVerify,        TRUE,
                        GA_TabCycle,         TRUE,
             //           STRINGA_Buffer,        buffer,
                    End,  // String
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_AREXX_CHOOSE) = ButtonObject,
                        GA_ID,              OBJ_FTP_AREXX_CHOOSE,
                        GA_RelVerify,       TRUE,
                        GA_Image,           &chooser_image,
                    End,  // Button
                    CHILD_WeightedWidth, 0,
                End,   // HLayout
                Label(getString(MSG_Edit_Port)),

                LAYOUT_AddChild,  HLayoutObject,
                    LAYOUT_AddChild, OBJ(OBJ_FTP_REMOVE) = CheckBoxObject,
                        GA_ID,               OBJ_FTP_REMOVE,
                        GA_RelVerify,        TRUE,
                        GA_Selected,         TRUE,
                    End,  // CheckBox
                End,   // HLayout
                Label(getString(MSG_FTP_RemoveURLQualifier)),
            End,   // VLayout


            LAYOUT_AddChild,  OBJ(OBJ_FTP_ALIGN2) = VLayoutObject,
                LAYOUT_SpaceOuter,  TRUE,
                LAYOUT_BevelStyle,  BVS_GROUP,
                LAYOUT_Label,       getString(MSG_Edit_ARexx),

                LAYOUT_AddChild,     OBJ(OBJ_FTP_SHOW_STR) = StringObject,
                    GA_ID,                 OBJ_FTP_SHOW_STR,
                    GA_RelVerify,          TRUE,
                    GA_TabCycle,           TRUE,
         //           STRINGA_Buffer,        buffer,
                End,  // String
                Label(getString(MSG_Edit_Show)),

                LAYOUT_AddChild,   OBJ(OBJ_FTP_FRONT_STR) = StringObject,
                    GA_ID,               OBJ_FTP_FRONT_STR,
                    GA_RelVerify,        TRUE,
                    GA_TabCycle,         TRUE,
             //           STRINGA_Buffer,        buffer,
                End,  // String
                Label(getString(MSG_Edit_Screen)),

                LAYOUT_AddChild,    HLayoutObject,
                    LAYOUT_SpaceInner,  FALSE,
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_OPEN_STR) = StringObject,
                        GA_ID,              OBJ_FTP_OPEN_STR,
                        GA_RelVerify,       TRUE,
                        GA_TabCycle,        TRUE,
             //           STRINGA_Buffer,        buffer,
                    End,  // String
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_OPEN_CHOOSE) = ButtonObject,
                            GA_ID,          OBJ_FTP_OPEN_CHOOSE,
                            GA_RelVerify,   TRUE,
                            GA_Image,       &chooser_image,
                        End,  // Button
                        CHILD_WeightedWidth, 0,
                    End,   // HLayout
                Label(getString(MSG_Edit_OpenURL)),

                LAYOUT_AddChild,    HLayoutObject,
                    LAYOUT_SpaceInner,  FALSE,
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_NEW_STR) = StringObject,
                        GA_ID,               OBJ_FTP_NEW_STR,
                        GA_RelVerify,        TRUE,
                        GA_TabCycle,         TRUE,
             //           STRINGA_Buffer,        buffer,
                    End,  // String
                    LAYOUT_AddChild,    OBJ(OBJ_FTP_NEW_CHOOSE) = ButtonObject,
                            GA_ID,          OBJ_FTP_NEW_CHOOSE,
                            GA_RelVerify,   TRUE,
                            GA_Image,       &chooser_image,
                        End,  // Button
                        CHILD_WeightedWidth, 0,
                End,   // HLayout
                Label(getString(MSG_Edit_NewWin)),
            End,   // VLayout

            LAYOUT_AddChild,    HLayoutObject,
                LAYOUT_AddChild,    SpaceObject,
                    SPACE_MinWidth,    2,
                End,  // Space
                CHILD_WeightedWidth,   0,

                LAYOUT_AddChild,     HLayoutObject,
                    LAYOUT_EvenSize,     TRUE,
                    LAYOUT_AddChild,     Button(getString(MSG_Edit_Use),OBJ_FTP_USE),
                    CHILD_WeightedWidth,   0,

                    LAYOUT_AddChild, Button(getString(MSG_Edit_Cancel),OBJ_FTP_CANCEL),
                    CHILD_WeightedWidth,   0,
                End,   // HLayout

                LAYOUT_AddChild,    SpaceObject,
                    SPACE_MinWidth,    2,
                End,  // Space
                CHILD_WeightedWidth,   0,
            End,   // HLayout
            CHILD_MinWidth, 300,  // sets a more attractive size for the whole Layout

        End,   // VLayout
    WindowEnd;
}

BOOL updateFTPList(struct List * list, struct MinList PrefsFTPList)
{
    struct URL_FTPNode  *   fn      = NULL,
                        *   newNode = NULL;

    // libération de la liste
    freeList(list);

    // ajout des nouvelles données
    for (fn = (struct URL_FTPNode *)PrefsFTPList.mlh_Head;
         fn->ufn_Node.mln_Succ;
         fn = (struct URL_FTPNode *)fn->ufn_Node.mln_Succ)
    {
        if((newNode = (struct URL_FTPNode*)IListBrowser->AllocListBrowserNode(3,
            LBNA_NodeSize,  sizeof(struct URL_FTPNode),
            LBNA_CheckBox,  TRUE,
            LBNA_Checked,   isFlagClear(fn->ufn_Flags, UNF_DISABLED),
            LBNA_Column,    1,
            LBNCA_Text, "",
            LBNA_Column, 2,
            LBNCA_Text, "",
            TAG_DONE)) != NULL)
        {
            IExec->CopyMem(fn, newNode, sizeof(struct URL_FTPNode));
            IListBrowser->SetListBrowserNodeAttrs((struct Node*)newNode, LBNA_Column,    1,
                                                            LBNCA_Text,     newNode->ufn_Name,
                                                            LBNA_Column,    2,
                                                            LBNCA_Text,     newNode->ufn_Path,
                                                            TAG_END);

            IIntuition->SetAttrs(edit_ftp_win,  WINDOW_UserData, fn, TAG_DONE);
            IExec->AddTail(list, (struct Node*)newNode);
        }
        else
        {
            IDOS->Printf(" AllocListBrowserNode() failed\n");
            return(FALSE);
        }
    }

    return TRUE;
}

void updateFTPWindow(struct URL_FTPNode  * pFTP)
{
    if(pFTP != NULL)
    {
        gadset(GAD(OBJ_FTP_NAME_STR), edit_ftp_window, STRINGA_TextVal, pFTP->ufn_Name);
        gadset(GAD(OBJ_FTP_PATH_GET), edit_ftp_window, GETFILE_File, pFTP->ufn_Path);
        gadset(GAD(OBJ_FTP_AREXX_STR), edit_ftp_window, STRINGA_TextVal, pFTP->ufn_Port);
        gadset(GAD(OBJ_FTP_REMOVE), edit_ftp_window, GA_Selected, isFlagSet(pFTP->ufn_Flags, UFNF_REMOVEFTP));
        gadset(GAD(OBJ_FTP_SHOW_STR), edit_ftp_window, STRINGA_TextVal, pFTP->ufn_ShowCmd);
        gadset(GAD(OBJ_FTP_FRONT_STR), edit_ftp_window, STRINGA_TextVal, pFTP->ufn_ToFrontCmd);
        gadset(GAD(OBJ_FTP_OPEN_STR), edit_ftp_window, STRINGA_TextVal, pFTP->ufn_OpenURLCmd);
        gadset(GAD(OBJ_FTP_NEW_STR), edit_ftp_window, STRINGA_TextVal, pFTP->ufn_OpenURLWCmd);
    }
}

void updateFTPNode()
{
    struct URL_FTPNode *pFTP;

    if((pFTP = (struct URL_FTPNode *)iget(edit_ftp_win, WINDOW_UserData)) != NULL)
    {
        STRPTR strValue;

        strValue = (STRPTR)iget(GAD(OBJ_FTP_NAME_STR), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_Name, strValue, sizeof(pFTP->ufn_Name));
        strValue = (STRPTR)iget(GAD(OBJ_FTP_PATH_GET), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_Path, strValue, sizeof(pFTP->ufn_Path));
        strValue = (STRPTR)iget(GAD(OBJ_FTP_AREXX_STR), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_Port, strValue, sizeof(pFTP->ufn_Port));
        strValue = (STRPTR)iget(GAD(OBJ_FTP_SHOW_STR), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_ShowCmd, strValue, sizeof(pFTP->ufn_ShowCmd));
        strValue = (STRPTR)iget(GAD(OBJ_FTP_FRONT_STR), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_ToFrontCmd, strValue, sizeof(pFTP->ufn_ToFrontCmd));
        strValue = (STRPTR)iget(GAD(OBJ_FTP_OPEN_STR), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_OpenURLCmd, strValue, sizeof(pFTP->ufn_OpenURLCmd));
        strValue = (STRPTR)iget(GAD(OBJ_FTP_NEW_STR), STRINGA_TextVal);
        IUtility->Strlcpy(pFTP->ufn_OpenURLWCmd, strValue, sizeof(pFTP->ufn_OpenURLWCmd));

        // now update the ListBrowser attributes
        IListBrowser->SetListBrowserNodeAttrs((struct Node*)pFTP,  LBNA_Column,    1,
                                               LBNCA_Text,             pFTP->ufn_Name,
                                               LBNA_Column,            2,
                                               LBNCA_Text,             pFTP->ufn_Path,
                                               TAG_END);
    }
}

