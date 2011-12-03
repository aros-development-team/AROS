/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: devicetuple.c $

    Desc: DeviceTuple() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

AROS_LH2(ULONG, DeviceTuple,
	AROS_LHA(UBYTE*, tuple_data, A0),
	AROS_LHA(struct DeviceTData*, storage, A1),
	struct CardResource*, CardResource, 13, Cardres)
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
