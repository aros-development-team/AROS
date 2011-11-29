/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: copytuple.c $

    Desc: CopyTuple() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

#define TUPLEDEBUG(x) do { if (buffer == NULL) x; } while(0);
#define TUPLEDEBUG2(x) x
#define TUPLELOGGING 1

#if TUPLELOGGING
static void byte2ascii(UBYTE **pp, UBYTE ch)
{
    UBYTE *p = *pp;
    *p++ = ' ';
    *p++ = (ch >> 4) > 9 ? (ch >> 4) - 10 + 'A' : (ch >> 4) + '0';
    *p++ = (ch & 15) > 9 ? (ch & 15) - 10 + 'A' : (ch & 15) + '0';
    *p = 0;
    *pp = p;
}
#endif

static BOOL getbyte(ULONG addr, UBYTE *out)
{
    volatile UBYTE *p;
    
    *out = 0;
    if (addr < GAYLE_RAM || (addr >= GAYLE_RAM + GAYLE_RAMSIZE && addr < GAYLE_ATTRIBUTE) || addr >= GAYLE_ATTRIBUTE + GAYLE_ATTRIBUTESIZE) {
    	TUPLEDEBUG2(bug("getbyte from invalid address %p\n", addr));
	return FALSE;
    }
    if (!pcmcia_havecard())
    	return FALSE;
    p = (UBYTE*)addr;
    *out = *p;
    return TRUE;
}
static BOOL getbytes(ULONG addr, UBYTE count, UBYTE nextbyte, UBYTE *out)
{
    while (count-- > 0) {
    	if (!getbyte(addr, out))
    	    return FALSE;
    	out++;
    	addr += nextbyte;
    }
    return TRUE;
}

static BOOL getlelong(ULONG addr, UBYTE offset, UBYTE nextbyte, ULONG *outp)
{
    UBYTE v;
    ULONG out;
    
    *outp = 0;
    if (!getbyte(addr + nextbyte, &v))
    	return FALSE;
    if (v < 4)
    	return FALSE;
    addr += offset * nextbyte;
    if (!getbyte(addr + 0 * nextbyte, &v))
    	return FALSE;
    out = v;
    if (!getbyte(addr + 1 * nextbyte, &v))
    	return FALSE;
    out |= v << 8;
    if (!getbyte(addr + 2 * nextbyte, &v))
    	return FALSE;
    out |= v << 16;
    if (!getbyte(addr + 3 * nextbyte, &v))
    	return FALSE;
    out |= v << 24;
    *outp = out;
    return TRUE;
}

