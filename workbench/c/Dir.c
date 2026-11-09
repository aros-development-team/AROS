/*
    Copyright (C) 1995-2012, The AROS Development Team. All rights reserved.
 
    Desc: Dir CLI command
*/

#define MAX_PATH_LEN                    512

#define USE_SOFTLINKCHECK               0

#include <exec/devices.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <aros/asmcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#define SH_GLOBAL_SYSBASE       1
#define SH_GLOBAL_DOSBASE       1

#include <aros/shcommands.h>

#include <string.h>
#include <stdlib.h>

/******************************************************************************
 
 
    NAME
 
    Dir [(dir | pattern)] [OPT A | I | D | F] [ALL] [DIRS] [FILES] [INTER]
 
 
    SYNOPSIS
 
    DIR,OPT/K,ALL/S,DIRS/S,FILES/S,INTER/S
 
    LOCATION
 
    C:
 
    FUNCTION
 
    DIR displays the file or directory contained in the current or
    specified directory. Directories get listed first, then in alphabetical
    order, the files are listed in two columns. Pressing CTRL-C aborts the
    directory listing.
 
 
    INPUTS
 
    ALL    --  Display all subdirectories and their files recursively
               (a link to a directory on the path walked to it, or
               above where the walk started, is listed but not
               entered).
    DIRS   --  Display only directories.
    FILES  --  Display only files.
    INTER  --  Enter interactive mode.
 
               Interactive listing mode stops after each name to display
               a question mark at which you can enter commands. These
               commands are:
 
               Return      --  Goto the next file or directory.
               E/ENTER     --  Enters a directory.
               B/BACK      --  Go back one directory level.
               DEL/DELETE  --  Delete a file or an empty directory.
               T/TYPE      --  Display content of a file.
               C/COM       --  Let the file or directory be the input of
                               a DOS command (which specified after the C or
                               COM or specified separately later).
               Q/QUIT      --  Quit interactive mode.
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    Interactive mode isn't fully working. It only walks stepwise
    through the directory.

    SEE ALSO
 
    INTERNALS

******************************************************************************/

struct table
{
    char **entries;
    LONG  *types;       /* typed tables only: ALL reads a directory's type */
    BOOL   typed;
    int    num;
    int    max;
};

/* The directories ALL has entered, innermost first */
struct DirLock
{
    BPTR            lock;
    struct DirLock *parent;
};

/* Global variables */
int g_indent;
int g_skipped;      /* directories ALL could not list */
struct UtilityBase *UtilityBase;

/*  Prototypes  */

static
LONG doPatternDir(CONST_STRPTR dirPat, BOOL all, BOOL doDirs, BOOL doFiles,
                  BOOL inter);
static
LONG doDir(CONST_STRPTR dir, struct DirLock *path, BOOL all, BOOL dirs,
           BOOL files, BOOL inter);

static
void showline(char *fmt, RAWARG args);
static
void maybeShowline(char *format, RAWARG args, BOOL doIt, BOOL inter);
static
void maybeShowlineCR(char *format, RAWARG args, BOOL doIt, BOOL inter);

static
int CheckDir(BPTR lock, struct ExAllData *ead, ULONG eadSize,
             struct ExAllControl *eac, struct table *dirs,
             struct table *files);

static
void freeTable(struct table *table);

static
BOOL isAbove(BPTR lock, BPTR dir);
static
BOOL canEnter(LONG type, BPTR dirlock, CONST_STRPTR name,
              struct DirLock *path);

static
LONG skipFailed(LONG error, CONST_STRPTR dir);

static
TEXT* getLine(CONST_STRPTR prompt, TEXT *buffer, LONG buflen);

#define  INTERARG_TEMPLATE  "E=ENTER/S,B=BACK/S,T=TYPE/S,DEL=DELETE/S,Q=QUIT/S,C=COM/S,COMMAND"

enum
{
    INTERARG_ENTER = 0,
    INTERARG_BACK,
    INTERARG_DELETE,
    INTERARG_TYPE,
    INTERARG_QUIT,
    INTERARG_COM,
    INTERARG_COMMAND,
    NOOFINTERARGS
};

