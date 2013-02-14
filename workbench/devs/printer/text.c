/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Text printing
 */

#include <string.h>

#include <aros/debug.h>

#include "printer_intern.h"

#define WBUFSIZE    256


#define FLUSH_BUFFER() \
    do { \
        if (blen > 0) { \
            pd->pd_PWrite(buffer, blen); \
            buffsel ^= 1; \
            buffer = buff[buffsel]; \
            blen = 0; \
        } \
    } while (0)

#define CSI                   '\033'
#define CSI_BRACK             '['
#define CSI_BRACK_SEMI        ';'
#define CSI_BRACK_QUOTE       '"'
#define CSI_PAREN             '('
#define CSI_HASH              '#'
#define CSI_END               -1

struct StateTable {
    BYTE state;
    UBYTE code;
    BYTE parm;
    BYTE cmd;
};

static const struct StateTable StateTable[] = {
    { .state = 0, .code = 0, .parm = -1, .cmd = -1 },
    { .state = CSI, .code = 'D', .parm = -1, .cmd = aIND },
    { .state = CSI, .code = 'E', .parm = -1, .cmd = aNEL },
    { .state = CSI, .code = 'H', .parm = -1, .cmd = aHTS },
    { .state = CSI, .code = 'J', .parm = -1, .cmd = aVTS },
    { .state = CSI, .code = 'K', .parm = -1, .cmd = aPLD },
    { .state = CSI, .code = 'L', .parm = -1, .cmd = aPLU },
    { .state = CSI, .code = 'M', .parm = -1, .cmd = aRI },
    { .state = CSI, .code = 'c', .parm = -1, .cmd = aRIS },
    { .state = CSI_HASH,  .code = '0', .parm = -1, .cmd = aRMS },
    { .state = CSI_HASH,  .code = '1', .parm = -1, .cmd = aRIN },
    { .state = CSI_HASH,  .code = '2', .parm = -1, .cmd = aBMS },
    { .state = CSI_HASH,  .code = '3', .parm = -1, .cmd = aCAM },
    { .state = CSI_HASH,  .code = '4', .parm = -1, .cmd = aTBCALL },
    { .state = CSI_HASH,  .code = '5', .parm = -1, .cmd = aTBSALL },
    { .state = CSI_HASH,  .code = '8', .parm = -1, .cmd = aTMS },
    { .state = CSI_HASH,  .code = '9', .parm = -1, .cmd = aLMS },
    { .state = CSI_PAREN, .code = '6', .parm = -1, .cmd = aFNT9 },
    { .state = CSI_PAREN, .code = 'A', .parm = -1, .cmd = aFNT3 },
    { .state = CSI_PAREN, .code = 'B', .parm = -1, .cmd = aFNT0 },
    { .state = CSI_PAREN, .code = 'C', .parm = -1, .cmd = aFNT10 },
    { .state = CSI_PAREN, .code = 'E', .parm = -1, .cmd = aFNT4 },
    { .state = CSI_PAREN, .code = 'H', .parm = -1, .cmd = aFNT5 },
    { .state = CSI_PAREN, .code = 'J', .parm = -1, .cmd = aFNT8 },
    { .state = CSI_PAREN, .code = 'K', .parm = -1, .cmd = aFNT2 },
    { .state = CSI_PAREN, .code = 'R', .parm = -1, .cmd = aFNT1 },
    { .state = CSI_PAREN, .code = 'Y', .parm = -1, .cmd = aFNT6 },
    { .state = CSI_PAREN, .code = 'Z', .parm = -1, .cmd = aFNT7 },
    { .state = CSI_BRACK, .code = 'E', .parm = -1, .cmd = aTSS },
    { .state = CSI_BRACK, .code = 'F', .parm = 0, .cmd = aJFY0 },
    { .state = CSI_BRACK, .code = 'F', .parm = 1, .cmd = aJFY1 },
    { .state = CSI_BRACK, .code = 'F', .parm = 3, .cmd = aJFY3 },
    { .state = CSI_BRACK, .code = 'F', .parm = 5, .cmd = aJFY5 },
    { .state = CSI_BRACK, .code = 'F', .parm = 6, .cmd = aJFY6 },
    { .state = CSI_BRACK, .code = 'F', .parm = 7, .cmd = aJFY7 },
    { .state = CSI_BRACK, .code = 'g', .parm = 0, .cmd = aTBC0 },
    { .state = CSI_BRACK, .code = 'g', .parm = 1, .cmd = aTBC1 },
    { .state = CSI_BRACK, .code = 'g', .parm = 3, .cmd = aTBC3 },
    { .state = CSI_BRACK, .code = 'g', .parm = 4, .cmd = aTBC4 },
    { .state = CSI_BRACK, .code = 'm', .parm = 0, .cmd = aSGR0 },
    { .state = CSI_BRACK, .code = 'm', .parm = 1, .cmd = aSGR1 },
    { .state = CSI_BRACK, .code = 'm', .parm = 22, .cmd = aSGR22 },
    { .state = CSI_BRACK, .code = 'm', .parm = 23, .cmd = aSGR23 },
    { .state = CSI_BRACK, .code = 'm', .parm = 24, .cmd = aSGR24 },
    { .state = CSI_BRACK, .code = 'm', .parm = 3, .cmd = aSGR3 },
    { .state = CSI_BRACK, .code = 'm', .parm = 30, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 31, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 32, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 33, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 34, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 35, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 36, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 37, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 38, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 39, .cmd = aSFC },
    { .state = CSI_BRACK, .code = 'm', .parm = 4, .cmd = aSGR4 },
    { .state = CSI_BRACK, .code = 'm', .parm = 40, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 41, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 42, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 43, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 44, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 45, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 46, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 47, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 48, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'm', .parm = 49, .cmd = aSBC },
    { .state = CSI_BRACK, .code = 'p', .parm = 0, .cmd = aPROP0 },
    { .state = CSI_BRACK, .code = 'p', .parm = 1, .cmd = aPROP1 },
    { .state = CSI_BRACK, .code = 'p', .parm = 2, .cmd = aPROP2 },
    { .state = CSI_BRACK, .code = 'q', .parm = -1, .cmd = aPERF },
    { .state = CSI_BRACK, .code = 'q', .parm = 0, .cmd = aPERF0 },
    { .state = CSI_BRACK, .code = 't', .parm = -1, .cmd = aSLPP },
    { .state = CSI_BRACK, .code = 'v', .parm = 0, .cmd = aSUS0 },
    { .state = CSI_BRACK, .code = 'v', .parm = 1, .cmd = aSUS1 },
    { .state = CSI_BRACK, .code = 'v', .parm = 2, .cmd = aSUS2 },
    { .state = CSI_BRACK, .code = 'v', .parm = 3, .cmd = aSUS3 },
    { .state = CSI_BRACK, .code = 'v', .parm = 4, .cmd = aSUS4 },
    { .state = CSI_BRACK, .code = 'w', .parm = 0, .cmd = aSHORP0 },
    { .state = CSI_BRACK, .code = 'w', .parm = 1, .cmd = aSHORP1 },
    { .state = CSI_BRACK, .code = 'w', .parm = 2, .cmd = aSHORP2 },
    { .state = CSI_BRACK, .code = 'w', .parm = 3, .cmd = aSHORP3 },
    { .state = CSI_BRACK, .code = 'w', .parm = 4, .cmd = aSHORP4 },
    { .state = CSI_BRACK, .code = 'w', .parm = 5, .cmd = aSHORP5 },
    { .state = CSI_BRACK, .code = 'w', .parm = 6, .cmd = aSHORP6 },
    { .state = CSI_BRACK, .code = 'z', .parm = 0, .cmd = aVERP0 },
    { .state = CSI_BRACK, .code = 'z', .parm = 1, .cmd = aVERP1 },
    { .state = CSI_BRACK_QUOTE, .code = 'r', .parm = -1, .cmd = aRAW },
    { .state = CSI_BRACK_QUOTE, .code = 'x', .parm = -1, .cmd = aEXTEND },
    { .state = CSI_BRACK_QUOTE, .code = 'z', .parm = 1, .cmd = aDEN1 },
    { .state = CSI_BRACK_QUOTE, .code = 'z', .parm = 2, .cmd = aDEN2 },
    { .state = CSI_BRACK_QUOTE, .code = 'z', .parm = 3, .cmd = aDEN3 },
    { .state = CSI_BRACK_QUOTE, .code = 'z', .parm = 4, .cmd = aDEN4 },
    { .state = CSI_BRACK_QUOTE, .code = 'z', .parm = 5, .cmd = aDEN5 },
    { .state = CSI_BRACK_QUOTE, .code = 'z', .parm = 6, .cmd = aDEN6 },
    { .state = CSI_BRACK_SEMI,  .code = 'r', .parm = -1, .cmd = aSTBM },
    { .state = CSI_BRACK_SEMI,  .code = 's', .parm = -1, .cmd = aSLRM },
    { .state = CSI_END },
};

