/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Assign CLI command
    Lang: english
*/

#include <aros/config.h>

#if !(AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#define ENABLE_RT 1
#endif

#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <aros/rt.h>

static const char version[] = "$VER: assign 41.4 (24.7.1997)\n";

/* Assign mode */
enum {ASSIGN_REPLACE = 0, ASSIGN_ADD };

void dolist()
{
    struct DosList *dlist, *curlist;
    int count;

    dlist = LockDosList(LDF_VOLUMES | LDF_ASSIGNS | LDF_DEVICES | LDF_READ);

    PutStr("Volumes:\n");
    curlist = dlist;

    /* Print mounted volumes */
    while((curlist = NextDosEntry(curlist, LDF_VOLUMES)))
    {
	VPrintf("%s [Mounted]\n", (IPTR *)&(curlist->dol_DevName));
    }

    PutStr("\nDirectories:\n");
    curlist = dlist;

    /* Print assigned directories */
    while((curlist = NextDosEntry(curlist, LDF_ASSIGNS)))
    {
	PutStr(curlist->dol_DevName);

	for(count = 15 - strlen(curlist->dol_DevName); count > 0; count--)
	    FPutC(Output(), ' ');

	FPutC(Output(), '\n'); /* !!! print directory !!! */
    }

    /* !!! late/nonbinding !!! */
    PutStr("\nDevices:\n");
    count = 0;
    curlist = dlist;

    while((curlist = NextDosEntry(curlist, LDF_DEVICES)))
    {
	VPrintf("%s ", (IPTR *)&(curlist->dol_DevName));
	count++;

	if(count == 5)
	{
	    FPutC(Output(), '\n');
	    count = 0;
	}
    }

    if(count < 5)
	FPutC(Output(), '\n');

    UnLockDosList(LDF_VOLUMES|LDF_ASSIGNS|LDF_DEVICES|LDF_READ);
}


int doassign(STRPTR name, STRPTR target, int mode)
{
    int error = RETURN_OK;
    BPTR dir;

    dir = Lock(target, SHARED_LOCK);

    if(dir)
    {
	STRPTR s = name;

	while(*s)
	{
	    if(*s == ':')
	    {
		*s = 0;
		break;
	    }
	    s++;
	}
	
	switch (mode)
	{
	case ASSIGN_REPLACE:
	    if(!AssignLock(name, dir))
	    	error = RETURN_FAIL;
	    break;
	    
	case ASSIGN_ADD:
	    if(!AssignAdd(name, dir))
	    	error = RETURN_FAIL;
	    
	    break;
	}
    }
    else
	error = RETURN_FAIL;

    return error;
}


int main (int argc, char ** argv)
{
    STRPTR args[4] = { NULL, NULL, NULL, NULL };
    struct RDArgs *rda;
    int error = RETURN_OK;

    RT_Init();

    rda = ReadArgs("NAME,TARGET,LIST/S,ADD/S", (IPTR *)args, NULL);

    if(rda != NULL)
    {
	if(args[0] != NULL && args[1] != NULL)
	{
	    int mode = ASSIGN_REPLACE;
	    
	    if (args[3] != NULL)
	    {
	    	mode = ASSIGN_ADD;
	    }
	    
	    error = doassign(args[0], args[1], mode);
	}

	if(args[0] == NULL || args[2] != NULL)
	    dolist();
	
	FreeArgs(rda);
    }
    else
	error = RETURN_FAIL;

    if(error)
    {
	PrintFault(IoErr(),"Assign");
    }

    RT_Exit();
    return error;
}
