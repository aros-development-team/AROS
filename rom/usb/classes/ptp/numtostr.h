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

#include "ptp.class.h"

#define NTS_OPCODE         0x01
#define NTS_RESPCODE       0x02
//#define NTS_OBJECTFMTCODE  0x03
//#define NTS_EVENTCODE      0x04
//#define NTS_DEVICEPROPCODE 0x05

/* Protos
*/

STRPTR nNumToStr(struct NepClassPTP *nch, UWORD type, ULONG id, STRPTR defstr);

STRPTR nConcatBitsStr(struct NepClassPTP *nch, UWORD type, ULONG bits);

struct PTPIDMap
{
    UWORD  pim_ID;
    STRPTR pim_String;
};

#endif /* NUMTOSTR_H */
