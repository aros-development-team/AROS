/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library graphical login (Zune LoginWindow.mcc)

          Used by secLoginA() when secT_Graphical is set and a screen exists.
          muimaster.library is disk based, so it is opened here on demand and
          closed again; nothing MUI-related is referenced at library init.
*/

#include <exec/pm.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <intuition/screens.h>
#include <libraries/mui.h>
#include <zune/loginwindow.h>
#include <string.h>

#include "security_intern.h"
#include "security_login.h"
#include "security_memory.h"

/*
 * S:Security-Startup makes the LIBS: assign itself before a graphical login.
 * Should it be missing anyway, try muimaster.library by its full path.
 */
static struct Library *OpenMUIMaster(struct SecurityBase *secBase)
{
    struct Library *mb = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN);

    if (mb == NULL)
    {
        BPTR lock = Lock("LIBS:", ACCESS_READ);

        if (lock)
            UnLock(lock);
        else
            mb = OpenLibrary("SYS:Libs/" MUIMASTER_NAME, MUIMASTER_VMIN);
    }
    return mb;
}

#define SYSTEM_SCREEN_NAME "SYSTEM"

/*
 * The boot login runs on the black "SYSTEM" public screen that dos/boot.c
 * opens around S:Security-Startup (it is the default public screen then, so
 * requesters land on it too). Outside the boot there is a normal Workbench;
 * the window just opens wherever MUI puts it.
 */
static BOOL SystemScreenPresent(struct SecurityBase *secBase)
{
    struct Screen *scr = LockPubScreen(SYSTEM_SCREEN_NAME);

    if (scr == NULL)
        return FALSE;
    UnlockPubScreen(NULL, scr);
    return TRUE;
}

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
 *   systemmode- the boot login: Shutdown/Reboot buttons instead of Cancel
 *   uid       - in: preset user id (may be empty), out: the entered user id
 *   pwd       - out: the entered password (may be empty)
 * Returns LOGINGUI_OK, LOGINGUI_CANCEL, or LOGINGUI_UNAVAILABLE when MUI
 * cannot be used (the caller then falls back to the console).
 */
LONG LoginGUI(struct SecurityBase *secBase, CONST_STRPTR pubscreen, CONST_STRPTR prompt, BOOL cancelok, BOOL systemmode,
              STRPTR uid, ULONG uidsize, STRPTR pwd, ULONG pwdsize)
{
    struct Library *MUIMasterBase;
    Object *app = NULL, *win;
    LONG ok = LOGINGUI_CANCEL;
    struct TagItem wintags[] =
    {
        { MUIA_LoginWindow_UserName,        (IPTR)uid                                       },
        { MUIA_LoginWindow_Prompt,          (IPTR)prompt                                    },
        { MUIA_LoginWindow_Cancel_Disabled, cancelok ? FALSE : TRUE                         },
        { MUIA_LoginWindow_Method_Status,   LWA_METH_None       /* local logins only */     },
        { MUIA_LoginWindow_SystemMode,      systemmode ? TRUE : FALSE                       },
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

    if (!(MUIMasterBase = OpenMUIMaster(secBase)))
        return LOGINGUI_UNAVAILABLE;

    if (systemmode && pubscreen == NULL && SystemScreenPresent(secBase))
        pubscreen = SYSTEM_SCREEN_NAME;
    wintags[5].ti_Tag  = pubscreen ? MUIA_Window_PublicScreen : TAG_IGNORE;
    wintags[5].ti_Data = (IPTR)pubscreen;

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
                        ok = LOGINGUI_OK;
                        break;
                    }
                    if (id == LWA_RV_SHUTDOWN || id == LWA_RV_REBOOT)
                    {
                        /* system mode: the machine goes down instead of logging in */
                        set(win, MUIA_Window_Open, FALSE);
                        if (id == LWA_RV_REBOOT)
                            ShutdownA(SD_ACTION_REBOOT);
                        else
                            ShutdownA(SD_ACTION_POWEROFF);
                        /* not supported on this machine: keep asking */
                        set(win, MUIA_Window_Open, TRUE);
                        continue;
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
            else
                ok = LOGINGUI_UNAVAILABLE;  /* no screen to open on: console */
            MUI_DisposeObject(app);         /* disposes the window too */
        }
        else
        {
            MUI_DisposeObject(win);
            ok = LOGINGUI_UNAVAILABLE;
        }
    }
    else
        ok = LOGINGUI_UNAVAILABLE;      /* LoginWindow.mcc not found */

    CloseLibrary(MUIMasterBase);
    return ok;
}

