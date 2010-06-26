/*
     Copyright 2010, The AROS Development Team. All rights reserved.
     $Id$
 */

/*
 * audio.device
 *
 * by Nexus Development 2003
 * coded by Emanuele Cesaroni
 *
 * $Id: audio_esys.c,v 1.11 2003/11/04 10:50:21 cesaroni Exp $
 */

#include "audio_intern.h"
#include <proto/dos.h>

/*
 *  InitSLAVE
 * -----------
 *
 * Initializes the slave process. Return 1 for successfull, 0 if fails.
 *
 */

STRPTR PROCESS_NAME = "audio.device slave";

BOOL InitSLAVE(struct IOAudio *ioaudio)
{
    ULONG oldsignals;

    struct TagItem NPTags[] =
    {
    { NP_Entry, (IPTR) & TaskBody },
    { NP_Name, (IPTR) PROCESS_NAME },
    { NP_Priority, 127 },
    { NP_StackSize, PROCESS_STACK },
    { TAG_DONE, 0 } };

    ematsys = &ematsys0; // Here is inited the pointer.
    ematsys->ts_slavetask = NULL;
    ematsys->ts_initio = ioaudio; // This is the audioio used to wait the init signal from the slave process.
    ematsys->ts_opentask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    if ((ematsys->ts_slavetask = (struct Task*) CreateNewProc(NPTags)))
    {
        Wait(STOLEN_SIG);
        if (ioaudio->ioa_Request.io_Error == IONOERROR)
        {
            SetSignal(oldsignals, STOLEN_SIG);
            return OKEY;
        }

        ematsys->ts_slavetask = NULL;
    }
    SetSignal(oldsignals, STOLEN_SIG);
    return KEYO;
}

/*
 *
 *  FreeSLAVE
 * -----------
 *
 * Frees the slave process.
 */

VOID FreeSLAVE(struct IOAudio *ioaudio)
{
    if (ematsys->ts_slavetask)
    {
        audio_CLOSE(ioaudio, global_eta);
    }
}

WORD GetMultiple(WORD d0, WORD d1)
{
    WORD d2;

    d2 = d0;
    d0 = d0 / d1;
    d0 = d0 * d1;

    if (d0 == d2)
    {
        return d0;
    }
    else
    {
        return d0 + d1;
    }
}

/*
 *
 *	InitESYS
 * ------------
 *
 *	Initialize some needed things for the system.
 *	0 success, 1 fail.
 */

BOOL InitESYS( VOID)
{
    emasys = &emasys0; // Here initilized the pointer.

    return 1;

}

/*
 *  FreeESYS
 * ----------
 *
 * Simply frees all the ESYS resources.
 *
 */

VOID FreeESYS( VOID)
{
    emasys = &emasys0;
}

