#include <proto/exec.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "iso9660.h"
#include "joliet.h"
#include "globals.h"
#include "aros_stuff.h"
#include "clib_stuff.h"
 
/*
 * History:
 *
 * 06-Mar-09 error   - Removed madness, fixed insanity. Cleanup started
 */

extern struct Globals *global;

#ifdef SysBase
#	undef SysBase
#endif
#define SysBase global->SysBase

t_bool Uses_Joliet_Protocol(CDROM *p_cdrom, t_ulong offset, t_ulong *p_svdoffset)
{
	t_ulong svd_offset;
	sup_vol_desc *svd;

	for (svd_offset = 17;;svd_offset++)
	{
		if (!Read_Chunk(p_cdrom, svd_offset + offset))
			return FALSE;
		svd = (sup_vol_desc *)p_cdrom->buffer;
		if (StrNCmp(svd->id, "CD001", 5) != 0)
			return FALSE;
		if (svd->type == 255)
			return FALSE;
		if ((svd->type == 2) && (svd->escape[0] == 0x25) && (svd->escape[1] == 0x2F) &&
		    ((svd->escape[2] == 0x40) || (svd->escape[2] == 0x43) || (svd->escape[2] == 0x45))) {
			*p_svdoffset = svd_offset;
			return TRUE;
		}
	}
}

int Get_Joliet_Name(char *from, char *to, unsigned char len)
{
	int i;
	int l = 0;
	UWORD *u_from;
	UWORD uc;

	u_from = (UWORD *)from;
	for (i = 0; i < len; i+=2) {
		uc = AROS_BE2WORD(u_from[l]);
		to[l++] = global->g_unicode_table[uc];
	}
	return l;
}

