/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$
 
    Desc: Dir CLI command
    Lang: English
*/

#define  DEBUG  1

#define MAX_PATH_LEN			512

#ifdef __AROS__
#define USE_SOFTLINKCHECK		0
#else
#define USE_SOFTLINKCHECK		1
#endif

#include <exec/devices.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>
#include <stdlib.h>
#include <memory.h>

//#include <stdio.h>

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
 
    HISTORY
 
    XY.11.2000  SDuvan  added pattern matching support and support for
                        FILES/S, DIRS/S and OPT/K and some support for
			INTER/S. Complete interactive support is still missing.
 
******************************************************************************/

#define	VERSTAG	"\0$VER: Dir 50.8 (27.11.2004) © AROS"

static const char version[] = VERSTAG;

struct table
{
    char **entries;
    int    num;
    int    max;
};


struct data
{
#ifndef __AROS__
    struct ExecBase	*SysBase;
    struct DosLibrary	*DOSBase;
#endif
    struct Library	*UtilityBase;
    int                 g_indent;
};

#ifndef __AROS__
#define SysBase		data->SysBase
#define DOSBase		data->DOSBase
#endif
#define UtilityBase	data->UtilityBase
#define g_indent	data->g_indent


/*  Prototypes  */

static
LONG doPatternDir(STRPTR dirPat, BOOL all, BOOL doDirs, BOOL doFiles,
                  BOOL inter, struct data *data);
static
LONG doDir(STRPTR dir, BOOL all, BOOL dirs, BOOL files, BOOL inter, struct data *data);

static
void showline(char *fmt, LONG *args, struct data *data);
static
void maybeShowline(char *format, LONG *args, BOOL doIt, BOOL inter, struct data *data);
static
void maybeShowlineCR(char *format, LONG *args, BOOL doIt, BOOL inter, struct data *data);

static
int CheckDir(BPTR lock, struct ExAllData *ead, ULONG eadSize,
             struct ExAllControl *eac, struct table *dirs,
             struct table *files, struct data *data);


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

int main(void)
{
    struct data _data, *data = &_data;
    struct RDArgs *rda;
    IPTR           args[] = { NULL, NULL, (IPTR)FALSE, (IPTR)FALSE, (IPTR)FALSE, (IPTR)FALSE };

    LONG error = RETURN_FAIL;

#ifndef __AROS__
    SysBase=*((struct ExecBase**) 4);
    if ((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",37)))
    {
#endif
        if ((UtilityBase=OpenLibrary("utility.library",37)))
        {
            error = RETURN_ERROR;

            rda = ReadArgs(ARG_TEMPLATE, args, NULL);

            if (rda != NULL)
            {
                STRPTR dir = (STRPTR)args[ARG_DIR];
                STRPTR opt = (STRPTR)args[ARG_OPT];
                BOOL   all = (BOOL)args[ARG_ALL];
                BOOL   dirs = (BOOL)args[ARG_DIRS];
                BOOL   files = (BOOL)args[ARG_FILES];
                BOOL   inter = (BOOL)args[ARG_INTER];

                LONG iswild;

                g_indent = 0;

                /* Convert the OPT arguments (if any) into the regular switches */
                if (opt != NULL)
                {
                    while (*opt != NULL)
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
                            Printf("%lc option ignored\n", *opt);
                            break;
                        }
                        opt++;
                    }
                }

                if(dir == NULL)
                {
                    dir = "";
                    iswild = 0;
                }
                else
                {
                    ULONG toklen = strlen(dir) * 2;
                    STRPTR pattok = AllocMem(toklen, MEMF_PUBLIC | MEMF_ANY);
                    iswild = ParsePattern(dir, pattok, toklen);
                    FreeMem(pattok, toklen);
                }

                if(!files && !dirs)
                {
                    files = TRUE;
                    dirs  = TRUE;
                }


                if (iswild == 1)
                {
                    error = doPatternDir(dir, all, dirs, files, inter, data);
                }
                else
                {
                    error = doDir(dir, all, dirs, files, inter, data);
                }

                if (error != RETURN_OK)
                {
                    LONG ioerr = IoErr();
                    switch (ioerr)
                    {
                    case ERROR_NO_MORE_ENTRIES:
                        ioerr = 0;
                        break;
                    case ERROR_OBJECT_WRONG_TYPE:
                        Printf("%s is not a directory\n", (IPTR)dir);
                        ioerr = ERROR_DIR_NOT_FOUND;
                        break;
                    default:
                        Printf("Could not get information for %s\n", (IPTR)dir);
                    }
                    PrintFault(ioerr, NULL);
                }

                FreeArgs(rda);
            }
            else
            {
		PrintFault(IoErr(), NULL);
            }

            CloseLibrary((struct Library *)UtilityBase);
        }
#ifndef __AROS__
        CloseLibrary((struct Library *)DOSBase);
    }
#endif

    return error;
}

