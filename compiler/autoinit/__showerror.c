#include <intuition/intuition.h>
#include <dos/dosextens.h>
#include <dos/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdarg.h>

/*
  Redefine this variable in your program and set
  its value to a value different than 0 to force
  the use of a requester in case of error
*/
int __forceerrorrequester __attribute__((weak)) = 0;

int __includeshowerror;

void __showerror_real(int code, char *title, char *format, ...)
{
    struct Process *me = (struct Process *)FindTask(0);

    va_list args;
    va_start(args, format);


    if (me->pr_CLI && !__forceerrorrequester)
    {
	if (DOSBase)
	{
	    PutStr(title);
	    PutStr(": ");
	    VPrintf(format, args);
	    PutStr("\n");
        }
    }
    else
    {
     	if (IntuitionBase)
	{
    	    struct EasyStruct es =
    	    {
		sizeof(struct EasyStruct),
		0,
		title,
		format,
		"Exit"
	    };

	    EasyRequestArgs(NULL, &es, NULL, (APTR)args);
	}
    }

    if (!IoErr()) SetIoErr(code);

    va_end(args);
}

const void *__showerrorptr = &__showerror_real;