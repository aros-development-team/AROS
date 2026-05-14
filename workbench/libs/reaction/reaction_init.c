/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction library initialization
*/
#define DEBUG 1

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

#include <string.h>

#include "reaction_intern.h"

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

static void Reaction_InitPrefs(LIBBASETYPEPTR LIBBASE)
{
    struct UIPrefs *p = &LIBBASE->aui_Prefs;

    memset(p, 0, sizeof(*p));
    InitSemaphore(&p->cap_Semaphore);

    /* Publish UIPrefs as a named system semaphore so any class can discover
     * it via FindSemaphore(RAPREFSSEMAPHORE) without needing to open
     * reaction.library directly. The cap_Semaphore being the first member
     * means &cap_Semaphore == prefs pointer. */
    strncpy(LIBBASE->aui_PrefsName, RAPREFSSEMAPHORE,
        sizeof(LIBBASE->aui_PrefsName) - 1);
    p->cap_Semaphore.ss_Link.ln_Name = LIBBASE->aui_PrefsName;
    p->cap_Semaphore.ss_Link.ln_Pri  = 0;

    /* Defaults match ClassAct/ReAction prefs out-of-the-box appearance */
    p->cap_PrefsVersion  = 1;
    p->cap_PrefsSize     = sizeof(struct UIPrefs);
    p->cap_BevelType     = BVT_GT;
    p->cap_LayoutSpacing = 4;
    p->cap_3DLook        = TRUE;
    p->cap_LabelPen      = 1;            /* TEXTPEN equivalent */
    p->cap_LabelPlace    = 1;            /* PLACETEXT_LEFT */
    p->cap_3DLabel       = FALSE;
    p->cap_SimpleRefresh = FALSE;
    p->cap_3DProp        = TRUE;
    p->cap_GlyphType     = GLT_GT;
    p->cap_FallbackAttr  = NULL;
    p->cap_LabelAttr     = NULL;
}

static int Reaction_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Reaction] Reaction_Init: LIBBASE=%p\n", LIBBASE));

    InitSemaphore(&LIBBASE->aui_Lock);

    if ((LIBBASE->aui_IntuitionBase = OpenLibrary("intuition.library", 39)) != NULL)
    {
        D(bug("[Reaction] Reaction_Init: IntuitionBase=%p\n", LIBBASE->aui_IntuitionBase));
        if ((LIBBASE->aui_UtilityBase = OpenLibrary("utility.library", 39)) != NULL)
        {
            D(bug("[Reaction] Reaction_Init: UtilityBase=%p, success\n", LIBBASE->aui_UtilityBase));

            Reaction_InitPrefs(LIBBASE);
            AddSemaphore(&LIBBASE->aui_Prefs.cap_Semaphore);
            D(bug("[Reaction] Reaction_Init: published UIPrefs semaphore '%s'\n",
                LIBBASE->aui_PrefsName));

            return TRUE;
        }
        D(bug("[Reaction] Reaction_Init: failed to open utility.library\n"));
        CloseLibrary((struct Library *)LIBBASE->aui_IntuitionBase);
    }
    else
    {
        D(bug("[Reaction] Reaction_Init: failed to open intuition.library\n"));
    }

    return FALSE;
}

static int Reaction_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Reaction] Reaction_Expunge: LIBBASE=%p\n", LIBBASE));

    if (LIBBASE->aui_Prefs.cap_Semaphore.ss_Link.ln_Name)
    {
        RemSemaphore(&LIBBASE->aui_Prefs.cap_Semaphore);
        LIBBASE->aui_Prefs.cap_Semaphore.ss_Link.ln_Name = NULL;
    }

    if (LIBBASE->aui_UtilityBase)
        CloseLibrary((struct Library *)LIBBASE->aui_UtilityBase);
    if (LIBBASE->aui_IntuitionBase)
        CloseLibrary((struct Library *)LIBBASE->aui_IntuitionBase);

    return TRUE;
}

ADD2INITLIB(Reaction_Init, 0);
ADD2EXPUNGELIB(Reaction_Expunge, 0);