static inline int isdigit(int x)
{
    return (x >= '0' && x <= '9');
}


LONG Printer_Text_Command(struct PrinterData *pd, UWORD command, UBYTE p0, UBYTE p1, UBYTE p2, UBYTE p3)
{
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    struct PrinterUnitText *txt = &pd->pd_PUnit->pu_Text;
    CONST_STRPTR *Commands = pd->pd_Device.dd_CmdVectors;
    LONG CommandMax = pd->pd_Device.dd_NumCommands;
    /* Use pd_Stack as the command buffer */
    /* pd_OldStk[8..11] is used for the 'TPMATCHWORD' magic
     */
    UBYTE *buff[2] = { (UBYTE *)&pd->pd_Stk[0], (UBYTE *)&pd->pd_OldStk[12] };
    int buffsel = 0;
    UBYTE *buffer = buff[buffsel];
    LONG buffmax = ((P_STKSIZE > (P_OLDSTKSIZE-12)) ? (P_OLDSTKSIZE-12) : P_STKSIZE)/2;
    CONST_STRPTR cmd;
    LONG blen = 0;
    UBYTE parm[4] = { p0, p1, p2, p3 };

    if (command >= CommandMax)
        return -1;

    cmd = Commands[command];

    D(bug("%s: cmd=%d(%s), (%d, %d, %d, %d)\n", __func__, command, cmd, p0, p1, p2, p3));

    if (cmd == NULL)
        return -1;

    if (command == aRIN || command == aRIS) {
        txt->pt_CurrentLine = 0;
        txt->pt_CRLF = 0;
        txt->pt_Spacing = pd->pd_Preferences.PrintSpacing;
    }

    for (; *cmd; cmd++) { 
        D(bug("%s: command=%d '%c'(%d), ped_DoSpecial=%p \n", __func__, command, *cmd, *cmd, ped->ped_DoSpecial));

        if ((TEXT)*cmd == (TEXT)'\377' && ped->ped_DoSpecial) {
            LONG err;

            if ((TEXT)cmd[1] == (TEXT)'\377') {
                /* NOTE: From printer.device 44.12: flush before calling DoSpecial */
                pd->pd_PBothReady();
                cmd++;
            }
            err = ped->ped_DoSpecial(&command, &buffer[blen], &txt->pt_CurrentLine, &txt->pt_Spacing, &txt->pt_CRLF, parm);
            D(bug("%s: ped_DoSpecial: cmd => %d, buf='%s', err=%d\n", __func__, command, &buffer[blen], err));
            if (err > 0) {
                blen += err;
            } else if (err == -2) {
                blen += 0;
            } else if (err == 0) {
                buffer[blen++] = *cmd;
            } else {
                return err;
            }
        } else {
            buffer[blen++] = *cmd;
        }

        if (blen >= buffmax) {
            FLUSH_BUFFER();
        }
    }
    FLUSH_BUFFER();

    return 0;
}

