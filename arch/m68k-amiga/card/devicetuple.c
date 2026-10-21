/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: DeviceTuple() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

/*****************************************************************************
    NAME
        ULONG DeviceTuple(

    SYNOPSIS
        UBYTE *tuple_data, struct DeviceTData *storage)

    FUNCTION
        Decode the first device descriptor of a m68k-Amiga CISTPL_DEVICE or
        CISTPL_DEVICE_A tuple.

    INPUTS
        tuple_data - Readable tuple including its code, link byte and
        complete device descriptor. The function has no buffer-length
        argument; provide accessible storage for every byte described and
        inspected.
        storage - Non-NULL writable DeviceTData. The input and output must
        not overlap.

    RESULT
        Nonzero if the implemented decoder accepts the descriptor; zero for
        an unsupported tuple code, zero link length, unsupported device type
        or reserved speed encoding. On all calls, size, speed and flags are
        first cleared. Type may be changed before failure; output is not
        transactional.

    NOTES
        Types 0-7 and 13 are accepted. Fixed speed encodings produce 250,
        200, 150 or 100 ns; encoding 0 leaves speed zero. Extended-speed
        encoding derives a value from its mantissa and exponent. Size is
        expressed in bytes; when a size byte is read, it is (512 << (2 *
        (byte & 7))) * ((byte >> 3) + 1). An I/O descriptor without a size
        byte receives size 1. Flags are always zero.

        This decoder reads caller memory; it does not access the card,
        require ownership or retain either pointer.

    BUGS
        The decoder compares the payload length with offsets that include
        the header, so it can skip valid trailing descriptor data and still
        succeed. A long extended-speed chain can wrap the 8-bit offset and
        cause header bytes to be decoded as size data. Reserved mantissa
        values are not rejected. TRUE does not establish a complete or
        correct card-memory description.

*****************************************************************************/
AROS_LH2(ULONG, DeviceTuple,
        AROS_LHA(UBYTE*, tuple_data, A0),
        AROS_LHA(struct DeviceTData*, storage, A1),
        struct CardResource*, CardResource, 13, Card)
{
    AROS_LIBFUNC_INIT

    UWORD type, tuplesize, units, size, mantissa, exponent;
    UBYTE offset;

    CARDDEBUG(bug("DeviceTuple(%p,%p)\n", tuple_data, storage));
    
    storage->dtd_DTspeed = 0;
    storage->dtd_DTsize = 0;
    storage->dtd_DTflags = 0;

    type = tuple_data[0];
    tuplesize = tuple_data[1];

    if ((type != CISTPL_DEVICE && type != CISTPL_DEVICE_A) || tuplesize == 0)
        return FALSE;

    offset = 2;

    storage->dtd_DTtype = tuple_data[offset] >> 4;
    if (storage->dtd_DTtype > 7 && storage->dtd_DTtype != 13)
        return FALSE;

    if (tuplesize >= offset) {
        switch (tuple_data[offset] & 7)
        {
            case 1:
            storage->dtd_DTspeed = 250;
            break;
            case 2:
            storage->dtd_DTspeed = 200;
            break;
            case 3:
            storage->dtd_DTspeed = 150;
            break;
            case 4:
            storage->dtd_DTspeed = 100;
            break;
            case 7: /* SPEED_EXT */
            offset++;
            mantissa = (tuple_data[offset] >> 3) & 15;
            exponent = tuple_data[offset] & 7;
            if (mantissa == 1)
                storage->dtd_DTspeed = 10;
            else if (mantissa == 2)
                storage->dtd_DTspeed = 12;
            else if (mantissa == 3)
                storage->dtd_DTspeed = 13;
            else if (mantissa == 4)
                storage->dtd_DTspeed = 15;
            else
                storage->dtd_DTspeed = 20 + (mantissa - 5) * 5;
            if (exponent == 0) {
                storage->dtd_DTspeed /= 2;
                if (!storage->dtd_DTspeed)
                    storage->dtd_DTspeed = 1;
            } else {
                while (exponent-- > 1)
                    storage->dtd_DTspeed *= 10;
            }
            /* skip possible extended data */
            while (tuplesize >= offset && (tuple_data[offset] & 0x80))
                offset++;
            break;
            case 0:
            break;
            default:
            return FALSE;
        }
        offset++;
    }
    
    if (tuplesize >= offset) {
        size = tuple_data[offset] & 7;
        units = tuple_data[offset] >> 3;
        storage->dtd_DTsize = (512 << (size * 2)) * (units + 1);
    } else if (storage->dtd_DTtype == 13) { /* IO device? */
        storage->dtd_DTsize = 1;
    }
        
    storage->dtd_DTflags = 0;
    
    return TRUE;

    AROS_LIBFUNC_EXIT
}