static
int AddEntry(struct table *table, char *entry, struct data *data)
{
    char *dup;
    int len;

    if (table->num == table->max)
    {
        int    new_max = table->max + 128;
        char **new_entries;

        new_entries = AllocVec(sizeof(char *) * new_max, MEMF_ANY);

        if (new_entries == NULL)
            return 0;

        if (table->num)
        {
            CopyMemQuick(table->entries, new_entries,
                         sizeof(char *) * table->num);
            FreeVec(table->entries);
        }

        table->entries = new_entries;
        table->max = new_max;
    }

    len = strlen(entry) + 1;
    if ((dup = AllocVec(len,MEMF_ANY)))
    {
        strcpy(dup,entry);
    }
    else
    {
        return 0;
    }
    table->entries[table->num++] = dup;

    return 1;
}


static __inline
int mystrcasecmp(const char *s1, const char *s2)
{
    int a, b;

    do
    {
        a = *s1++;
        b = *s2++;

        if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
        if (b >= 'a' && b <= 'z') b -= 'a' - 'A';

        a -= b;
        if (a) return (int) a;

    } while (b);

    return 0;
}

static
int compare_strings(const void * s1, const void * s2)
{
    return mystrcasecmp(*(char **)s1, *(char **)s2);
}


static
void maybeShowlineCR(char *format, LONG *args, BOOL doIt, BOOL inter, struct data *data)
{
    maybeShowline(format, args, doIt, inter, data);

    if (!inter)
    {
        PutStr("\n");
    }
}