/* A modal message (login failed etc.) in the same look */
void LoginMessageGUI(struct SecurityBase *secBase, CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadgets)
{
    struct Library *MUIMasterBase;

    if ((MUIMasterBase = OpenMUIMaster(secBase)))
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

/*
 * While a hardware token is waited for: a window with the status text and
 * a Cancel button, in the login's look. The status hook calls this again
 * and again, so the window's input is handled here one round at a time.
 */
BOOL StatusGUIShow(struct SecurityBase *secBase, struct LoginStatusGUI *s, CONST_STRPTR pubscreen, BOOL systemmode,
                   CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR cancel, BOOL *unavailable)
{
    struct Library *MUIMasterBase = s->MUIMasterBase;
    ULONG sigs = 0;
    LONG id;

    *unavailable = FALSE;
    if (s->Cancelled)
        return FALSE;
    if (!MUIMasterBase)
    {
        if (!(MUIMasterBase = s->MUIMasterBase = OpenMUIMaster(secBase)))
        {
            *unavailable = TRUE;
            return TRUE;
        }
    }
    if (!s->Win)
    {
        /* the varargs MUI_NewObject() is a link-library stub that would make
         * this ROM module autoopen muimaster.library: tag arrays, as above */
        Object *btn, *group = NULL, *space;
        struct TagItem texttags[] =
        {
            { MUIA_Text_Contents,       (IPTR)text      },
            { MUIA_Text_PreParse,       (IPTR)"\33c"    },
            { MUIA_Text_SetMax,         FALSE           },
            { TAG_DONE,                 0               }
        };
        struct TagItem buttontags[] = { { MUIA_Text_Contents, (IPTR)cancel }, { TAG_DONE, 0 } };
        struct TagItem spacetags[] = { { MUIA_FixHeight, 8 }, { TAG_DONE, 0 } };
        struct TagItem grouptags[] =
        {
            { MUIA_Frame,               MUIV_Frame_None },
            { MUIA_Group_Child,         0               },
            { MUIA_Group_Child,         0               },
            { MUIA_Group_Child,         0               },
            { TAG_DONE,                 0               }
        };
        struct TagItem wintags[] =
        {
            { MUIA_Window_Title,        (IPTR)title     },
            { MUIA_Window_CloseGadget,  FALSE           },
            { MUIA_Window_SizeGadget,   FALSE           },
            { MUIA_Window_DepthGadget,  FALSE           },
            { TAG_IGNORE,               0               },
            { MUIA_Window_RootObject,   0               },
            { TAG_DONE,                 0               }
        };
        struct TagItem apptags[] =
        {
            { MUIA_Application_Title,       (IPTR)"Login"   },
            { MUIA_Application_Base,        (IPTR)"LOGIN"   },
            { MUIA_Application_SingleTask,  FALSE           },
            { MUIA_Application_Window,      0               },
            { TAG_DONE,                     0               }
        };

        if (systemmode && pubscreen == NULL && SystemScreenPresent(secBase))
            pubscreen = SYSTEM_SCREEN_NAME;
        s->Text = MUI_NewObjectA(MUIC_Text, texttags);
        btn = MUI_MakeObjectA(MUIO_Button, (IPTR *)&buttontags[0].ti_Data);
        space = MUI_NewObjectA(MUIC_Rectangle, spacetags);
        if (s->Text && btn && space)
        {
            grouptags[1].ti_Data = (IPTR)s->Text;
            grouptags[2].ti_Data = (IPTR)space;
            grouptags[3].ti_Data = (IPTR)btn;
            group = MUI_NewObjectA(MUIC_Group, grouptags);
        }
        if (group)
        {
            wintags[4].ti_Tag = pubscreen ? MUIA_Window_PublicScreen : TAG_IGNORE;
            wintags[4].ti_Data = (IPTR)pubscreen;
            wintags[5].ti_Data = (IPTR)group;
            s->Win = MUI_NewObjectA(MUIC_Window, wintags);
        }
        if (s->Win)
        {
            apptags[3].ti_Data = (IPTR)s->Win;
            s->App = MUI_NewObjectA(MUIC_Application, apptags);
        }
        if (!s->App)
        {
            if (s->Win)
                MUI_DisposeObject(s->Win);
            else if (group)
                MUI_DisposeObject(group);
            else
            {
                if (s->Text)
                    MUI_DisposeObject(s->Text);
                if (btn)
                    MUI_DisposeObject(btn);
                if (space)
                    MUI_DisposeObject(space);
            }
            s->Win = s->Text = NULL;
            *unavailable = TRUE;
            return TRUE;
        }
        DoMethod(btn, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)s->App, 2, MUIM_Application_ReturnID, 1);
        set(s->Win, MUIA_Window_Open, TRUE);
        if (!XGET(s->Win, MUIA_Window_Open))
        {
            MUI_DisposeObject(s->App);
            s->App = s->Win = s->Text = NULL;
            *unavailable = TRUE;
            return TRUE;
        }
        strncpy(s->Shown, text, sizeof(s->Shown) - 1);
    }
    else if (strcmp(text, s->Shown))
    {
        strncpy(s->Shown, text, sizeof(s->Shown) - 1);
        set(s->Text, MUIA_Text_Contents, (IPTR)text);
    }
    /* one round of input: the button, or the window being closed */
    id = DoMethod(s->App, MUIM_Application_NewInput, (IPTR)&sigs);
    if (id == 1 || id == MUIV_Application_ReturnID_Quit || (SetSignal(0, 0) & SIGBREAKF_CTRL_C))
        s->Cancelled = TRUE;
    return !s->Cancelled;
}

void StatusGUIEnd(struct SecurityBase *secBase, struct LoginStatusGUI *s)
{
    struct Library *MUIMasterBase = s->MUIMasterBase;

    if (s->App)
    {
        set(s->Win, MUIA_Window_Open, FALSE);
        MUI_DisposeObject(s->App);
    }
    if (MUIMasterBase)
        CloseLibrary(MUIMasterBase);
    s->App = s->Win = s->Text = NULL;
    s->MUIMasterBase = NULL;
    s->Shown[0] = '\0';
}
