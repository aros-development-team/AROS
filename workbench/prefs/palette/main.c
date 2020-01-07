/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>

#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include <prefs/palette.h>

#include "locale.h"
#include "paleditor.h"
#include "args.h"
#include "prefs.h"

#define VERSION "$VER: Palette 1.5 (07.01.2020) AROS Dev Team"
/*********************************************************************************************/

BOOL allocPens(struct ColorMap *cm, ULONG * pens)
{
    if ((pens[0] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
    {
        D(bug("[PaletteEditor] %s: pen #0 = %d\n", __func__, pens[0]);)
        if ((pens[1] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
        {
            D(bug("[PaletteEditor] %s: pen #1 = %d\n", __func__, pens[1]);)
            if ((pens[2] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
            {
                D(bug("[PaletteEditor] %s: pen #2 = %d\n", __func__, pens[2]);)
                if ((pens[3] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
                {
                    D(bug("[PaletteEditor] %s: pen #3 = %d\n", __func__, pens[3]);)
                    if ((pens[4] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
                    {
                        D(bug("[PaletteEditor] %s: pen #4 = %d\n", __func__, pens[4]);)
                        if ((pens[5] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
                        {
                            D(bug("[PaletteEditor] %s: pen #5 = %d\n", __func__, pens[5]);)
                            if ((pens[6] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
                            {
                                D(bug("[PaletteEditor] %s: pen #6 = %d\n", __func__, pens[6]);)
                                if ((pens[7] = ObtainPen(cm, -1, 0, 0, 0, PEN_EXCLUSIVE | PEN_NO_SETCOLOR)) != -1)
                                {
                                    D(bug("[PaletteEditor] %s: pen #7 = %d\n", __func__, pens[7]);)
                                    return TRUE;
                                }
                                ReleasePen(cm, pens[6]);
                            }
                            ReleasePen(cm, pens[5]);
                        }
                        ReleasePen(cm, pens[4]);
                    }
                    ReleasePen(cm, pens[3]);
                }
                ReleasePen(cm, pens[2]);
            }
            ReleasePen(cm, pens[1]);
        }
        ReleasePen(cm, pens[0]);
    }
    return FALSE;                            
}

int main(int argc, char **argv)
{
    Object *application;
    Object *window;
    ULONG pens[8] = { -1 };
    ULONG *usepens = NULL;

    Locale_Initialize();

    /* init */
    if (ReadArguments(argc, argv))
    {
        if (ARG(USE) || ARG(SAVE))
        {
            Prefs_HandleArgs((STRPTR)ARG(FROM), ARG(USE), ARG(SAVE));
        }
        else
        {
            struct Screen *pScreen = NULL, *appScreen = NULL;
            char *pubname = NULL;

            if (ARG(PUBSCREEN))
                pubname = (char *)ARG(PUBSCREEN);

            pScreen = LockPubScreen(pubname);
            if (pScreen)
            {
                if (GetBitMapAttr(pScreen->RastPort.BitMap, BMA_DEPTH) > 4)
                {
                    if (allocPens(pScreen->ViewPort.ColorMap, pens))
                    {
                        usepens = pens;
                        appScreen = pScreen;
                    }
                }
                else
                {
                    UnlockPubScreen(NULL, pScreen);
                    pScreen = NULL;
                }
            }

            if (!appScreen)
            {
                appScreen = OpenScreenTags( NULL,
                    SA_Depth, 5,
                    SA_Width, 320,
                    SA_Height, 200,
                    SA_ShowTitle, TRUE,
                    SA_Title, "",
                    TAG_END);
            }                

            application = (Object *)ApplicationObject,
                MUIA_Application_Title, __(MSG_WINTITLE),
                MUIA_Application_Version, (IPTR) VERSION,
                MUIA_Application_Description, __(MSG_WINTITLE),
                MUIA_Application_SingleTask, TRUE,
                MUIA_Application_Base, (IPTR) "PALETTEPREF",
                SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                    MUIA_Window_Screen, (IPTR)appScreen,
                    MUIA_Window_ID, ID_PALT,
                    WindowContents, (IPTR) PalEditorObject,
                        (usepens) ? MUIA_PalEditor_Pens : TAG_IGNORE,
                        (IPTR)usepens,
                    End,
                End),
            End;

            if (application != NULL)
            {
                SET(window, MUIA_Window_Open, TRUE);
                DoMethod(application, MUIM_Application_Execute);

                MUI_DisposeObject(application);
            }
            if (pScreen)
            {
                UnlockPubScreen(NULL, pScreen);
                pScreen = appScreen = NULL;
            }
            if (appScreen)
                CloseScreen(appScreen);
        }
        FreeArguments();
    }

    Locale_Deinitialize();
    return 0;
}

/*********************************************************************************************/
