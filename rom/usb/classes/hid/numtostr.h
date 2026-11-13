#ifndef NUMTOSTR_H
#define NUMTOSTR_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for numtostr.c
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <exec/types.h>

#include "hid.class.h"

#define NTS_USAGEPAGE 0x01
#define NTS_USAGEID   0x02
#define NTS_EXTRAWKEY 0x7e
#define NTS_RAWKEY    0x7f

#define UT_LC   0x01 /* Linear control */
#define UT_OOC  0x02 /* On/Off Control */
#define UT_MC   0x03 /* Momentary Control */
#define UT_OSC  0x04 /* One Shot Control */
#define UT_RTC  0x05 /* Retrigger Control */

/* Protos
*/

STRPTR nNumToStr(struct NepClassHid *nch, UWORD type, ULONG id, STRPTR defstr);

struct HidUsageIDMap
{
    UWORD  hum_ID;
    STRPTR hum_String;
};

struct HidUsagePageMap
{
    WORD                  hupm_ID;
    STRPTR                hupm_String;
    const struct HidUsageIDMap *hupm_UsageIDMap;
};

extern const struct HidUsageIDMap hidusage07[];

#endif /* NUMTOSTR_H */
