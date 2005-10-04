/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Dir CLI command.
*/

#define  DEBUG  1
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>



/******************************************************************************


    NAME

    Dir [(dir | pattern)] [OPT A | I | D | F] [ALL] [DIRS] [FILES] [INTER]


    SYNOPSIS

    DIR,OPT/K,ALL/S,DIRS/S,FILES/S,INTER/S

    LOCATION

    Workbench:C

    FUNCTION

    DIR displays the file or directory contained in the current or 
    specified directory. Directories get listed first, then in alphabetical
    order, the files are listed in two columns. Pressing CTRL-C aborts the
    directory listing.


    INPUTS

    ALL    --  Display all subdirectories and their files recursively.
    DIRS   --  Display only directories.
    FILES  --  Display only files.
    INTER  --  Enter interactive mode.

               Interactive listing mode stops after each name to display
	       a question mark at which you can enter commands. These
	       commands are:

	       Return      --  Goto the next file or directory.
	       E/ENTER     --  Enters a directory.
	       DEL/DELETE  --  Delete a file or an empty directory.
	       C/COM       --  Let the file or directory be the input of
	                       a DOS command (which specified after the C or
			       COM or specified separately later).
	       Q/QUIT      --  Quit interactive mode.
	       B/BACK      --  Go back one directory level.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

static const char version[] = "$VER: Dir 41.12 (27.11.2000)\n";

struct table
{
    char **entries;
    int    num;
    int    max;
};


/*  Prototypes  */

static LONG doPatternDir(STRPTR dirPat, BOOL all, BOOL doDirs, BOOL doFiles,
			 BOOL inter);
static LONG doDir(STRPTR dir, BOOL all, BOOL dirs, BOOL files, BOOL inter);

static void showline(char *fmt, LONG args[]);
static void maybeShowline(char *format, LONG args[], BOOL doIt, BOOL inter);
static void maybeShowlineCR(char *format, LONG args[], BOOL doIt, BOOL inter);

static int CheckDir(BPTR lock, struct ExAllData *ead, ULONG eadSize,
		    struct ExAllControl *eac, struct table *dirs,
		    struct table *files);


static int AddEntry(struct table *table, char *entry)
{
    char *dup;
    
    if (table->num == table->max)
    {
	int    new_max = table->max + 128;
	char **new_entries;

	new_entries = AllocVec(sizeof(char *)*new_max, MEMF_ANY);

	if (new_entries == NULL)
	    return 0;
	
	if (table->num)
	{
	    CopyMemQuick(table->entries, new_entries,
			 sizeof(char *)*table->num);
	    FreeVec(table->entries);
	}
	
	table->entries = new_entries;
	table->max = new_max;
    }
    
    if (!(dup = strdup (entry)) )
	return 0;
    
    table->entries[table->num++] = dup;

    return 1;
}


static int compare_strings(const void * s1, const void * s2)
{
    return strcasecmp(*(char **)s1, *(char **)s2);
}


int indent = 0;


static void maybeShowlineCR(char *format, LONG args[], BOOL doIt, BOOL inter)
{
    maybeShowline(format, args, doIt, inter);

    if (!inter)
    {
	VPrintf("\n", NULL);
    }
}


#define  INTERARG_TEMPLATE  "E=ENTER/S,B=BACK/S,DEL=DELETE/S,Q=QUIT/S,C=COM/S,COMMAND"

enum
{
    INTERARG_ENTER = 0,
    INTERARG_BACK,
    INTERARG_DELETE,
    INTERARG_QUIT,
    INTERARG_COM,
    INTERARG_COMMAND,
    NOOFINTERARGS
};


static void maybeShowline(char *format, LONG args[], BOOL doIt, BOOL inter)
{
    if(doIt)
    {
	showline(format, args);

#if 0
	if(inter)
	{
	    struct ReadArgs *rda;

	    IPTR  interArgs[NOOFINTERARGS] = { (IPTR)FALSE,
					       (IPTR)FALSE,
					       (IPTR)FALSE,
					       (IPTR)FALSE,
					       (IPTR)FALSE,
					       NULL };
	    
	    rda = ReadArgs(INTERARG_TEMPLATE, interArgs, NULL);
	    
	    if (rda != NULL)
	    {
		if (interArgs[ARG_ENTER])
		{
		    return c_Enter;
		}
		else if (interArgs[ARG_BACK])
		{
		    return c_Back;
		}
		else if (interArgs[ARG_DELETE])
		{
		    return c_Delete;
		}
		else if (interArgs[ARG_QUIT])
		{
		    return c_Quit;
		}
		else if (interArgs[ARG_COM])
		{
		    return c_Com;
		}
		else if (interArgs[ARG_COMMAND] != NULL)
		{
		    command = 
		    return c_Command;
		}
	    }
	}
#endif

    }
}


