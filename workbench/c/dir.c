/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Dir CLI command
    Lang: english
*/
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
#include <aros/debug.h>

static const char version[] = "$VER: dir 41.11 (26.10.1997)\n";

struct UtilityBase *UtilityBase;

struct table
{
    char ** entries;
    int     num, max;
};

char ** files;
int num_files, max_files;
char ** dirs;
int num_dirs, max_dirs;

static int AddEntry (struct table * table, char * entry)
{
    char * dup;

    if (table->num == table->max)
    {
	int new_max = table->max + 128;
	char ** new_entries;

	new_entries = AllocVec (sizeof(char *)*new_max, MEMF_ANY);

	if (!new_entries)
	    return 0;

	if (table->num)
	{
	    CopyMemQuick (table->entries, new_entries, sizeof(char *)* table->num);
	    FreeVec (table->entries);
	}

	table->entries = new_entries;
	table->max = new_max;
    }

    if (!(dup = strdup (entry)) )
	return 0;

    table->entries[table->num ++] = dup;
    return 1;
}

static int compare_strings (const void * s1, const void * s2)
{
    return strcasecmp (*(char **)s1, *(char **)s2);
}

int indent = 0;
static void showline (char * fmt, LONG args[])
{
    int t;

    for (t=0; t<indent; t++)
	VPrintf ("    ", NULL);

    VPrintf (fmt, args);
}

struct
{
    char * dir;
    char * opt;
    ULONG all;
} args = {
    NULL,
    NULL,
    0
};

static LONG do_dir (char *path)
{
    BPTR dir;
    LONG loop;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    static UBYTE buffer[4096];
    LONG error=RETURN_OK;
    struct table dirs, files;

    dirs.entries = files.entries = NULL;
    dirs.max = files.max = 0;
    dirs.num = files.num = 0;

    dir=Lock(path,SHARED_LOCK);
    if(dir)
    {
	eac=AllocDosObject(DOS_EXALLCONTROL,NULL);
	if(eac!=NULL)
	{
	    int t;
	    IPTR argv[3];

	    eac->eac_LastKey=0;
	    do
	    {
		loop=ExAll(dir,(struct ExAllData *)buffer,4096,ED_COMMENT,eac);
		if(!loop&&IoErr()!=ERROR_NO_MORE_ENTRIES)
		{
		    error=RETURN_ERROR;
		    break;
		}
		if(eac->eac_Entries)
		{
		    ead=(struct ExAllData *)buffer;
		    do
		    {
			if (!AddEntry (ead->ed_Type > 0 ? &dirs : &files, ead->ed_Name))
			{
			    loop = 0;
			    error=RETURN_FAIL;
                            SetIoErr(ERROR_NO_FREE_STORE);
			    break;
			}

			ead=ead->ed_Next;
		    }while(ead!=NULL);
		}
	    }while((loop) && (error==RETURN_OK));
	    FreeDosObject(DOS_EXALLCONTROL,eac);

	    if (!error)
	    {
		if (dirs.num)
		{
		    indent ++;

		    qsort (dirs.entries, dirs.num, sizeof (char *),
			compare_strings);

		    for (t=0; t<dirs.num; t++)
		    {
			argv[0] = (IPTR) dirs.entries[t];

			if (args.all)
			{
                            char * newpath;
                            int len, pathlen = strlen(path);
                            len = pathlen + strlen(dirs.entries[t]) + 2;

                            newpath = AllocVec(len, MEMF_ANY);
                            if (newpath)
                            {
                                CopyMem(path, newpath, pathlen + 1);
                                if (AddPart(newpath, dirs.entries[t], len))
                                {
                                    showline ("%-25.s <DIR>\n", argv);
                                    error = do_dir (newpath);
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
                            if (error)
                                break;
			}
			else
			    showline ("    %-25.s <DIR>\n", argv);
		    }

		    indent --;
		}

		if (files.num)
		{
		    qsort (files.entries, files.num, sizeof (char *),
			compare_strings);

		    for (t=0; t<files.num; t+=2)
		    {
			argv[0] = (IPTR) (files.entries[t]);
			argv[1] = (IPTR) (t+1 < files.num ? files.entries[t+1] : "");
			if (args.all)
			    showline ("    %-25.s %-25.s\n", argv);
			else
			    showline ("%-25.s %-25.s\n", argv);
		    }
		}
	    }

	    if (dirs.num)
	    {
		for (t=0; t<dirs.num; t++)
		{
		    free (dirs.entries[t]);
		}

		if (dirs.entries)
		    FreeVec (dirs.entries);
	    }

	    if (files.num)
	    {
		for (t=0; t<files.num; t++)
		{
		    free (files.entries[t]);
		}

		if (files.entries)
		    FreeVec (files.entries);
	    }
	}else
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    error=RETURN_FAIL;
	}
	UnLock(dir);
    } else
        error = RETURN_FAIL;

    return error;
}

int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    LONG error=0;

    UtilityBase=(struct UtilityBase *)OpenLibrary(UTILITYNAME,39);

    if (!UtilityBase)
	return RETURN_ERROR;

    rda=ReadArgs("DIR,OPT/K,ALL/S",(IPTR *)&args,NULL);
    if(rda!=NULL)
    {
	error = do_dir (args.dir!=NULL?args.dir:"");

	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(), NULL);

    CloseLibrary((struct Library *)UtilityBase);

    return error;
}