static
void maybeShowline(char *format, LONG *args, BOOL doIt, BOOL inter, struct data *data)
{
    if(doIt)
    {
        showline(format, args, data);

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


static
void showline(char *fmt, LONG *args, struct data *data)
{
    int t;

    for (t = 0; t < g_indent; t++)
        PutStr("     ");

    VPrintf(fmt, args);
}

// Returns TRUE if all lines shown, FALSE if broken by SIGBREAKF_CTRL_C
static
BOOL showfiles(struct table *files, BOOL inter, struct data *data)
{
    IPTR argv[2];
    ULONG t;

    qsort(files->entries, files->num, sizeof(char *),compare_strings);

    for (t = 0; t < files->num; t += 2)
    {
        argv[0] = (IPTR)(files->entries[t]);
        argv[1] = (IPTR)(t + 1 < files->num ? files->entries[t+1] : "");

        if (SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
        {
            SetIoErr(ERROR_BREAK);
            return FALSE;
        }

        maybeShowlineCR("  %-32.s %s", argv, TRUE, inter, data);
    }
    return TRUE;
}

static
BOOL showdir(char *dirName, BOOL inter, struct data *data)
{
    IPTR argv[1] = {(IPTR)dirName};
    maybeShowlineCR("%s (dir)", argv, TRUE, inter, data);
    return TRUE;
}

static
LONG doPatternDir(STRPTR dirPat, BOOL all, BOOL doDirs, BOOL doFiles, BOOL inter, struct data *data)
{
    struct table files;
    struct AnchorPath *ap;	/* Matching structure */

    LONG  match;		/* Loop variable */
    LONG  error = RETURN_FAIL;

    LONG  ioerr = 0;

    files.entries = NULL;
    files.max = 0;
    files.num = 0;

    ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN, MEMF_CLEAR);

    if(ap != NULL)
    {
        ap->ap_Strlen = MAX_PATH_LEN;
        ap->ap_BreakBits = SIGBREAKF_CTRL_C;

        if ((match = MatchFirst(dirPat, ap)) == 0)
        {
            error = RETURN_OK;
            g_indent++;
            for (;;)
            {
#if USE_SOFTLINKCHECK
                if (SoftlinkDODIR(ap, TRUE, FALSE, DOSBase))
#else /* USE_SOFTLINKCHECK */
                if (ap->ap_Info.fib_DirEntryType > 0)
#endif /* USE_SOFTLINKCHECK */
                {
                    if (doDirs)
                    {
                        BPTR l = Lock(ap->ap_Buf, SHARED_LOCK);
                        if (l != NULL)
                        {
                            UBYTE name[512];
                            name[0] = 0;
                            NameFromLock(l, name, 512);
                            UnLock(l);
                            showdir(name, inter, data);
                        }
                    }
                    if (all)
                    {
                        error = doDir(ap->ap_Buf, all, doDirs, doFiles, inter, data);
                    }
                }
                else if (doFiles)
                {
                    if (!AddEntry(&files, ap->ap_Info.fib_FileName, data))
                    {
                        ioerr = ERROR_NO_FREE_STORE;
                        error = RETURN_FAIL;
                        break;
                    }
                }
                ioerr = IoErr();

                if (error != RETURN_OK)
                {
                    break;
                }
                ap->ap_Strlen = MAX_PATH_LEN;
                match = MatchNext(ap);
                if (match != 0)
                    break;
            }
            g_indent--;
            if (error == RETURN_OK && files.num != 0)
            {
                if (!showfiles(&files, inter, data))
                {
                    error = RETURN_FAIL;
                }

            }
            MatchEnd(ap);
        }
        else
        {
            ioerr = match;
            error = RETURN_FAIL;
        }
        FreeVec(ap);
    }

    SetIoErr(ioerr);

    return error;
}




static
LONG doDir(STRPTR dir, BOOL all, BOOL doDirs, BOOL doFiles, BOOL inter, struct data *data)
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

    if (lock != NULL)
    {
        eac = AllocDosObject(DOS_EXALLCONTROL, NULL);

        if (eac != NULL)
        {
            int t;

            eac->eac_LastKey = 0;

            error = CheckDir(lock, (struct ExAllData *)buffer, sizeof(buffer), eac, &dirs, &files, data);
            FreeDosObject(DOS_EXALLCONTROL, eac);

            if (SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            {
                SetIoErr(ERROR_BREAK);
                error = RETURN_FAIL;
            }

            if (error == 0 && doDirs)
            {
                if (dirs.num != 0)
                {
                    g_indent++;

                    //qsort(dirs.entries, dirs.num, sizeof(char *), compare_strings);

                    // Output the directories
                    for (t = 0; t < dirs.num; t++)
                    {
                        STRPTR dirname = dirs.entries[t];

                        // Recurse into subdirectories if "ALL" was specified by the user
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
                                    if (doDirs)
                                        showdir(dirname, inter, data);
                                    error = doDir(newpath, all, doDirs, doFiles, inter, data);
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
                        else if (doDirs)
                        {
                            showdir(dirname, inter, data);
                        }
                    }
                    g_indent--;
                }
            }

            // Output the files
            if (error == RETURN_OK && (files.num != 0 && doFiles))
            {
                if (!showfiles(&files, inter, data))
                {
                    error = RETURN_FAIL;
                }

            }

            if (dirs.num != 0)
            {
                for (t = 0; t < dirs.num; t++)
                {
                    FreeVec(dirs.entries[t]);
                }

                if (dirs.entries)
                {
                    FreeVec(dirs.entries);
                }
            }

            if (files.num != 0)
            {
                for (t = 0; t < files.num; t++)
                {
                    FreeVec(files.entries[t]);
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
#if USE_SOFTLINKCHECK

        struct DevProc *dvp;
        LONG ioerr;

        error = RETURN_FAIL;
        ioerr = IoErr();

        if (ioerr == ERROR_OBJECT_NOT_FOUND &&
            (dvp = GetDeviceProc(dir, NULL)))
        {
            if (ReadLink(dvp->dvp_Port, dvp->dvp_Lock, dir, buffer, sizeof(buffer) - 1) > 0)
            {
                buffer[sizeof(buffer) - 1] = '\0';

                Printf("Warning: Skipping dangling softlink %s -> %s\n",
                       (LONG) dir, (LONG) buffer);

                error = RETURN_OK;
            }

            FreeDeviceProc(dvp);
        }

        SetIoErr(ioerr);

#else /* USE_SOFTLINKCHECK */

        error = RETURN_FAIL;

#endif /* USE_SOFTLINKCHECK */
    }

    return error;
}


static
int CheckDir(BPTR lock, struct ExAllData *ead, ULONG eadSize,
             struct ExAllControl *eac, struct table *dirs,
             struct table *files, struct data *data)
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

        if(eac->eac_Entries != NULL)
        {
            do
            {

#if USE_SOFTLINKCHECK
                if (ead->ed_Type == ST_SOFTLINK)
                {
                    BPTR dirlock, l;

                    dirlock = CurrentDir(lock);
                    l = Lock(ead->ed_Name, ACCESS_READ);
                    CurrentDir(dirlock);

                    if (l)
                    {
                        UBYTE _fib[sizeof(struct FileInfoBlock) + 3];
                        struct FileInfoBlock *fib = (APTR) (((IPTR) _fib + 3) & ~3);

                        if (Examine(l, fib))
                        {
                            ead->ed_Type = fib->fib_DirEntryType;
                            //ead->ed_Size = fib->fib_Size;
                        }

                        UnLock(l);
                    }
                }
#endif /* USE_SOFTLINKCHECK */

                if (!AddEntry(ead->ed_Type > 0 ? dirs : files,ead->ed_Name, data))
                {
                    loop = 0;
                    error = RETURN_FAIL;
                    SetIoErr(ERROR_NO_FREE_STORE);
                    break;
                }

                ead = ead->ed_Next;
            }
            while(ead != NULL);
        }
    }
    while((loop) && (error == RETURN_OK));

    return error;
}
