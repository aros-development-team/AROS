/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Assign CLI command
    Lang: English
*/

#define  DEBUG  0
#include <aros/debug.h>
#include <aros/config.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <string.h>


/******************************************************************************


    NAME

        Assign [(name):] [{(target)}] [LIST] [EXISTS] [DISMOUNT] [DEFER]
	       [PATH] [ADD] [REMOVE] [VOLS] [DIRS] [DEVICES]

    SYNOPSIS

        NAME, TARGET/M, LIST/S, EXISTS/S, DISMOUNT/S, DEFER/S, PATH/S, ADD/S,
	REMOVE/S, VOLS/S, DIRS/S, DEVICES/S

    LOCATION

        Workbench:C

    FUNCTION

        ASSIGN creates a reference to a file or directory. The reference 
	is a logical device name which makes it very convenient to specify
	assigned objects using the reference instead of their paths.

	If the NAME and TARGET arguments are given, ASSIGN assigns the
	given logical name to the specified target. If the NAME given is
	already assigned to a file or directory the new target replaces the 
	previous target. A colon must be included after the NAME argument.

	If only the NAME argument is given, any assigns to that NAME are
	removed. If no arguments whatsoever are given, all logical
	assigns are listed.

    INPUTS

        NAME      --  the name that should be assigned to a file or directory
	TARGET    --  one or more files or directories to assign the NAME to
	LIST      --  list all assigns made
	EXISTS    --  if NAME is already assigned, set the condition flag to
	              WARN
	DISMOUNT  --  remove the volume or device NAME from the dos list
	DEFER     --  make an ASSIGN to a path or directory that not need to
	              exist at the time of assignment. The first time the
		      NAME is referenced the NAME is bound to the object
	PATH      --  path to assign with a non-binding assign. This means
	              that the assign is re-evaluated each time a reference
		      to NAME is done. Like for DEFER, the path doesn't have
		      to exist when the ASSIGN command is executed
	ADD       --  don't replace an assign but add another object for a
                      NAME (multi-assigns)
	REMOVE    --  remove an ASSIGN
	VOLS      --  show assigned volumes if in LIST mode
	DIRS      --  show assigned directories if in LIST mode
	DEVICES   --  show assigned devices if in LIST mode
        

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

        The assign command has many switches. This together with the somewhat
	messy handling of DosList:s by dos.library makes the operation rather
	complicated.

	There are some fundamental building blocks that defines the semantics
	of the Assign command.

	Only one of the switches ADD, REMOVE, PATH and DEFER may be specified.
	
	If EXISTS is specified, only the name parameter is important.
	
	The implementation is split up in two fundamental procedures.
	
	doAssign()     --  make [a number of] assigns
	showAssigns()  --  show the available assigns
	checkAssign()  --  check if a particular assign exists
	
    HISTORY

******************************************************************************/

static const char version[] = "$VER: assign 41.5 (12.11.2000)\n";


/* Prototypes */

int checkAssign(STRPTR name);
int doAssign(STRPTR name, STRPTR *target, BOOL dismount, BOOL defer, BOOL path,
	     BOOL add, BOOL remove);
void showAssigns(BOOL vols, BOOL dirs, BOOL devices);
int removeAssign(STRPTR name);
STRPTR GetFullPath(BPTR lock);


#define  ARG_TEMPLATE  "NAME,TARGET/M,LIST/S,EXISTS/S,DISMOUNT/S,DEFER/S,PATH/S,ADD/S,REMOVE/S,VOLS/S,DIRS/S,DEVICES/S"

enum
{
    ARG_NAME = 0,
    ARG_TARGET,
    ARG_LIST,
    ARG_EXISTS,
    ARG_DISMOUNT,
    ARG_DEFER,
    ARG_PATH,
    ARG_ADD,
    ARG_REMOVE,
    ARG_VOLS,
    ARG_DIRS,
    ARG_DEVICES,

    NOOFARGS
};


