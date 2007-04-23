#include <exec/types.h>
#include <exec/ptrace.h>

#include <proto/exec.h>

#define DEBUG 1
#include <aros/debug.h>

void show_reg(ULONG reg)
{
	D(bug("CONTENT: %x\n",reg));
}

void show_r0(ULONG reg)
{
	D(bug("CONTENT R0: %x\n",reg));
}
