#define  DEBUG  1
#include <aros/debug.h>

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

static const char version[] = "$VER: openlib.c 41.0 (16.1.2004)\n";

#define ARG_TEMPLATE "LIBRARY/A,DIRTY/S"

enum 
{
	ARG_LIBNAME,
	ARG_DIRTY,
	NOOFARGS
};

int main(int argc, char **argv)
{
	IPTR args[NOOFARGS] = {0,      // ARG_LIBNAME
	                       FALSE   // ARG_DIRTY
	                     };
	struct RDArgs *rda;
	rda = ReadArgs(ARG_TEMPLATE, args, NULL);
	if (NULL != rda)
	{
		if (args[ARG_LIBNAME]!=0)
		{
			APTR tmpBase = OpenLibrary((STRPTR *)args[ARG_LIBNAME],0);
			if (tmpBase != 0)
			{
				printf ("OPENLIB.c: Succesfully opened !\n");

				if (args[ARG_DIRTY] == FALSE)
				{
					CloseLibrary(tmpBase);
					printf ("OPENLIB.c: Library closed\n");
				} 

			}else{
				printf("OPENLIB.c: Library open FAILED!\n");
			}
		}else{
			printf("OPENLIB.c: Please specify a library to open!\n");
		}
		FreeArgs(rda);
	}	
	return 0;
}
