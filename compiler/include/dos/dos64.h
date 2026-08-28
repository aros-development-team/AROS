#ifndef DOS_DOS64_H
#define DOS_DOS64_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: 64-bit filesystem I/O support definitions, for use with
          dos64.library and 64-bit aware filesystem handlers.
*/

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif
#ifndef DOS_RECORD_H
#   include <dos/record.h>
#endif

/**********************************************************************
 ********************** 64-bit Packet types ***************************
 **********************************************************************/

/*
 * MorphOS (tm) compatable extensions. These operate on the same
 * argument layout as their 32-bit counterparts, but with 64-bit
 * position/size values, and (for the EXAMINE variants) a
 * struct FileInfoBlock64 in place of the 32-bit FileInfoBlock.
 *
 * NB: On 64-bit AROS builds the standard DosPacket argument and result
 * fields are already 64-bit wide (SIPTR), so 64-bit values are passed
 * directly in dp_Arg#/dp_Res1. On 32-bit builds these packets cannot
 * carry 64-bit values in the packet arguments; there the AmigaOS 4 (tm)
 * style packets (ACTION_*64, see <dos/dosextens.h>) together with
 * struct DosPacket64 (below) are used instead.
 */
#ifndef ACTION_SEEK64
#define ACTION_SEEK64                   26400
#define ACTION_SET_FILE_SIZE64          26401
#define ACTION_LOCK_RECORD64            26402
#define ACTION_FREE_RECORD64            26403
#define ACTION_QUERY_ATTR               26407
#define ACTION_EXAMINE_OBJECT64         26408
#define ACTION_EXAMINE_NEXT64           26409
#define ACTION_EXAMINE_FH64             26410
#endif

/*
 * AROS extension: 64-bit volume information.
 * Arguments as for ACTION_INFO, but dp_Arg2 points (BPTR) to a
 * struct InfoData64 instead of a 32-bit InfoData.
 */
#define ACTION_INFO64                   2020

#if (__WORDSIZE != 64)
/*
 * AmigaOS 4 (tm) compatable 64-bit packet layout, used on 32-bit
 * systems for the ACTION_CHANGE_FILE_POSITION64, ACTION_GET_FILE_POSITION64,
 * ACTION_CHANGE_FILE_SIZE64 and ACTION_GET_FILE_SIZE64 packets, where the
 * standard DosPacket fields are too small to carry 64-bit values.
 *
 * This structure overlays the standard DosPacket. The originator must
 * additionally store the object (fh_Arg1) in the *standard* packet's
 * dp_Arg1 field, set dp_Res0 to DP64_INIT, and place the 64-bit
 * argument(s) in the fields below.
 *
 * On 64-bit AROS builds this structure is not used (see above).
 */
struct DosPacket64
{
    struct Message * dp_Link;   /* Pointer to a standard exec message. */
    struct MsgPort * dp_Port;   /* Reply-Port of that packet. */

    LONG  dp_Type;              /* 8:  Packet type (ACTION_...)          */
    LONG  dp_Res0;              /* 12: Set to DP64_INIT by the originator */
    ULONG dp_Res2;              /* 16: Secondary result (IoErr() value)  */
    LONG  dp_Pad0;              /* 20: see the alignment note below      */
    QUAD  dp_Res1;              /* 24: 64-bit primary result             */
    LONG  dp_Arg1;              /* 32: the object (fh_Arg1)              */
    LONG  dp_Pad1;              /* 36                                    */
    QUAD  dp_Arg2;              /* 40: 64-bit argument                   */
    ULONG dp_Arg3;              /* 48: 32-bit argument (e.g. seek mode)  */
    ULONG dp_Arg4;              /* 52: struct FileHandle *               */
    QUAD  dp_Arg5;              /* 56: must be zero to validate dp_Arg4  */
};

/*
 * The padding above is deliberate and must not be removed. The 64-bit
 * fields of this packet sit at fixed offsets (dp_Res1 at 24, dp_Arg2 at
 * 40, dp_Arg5 at 56), which is what a handler on the other side reads
 * and writes. Those offsets are what a compiler that gives a 64-bit type
 * 8-byte alignment produces on its own - but m68k aligns them to 4, so
 * without the padding every 64-bit field would sit 4 bytes early and the
 * two sides would disagree about where the result is. Spelling the
 * padding out keeps the layout identical whatever the ABI decides.
 */

#define DP64_INIT               (-3)
#endif

/**********************************************************************
 ******************* AllocDosObject64() types *************************
 **********************************************************************/

/* Objects allocatable with dos64.library/AllocDosObject64() */
#define DOS64_FIB               1   /* struct FileInfoBlock64 */
#define DOS64_INFODATA          2   /* struct InfoData64      */
#define DOS64_RECORDLOCK        3   /* struct RecordLock64    */

#endif /* DOS_DOS64_H */
