#ifndef NUMTOSTR_H
#define NUMTOSTR_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for numtostr.c
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <hodges@in.tum.de>
 *
 * History
 *
 *  11-03-2002  - Initial
 *
 */

#include <exec/types.h>

#include "usbaudio.class.h"

#define NTS_TERMINALTYPE    0x01
#define NTS_SPATIALLOCATION 0x02
#define NTS_FEATURE         0x03
#define NTS_AUDIOFORMAT     0x04
#define NTS_UNITTYPE        0x05

/* Protos
*/

STRPTR nNumToStr(struct NepClassAudio *nch, UWORD type, ULONG id, STRPTR defstr);

STRPTR nConcatBitsStr(struct NepClassAudio *nch, UWORD type, ULONG bits);

struct AudioIDMap
{
    UWORD  aim_ID;
    STRPTR aim_String;
};

#endif /* NUMTOSTR_H */
