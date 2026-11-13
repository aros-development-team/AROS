/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2026 The AROS Dev Team.

All Rights Reserved.
*/

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <exec/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>
#ifdef __AROS__
struct DosLibrary *DOSBase;
struct Library *StdCBase = NULL;
#endif

#include "library.h"
#include "version.h"
#include "hda_hidd.h"
#include "hdmi.h"



struct DriverBase *AHIsubBase;


/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

#define MAX_CARDS 8

BOOL DriverInit(struct DriverBase *ahisubbase)
{
    struct NVHDMIBase *card_base = (struct NVHDMIBase *) ahisubbase;
    APTR controllers[MAX_CARDS];
    ULONG count, i;
    int card_no;

    D(bug("[NVHDMI] %s()\n", __func__));
    AHIsubBase = ahisubbase;

    DOSBase = (struct DosLibrary *) OpenLibrary(DOSNAME, 37);

    if(DOSBase == NULL) {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

#ifdef __AROS__
    StdCBase = OpenLibrary("stdc.library", 0);
    if(StdCBase == NULL) {
        Req("Unable to open 'stdc.library'.\n");
        return FALSE;
    }
#endif

    if(!ahi_hda_init(ahisubbase)) {
        return FALSE;
    }

    InitSemaphore(&card_base->semaphore);

    /*
        Claim every NVidia display-audio function. Fail if there are
        none (prevents the audio modes from being added to the database
        if the driver cannot be used).
    */
    count = ahi_hda_obtain_controllers(TRUE, 0x10DE, controllers, MAX_CARDS);
    if(count == 0) {
        D(bug("[NVHDMI] No NVidia display-audio controller found.\n"));
        return FALSE;
    }

    card_base->driverdatas = (struct NVHDMIChip **) AllocVec(
                                 sizeof(*card_base->driverdatas) * count,
                                 MEMF_PUBLIC | MEMF_CLEAR);

    if(card_base->driverdatas == NULL) {
        D(bug("[NVHDMI] Out of memory.\n"));
        for(i = 0; i < count; i++) {
            ahi_hda_release_controller(controllers[i]);
        }
        return FALSE;
    }

    card_no = 0;
    for(i = 0; i < count; i++) {
        struct NVHDMIChip *card = AllocDriverData(controllers[i], AHIsubBase);

        if(card != NULL) {
            card_base->driverdatas[card_no++] = card;
        }
    }

    card_base->cards_found = card_no;

    if(card_no == 0) {
        D(bug("[NVHDMI] No usable codec on any controller!\n"));
        FreeVec(card_base->driverdatas);
        card_base->driverdatas = NULL;
        return FALSE;
    }

    D(bug("[NVHDMI] exit init (%d cards)\n", card_no));
    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID DriverCleanup(struct DriverBase *AHIsubBase)
{
    struct NVHDMIBase *card_base = (struct NVHDMIBase *) AHIsubBase;
    int i;

    if(card_base->driverdatas) {
        for(i = 0; i < card_base->cards_found; ++i) {
            FreeDriverData(card_base->driverdatas[i], AHIsubBase);
        }

        FreeVec(card_base->driverdatas);
    }

    ahi_hda_exit();

#ifdef __AROS__
    if(StdCBase) {
        CloseLibrary(StdCBase);
    }
#endif

    if(DOSBase) {
        CloseLibrary((struct Library *) DOSBase);
    }
}
