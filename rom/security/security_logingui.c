/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library graphical login (Zune LoginWindow.mcc)

          Used by secLoginA() when secT_Graphical is set and a screen exists.
          muimaster.library is disk based, so it is opened here on demand and
          closed again; nothing MUI-related is referenced at library init.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <libraries/mui.h>
#include <zune/loginwindow.h>
#include <string.h>

#include "security_intern.h"
#include "security_login.h"
#include "security_memory.h"

static void CopyStr(STRPTR dst, ULONG size, CONST_STRPTR src)
{
    if (src == NULL)
        src = "";
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

/*
 * Put up the login window and wait for the user.
 *   prompt    - text shown above the inputs (already localised/formatted)
 *   cancelok  - TRUE: the user may cancel (secLoginA), FALSE: must log in
 *   uid       - in: preset user id (may be empty), out: the entered user id
 *   pwd       - out: the entered password (may be empty)
 * Returns FALSE when cancelled or when MUI is not available.
 */
BOOL LoginGUI(struct SecurityBase *secBase, CONST_STRPTR pubscreen, CONST_STRPTR prompt, BOOL cancelok,
              STRPTR uid, ULONG uidsize, STRPTR pwd, ULONG pwdsize)
{
    struct Library *MUIMasterBase;
    Object *app = NULL, *win;
    BOOL ok = FALSE;
    struct TagItem wintags[] =
    {
        { MUIA_LoginWindow_UserName,        (IPTR)uid                                       },
        { MUIA_LoginWindow_Prompt,          (IPTR)prompt                                    },
        { MUIA_LoginWindow_Cancel_Disabled, cancelok ? FALSE : TRUE                         },
        { MUIA_LoginWindow_Method_Status,   LWA_METH_None       /* local logins only */     },
        { pubscreen ? MUIA_Window_PublicScreen : TAG_IGNORE, (IPTR)pubscreen                },
        { TAG_DONE,                         0                                               }
    };
    struct TagItem apptags[] =
    {
        { MUIA_Application_Title,       (IPTR)"Login"                   },
        { MUIA_Application_Base,        (IPTR)"LOGIN"                   },
        { MUIA_Application_SingleTask,  FALSE                           },
        { MUIA_Application_Window,      0                               },
        { TAG_DONE,                     0                               }
    };

    if (!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
        return FALSE;

    if ((win = MUI_NewObjectA(MUIC_LoginWindow, wintags)))
    {
        apptags[3].ti_Data = (IPTR)win;
        if ((app = MUI_NewObjectA(MUIC_Application, apptags)))
        {
            ULONG sigs = 0;
            LONG id;

            set(win, MUIA_Window_Open, TRUE);
            if (XGET(win, MUIA_Window_Open))
            {
                while ((id = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)) != LWA_RV_CANCEL)
                {
                    if (id == LWA_RV_OK)
                    {
                        STRPTR s;

                        s = (STRPTR)XGET(win, MUIA_LoginWindow_UserName);
                        if (s == NULL || s[0] == '\0')
                            continue;       /* no user id: keep asking */
                        CopyStr(uid, uidsize, s);
                        s = (STRPTR)XGET(win, MUIA_LoginWindow_UserPass);
                        CopyStr(pwd, pwdsize, s);
                        ok = TRUE;
                        break;
                    }
                    if (sigs)
                    {
                        sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                        if (sigs & SIGBREAKF_CTRL_C)
                            break;
                    }
                }
                set(win, MUIA_Window_Open, FALSE);
            }
            MUI_DisposeObject(app);         /* disposes the window too */
        }
        else
            MUI_DisposeObject(win);
    }

    CloseLibrary(MUIMasterBase);
    return ok;
}

/* A modal message (login failed etc.) in the same look */
void LoginMessageGUI(struct SecurityBase *secBase, CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadgets)
{
    struct Library *MUIMasterBase;

    if ((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        MUI_RequestA(NULL, NULL, 0, (CONST_STRPTR)title, (CONST_STRPTR)gadgets, (CONST_STRPTR)text, NULL);
        CloseLibrary(MUIMasterBase);
    }
    else
    {
        struct EasyStruct es = { sizeof(struct EasyStruct), 0, (STRPTR)title, "%s", (STRPTR)gadgets };
        SIPTR args[1] = { (SIPTR)text };

        EasyRequestArgs(NULL, &es, NULL, (RAWARG)args);
    }
}
