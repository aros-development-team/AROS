#include <exec/types.h>
#include <exec/ptrace.h>
#define DEBUG 1
#include <aros/debug.h>

void show_reg(ULONG reg)
{
	AROS_GET_SYSBASE
	D(bug("CONTENT: %x\n",reg));
}

void show_r0(ULONG reg)
{
	AROS_GET_SYSBASE
	D(bug("CONTENT R0: %x\n",reg));
}