static void showline(char *fmt, LONG args[])
{
    int t;
    
    for (t = 0; t < indent; t++)
	VPrintf("    ", NULL);

    VPrintf(fmt, args);
}


#define  ARG_TEMPLATE  "DIR,OPT/K,ALL/S,DIRS/S,FILES/S,INTER/S"

enum
{
    ARG_DIR = 0,
    ARG_OPT,
    ARG_ALL,
    ARG_DIRS,
    ARG_FILES,
    ARG_INTER
};

int main(int argc, char **argv)
{
    struct RDArgs *rda;
    IPTR           args[] =
    {
        (IPTR) NULL,
	    (IPTR) NULL,
			   FALSE,
			   FALSE,
			   FALSE,
               FALSE
    };

    LONG  error = RETURN_ERROR;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if (rda != NULL)
    {
	STRPTR dir = (STRPTR)args[ARG_DIR];
	STRPTR opt = (STRPTR)args[ARG_OPT];
	BOOL   all = (BOOL)args[ARG_ALL];
	BOOL   dirs = (BOOL)args[ARG_DIRS];
	BOOL   files = (BOOL)args[ARG_FILES];
	BOOL   inter = (BOOL)args[ARG_INTER];

	/* Convert the OPT arguments (if any) into the regular switches */
	if (opt != NULL)
	{
	    while (*opt != (IPTR) NULL)
	    {
		switch (ToUpper(*opt))
		{
		case 'D':
		    dirs = TRUE;
		    break;

		case 'F':
		    files = TRUE;
		    break;

		case 'A':
		    all = TRUE;
		    break;

		case 'I':
		    inter = TRUE;
		    break;

		default:
		    Printf("%c option ignored\n", *opt);
		    break;
		}

	        opt++;
	    }
	}

	if(dir == NULL)
	{
	    dir = "";
	}

	if(!files && !dirs)
	{
	    files = TRUE;
	    dirs  = TRUE;
	}

	error = doPatternDir(dir, all, dirs, files, inter);
        if (error != RETURN_OK)
        {
                    LONG ioerr = IoErr();
                    switch (ioerr)
                    {
                    case ERROR_NO_MORE_ENTRIES:
                        ioerr = 0;
                        break;
                    case ERROR_OBJECT_WRONG_TYPE:
                        Printf("%s is not a directory\n", (ULONG)dir);
                        ioerr = ERROR_DIR_NOT_FOUND;
                        break;
                    default:
                        Printf("Could not get information for %s\n", (ULONG)dir);
                    }
                    PrintFault(ioerr, NULL);
        }
	FreeArgs(rda);
    } else
	PrintFault(IoErr(), NULL);

    return error;
}


#define  MAX_PATH_LEN  512

static LONG doPatternDir(STRPTR dirPat, BOOL all, BOOL doDirs, BOOL doFiles,
			 BOOL inter)
{
    struct AnchorPath *ap;	/* Matching structure */

    LONG  match;		/* Loop variable */
    LONG  error = RETURN_FAIL;

    ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + 
				       MAX_PATH_LEN, MEMF_CLEAR);

    if(ap != NULL)
    {
	ap->ap_Strlen = MAX_PATH_LEN;

	for(match = MatchFirst(dirPat, ap); match == 0; match = MatchNext(ap))
	{
	    error = doDir(ap->ap_Buf, all, doDirs, doFiles, inter);

	    if(error == RETURN_FAIL)
	    {
		break;
	    }
	}

	MatchEnd(ap);
    }

    FreeVec(ap);

    return error;
}