LONG noopConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag)
{
    return -1;
}

/* Returns:
 * -1 if not in a command string
 * -2 if processing a possible command,
 * >= 0 if a command found
 */
static LONG doState(UBYTE *state, UBYTE c, UBYTE *parm, UBYTE *parm_index)
{
    const struct StateTable *loc = &StateTable[*state];
    BYTE currstate = loc->state;

    if (currstate == 0) {
        if (c != CSI)
            return -1;

        parm[0] = 0;
        *state = 1;
        return -2;
    }

    if (currstate != 0 && currstate != CSI && currstate != CSI_HASH) {
        if (isdigit(c)) {
            UWORD p = parm[*parm_index];

            p = (p * 10) + (c - '0');
            if (p > 100) {
                *state = 0;
                return -1;
            }
            parm[*parm_index] = (UBYTE)p;
            return -2;
        }
    }

    /* State promotion */
    if ((currstate == CSI && (c == CSI_HASH || c == CSI_PAREN || c == CSI_BRACK)) ||
        (currstate == CSI_BRACK && (c == CSI_BRACK_SEMI || c == CSI_BRACK_QUOTE))) {
        /* Advance to next matching state */
        for (; loc->state != CSI_END && loc->state != c; loc++, (*state)++);
        if (loc->state == CSI_END) {
            *state = 0;
            return -1;
        }

        if (c == CSI_BRACK_SEMI) {
            parm[++(*parm_index)]=0;
        }
        return -2;
    }

    /* Code search */
    for (; loc->state == currstate && loc->code != c; loc++, (*state)++);

    if (loc->state != currstate) {
        *state = 0;
        return -1;
    }

    /* Parm search */
    for (; loc->state == currstate; loc++, (*state)++) {
        if ((loc->parm == -1) || (loc->parm == parm[0])) {
            *state = 0;
            return loc->cmd;
        }
    }

    /* No match */
    *state = 0;
    return -1;
}

