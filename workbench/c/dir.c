/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1996/09/21 15:49:26  digulla
    Use AROS instead of C.lib functions

    Revision 1.7  1996/09/17 16:42:59  digulla
    Use general startup code

    Revision 1.6  1996/09/13 17:52:10  digulla
    Use IPTR

    Revision 1.5  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <clib/aros_protos.h>

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

    if (!(dup = StrDup (entry)) )
	return 0;

    table->entries[table->num ++] = dup;
    return 1;
}

static int compare_strings (const void * s1, const void * s2)
{
    return Stricmp (*(char **)s1, *(char **)s2);
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
char path[1024];

static LONG do_dir (void)
{
    BPTR dir;
    LONG loop;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    static UBYTE buffer[4096];
    LONG error=0;
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
			    error=RETURN_ERROR;
			    VPrintf ("out of memory\n", NULL);
			    break;
			}

			ead=ead->ed_Next;
		    }while(ead!=NULL);
		}
	    }while(loop);
	    FreeDosObject(DOS_EXALLCONTROL,eac);

	    if (!error)
	    {
		if (dirs.num)
		{
		    char * ptr;
		    int added_slash = 0;

		    indent ++;

		    QSort (dirs.entries, dirs.num, sizeof (char *),
			compare_strings);

		    ptr = path + StrLen (path);

		    if (*path && ptr[-1] != ':' && ptr[-1] != '/')
		    {
			*ptr ++ = '/';
			*ptr = 0;

			added_slash = 1;
		    }

		    for (t=0; t<dirs.num; t++)
		    {
			argv[0] = (IPTR) dirs.entries[t];

			if (args.all)
			{
			    StrCpy (ptr, dirs.entries[t]);

			    showline ("%-25.s <DIR>\n", argv);
			    do_dir ();
			}
			else
			    showline ("    %-25.s <DIR>\n", argv);
		    }

		    if (added_slash)
			ptr[-1] = 0;

		    indent --;
		}

		if (files.num)
		{
		    QSort (files.entries, files.num, sizeof (char *),
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
		    FreeVec (dirs.entries[t]);
		}

		if (dirs.entries)
		    FreeVec (dirs.entries);
	    }

	    if (files.num)
	    {
		for (t=0; t<files.num; t++)
		{
		    FreeVec (files.entries[t]);
		}

		if (files.entries)
		    FreeVec (files.entries);
	    }
	}else
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    error=RETURN_ERROR;
	}
	UnLock(dir);
    }

    return error;
}

int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    LONG error=0;

    UtilityBase=(struct UtilityBase *)OpenLibrary(UTILITYNAME,39);

    if (!UtilityBase)
	return RETURN_ERROR;

    rda=ReadArgs("Dir,OPT/K,ALL/S",(IPTR *)&args,NULL);
    if(rda!=NULL)
    {
	StrCpy (path, args.dir!=NULL?args.dir:"");

	error = do_dir ();

	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"List");

    CloseLibrary((struct Library *)UtilityBase);

    return error;
}

