/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <exec/resident.h>

#include <proto/dos.h>

#include <stdio.h>

extern struct ExecBase *SysBase;

const UBYTE ver[] = "$VER: printresmodules 41.2 (15.3.1997)\n\r";
const UBYTE nul[] = "(null)";

void ResType(UBYTE);

int main(void)
{
    int i;
    ULONG *ptr;

    for(i = 1, ptr = SysBase->ResModules; *ptr; i++, ptr++)
    {
	if(*ptr & 0x80000000) ptr = (ULONG *)(*ptr & 0x7fffffff);

	{
	    struct Resident *res = (struct Resident *)*ptr;

	    Printf("%2lu: 0x%08lx %4ld ", i, *ptr, res->rt_Pri);
	    ResType(res->rt_Flags);
	    Printf("%s\n", (ULONG)res->rt_Name ? (ULONG)res->rt_Name : (ULONG)&nul);
	}
    }

    return 0;
}

void ResType(UBYTE type)
{
    int i = 0;

    if (type == RTF_AUTOINIT)
    {
	PutStr("               RTF_AUTOINIT   "); i++;
    }
    else
    if (type == RTF_SINGLETASK)
    {
	PutStr("               RTF_SINGLETASK "); i++;
    }
    else
    {
	if (type & RTF_COLDSTART ) { PutStr("RTF_COLDSTART  "); i++; }
	if (type & RTF_AUTOINIT  ) { PutStr("RTF_AUTOINIT   "); i++; }
	if (type & RTF_AFTERDOS  ) { PutStr("RTF_AFTERDOS   "); i++; }
	if (type & RTF_SINGLETASK) { PutStr("RTF_SINGLETASK "); i++; }
	switch(i)
	{
	    case 0:
		PutStr("                              ");
		break;
	    case 1:
		PutStr("               ");
		break;
	    default:
		break;
	} /* switch */
    }
}
