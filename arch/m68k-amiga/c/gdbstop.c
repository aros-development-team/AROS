#include <proto/dos.h>
#include <proto/exec.h>


int main(int argc, char **argv)
{
    APTR DOSBase;

    if ((DOSBase = OpenLibrary("dos.library", 0))) {
	PutStr("Trapping GDB breakpoint\n");
	asm volatile ("trap #1\n");
	PutStr("Back from GDB breakpoint\n");
	CloseLibrary(DOSBase);
    } else {
    	return RETURN_ERROR;
    }
    return RETURN_OK;
}
