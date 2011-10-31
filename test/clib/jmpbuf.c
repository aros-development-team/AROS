#include <exec/tasks.h>
#include <proto/exec.h>

#include <setjmp.h>
#include <stdio.h>

jmp_buf buf;

static void HexDump(const UBYTE *data, ULONG count)
{
    ULONG t, end;

    end = (count + 15) & -16;

    for (t=0; t<end; t++)
    {
	if ((t&15) == 0)
	    printf("0x%08X:", (unsigned)t);

	if ((t&3) == 0)
	    printf(" ");

	if (t < count)
	    printf("%02x", ((UBYTE *)data)[t]);
	else
	    printf("  ");

	if ((t&15) == 15)
	{
	    printf("\n");
	}
    }
} /* hexdump */

int main(void)
{
    struct Task *me;

    setjmp(buf);

    me = FindTask(NULL);    
    printf("Task 0x%p (%s), stack 0x%p - 0x%p\n", me, me->tc_Node.ln_Name, me->tc_SPLower, me->tc_SPUpper);
    printf("Function at 0x%p%p\n", main);
    printf("Buffer at 0x%p (%u bytes)\n", buf, (unsigned int)sizeof(buf));

    HexDump((const UBYTE *)buf, sizeof(buf));
    
    return 0;
}