AROS_LH4(ULONG, CopyTuple,
	AROS_LHA(struct CardHandle*, handle, A1),
	AROS_LHA(UBYTE*, buffer, A0),
	AROS_LHA(ULONG, tuplecode, D1),
	AROS_LHA(ULONG, size, D0),
	struct CardResource*, CardResource, 12, Cardres)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    ULONG addr;
    UWORD nextbyte, tuplecnt;
    ULONG nextjump;
    UBYTE oldconfig;
    BOOL ret, first;

    CARDDEBUG(bug("CopyTuple(%p,%p,%08x,%d)\n", handle, buffer, tuplecode, size));
    
    /* buffer == NULL: output all tuples to debug log */

    if (!ISMINE)
    	return FALSE;

    ret = FALSE;

    Forbid();
    
    oldconfig = gio->config;
    gio->config = GAYLE_CFG_720NS;

    tuplecnt = tuplecode >> 16;

    nextjump = GAYLE_RAM;
    addr = GAYLE_ATTRIBUTE;
    nextbyte = 2;
    first = TRUE;

    for (;;) {
    	BOOL final = FALSE;
    	UBYTE v, type;
    	UBYTE tuplesize;

    	if (!getbyte(addr, &type))
	    break;

	TUPLEDEBUG(bug("PCMCIA Tuple %02x @%p\n", type, addr));

	/* First attribute memory tuple must be CISTPL_DEVICE */
	if (first && type != CISTPL_DEVICE)
	    final = TRUE;
	first = FALSE;

    	switch (type)
    	{
    	    case CISTPL_NULL:
    	    TUPLEDEBUG(bug("CISTPL_NULL\n"));
    	    break;
    	    case CISTPL_END:
    	    final = TRUE;
    	    TUPLEDEBUG(bug("CISTPL_END\n"));
    	    break;
    	    case CISTPL_NO_LINK:
    	    nextjump = 0xffffffff;
    	    TUPLEDEBUG(bug("CISTPL_NO_LINK\n"));
    	    break;
    	    case CISTPL_LONGLINK_A:
   	    if (!getlelong(addr, 2, nextbyte, &nextjump))
   	    	goto end;
	    TUPLEDEBUG(bug("CISTPL_LONGLINK_A %08x\n", nextjump));
	    if (nextjump >= GAYLE_ATTRIBUTESIZE)
	    	goto end;
	    nextjump += GAYLE_ATTRIBUTE;
	    break;
    	    case CISTPL_LONGLINK_C:
   	    if (!getlelong(addr, 2, nextbyte, &nextjump))
   	    	goto end;
	    TUPLEDEBUG(bug("CISTPL_LONGLINK_C %08x\n", nextjump));
	    if (nextjump >= GAYLE_RAMSIZE)
	    	goto end;
	    nextjump += GAYLE_RAM;
	    break;
	}

    	if (buffer != NULL && type == (UBYTE)tuplecode) {
    	    if (tuplecnt > 0) {
    	    	tuplecnt--;
    	    } else {
    	    	getbyte(addr, &v);
    	    	*buffer++ = v;
    	    	addr += nextbyte;
 		getbyte(addr, &tuplesize);
    	    	*buffer++ = tuplesize;
     	    	addr += nextbyte;
  	    	while (size-- > 0 && tuplesize-- > 0) {
		    getbyte(addr, &v);
		    *buffer++ = v;
     	    	    addr += nextbyte;
		}
		ret = TRUE;
		goto end;
	    }
	}

        tuplesize = 0;
        if (!final && type != CISTPL_NULL) {
            if (!getbyte(addr + nextbyte, &tuplesize))
            	goto end;
            if (tuplesize == 0xff)
            	final = TRUE;
        }

#if TUPLELOGGING
    if (buffer == NULL) {
        UBYTE outbuf[(256 + 2 + 1) * 3];
        UWORD ts;
        UBYTE *p = outbuf;
        for (ts = 0; ts < tuplesize + 2; ts++) {
            getbyte(addr + ts * nextbyte, &v);
            byte2ascii(&p, v);
        }
        bug("%s\n", outbuf);
    }
#endif

   	if (final) {
    	    UBYTE buf[5];
    	    TUPLEDEBUG(bug("Next link %08x\n", nextjump));
	    if (nextjump == 0xffffffff) {
	    	if (buffer == NULL)
	    	    ret = TRUE;
	    	break;
	    }
	    addr = nextjump;
	    nextbyte = nextjump < GAYLE_ATTRIBUTE ? 1 : 2;
	    nextjump = 0xffffffff;
	    /* valid LINKTARGET? */
	    if (!getbytes(addr, 5, nextbyte, buf))
	    	goto end;
	    if (buf[0] != CISTPL_LINKTARGET || buf[1] < 3 ||
		buf[2] != 'C' || buf[3] != 'I' || buf[4] != 'S') {
	    	 TUPLEDEBUG(bug("Invalid or missing linktarget\n"));
	    	 break;
	    }
	    continue;
	}
	
	if (type == CISTPL_NULL) {
	    addr += nextbyte;
	} else {
	    if (!getbyte(addr + nextbyte, &v))
	    	goto end;
	    addr += (2 + tuplesize) * nextbyte;
	}
	
    }
    
end:
    gio->config = oldconfig;

    Permit();
    
    TUPLEDEBUG(bug ("CopyTuple finished\n"));

    return ret;

    AROS_LIBFUNC_EXIT
}