AROS_SH6(Dir, 50.10,
        AROS_SHA(CONST_STRPTR, ,DIR  ,    , NULL),
        AROS_SHA(CONST_STRPTR, ,OPT  , /K , NULL),
        AROS_SHA(BOOL,         ,ALL  , /S , FALSE),
        AROS_SHA(BOOL,         ,DIRS , /S , FALSE),
        AROS_SHA(BOOL,         ,FILES, /S , FALSE),
        AROS_SHA(BOOL,         ,INTER, /S , FALSE)
)
{
    AROS_SHCOMMAND_INIT

    LONG         error = RETURN_FAIL;
    CONST_STRPTR dir = SHArg(DIR);
    CONST_STRPTR opt = SHArg(OPT);
    BOOL         all = SHArg(ALL);
    BOOL         dirs = SHArg(DIRS);
    BOOL         files = SHArg(FILES);
    BOOL         inter = SHArg(INTER);

    LONG iswild;

    g_indent = 0;

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37);
    if (!UtilityBase)
        return error;

    /* Convert the OPT arguments (if any) into the regular switches */
    if (opt != NULL)
    {
        while (*opt != 0)
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
        error = doPatternDir(dir, all, dirs, files, inter);
    }
    else
    {
        error = doDir(dir, NULL, all, dirs, files, inter);
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
    else if (g_skipped != 0)
    {
        error = RETURN_WARN;
    }

    CloseLibrary((struct Library *)UtilityBase);
    return error;

    AROS_SHCOMMAND_EXIT
}