LONG Printer_Text_Write(struct PrinterData *pd, UBYTE *text, LONG length)
{
    struct PrinterExtendedData *ped = &pd->pd_SegmentData->ps_PED;
    struct PrinterUnitText *txt = &pd->pd_PUnit->pu_Text;
    LONG (*ConvFunc)(UBYTE *buf, UBYTE c, LONG crlf_flag);
    CONST_STRPTR *Char8Bit = NULL;
    UBYTE parm[4];
    UBYTE state = 0;
    UBYTE pindex = 0;
    UBYTE buff[2][WBUFSIZE];
    UBYTE *buffer = &buff[0][0];
    int buffsel = 0;
    int blen = 0;
    LONG err;

    if (pd->pd_SegmentData->ps_Version >= 33) {
        Char8Bit = (CONST_STRPTR *)ped->ped_8BitChars;
    }

    if (pd->pd_SegmentData->ps_Version >= 34) {
        ConvFunc = ped->ped_ConvFunc;
    } else {
        ConvFunc = noopConvFunc;
    }

    for (;(length < 0 && *text) || (length > 0); text++, length--) {
        LONG cmd;
        D(bug("%s: c='%c' (\\%o)\n", __func__, *text, *text)); 
        cmd = doState(&state, *text, parm, &pindex);
        if (cmd == -2)
            continue;

        if (cmd < 0) {
            if (Char8Bit && *text >= 0xa0) {
                CONST_STRPTR str = Char8Bit[*text-0xa0];
                pd->pd_PWrite((APTR)str, strlen(str));
                continue;
            }
            err = ConvFunc(&buffer[blen], *text, txt->pt_CRLF);
            D(bug("%s: ConvFunc(%d) => %d\n", __func__, *text, err));
            if (err < 0) {
                if (*text == '\n') {
                    FLUSH_BUFFER();
                    Printer_Text_Command(pd, aNEL, 0, 0, 0, 0);
                } else {
                    buffer[blen++] = *text;
                }
            } else {
                blen += err;
            }
        } else {
            FLUSH_BUFFER();
            Printer_Text_Command(pd, cmd, parm[0], parm[1], parm[2], parm[3]);
        }

        if (blen > WBUFSIZE/2)
            FLUSH_BUFFER();
    }

    return 0;
}
