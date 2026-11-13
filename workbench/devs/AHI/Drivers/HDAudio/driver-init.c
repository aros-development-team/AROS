/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2010-2026 The AROS Dev Team
(C) Copyright 2009-2010 Stephen Jones.
(C) Copyright xxxx-2009 Davy Wentzler.

The Initial Developer of the Original Code is Davy Wentzler.

All Rights Reserved.
*/

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <exec/memory.h>

#include <proto/expansion.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#ifdef __AROS__
struct DosLibrary *DOSBase;
struct Library *StdCBase = NULL;
#endif
#include <stdlib.h>

#include "library.h"
#include "version.h"
#include "hda_hidd.h"
#include "misc.h"



struct DriverBase *AHIsubBase;

static void parse_config_file(void);
static int hex_char_to_int(char c);


/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

#define MAX_CARDS 8

BOOL DriverInit(struct DriverBase *ahisubbase)
{
    struct HDAudioBase *card_base = (struct HDAudioBase *) ahisubbase;
    APTR controllers[MAX_CARDS];
    ULONG count, i;
    int card_no;

    D(bug("[HDAudio] %s()\n", __func__));
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

    // Parse the hdaudio.config file for QUERY/speaker overrides, if present
    parse_config_file();

    /*
        Claim every controller that is not the audio function of a
        display adapter; those are handled by display-audio drivers
        such as nvhdmi.audio.

        Fail if no hardware (prevents the audio modes from being added to
        the database if the driver cannot be used).
    */
    count = ahi_hda_obtain_controllers(FALSE, 0, controllers, MAX_CARDS);
    if(count == 0) {
        D(bug("[HDAudio] No HDaudio controller found! :-(\n"));
        return FALSE;
    }

    /*** Allocate and init all cards ******************************************/

    card_base->driverdatas = (struct HDAudioChip **) AllocVec(
                                 sizeof(*card_base->driverdatas) * count,
                                 MEMF_PUBLIC | MEMF_CLEAR);

    if(card_base->driverdatas == NULL) {
        D(bug("[HDAudio] Out of memory.\n"));
        for(i = 0; i < count; i++) {
            ahi_hda_release_controller(controllers[i]);
        }
        return FALSE;
    }

    card_no = 0;
    for(i = 0; i < count; i++) {
        struct HDAudioChip *card = AllocDriverData(controllers[i], AHIsubBase);

        if(card != NULL) {
            card_base->driverdatas[card_no++] = card;
        }
    }

    card_base->cards_found = card_no;

    if(card_no == 0) {
        D(bug("[HDAudio] No usable codec on any controller!\n"));
        FreeVec(card_base->driverdatas);
        card_base->driverdatas = NULL;
        return FALSE;
    }

    D(bug("[HDAudio] exit init (%d cards)\n", card_no));
    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID DriverCleanup(struct DriverBase *AHIsubBase)
{
    struct HDAudioBase *card_base = (struct HDAudioBase *) AHIsubBase;
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


static void parse_config_file(void)
{
    BPTR config_file;
    BPTR handle;

    handle = Lock("ENVARC:hdaudio.config", SHARED_LOCK);

    if(handle == 0) {
        bug("No handle found. IoErr()=%d\n", IoErr());
        return;
    }

    UnLock(handle);

    config_file = Open("ENVARC:hdaudio.config", MODE_OLDFILE);

    if(config_file) {
        BOOL Continue = TRUE;
        bug("Opened config file\n");

        while(Continue) {
            char *line = (char *) AllocVec(512, MEMF_CLEAR);
            char *ret;

            ret = FGets(config_file, line, 512);

            if(ret == NULL) {
                FreeVec(line);
                break;
            }

            if(ret[0] == '0' &&
                    ret[1] == 'x' &&
                    ret[6] == ',' &&
                    ret[7] == ' ' &&
                    ret[8] == '0' &&
                    ret[9] == 'x' &&
                    ret[15] == '\0') {
                /* Vendor/device ID lines are obsolete: controllers are
                   discovered by PCI class through hdaudio.hidd. */
            } else if(ret[0] == 'Q' && ret[1] == 'U' && ret[2] == 'E' && ret[3] == 'R' && ret[4] == 'Y') {
                bug("QUERY found!\n");
                setForceQuery();

                if(ret[5] == 'D') { // debug
                    setDumpAll();
                }
            } else if(Strnicmp(ret, "speaker=0x", 10) == 0) {
                int speaker = 0;
                char *tmp = (char *) AllocVec(16, MEMF_CLEAR);

                CopyMem(line + 10, tmp, 2);
                tmp[2] = '\0';

                // convert hex to decimal
                speaker = hex_char_to_int(tmp[0]) * 16 + hex_char_to_int(tmp[1]);

                bug("Speaker in config = %x!\n", speaker);

                setForceSpeaker(speaker);
            }

            FreeVec(line);
        }

        Close(config_file);
    } else {
        bug("Couldn't open config file!\n");
    }
}


static int hex_char_to_int(char c)
{
    if(c >= '0' && c <= '9') {
        return (c - '0');
    } else if(c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    } else if(c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    } else {
        bug("Error in hex_char_to_int: char was %c\n", c);
        return 0;
    }
}