static
int AddEntry(struct table *table, char *entry, LONG type)
{
    char *dup;
    int len;

    if (table->num == table->max)
    {
        int    new_max = table->max + 128;
        char **new_entries;
        LONG  *new_types;

        new_entries = AllocVec(sizeof(char *) * new_max, MEMF_ANY);
        new_types = NULL;
        if (table->typed)
            new_types = AllocVec(sizeof(LONG) * new_max, MEMF_ANY);

        if (new_entries == NULL || (table->typed && new_types == NULL))
        {
            FreeVec(new_entries);
            FreeVec(new_types);
            return 0;
        }

        if (table->num)
        {
            CopyMemQuick(table->entries, new_entries,
                         sizeof(char *) * table->num);
            if (table->typed)
                CopyMemQuick(table->types, new_types,
                             sizeof(LONG) * table->num);
            FreeVec(table->entries);
            FreeVec(table->types);
        }

        table->entries = new_entries;
        table->types = new_types;
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
    if (table->typed)
        table->types[table->num] = type;
    table->entries[table->num++] = dup;

    return 1;
}


static
void freeTable(struct table *table)
{
    int t;

    for (t = 0; t < table->num; t++)
    {
        FreeVec(table->entries[t]);
    }
    FreeVec(table->entries);
    FreeVec(table->types);
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
void maybeShowlineCR(char *format, RAWARG args, BOOL doIt, BOOL inter)
{
    maybeShowline(format, args, doIt, inter);

    if (!inter)
    {
        PutStr("\n");
    }
}


static
void maybeShowline(char *format, RAWARG args, BOOL doIt, BOOL inter)
{
    if (doIt)
    {
        showline(format, args);

        if (inter)
        {
            struct RDArgs *inter_rdargs = AllocDosObject(DOS_RDARGS, NULL);
            if (inter_rdargs)
            {
                IPTR  interArgs[NOOFINTERARGS] = { (IPTR)FALSE,
                                                   (IPTR)FALSE,
                                                   (IPTR)FALSE,
                                                   (IPTR)FALSE,
                                                   (IPTR)FALSE,
                                                   (IPTR)FALSE,
                                                   (IPTR)NULL };
                TEXT buffer[80];
                memset(inter_rdargs, 0, sizeof *inter_rdargs);
                
                if (getLine(" ? ", buffer, sizeof buffer))
                {
                    inter_rdargs->RDA_Source.CS_Buffer = buffer;
                    inter_rdargs->RDA_Source.CS_Length = sizeof buffer;

                    if (ReadArgs(INTERARG_TEMPLATE, interArgs, inter_rdargs))
                    {
#if 0
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
#endif
                        FreeArgs(inter_rdargs);
                    }
                }
                FreeDosObject(DOS_RDARGS, inter_rdargs);
            }
        }
    }
}


static
void showline(char *fmt, RAWARG args)
{
    int t;

    for (t = 0; t < g_indent; t++)
        PutStr("     ");

    VPrintf(fmt, args);
}


// Returns TRUE if all lines shown, FALSE if broken by SIGBREAKF_CTRL_C
static
BOOL showfiles(struct table *files, BOOL inter)
{
    CONST_STRPTR argv[2];
    ULONG t;

    qsort(files->entries, files->num, sizeof(char *),compare_strings);

    if (inter)
    {
        for (t = 0; t < files->num; t++)
        {
            argv[0] = files->entries[t];

            if (SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            {
                SetIoErr(ERROR_BREAK);
                return FALSE;
            }

            maybeShowlineCR("  %s", (RAWARG)argv, TRUE, inter);
        }
    }
    else
    {
        for (t = 0; t < files->num; t += 2)
        {
            argv[0] = files->entries[t];
            argv[1] = t + 1 < files->num ? files->entries[t+1] : "";

            if (SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            {
                SetIoErr(ERROR_BREAK);
                return FALSE;
            }

            maybeShowlineCR("  %-32.s %s", (RAWARG)argv, TRUE, inter);
        }
    }
    return TRUE;
}


static
BOOL showdir(char *dirName, BOOL inter)
{
    maybeShowlineCR("%s (dir)", (RAWARG)&dirName, TRUE, inter);
    return TRUE;
}


static
LONG doPatternDir(CONST_STRPTR dirPat, BOOL all, BOOL doDirs, BOOL doFiles, BOOL inter)
{
    struct table files;
    struct AnchorPath *ap;      /* Matching structure */

    LONG  match;                /* Loop variable */
    LONG  error = RETURN_FAIL;

    LONG  ioerr = 0;

    files.entries = NULL;
    files.types = NULL;
    files.typed = FALSE;
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
                        if (l != BNULL)
                        {
                            UBYTE name[512];
                            name[0] = 0;
                            NameFromLock(l, name, 512);
                            UnLock(l);
                            showdir(name, inter);
                        }
                    }
                    if (all)
                    {
                        struct DirLock up;

                        up.lock = ap->ap_Current->an_Lock;
                        up.parent = NULL;

                        if (canEnter(ap->ap_Info.fib_DirEntryType, up.lock,
                                     ap->ap_Info.fib_FileName, &up))
                        {
                            error = doDir(ap->ap_Buf, &up, all, doDirs,
                                          doFiles, inter);
                            error = skipFailed(error, ap->ap_Buf);
                        }
                    }
                }
                else if (doFiles)
                {
                    if (!AddEntry(&files, ap->ap_Info.fib_FileName,
                                  ap->ap_Info.fib_DirEntryType))
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
                if (!showfiles(&files, inter))
                {
                    error = RETURN_FAIL;
                }

            }
        }
        else
        {
            ioerr = match;
            error = RETURN_FAIL;
        }
        MatchEnd(ap);
        FreeVec(ap);
    }

    freeTable(&files);

    SetIoErr(ioerr);

    return error;
}