int __nocommandline = 1;
int main (void)
{
    IPTR args[NOOFARGS] = { NULL, NULL, (IPTR)FALSE, (IPTR)FALSE, (IPTR)FALSE,
			    (IPTR)FALSE, (IPTR)FALSE, (IPTR)FALSE, (IPTR)FALSE,
			    (IPTR)FALSE, (IPTR)FALSE, (IPTR)FALSE };
    struct RDArgs *rda;
    int error = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if(rda != NULL)
    {
	/* Get correct types and reduce complexity for clarity */
	STRPTR  name     = (STRPTR)args[ARG_NAME];
	STRPTR *target   = (STRPTR *)args[ARG_TARGET];
	BOOL    list     = (BOOL)args[ARG_LIST];
	BOOL    exists   = (BOOL)args[ARG_EXISTS];
	BOOL    dismount = (BOOL)args[ARG_DISMOUNT];
	BOOL    defer    = (BOOL)args[ARG_DEFER];
	BOOL    path     = (BOOL)args[ARG_PATH];
	BOOL    add      = (BOOL)args[ARG_ADD];
	BOOL    remove   = (BOOL)args[ARG_REMOVE];
	BOOL    vols     = (BOOL)args[ARG_VOLS];
	BOOL    dirs     = (BOOL)args[ARG_DIRS];
	BOOL    devices  = (BOOL)args[ARG_DEVICES];

	int     attributes = 0;

	int     i;

	D(bug("Assign: Starting up\n"
	      "list = %i\n"
	      "exists = %i\n"
	      "dismount = %i\n"
	      "defer = %i\n"
	      "path = %i\n"
	      "add = %i\n"
	      "remove = %i\n"
	      "vols = %i\n"
	      "dirs = %i\n"
	      "devices = %i\n",
	      list, exists, dismount, defer, path, add, remove, vols, dirs,
	      devices));

	for(i = 0; target[i] != NULL; i++)
	{
	    D(bug("%s\n", target[i]));
	}
	
	/* This is a type hack */
	attributes += add;
	attributes += remove;
	attributes += path;
	attributes += defer;

	if(attributes > 1)
	{
	    printf("Only one of ADD, REMOVE, PATH or DEFER is allowed\n");
	    FreeArgs(rda);

	    return RETURN_FAIL;
	}
	
	/* If the EXISTS keyword is specified, we only care about NAME */
	if(exists)
	{
	    D(bug("Calling checkAssign()\n"));
	    error = checkAssign(name);
	}
	else if(name != NULL)
	{
	    /* If a NAME is specified, our primary task is to add or
	       remove an assign */
	    
	    D(bug("Calling doAssign()\n"));
	    error = doAssign(name, target, dismount, defer, path, add,
			     remove);
	    
	    if(list)
	    {
		/* With the LIST keyword, the current assigns will be
		   displayed also when (after) making an assign */
		
		showAssigns(vols, dirs, devices);
	    }
	}
	else
	{
	    /* If no NAME was given, we just show the current assigns
	       as specified by the user (VOLS, DIRS, DEVICES) */
	    
	    D(bug("Calling showAssigns()\n"));
	    showAssigns(vols, dirs, devices);
	}
	
	FreeArgs(rda);
    }
    else
    {
	error = RETURN_FAIL;
    }
    
    return error;
}


void showAssigns(BOOL vols, BOOL dirs, BOOL devices)
{
    ULONG           lockBits = LDF_READ;
    struct DosList *dl;

    /* If none of the parameters are specified, everything should be
       displayed */
    if(!(vols || dirs || devices))
    {
	vols    = TRUE;
	dirs    = TRUE;
	devices = TRUE;
    }

    lockBits |= vols    ? LDF_VOLUMES : 0;
    lockBits |= dirs    ? LDF_ASSIGNS : 0;
    lockBits |= devices ? LDF_DEVICES : 0;

    dl = LockDosList(lockBits);

    if(vols)
    {
	struct DosList *tdl = dl;	/* Loop variable */

	printf("Volumes:\n");

	/* Print all mounted volumes */
	while((tdl = NextDosEntry(tdl, LDF_VOLUMES)) != NULL)
	{
	    printf("%s [Mounted]\n", tdl->dol_DevName);
	}
	
	printf("\n");
    }

    if(dirs)
    {
	struct DosList *tdl = dl;       /* Loop variable */
	int             count;	        /* Loop variable */

	printf("Directories:\n");
	
	/* Print all assigned directories */
	while((tdl = NextDosEntry(tdl, LDF_ASSIGNS)) != NULL)
	{
	    printf(tdl->dol_DevName);
	    
	    for(count = 15 - strlen(tdl->dol_DevName); count > 0; count--)
	    {
		printf(" ");
	    }
	    
	    switch(tdl->dol_Type)
	    {
	    case DLT_LATE:
		printf("<%s>\n", tdl->dol_misc.dol_assign.dol_AssignName);
		break;
		
	    case DLT_NONBINDING:
		printf("[%s]\n", tdl->dol_misc.dol_assign.dol_AssignName);
		break;
		
	    default:
		{
		    STRPTR             dirName;     /* For NameFromLock() */
		    struct AssignList *nextAssign;  /* For multiassigns */

		    dirName = GetFullPath(tdl->dol_Lock);
		    
		    if(dirName != NULL)
		    {
			printf("%s\n", dirName);
			FreeVec(dirName);
		    }
		    else
		    {
			printf("\n");
		    }
		    
		    nextAssign = tdl->dol_misc.dol_assign.dol_List;
		    
		    while(nextAssign != NULL)
		    {
			dirName = GetFullPath(nextAssign->al_Lock);
			
			if(dirName != NULL)
			{
			    printf("             + %s\n", dirName);
			    FreeVec(dirName);
			}
			
			nextAssign = nextAssign->al_Next;
		    }
		}

		break;
	    }
	} /* while(NextDosEntry() != NULL) */

	printf("\n");
    }
    
    if(devices)
    {
	struct DosList *tdl = dl;     /* Loop variable */
	int             count = 0;    /* Used to make sure that as most 5
				         entries are printed per line */
	
	printf("Devices:\n");
	
	/* Print all assigned devices */
	while((tdl = NextDosEntry(tdl, LDF_DEVICES)) != NULL)
	{
	    printf("%s ", tdl->dol_DevName);
	    count++;
	    
	    if(count == 5)
	    {
		printf("\n");
		count = 0;
	    }
	}
	
	if(count <  5)
	{
	    printf("\n");
	}
    }
    
    UnLockDosList(lockBits);
}


