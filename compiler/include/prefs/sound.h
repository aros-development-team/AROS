#ifndef PREFS_SOUND_H
#define PREFS_SOUND_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif


#define ID_SOND MAKE_ID('S','O','N','D')


struct SoundPrefs
{
    LONG  sop_Reserved[4];
    BOOL  sop_DisplayQueue;
    BOOL  sop_AudioQueue;
    UWORD sop_AudioType;
    UWORD sop_AudioVolume;
    UWORD sop_AudioPeriod;
    UWORD sop_AudioDuration;
    char  sop_AudioFileName[256];
};


#define SPTYPE_BEEP	0
#define SPTYPE_SAMPLE	1

#endif /* PREFS_SOUND_H */
