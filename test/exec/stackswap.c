#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define print Printf
/*
#include <aros/debug.h>
#define print bug
*/
#define STACK_SIZE 32768

#ifdef __MORPHOS__
#define StackSwapArgs PPCStackSwapArgs
#define NewStackSwap NewPPCStackSwap
#endif

struct StackSwapStruct sss;

int __nocommandline = 1;

void PrintSSS(struct StackSwapStruct *ss)
{
    print("StackSwapStruct contents:\n");
    print("Lower: 0x%P, Upper: 0x%P, SP: 0x%P\n", ss->stk_Lower, ss->stk_Upper, ss->stk_Pointer);
}

void PrintTaskStack(void)
{
    struct Task *t = FindTask(NULL);
    
    print("Current program stack:\n");
    print("Lower: 0x%P, Upper: 0x%P, SP: 0x%P\n", t->tc_SPLower, t->tc_SPUpper, AROS_GET_SP);
}

void Sub(IPTR a1, IPTR a2)
{
    print("Stack swap done\n");
    PrintSSS(&sss);
    PrintTaskStack();
    print("Arguments: %08lX, %08lX\n", a1, a2);
    print("Coming back to original stack...\n");
}

int main(void)
{
    struct StackSwapArgs args;

    sss.stk_Lower = AllocMem(STACK_SIZE, MEMF_ANY);
    if (!sss.stk_Lower)
    {
	print("Failed to allocate new stack!\n");

	return 1;
    }

    sss.stk_Upper = sss.stk_Lower + STACK_SIZE;
    sss.stk_Pointer = sss.stk_Upper - sizeof(IPTR);
    PrintSSS(&sss);
    PrintTaskStack();

    print("Checking StackSwap()...\n");
    StackSwap(&sss);

    Sub(0x12345678, 0xC001C0DE);

    StackSwap(&sss);
    print("Came back from StackSwap()\n");
    PrintSSS(&sss);
    PrintTaskStack();

    print("Checking NewStackSwap()...\n");
    args.Args[0] = 0x1234ABCD;
    args.Args[1] = 0xC0DEC001;

    NewStackSwap(&sss, Sub, &args);
    print("Came back from NewStackSwap()\n");
    PrintSSS(&sss);
    PrintTaskStack();

    FreeMem(sss.stk_Lower, STACK_SIZE);

    print("Done!\n");
    return 0;
}