STRPTR GetFullPath(BPTR lock)
{
    UBYTE  *buf;       /* Pointer to the memory allocated for the string */
    ULONG   size;      /* Holder of the (growing) size of the string */

    for(size = 512; ; size += 512)
    {
	buf = AllocVec(size, MEMF_ANY);

	if(buf == NULL)
	{
	    break;
	}

	if(NameFromLock(lock, buf, size))
	{
	    return (STRPTR)buf;
	}

	FreeVec(buf);
    }

    return NULL;
}


int doAssign(STRPTR name, STRPTR *target, BOOL dismount, BOOL defer, BOOL path,
	     BOOL add, BOOL remove)
{
    STRPTR colon;
    BPTR   lock = NULL;
    int    i;			/* Loop variable */

    int  error = RETURN_OK;

    colon = strchr(name, ':');

    /* Correct assign name construction? The rule is that the device name
       should end with a colon at the same time as no other colon may be
       in the name. */
    if(colon == NULL || colon[1] != 0)
    {
	printf("Invalid device name %s\n", name);

	return RETURN_FAIL;
    }

    *colon = 0;		      /* Remove trailing colon; name[] is changed! */

    /* This is a little bit messy... We first remove the 'name' assign
       and later in the loop the target assigns. */
    if(*target == NULL || remove)
    {
	removeAssign(name);
    }

    // The Loop over multiple targets starts here

    for(i = 0; target[i] != NULL; i++)
    {
	if(!(path || defer || dismount))
	{
	    lock = Lock(target[i], SHARED_LOCK);
	    
	    if(lock == NULL)
	    {
		printf("Can't find %s\n", target[i]);
		return RETURN_FAIL;
	    }
	}
	
	if(remove)
	{
	    removeAssign(target[i]);
	}
	else if(dismount)
	{
	    struct DosList *dl;
	    BOOL            success;
	    
	    dl = LockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);
	    
	    /* Note the ! for conversion to boolean value */
	    success = !RemDosEntry(FindDosEntry(dl, name,
						LDF_VOLUMES | LDF_DEVICES));
	    
	    UnLockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);
	    
	    /* AmigaDOS doesn't use RETURN_WARN here... but it should? */
	    error = success ? error : RETURN_WARN;
	}
	else if(path)
	{
	    error = AssignPath(name, target[i]) == DOSTRUE ? error : RETURN_FAIL;
	}
	else if(add)
	{
	    if(AssignAdd(name, lock) == DOSFALSE)
	    {
		UnLock(lock);
		error = RETURN_FAIL;
	    }
	}
	else if(defer)
	{
	    if(AssignLate(name, target[i]) == DOSFALSE)
	    {
		UnLock(lock);
		error = RETURN_FAIL;
	    }
	}
	else 
	{
	    /* If no extra parameters are specified we just do a regular
	       assign (replacing any possible previous assign with that
	       name. The case of target being NULL is taken care of above. */
	    if(AssignLock(name, lock) == DOSFALSE)
	    {
		UnLock(lock);
		error = RETURN_FAIL;
	    }	
	}

	/* We break as soon as we get a serious error */
	if(error == RETURN_FAIL)
	{
	    return error;
	}

    } /* loop through all targets */

    return error;
}


int removeAssign(STRPTR name)
{
    /* In case no target is given, the 'name' assign should be removed.
       The AmigaDOS semantics for this is apparently that the error
       code is never set even if the assign didn't exist. */
    
    struct DosList *dl;
    struct DosList *element;
    
    dl = LockDosList(LDF_ASSIGNS | LDF_WRITE);
    
    element = FindDosEntry(dl, name, LDF_ASSIGNS);

    if(element != NULL)
    {
	UnLock(element->dol_Lock);
    }

    /* For now, we don't free any (possible) multiassign locks. Hopefully
       RemAssignList() should be able to do this, but I'm not sure... */

    RemDosEntry(element);
    
    UnLockDosList(LDF_ASSIGNS | LDF_WRITE);
    
    return RETURN_OK;
}


int checkAssign(STRPTR name)
{
    struct DosList *dl;
    int             error = RETURN_OK;

    dl = LockDosList(LDF_DEVICES | LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);

    if (FindDosEntry(dl, name, LDF_DEVICES | LDF_ASSIGNS | LDF_VOLUMES) == NULL)
    {
	error = RETURN_WARN;
    }

    UnLockDosList(LDF_DEVICES | LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);

    return error;
}

