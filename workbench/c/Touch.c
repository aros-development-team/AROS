#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/filesystem.h>

int __nocommandline;

int main(void)
{
    IPTR           args[1] = {NULL};
    struct RDArgs *rda     = ReadArgs("NAME/A", args, NULL);

    if (rda)
    {
	BPTR fh = Open((STRPTR)args[0], FMF_CREATE);
        if (fh)
	{
	    Close(fh);
	    return RETURN_OK;
	}

	PrintFault(IoErr(), NULL);
   }
   return RETURN_FAIL;
}
