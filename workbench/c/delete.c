#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

int main (int argc, char ** argv)
{
    STRPTR args[1]={ 0 };
    struct RDArgs *rda;
    LONG error=0;

    rda=ReadArgs("FILE/A",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	DeleteFile(args[0]);
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Delete");
    return error;
}
