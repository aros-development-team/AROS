#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define STACK_SIZE 16384

struct StackSwapStruct sss;

int __nocommandline = 1;

void PrintSSS(struct StackSwapStruct *ss)
{
    Printf("StackSwapStruct contents:\n");
    Printf("Lower: 0x%P, Upper: 0x%P, SP: 0x%P\n", ss->stk_Lower, ss->stk_Upper, ss->stk_Pointer);
}

void PrintTaskStack(void)
{
    struct Task *t = FindTask(NULL);
    
    Printf("Current program stack:\n");
    Printf("Lower: 0x%P, Upper: 0x%P, SP: 0x%P\n", t->tc_SPLower, t->tc_SPUpper, t->tc_SPReg);
}

void Sub(IPTR a1, IPTR a2)
{
    Printf("Stack swap done\n");
    PrintSSS(&sss);
    PrintTaskStack();
    Printf("Arguments: %08lX, %08lX\n", a1, a2);
    Printf("Coming back to original stack...\n");
}

int main(void)
{
    struct StackSwapArgs args;

    sss.stk_Lower = AllocMem(STACK_SIZE, MEMF_ANY);
    if (!sss.stk_Lower)
    {
	Printf("Failed to allocate new stack!\n");

	return 1;
    }

    sss.stk_Upper = sss.stk_Lower + STACK_SIZE;
    sss.stk_Pointer = sss.stk_Upper;
    PrintSSS(&sss);
    PrintTaskStack();
/* Commented out because it fails. I wonder why...
   Perhaps out StackSwap() is broken, at least on i386...
   Okay, let's see...
    Printf("Checking StackSwap()...\n");
    StackSwap(&sss);

    Sub(0x12345678, 0xC001C0DE);

    StackSwap(&sss);
    Printf("Came back from StackSwap()\n");
    PrintSSS(&sss);
    PrintTaskStack();
*/
    Printf("Checking NewStackSwap()...\n");
    args.Args[0] = 0x1234ABCD;
    args.Args[1] = 0xC0DEC001;

    NewStackSwap(&sss, Sub, &args);
    Printf("Came back from StackSwap()\n");
    PrintSSS(&sss);
    PrintTaskStack();

    FreeMem(sss.stk_Lower, STACK_SIZE);

    Printf("Done!\n");
    return 0;
}