static
LONG doDir(CONST_STRPTR dir, struct DirLock *path, BOOL all, BOOL doDirs,
           BOOL doFiles, BOOL inter)
{
    BPTR  lock;
    struct DirLock here;
    static UBYTE buffer[4096];
    struct ExAllControl  *eac;
    LONG error = RETURN_OK;

    struct table  dirs;
    struct table  files;

    dirs.entries = files.entries = NULL;
    dirs.types = files.types = NULL;
    dirs.typed = TRUE;
    files.typed = FALSE;
    dirs.max = files.max = 0;
    dirs.num = files.num = 0;

    lock = Lock(dir, SHARED_LOCK);

    if (lock != BNULL)
    {
        here.lock = lock;
        here.parent = path;

        eac = AllocDosObject(DOS_EXALLCONTROL, NULL);

        if (eac != NULL)
        {
            int t;

            eac->eac_LastKey = 0;

            error = CheckDir(lock, (struct ExAllData *)buffer, sizeof(buffer), eac, &dirs, &files);
            FreeDosObject(DOS_EXALLCONTROL, eac);

            if (SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            {
                SetIoErr(ERROR_BREAK);
                error = RETURN_FAIL;
            }

            /* ALL walks the subdirectories even when only files are
               listed; their names are shown only when directories are */
            if (error == 0 && (doDirs || all))
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
                        if (all && canEnter(dirs.types[t], lock, dirname, &here))
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
                                        showdir(dirname, inter);
                                    error = doDir(newpath, &here, all, doDirs, doFiles, inter);
                                }
                                else
                                {
                                    SetIoErr(ERROR_LINE_TOO_LONG);
                                    error = RETURN_ERROR;
                                }
                                error = skipFailed(error, newpath);
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
                            showdir(dirname, inter);
                        }
                    }
                    g_indent--;
                }
            }

            // Output the files
            if (error == RETURN_OK && (files.num != 0 && doFiles))
            {
                if (!showfiles(&files, inter))
                {
                    error = RETURN_FAIL;
                }

            }

            freeTable(&dirs);
            freeTable(&files);

        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            error = RETURN_FAIL;
        }

        /* UnLock() resets IoErr(), which tells the caller why we failed */
        {
            LONG ioerr = IoErr();

            UnLock(lock);
            SetIoErr(ioerr);
        }
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

                if (!AddEntry(ead->ed_Type > 0 ? dirs : files, ead->ed_Name,
                              ead->ed_Type))
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


/* Is lock a directory above dir? */
static
BOOL isAbove(BPTR lock, BPTR dir)
{
    BPTR parent;
    BOOL same = FALSE;

    dir = ParentDir(dir);

    while (dir && !same)
    {
        same = (SameLock(lock, dir) == LOCK_SAME);
        parent = same ? BNULL : ParentDir(dir);
        UnLock(dir);
        dir = parent;
    }

    return same;
}


/* May ALL enter the directory name of this type in dirlock? A soft link
   or hard-linked directory is not entered if it leads to a file, or to a
   directory on the path walked to it or above where the walk started
   (the path's last entry), which would loop. One that cannot be locked
   is entered, so that listing it reports the error. */
static
BOOL canEnter(LONG type, BPTR dirlock, CONST_STRPTR name,
              struct DirLock *path)
{
    struct FileInfoBlock *fib;
    BPTR  lock, olddir;
    BOOL  enter = TRUE;

    if (type != ST_SOFTLINK && type != ST_LINKDIR)
        return TRUE;

    olddir = CurrentDir(dirlock);
    lock = Lock(name, SHARED_LOCK);
    CurrentDir(olddir);

    if (lock == BNULL)
        return TRUE;

    if ((fib = AllocDosObject(DOS_FIB, NULL)) != NULL)
    {
        if (Examine(lock, fib) && fib->fib_DirEntryType < 0)
            enter = FALSE;
        FreeDosObject(DOS_FIB, fib);
    }

    for (; enter && path != NULL; path = path->parent)
    {
        if (SameLock(lock, path->lock) == LOCK_SAME)
            enter = FALSE;
        else if (path->parent == NULL && isAbove(lock, path->lock))
            enter = FALSE;
    }

    UnLock(lock);

    return enter;
}


/* A directory found by ALL that cannot be listed doesn't end the walk:
   report it and go on with the next one. Only a break or running out of
   memory stops it. */
static
LONG skipFailed(LONG error, CONST_STRPTR dir)
{
    LONG ioerr = IoErr();

    if (error == RETURN_OK || ioerr == ERROR_BREAK ||
        ioerr == ERROR_NO_FREE_STORE)
        return error;

    PrintFault(ioerr, dir);
    g_skipped++;

    return RETURN_OK;
}


static
TEXT* getLine(CONST_STRPTR prompt, TEXT *buffer, LONG buflen)
{
    if (prompt)
    {
        PutStr(prompt);
    }
    Flush(Output());
    return FGets(Input(), buffer, buflen);
}