static LONG doDir(STRPTR dir, BOOL all, BOOL doDirs, BOOL doFiles, BOOL inter)
{
    BPTR  lock;

    static UBYTE buffer[4096];

    struct ExAllControl  *eac;

    LONG error = RETURN_OK;

    struct table  dirs;
    struct table  files;
    
    dirs.entries = files.entries = NULL;
    dirs.max = files.max = 0;
    dirs.num = files.num = 0;
    
    lock = Lock(dir, SHARED_LOCK);

    if(lock != NULL)
    {
	eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
	
	if(eac != NULL)
	{
	    int t;
	    IPTR argv[3];
	    
	    eac->eac_LastKey = 0;
	
	    error = CheckDir(lock, (struct ExAllData *)buffer, sizeof(buffer),
			     eac, &dirs, &files);

	    FreeDosObject(DOS_EXALLCONTROL, eac);
	    
	    if (error == 0 && doDirs)
	    {
		if (dirs.num != 0)
		{
		    indent ++;
		    
		    qsort(dirs.entries, dirs.num, sizeof(char *),
			  compare_strings);
		    
		    for (t = 0; t < dirs.num; t++)
		    {
			argv[0] = (IPTR)dirs.entries[t];
			
			if (all)
			{
                            char *newpath;
                            int len;
			    int pathlen = strlen(dir);
			    
                            len = pathlen + strlen(dirs.entries[t]) + 2;
			    
                            newpath = AllocVec(len, MEMF_ANY);
			    
                            if (newpath != NULL)
                            {
                                CopyMem(dir, newpath, pathlen + 1);
				
                                if (AddPart(newpath, dirs.entries[t], len))
                                {
                                    maybeShowlineCR(" %s (dir)", argv,
						    doDirs, inter);
                                    error = doDir(newpath, all, doDirs,
						  doFiles, inter);
                                }
                                else
                                {
                                    SetIoErr(ERROR_LINE_TOO_LONG);
                                    error = RETURN_ERROR;
                                }
				
                                FreeVec(newpath);
                            }
                            else
                            {
                                SetIoErr(ERROR_NO_FREE_STORE);
                                error = RETURN_FAIL;
                            }
			    
                            if (error != RETURN_OK)
                                break;
			}
			else
			{
			    maybeShowlineCR(" %s (dir)", argv, doDirs, inter);
			}
		    }
		    
		    indent--;
		}
	    }
	    
	    if (files.num != 0 && doFiles)
	    {
		qsort(files.entries, files.num, sizeof(char *),
		      compare_strings);
		
		for (t = 0; t < files.num; t += 2)
		{
		    argv[0] = (IPTR)(files.entries[t]);
		    argv[1] = (IPTR)(t + 1 < files.num ?
				     files.entries[t+1] : "");
		    
		    if (all)
		    {
			maybeShowlineCR("    %-25.s", argv,
					doFiles, inter);
			
			if (files.num != t + 1)
			{
			    maybeShowlineCR("    %-25.s", argv + 1,
					    doFiles, inter);
			}
		    }
		    else
		    {
			maybeShowline(" %-25.s", argv, doFiles,
				      inter);
			maybeShowlineCR(" %-25.s", argv + 1, doFiles,
					inter);
		    }
		}
	    }
	    
	    if (dirs.num != 0)
	    {
		for (t = 0; t < dirs.num; t++)
		{
		    free(dirs.entries[t]);
		}
		
		if (dirs.entries)
		    FreeVec(dirs.entries);
	    }
	    
	    if (files.num != 0)
	    {
		for (t = 0; t < files.num; t++)
	    {
		free(files.entries[t]);
	    }
		
		if (files.entries)
		{
		    FreeVec(files.entries);
		}
	    }
	}
	else
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    error = RETURN_FAIL;
	}
	
	UnLock(lock);
    }
    else
    {
	error = RETURN_FAIL;
    }
    
    return error;
}


static int CheckDir(BPTR lock, struct ExAllData *ead, ULONG eadSize,
		    struct ExAllControl *eac, struct table *dirs,
		    struct table *files)
{
    int   error = RETURN_OK;
    BOOL  loop;
    struct ExAllData *oldEad = ead;

    do
    {
	ead = oldEad;
	loop = ExAll(lock, ead, eadSize, ED_COMMENT, eac);
	
	if(!loop && IoErr() != ERROR_NO_MORE_ENTRIES)
	{
	    error = RETURN_ERROR;
	    break;
	}
	
	if(eac->eac_Entries != 0)
	{
	    do
	    {
		if (!AddEntry(ead->ed_Type > 0 ? dirs : files,
			      ead->ed_Name))
		{
		    loop = 0;
		    error = RETURN_FAIL;
		    SetIoErr(ERROR_NO_FREE_STORE);
		    break;
		}
	
		ead = ead->ed_Next;
	    } while(ead != NULL);
	    
	}
    } while((loop) && (error == RETURN_OK));

    return error;
}
