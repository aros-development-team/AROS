#include <exec/execbase.h>
#include <exec/resident.h>

#include <proto/dos.h>

#include <stdio.h>

extern struct ExecBase *SysBase;

const UBYTE ver[] = "$VER: printresmodules 41.1 (8.2.97)\n\r";

void ResType(UBYTE);

int main(void)
{
    int i;
    ULONG *ptr;

    for(i = 1, ptr = SysBase->ResModules; *ptr; i++, ptr++)
    {
	struct Resident *res = (struct Resident *)*ptr;

	Printf("%2lu: 0x%08lx %4ld ", i, *ptr, res->rt_Pri);
	ResType(res->rt_Flags);
	Printf("%s\n", (ULONG)res->rt_Name);
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
