#include <proto/exec.h>
#include <proto/dos.h>

struct DosLibrary *DOSBase;
static unsigned int level = 0;

void test()
{
    Printf("Nest level: %lu\n", ++level);

    if (level < 20)
    	test();

    Exit(0);
    Printf("Exit() did not work!\n");
}

AROS_ENTRY(__startup static int, Start,
	   AROS_UFHA(char *, argstr, A0),
	   AROS_UFHA(ULONG, argsize, D0),
	   struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    struct Process *me;
    IPTR *stackbase;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (!DOSBase)
    	return RETURN_FAIL;

    /* Let's check correctness of stack size passing */
    me = (struct Process *)FindTask(NULL);
    stackbase = me->pr_ReturnAddr;
    Printf("Stack size is set to %ld (should be %ld)\n", stackbase[0], me->pr_Task.tc_SPUpper - me->pr_Task.tc_SPLower);

    test();
    
    CloseLibrary(&DOSBase->dl_lib);
    return RETURN_OK;

    AROS_USERFUNC_EXIT
}
