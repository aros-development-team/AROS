 /* copy t:AmiTCP.#? big: buf=2000000 verbose
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Copy CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Copy

    SYNOPSIS

        FROM/M, TO, ALL/S, QUIET/S, BUF=BUFFER/K/N, CLONE/S, DATES/S, NOPRO/S,
        COM=COMMENT/S, NOREQ/S,

        PAT=PATTERN/K, DIRECT/S,SILENT/S, ERRWARN/S, MAKEDIR/S, MOVE/S,
        DELETE/S, HARD=HARDLINK/S, SOFT=SOFTLINK/S, FOLNK=FORCELINK/S,
        FODEL=FORCEDELETE/S, FOOVR=FORCEOVERWRITE/S, DONTOVR=DONTOVERWRITE/S,
        FORCE/S

    LOCATION

        Workbench:C

    FUNCTION

        Creates identical copies of one or more files.

    INPUTS

        FROM      --  multiple input files
        TO        --  destination file or directory
        ALL       --  deep scan into sub directories
        QUIET     --  suppress all output and requesters
        BUFFER    --  buffer size for copy buffer in 512 byte blocks
                      (default 1024 (= 512K))
        CLONE     --  copy comment, protection bits and date as well
        DATES     --  copy dates
        NOPRO     --  do not copy protection bits
        COMMENT   --  copy filecomment
        NOREQ     --  suppress requesters

        PATTERN   --  a pattern the filenames must match
        DIRECT    --  copy mode only: copy file without any tests or options
        VERBOSE   --  gives more output
        ERRWARN   --  do not proceed, when one file failed
        MAKEDIR   --  produce directories
        MOVE      --  delete source files after copying successful
        DELETE    --  do not copy, but delete the source files
        HARDLINK  --  make a hardlink to source instead of copying
        SOFTLINK  --  make a softlink to source instead of copying
        FOLNK     --  also makes links to directories
        FODEL     --  delete protected files also
        FOOVR     --  also overwrite protected files
        DONTOVR   --  do never overwrite destination
        FORCE     --  DO NOT USE. Call compatibility only.


    More detailed descriptions:

    FROM:
    Source file(s). For directories, all contained files are source files. May
    have standard patterns.

    TO:
    Destination file or for multiple sources destination directory. Destination
    directories are created (including all needed parent directories).

    ALL:
    Scan directories recursively

    QUIET:
    Copy is completely silent here. Really no output is given, also no requests
    for missing disks or other problems!

    BUF=BUFFER:
    Specify the number of 512 byte buffers for copying. Default are 200 buffers
    [100KB memory]. One buffer is minimum size, but should never be used.

    PAT=PATTERN:
    PATTERN allows to specify a standard dos pattern, all file have to match.
    This is useful with ALL option.

    Example:
    When you want to delete all .info files in a directory tree, you need
    this option: Copy DELETE #? ALL PAT #?.info

    CLONE:
    The filecomment, date and protection bits of the source files are copied to
    destination file or directory.

    DATES:
    The date information of source is copied to destination.

    NOPRO:
    The protection bits of sources are NOT copied. So the destination gets
    default bits [rwed].

    COM=COMMENT:
    The filecomment is copied to destination.

    NOREQ:
    No standard DOS requests are displayed, when an error occurs.


    DIRECT:
    Certain devices do not allow some of the used DOS packet request types.
    This option is a really easy copy command, which only opens source and
    destination directly without any tests and checks.
    Options ALL, PAT, CLONE, DATES, NOPRO, COM, MAKEDIR, MOVE, DELETE, HARD,
    SOFT, FOLNK, FODEL, FOOVR, DONTOVR and multiple input files cannot be
    specified together with DIRECT. This options needs one input and one output
    file.
    When you want to delete a softlink, which does no longer point to a valid
    file, you need this option as well.
    Example use: 'Copy DIRECT text PRT:' to print a file called text.
    - Copy manages a lot of such cases automatically, but maybe this option is
    needed sometimes.

    VERBOSE:
    Copy gives additional output.

    ERRWARN:
    Copy knows and returns the 3 types of dos.library errors:
    5   WARN    The processing of one file failed, Copy skips this file
                and proceeds the next.
    10  ERROR   The creation of a directory or any other bad error happend.
                Copy quits after that.
    20  FAIL    A really hard error happend (No memory, Examine failed, ...)
                Copy quits after that.
    When option ERRWARN is used, the result 5 (WARN) gets result 10 (ERROR). So
    Copy aborts everytime an error occured.

    MAKEDIR:
    All names specified in FROM field are taken as directories, which must be
    created.

    MOVE:
    The files are not copied, but moved (or renamed). This means that after
    move operation the source does no longer exist.

    DELETE:
    This does not copy anything, but delete the source files!

    HARD=HARDLINK:
    Instead of copying the files, a hard link is created. This only works,
    when destination is on same device as source.
    When ALL option is specified, the directories are scanned recursively, else
    Copy produces links to the directories.

    SOFT=SOFTLINK:
    Instead of copying directories, a soft link is created. These links are
    useable between different devices also. Soft links are only created for
    directories. Files are skipped here. Option FORCELINK is therefor always
    set to true.
    NOTE: Softlinks are not official supported by OS and may be dangerous.
    I suggest not to use this option! See description below.

    FOLNK=FORCELINK:
    When linking of directories should be possible, this option is needed. See
    section "About links" for possible problems.

    FODEL=FORCEDELETE:
    When this option is enabled, files are deleted also, when they are delete
    protected.

    FOOVR=FORCEOVERWRITE:
    When this option is enabled, files are overwritten also, when they are
    protected.

    DONTOVR=DONTOVERWRITE:
    This option prevents overwriting of destination files.


    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        Delete, Rename, MakeDir, MakeLink

    INTERNALS

        The separation of the different switches above is according to
        what the AmigaDOS Copy command had, and the extensions respectively.

        Some comments on how the program does it's job:

        The program has 6 working modes: COPY, MOVE, SOFTLINK, HARDLINK,
        DELETE and MAKEDIR. Only one of these can be used at same time!

        Move option renames the files, when on same device, else the file
        is copied and the source deleted after that.
            When a directory is processed and on the same device it is
        renamed! This means MOVE option copies complete directories also
        without ALL option (when on same device).

        In Copy mode you may use f.e. "Copy C: RAM:K" instead of
        "Copy C:#? RAM:K". For the other modes this does not work!

        Destination files are always overwritten, except DONTOVERWRITE is
        turned on, or they are protected.

        When the destination directory does not exists, it is created.
        Existing files with same name are overwritten. When some parent
        directories do not exist, they are also created. This is also done,
        when only one file is copied.

        The program does a loop detection, so that copying/moving/linking
        with a sub directory of source as destination and ALL option is not
        possible.

        Example: Copy RAM:S RAM:S/C ALL


        Useful aliases you may add to S:User-StartUp:

        Alias Delete   Copy [] DELETE VERBOSE
        Alias MakeDir  Copy [] MAKEDIR
        Alias MakeLink Copy TO [] HARDLINK
        Alias Move     Copy [] MOVE CLONE VERBOSE
        Alias Rename   Copy [] MOVE CLONE

        Some programs do want the files to be in C: directory. For these
        you may additionally use following lines:

        Alias C:Delete   Copy [] DELETE VERBOSE
        Alias C:MakeDir  Copy [] MAKEDIR
        Alias C:MakeLink Copy TO [] HARDLINK
        Alias C:Rename   Copy [] MOVE CLONE


        About links:

        HARDLINKS:
        When copying one file to annother place on same disk, the file
        afterwards uses double space. Links are a method to resolve that
        problem. When using a link, the file is not copied, but only a new
        entry to the same data as created. This saves space and allows to
        have copies of files always up-to-date (as when on link is updated,
        all the others are new as well).

        SOFTLINKS:
        This is a link method, which is NOT official supported by the OS.
        Soft links do not need to be on the same partition. The may be used
        for references between different partitions. NOTE: Using this links
        may cause lots of problems. You may test for yourself if it works for
        you!

        PROBLEMS:
        Links to directories may cause infinite directory loops!

        Example: Having following directory tree:

                                 DEV:
                                /    \
                               A      B

        Some loops are detected, for example when trying to do:
        MakeLink DEV:A/C DEV:A FORCE
        Here you get an error message, that loops are not allowed.

        Some more complicated links cannot be detected:
                MakeLink DEV:A/C DEV:B FORCE
        and
                MakeLink DEV:B/C DEV:A FORCE
        Till now no error message is possible, so the result is an infinite
        loop.


    HISTORY

        Copy was done by Dirk Stoecker (stoecker@amigaworld.com), donated
        to AROS in March 2001

        3.3.2001  --  AROSified by Johan 'S.Duvan' Alfredsson
        29.7.2002 --  Fixed silly IoErr trashing bug, bumped to 50.1 - Piru
        7.11.2002 --  Fixed even silier bug where it would put out a bogus
                      errormessage when the copy destination was a volume
                      root. bumped to 50.2 - Piru

******************************************************************************/

static const char version[] = "\0$VER: Copy 50.14 (27.11.2004) © AROS";

#define CTRL_C          (SetSignal(0L,0L) & SIGBREAKF_CTRL_C)

#define  DEBUG  0

#ifdef __AROS__
#warning "FIXME: Not trusting softlinks in AROS DOS!"
#define USE_SOFTLINKCHECK               0
#else
#define USE_SOFTLINKCHECK               1
#endif

#define USE_ALWAYSVERBOSE               1
#define USE_BOGUSEOFWORKAROUND          0

#include <exec/devices.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/types.h>
#include <dos/exall.h>

#ifdef __SASC
typedef ULONG IPTR;
#else /* __SASC */
#include <aros/debug.h>
#endif /* __SASC */

#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>
#include <stdio.h>


static const UBYTE *PARAM =
"FROM/M,TO,PAT=PATTERN/K,BUF=BUFFER/K/N,ALL/S,"
"DIRECT/S,CLONE/S,DATES/S,NOPRO/S,COM=COMMENT/S,"
"QUIET/S,"
#if !USE_ALWAYSVERBOSE
"VERBOSE/S,"
#endif
"NOREQ/S,ERRWARN/S,MAKEDIR/S,"
"MOVE/S,DELETE/S,HARD=HARDLINK/S,SOFT=SOFTLINK/S,"
"FOLNK=FORCELINK/S,FODEL=FORCEDELETE/S,"
"FOOVR=FORCEOVERWRITE/S,DONTOVR=DONTOVERWRITE/S,"
"FORCE/S";

#define COPYFLAG_ALL            (1<<0)
#define COPYFLAG_DATES          (1<<1)
#define COPYFLAG_NOPRO          (1<<2)
#define COPYFLAG_COMMENT        (1<<3)
#define COPYFLAG_FORCELINK      (1<<4)
#define COPYFLAG_FORCEDELETE    (1<<5)
#define COPYFLAG_FORCEOVERWRITE (1<<6)
#define COPYFLAG_DONTOVERWRITE  (1<<7)
#define COPYFLAG_QUIET          (1<<8)
#define COPYFLAG_VERBOSE        (1<<9)
#define COPYFLAG_ERRWARN        (1<<10)
#define COPYFLAG_PROTECTION     (1<<11)

#define COPYFLAG_SOFTLINK       (1<<20) /* produce softlinks */
#define COPYFLAG_DEST_FILE      (1<<21) /* one file mode */
#define COPYFLAG_DONE           (1<<22) /* did something in DoWork */
#define COPYFLAG_ENTERSECOND    (1<<23) /* entered directory second time */

#define COPYFLAG_SRCNOFILESYS   (1<<24) /* source is no filesystem */
#define COPYFLAG_DESNOFILESYS   (1<<25) /* destination is no filesystem */

#define COPYMODE_COPY           0
#define COPYMODE_MOVE           1
#define COPYMODE_DELETE         2
#define COPYMODE_MAKEDIR        3
#define COPYMODE_LINK           4

#define PRINTOUT_SIZE           50      /* maximum size of name printout */
#define PRINTOUT_SPACES         10      /* maximum number of spaces      */

#define FILEPATH_SIZE           2048    /* maximum size of filepaths     */

/* return values */
#define TESTDEST_DIR_OK         2       /* directory exists, go in */
#define TESTDEST_DELETED        1       /* file or empty directory deleted */
#define TESTDEST_NONE           0       /* nothing existed */
#define TESTDEST_ERROR          -1      /* an error occured */
#define TESTDEST_CANTDELETE     -2      /* deletion not allowed (DONTOV) */

struct IptrArgs
{
    IPTR  from;
    IPTR  to;
    IPTR  pattern;
    IPTR  buffer;
    IPTR  all;
    IPTR  direct;
    IPTR  clone;
    IPTR  dates;
    IPTR  nopro;
    IPTR  comment;
    IPTR  quiet;
#if !USE_ALWAYSVERBOSE
    IPTR  verbose;
#endif
    IPTR  noreq;
    IPTR  errwarn;
    IPTR  makedir;
    IPTR  move_mode;
    IPTR  delete_mode;
    IPTR  hardlink;
    IPTR  softlink;
    IPTR  forcelink;
    IPTR  forcedelete;
    IPTR  forceoverwrite;
    IPTR  dontoverwrite;
    IPTR  force;
};


struct Args
{
    STRPTR *from;
    STRPTR  to;
    STRPTR  pattern;
    LONG   *buffer;
    LONG    all;
    LONG    direct;
    LONG    clone;
    LONG    dates;
    LONG    nopro;
    LONG    comment;
    LONG    quiet;
    LONG    verbose;
    LONG    noreq;
    LONG    errwarn;
    LONG    makedir;
    LONG    move_mode;
    LONG    delete_mode;
    LONG    hardlink;
    LONG    softlink;
    LONG    forcelink;
    LONG    forcedelete;
    LONG    forceoverwrite;
    LONG    dontoverwrite;
    LONG    force;
};


struct CopyData
{
    struct ExecBase     *SysBase;
    struct DosLibrary   *DOSBase;

    ULONG       Flags;
    ULONG       BufferSize;
    STRPTR      Pattern;
    BPTR        Destination;
    BPTR        CurDest;  /* Current Destination */
    ULONG       DestPathSize;
    struct FileInfoBlock Fib;
    UBYTE       Mode;
    UBYTE       RetVal;         /* when set, error output is already done */
    UBYTE       RetVal2;        /* when set, error output must be done */
    UBYTE       Deep;
    UBYTE       FileName[FILEPATH_SIZE];
    UBYTE       DestName[FILEPATH_SIZE];

    STRPTR      CopyBuf;
    ULONG       CopyBufLen;
};

#define TEXT_READ               texts[0]
#define TEXT_COPIED             texts[1]
#define TEXT_MOVED              texts[2]
#define TEXT_DELETED            texts[3]
#define TEXT_LINKED             texts[4]
#define TEXT_RENAMED            texts[5]
#define TEXT_CREATED            texts[6]
#define TEXT_ENTERED            texts[7]
#define TEXT_OPENED_FOR_OUTPUT  texts[8]
#define TEXTNUM_MODE            9
#define TEXT_DIRECTORY          texts[15]
#define TEXT_NOT_DONE           texts[16]
#define TEXT_NOTHING_DONE       texts[17]
#define TEXT_ERR_FORCELINK      texts[18]
#define TEXT_ERR_DELETE_DEVICE  texts[19]
#define TEXT_ERR_DEST_DIR       texts[20]
#define TEXT_ERR_INFINITE_LOOP  texts[21]
#define TEXT_ERR_WILDCARD_DEST  texts[22]

const CONST_STRPTR texts[] =
    {
        "read.",
        "copied.",
        "moved.",
        "deleted.",
        "linked.",
        "renamed.",
        "   [created]",
        "entered",
        "opened for output",
        "COPY mode\n",
        "MOVE mode\n",
        "DELETE mode\n",
        "MAKEDIR mode\n",
        "HARDLINK mode\n",
        "SOFTLINK mode\n",
        "%s (Dir)",         /* output of directories */
        " not %s: ",
        "No file was processed.\n",
        "FORCELINK keyword required.\n",
        "A device cannot be deleted.",
        "Destination must be a directory.\n",
        "Infinite loop not allowed.\n",
        "Wildcard destination invalid.\n",
    };

LONG  CopyFile(BPTR, BPTR, ULONG, struct CopyData *);
void  DoWork(STRPTR, struct CopyData *);
LONG  IsMatchPattern(STRPTR name, struct CopyData *cd);
LONG  IsPattern(STRPTR, struct CopyData *); /* return 0 -> NOPATTERN, return -1 --> ERROR */
LONG  KillFile(STRPTR, ULONG, struct CopyData *);
LONG  KillFileKeepErr(STRPTR name, ULONG doit, struct CopyData *);
LONG  LinkFile(BPTR, STRPTR, ULONG, struct CopyData *);
BPTR OpenDestDir(STRPTR, struct CopyData *);
void  PatCopy(STRPTR, struct CopyData *);
void  PrintName(CONST_STRPTR, ULONG, ULONG, ULONG, struct CopyData *);
void  PrintNotDone(CONST_STRPTR, CONST_STRPTR, ULONG, ULONG, struct CopyData *);
ULONG TestFileSys(STRPTR, struct CopyData *); /* returns value, when is a filesystem */
void  SetData(STRPTR, struct CopyData *);
LONG  TestDest(STRPTR, ULONG, struct CopyData *);
ULONG TestLoop(BPTR, BPTR, struct CopyData *);

int main(void)
{
#ifndef __AROS__
    struct ExecBase *SysBase;
    struct DosLibrary *DOSBase;
#endif
    struct Process *task;
    struct CopyData *cd;
    int retval = RETURN_FAIL;

#ifndef __AROS__
    SysBase=*((struct ExecBase**) 4);
#endif

    /* test for WB and reply startup-message */
    if (!(task = (struct Process *)FindTask(NULL))->pr_CLI)
    {
        WaitPort(&task->pr_MsgPort);
        Forbid();
        ReplyMsg(GetMsg(&task->pr_MsgPort));

        return RETURN_FAIL;
    }

#ifndef __AROS__
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
#endif
    cd = AllocMem(sizeof(*cd), MEMF_PUBLIC | MEMF_CLEAR);

    if (DOSBase && cd)
    {
        STRPTR a[2] = { "", 0 };
        struct RDArgs *rda;
        struct IptrArgs iArgs;
        struct Args args;

        cd->SysBase = SysBase;
        cd->DOSBase = DOSBase;
#define SysBase cd->SysBase
#define DOSBase cd->DOSBase

        cd->BufferSize = 512*1024;
        cd->Mode = COPYMODE_COPY;
        cd->RetVal2 = RETURN_FAIL;
        cd->Deep = 1;

        memset(&iArgs, 0, sizeof(struct IptrArgs));

        rda = (struct RDArgs *)AllocDosObject(DOS_RDARGS, NULL);
        if (rda)
        {
            rda->RDA_ExtHelp =
                "FROM     multiple input files\n"
                "TO       destination file or directory\n"
                "PATTERN  a pattern the filenames must match\n"
                "BUFFER   buffersize for copy buffer (default 200 [100K])\n"
                "ALL      deep scan into sub directories\n"
                "DIRECT   copy/delete only: work without any tests or options\n"
                "CLONE    copy comment, protection bits and date as well\n"
                "DATES    copy dates\n"
                "NOPRO    do not copy protection bits\n"
                "COMMENT  copy filecomment\n"
                "QUIET    suppress all output and requesters\n"
#if !USE_ALWAYSVERBOSE
                "VERBOSE  give additional output\n"
#endif
                "NOREQ    suppress requesters\n"
                "ERRWARN  do not proceed, when one file failed\n"
                "MAKEDIR  produce directories\n"
                "MOVE     delete source files after copying successful\n"
                "DELETE   do not copy, but delete the source files\n"
                "HARDLINK make a hardlink to source instead of copying\n"
                "SOFTLINK make a softlink to source instead of copying\n"
                "FOLNK    also makes links to directories\n"
                "FODEL    delete protected files also\n"
                "FOOVR    also overwrite protected files\n"
                "DONTOVR  do never overwrite destination\n"
                "FORCE    DO NOT USE. Call compatibility only.\n";

            if (ReadArgs(PARAM, (IPTR *)&iArgs, rda))
            {
                ULONG patbufsize = 0;
                LONG i = 0;
                APTR win = task->pr_WindowPtr;

                args.from = (STRPTR *)iArgs.from;
                args.to   = (STRPTR)iArgs.to;
                args.pattern = (STRPTR)iArgs.pattern;
                args.buffer = (LONG *)iArgs.buffer;
                args.all = (LONG)iArgs.all;
                args.direct = (LONG)iArgs.direct;
                args.clone = (LONG)iArgs.clone;
                args.dates = (LONG)iArgs.dates;
                args.nopro = (LONG)iArgs.nopro;
                args.comment = (LONG)iArgs.comment;
                args.quiet = (LONG)iArgs.quiet;
#if USE_ALWAYSVERBOSE
                args.verbose = FALSE;
#else
                args.verbose = (LONG)iArgs.verbose;
#endif
                args.noreq = (LONG)iArgs.noreq;
                args.errwarn = (LONG)iArgs.errwarn;
                args.makedir = (LONG)iArgs.makedir;
                args.move_mode = (LONG)iArgs.move_mode;
                args.delete_mode = (LONG)iArgs.delete_mode;
                args.hardlink = (LONG)iArgs.hardlink;
                args.softlink = (LONG)iArgs.softlink;
                args.forcelink = (LONG)iArgs.forcelink;
                args.forcedelete = (LONG)iArgs.forcedelete;
                args.forceoverwrite = (LONG)iArgs.forceoverwrite;
                args.dontoverwrite = (LONG)iArgs.dontoverwrite;
                args.force = (LONG)iArgs.force;

                if (args.quiet) /* when QUIET, SILENT and NOREQ are also
                                   true! */
                {
                    /* Original doesn't hide requesters with QUIET */
                    /*args.noreq = 1;*/
                    args.verbose = FALSE;
                }

                if (args.buffer && *args.buffer > 0) /* minimum buffer size */
                {
                    cd->BufferSize = *args.buffer * 512;
                }

                if (args.quiet)
                {
                    cd->Flags |= COPYFLAG_QUIET;
                }

#if !USE_ALWAYSVERBOSE
                if (args.verbose)
                {
                    cd->Flags |= COPYFLAG_VERBOSE;
                }
#endif
                if (args.all)
                {
                    cd->Flags |= COPYFLAG_ALL;
                }

                /* 12-jul-03 bugfix: always copy protection flags! -Piru */
                cd->Flags |= COPYFLAG_PROTECTION;
                if (args.clone)
                {
                    cd->Flags |= COPYFLAG_DATES | COPYFLAG_COMMENT | COPYFLAG_PROTECTION;
                }

                if (args.dates)
                {
                    cd->Flags |= COPYFLAG_DATES;
                }

                if (args.comment)
                {
                    cd->Flags |= COPYFLAG_COMMENT;
                }

                if (args.nopro)
                {
                    cd->Flags |= COPYFLAG_NOPRO;
                }

                if (args.forcelink)
                {
                    cd->Flags |= COPYFLAG_FORCELINK;
                }

                if (args.forcedelete)
                {
                    cd->Flags |= COPYFLAG_FORCEDELETE;
                }

                if (args.forceoverwrite)
                {
                    cd->Flags |= COPYFLAG_FORCEOVERWRITE;
                }

                if (args.dontoverwrite)
                {
                    cd->Flags |= COPYFLAG_DONTOVERWRITE;
                }

                if (args.errwarn)
                {
                    cd->Flags |= COPYFLAG_ERRWARN;
                }

                if (args.force) /* support OS Delete and MakeLink command
                                   options */
                {
                    if (args.delete_mode)
                    {
                        cd->Flags |= COPYFLAG_FORCEDELETE;
                    }

                    if (args.hardlink || args.softlink)
                    {
                        cd->Flags |= COPYFLAG_FORCELINK;
                    }
                }

                if (!args.from)  /* no args.from means currentdir */
                {
                    args.from = a;
                }

                if (args.noreq) /* no dos.library requests allowed */
                {
                    task->pr_WindowPtr = (APTR)-1;
                }

                if (args.delete_mode)
                {
                    ++i;
                    cd->Mode = COPYMODE_DELETE;
                }

                if (args.move_mode)
                {
                    ++i;
                    cd->Mode = COPYMODE_MOVE;
                }

                if (args.makedir)
                {
                    ++i;
                    cd->Mode = COPYMODE_MAKEDIR;
                }

                if (args.hardlink)
                {
                    ++i;
                    cd->Mode = COPYMODE_LINK;
                }

                if (args.softlink)
                {
                    ++i;
                    cd->Mode = COPYMODE_LINK;
                    cd->Flags |= COPYFLAG_SOFTLINK | COPYFLAG_FORCELINK;
                }

#ifdef __AROS__
#warning "CHECKME: Is this still needed?"
                if (*args.from == NULL)
                {
                    PutStr("No arguments specified\n");
                    FreeArgs(rda);
                    FreeDosObject(DOS_RDARGS, rda);
                    return RETURN_ERROR;
                }
#endif

                if (cd->Mode != COPYMODE_DELETE &&
                    cd->Mode != COPYMODE_MAKEDIR && !args.to)
                {
                    if (*(args.from + 1)) /* when no TO is specified, the arg
                                             is last */
                    {                     /* one of from. Copy this argument into */
                        STRPTR *a;        /* args.to */

                        a = args.from;

                        while(*(++a))
                            ;

                        args.to = *(--a);
                        *a = 0;
                    }
                }
#if USE_ALWAYSVERBOSE

                /* Only do this if quiet isn't set - bigfoot */
                if (!args.quiet)
                {
                    /* If more than two args, be verbose... - Piru */
                    if (args.from[0] && args.from[1])
                    {
                        args.verbose = TRUE;
                        cd->Flags |= COPYFLAG_VERBOSE;
                    }

                    /* If any of the sources is a pattern, be verbose... - Piru */
                    {
                        STRPTR *a;

                        for (a = args.from; *a; a++)
                        {
                            if (IsMatchPattern(*a, cd) != 0)
                            {
                                args.verbose = TRUE;
                                cd->Flags |= COPYFLAG_VERBOSE;
                            }
                        }
                    }
                }
#endif

                /* test if more than one of the above four or any other wrong
                   arguments */

                if (i > 1 ||
                        (args.from == a && cd->Mode == COPYMODE_MAKEDIR) ||
                        (args.direct && (args.from == a || !*args.from ||
                                         args.pattern ||
                                         (cd->Flags & ~(COPYFLAG_QUIET | COPYFLAG_VERBOSE | COPYFLAG_ERRWARN)) ||
                                         (cd->Mode != COPYMODE_DELETE && (cd->Mode != COPYMODE_COPY ||
                                                                         !args.to || args.from[1])))) ||
                        (args.dontoverwrite && args.forceoverwrite) ||
                        /* (args.nopro && args.clone) ||*/ /* Ignore, like original - Piru */
                        (args.softlink && args.all) ||
                        (!args.to && cd->Mode != COPYMODE_DELETE && cd->Mode != COPYMODE_MAKEDIR))
                {
                    SetIoErr(ERROR_TOO_MANY_ARGS);
                }
                else if (cd->Mode == COPYMODE_MAKEDIR)
                {
                    LONG i;
		    BPTR dir;
                    cd->RetVal2 = RETURN_OK;

#if !USE_ALWAYSVERBOSE
                    if (args.verbose)
                    {
                        PutStr(texts[TEXTNUM_MODE + COPYMODE_MAKEDIR]);
                    }
#endif

                    while (!cd->RetVal && !cd->RetVal2 && *args.from)
                    {
                        if ((i = IsPattern(*args.from, cd)))
                        {
                            if (i != -1)
                            {
                                cd->RetVal = RETURN_ERROR;

                                if (!args.quiet)
                                {
                                    PutStr(TEXT_ERR_WILDCARD_DEST);
                                }
                            }
                            else
                            {
                                cd->RetVal2 = RETURN_FAIL;
                            }
                        }

                        if ((dir = OpenDestDir(*args.from, cd)))
                        {
                            UnLock(dir);
                            cd->Flags |= COPYFLAG_DONE;
                        }

                        ++args.from;
                    }
                } /* cd->Mode == COPYMODE_MAKEDIR */
                else if (args.direct)
                {
                    if (cd->Mode == COPYMODE_COPY)
                    {
                        BPTR in, out;

                        if ((in = Open(*args.from, MODE_OLDFILE)))
                        {
                            if ((out = Open(args.to, MODE_NEWFILE)))
                            {
                                cd->RetVal2 = CopyFile(in, out, cd->BufferSize, cd);
                                Close(out);
                            }

                            Close(in);
                        }
                    }
                    else /* COPYMODE_DELETE */
                    {
                        while (*args.from)
                        {
                            KillFile(*(args.from++), cd->Flags & COPYFLAG_FORCEDELETE, cd);
                        }

                        cd->RetVal2 = RETURN_OK;
                    }
                }
                else
                {
                    if (args.pattern && *args.pattern)
                    {
                        patbufsize = (strlen(args.pattern) << 1) + 3;

                        if ((cd->Pattern = (STRPTR)AllocMem(patbufsize,
                                                            MEMF_ANY)))
                        {
                            if (ParsePatternNoCase(args.pattern, cd->Pattern,
                                                   patbufsize) < 0)
                            {
                                FreeMem(cd->Pattern, patbufsize);
                                cd->Pattern = 0;
                            }
                        }
                    }

                    if (1) // (cd->Fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
                    {
#if !USE_ALWAYSVERBOSE
                        if (args.verbose)
                        {
                            PutStr(texts[TEXTNUM_MODE + cd->Mode +
                                         (cd->Flags & COPYFLAG_SOFTLINK ? 1 : 0)]);
                        }
#endif
                        if (args.pattern && !cd->Pattern)
                        {
                            if (!*args.pattern)
                            {
                                SetIoErr(ERROR_BAD_TEMPLATE);
                            }
                        }
                        else if (cd->Mode == COPYMODE_DELETE)
                        {
                            cd->RetVal2 = RETURN_OK;

                            while (cd->RetVal <= (args.errwarn ? RETURN_OK : RETURN_WARN)
                                    && *args.from)
                            {
                                PatCopy(*(args.from++), cd);
                            }
                        }
                        else if ((i = IsPattern(args.to, cd)))
                        {
                            if (i != -1)
                            {
                                cd->RetVal = RETURN_ERROR;

                                if (!args.quiet)
                                {
                                    PutStr(TEXT_ERR_WILDCARD_DEST);
                                }
                            }
                        }
                        else
                        {
                            STRPTR path;

                            if (*(path = PathPart(args.to)) == '/')
                            {
                                ++path; /* is destination a path description ? */
                            }

                            if (*path && !*(args.from+1) &&
                                !(i = IsMatchPattern(*args.from, cd)))
                            {
                                BPTR lock;
                                LONG lockioerr;

                                /* is destination an existing directory */
                                if ((lock = Lock(args.to, SHARED_LOCK)))
                                {
                                    if (Examine(lock, &cd->Fib))
                                    {
                                        if (cd->Fib.fib_DirEntryType > 0)
                                        {
                                            cd->RetVal2 = RETURN_OK;
                                        }

                                        /* indicate dir-mode for next if */
                                    }
                                    else
                                    {
                                        i = 1;
                                    }

                                    UnLock(lock);
                                }

/* Some magic to handle tick quoted pattern object names. Quite crude way to
 * handle it, but I couldn't think of anything better. - Piru
 */
#if 1
                                if (!i && cd->RetVal2 && !IsMatchPattern(*args.from, cd))
                                {
                                    ULONG len;
                                    STRPTR pat;

                                    //Printf("pattern check <%s>\n", *args.from);

                                    len = (strlen(*args.from) << 1) + 3;

                                    if ((pat = (STRPTR)AllocMem(len,
                                                                MEMF_ANY)))
                                    {
                                        if (ParsePattern(*args.from, pat, len) > -1 &&
                                            strlen(pat) <= strlen(*args.from))
                                        {
                                            lock = Lock(pat, SHARED_LOCK);
                                            if (lock)
                                            {
                                                UnLock(lock);

                                                strcpy(*args.from, pat);
                                            }
                                        }
                                        
                                        FreeMem(pat, len);
                                    }
                                }
#endif

                                /* is source a directory */
                                if (!i && cd->RetVal2 &&
                                    (lock = Lock(*args.from, SHARED_LOCK)))
                                {
                                    if (Examine(lock, &cd->Fib))
                                    {
                                        cd->RetVal2 = RETURN_OK;
                                        if (cd->Mode != COPYMODE_COPY ||
                                            cd->Fib.fib_DirEntryType < 0)
                                        {
                                            UBYTE sep;

                                            cd->Flags |= COPYFLAG_DEST_FILE;

                                            /* produce missing destination directories */
                                            sep = *path;
                                            *path = 0;

                                            if ((cd->CurDest = OpenDestDir(args.to, cd)))
                                            {
                                                *path = sep;

                                                /* do the job */
                                                UnLock(lock);
                                                lock = 0;
                                                CopyMem(*args.from, cd->FileName,
                                                        1 + strlen(*args.from));
                                                DoWork(FilePart(args.to), cd); /* on file call */
                                                UnLock(cd->CurDest);
                                            }
                                        }
                                    }

                                    if (lock)
                                    {
                                        UnLock(lock);
                                    }
                                }

                                lockioerr = IoErr(); /* We save ioerr here, because TestFileSys changes it */

                                if (lock == 0 && cd->Mode == COPYMODE_COPY && !TestFileSys(*args.from, cd))
                                {
                                    UBYTE sep;
                                    cd->Flags |= COPYFLAG_DEST_FILE | COPYFLAG_SRCNOFILESYS;
                                    cd->RetVal2 = RETURN_OK;

                                    /* produce missing destination directories */
                                    sep = *path;
                                    *path = 0;

                                    if ((cd->CurDest = OpenDestDir(args.to, cd)))
                                    {
                                        *path = sep;

                                        /* do the job */
                                        CopyMem(*args.from, cd->FileName, 1 + strlen(*args.from));
                                        DoWork(FilePart(args.to), cd); /* on file call */
                                        UnLock(cd->CurDest);
                                    }
                                }
                                else
                                    SetIoErr(lockioerr);
                            }
                            else if (i != -1)
                            {
                                cd->RetVal2 = RETURN_OK;
                            }

                            if (!cd->RetVal && !cd->RetVal2 && !(cd->Flags & COPYFLAG_DEST_FILE) &&
                                (cd->Destination = OpenDestDir(args.to, cd)))
                            {
                                while (cd->RetVal <= (args.errwarn ? RETURN_OK : RETURN_WARN)
                                        && *args.from && !CTRL_C)
                                {
                                    PatCopy(*(args.from++), cd);
                                }

                                UnLock(cd->Destination);
                            }
                        } /* else */

                        if (!(cd->Flags & COPYFLAG_DONE) && args.verbose &&
                            !cd->RetVal && !cd->RetVal2)
                        {
                            PutStr(TEXT_NOTHING_DONE);
                        }

                    } /* if (1) */

                    if (cd->Pattern)
                    {
                        FreeMem(cd->Pattern, patbufsize);
                    }
                } /* else */

                task->pr_WindowPtr = win;

                FreeArgs(rda);
            } /* ReadArgs */

            FreeDosObject(DOS_RDARGS, rda);
        } /* AllocDosObject */

        if (!cd->RetVal2 && CTRL_C)
        {
            SetIoErr(ERROR_BREAK);
            cd->RetVal2 = RETURN_WARN;
        }

        if (cd->RetVal2 && !args.quiet && !cd->RetVal)
        {
            PrintFault(IoErr(), NULL);
        }

        if (cd->RetVal)
        {
            cd->RetVal2 = cd->RetVal;
        }

        if (args.errwarn && cd->RetVal2 == RETURN_WARN)
        {
            cd->RetVal2 = RETURN_ERROR;
        }

        if (cd->CopyBuf)
        {
            FreeMem(cd->CopyBuf, cd->CopyBufLen);
        }

#undef SysBase
#undef DOSBase
    }
    if (cd)
    {
        retval = cd->RetVal2;
        FreeMem(cd, sizeof(*cd));
    }
    else if (DOSBase)
    {
        PrintFault(IoErr(), NULL);
    }
#ifndef __AROS__
    if (DOSBase)
    {
        CloseLibrary((struct Library *)DOSBase);
    }
#endif
    return retval;
}

#define SysBase cd->SysBase
#define DOSBase cd->DOSBase

void PatCopy(STRPTR name, struct CopyData *cd)
{
    struct AnchorPath *APath;
    ULONG retval, doit = 0, deep = 0, failval = RETURN_WARN, first = 0;

#if DEBUG
    Printf("PatCopy(%s, .)\n", name);
#endif

    if ((cd->Mode == COPYMODE_COPY || (cd->Flags & COPYFLAG_ALL)) && !IsMatchPattern(name, cd))
    {
        first = 1; /* enter first directory (support of old copy style) */
    }

    if (cd->Flags & COPYFLAG_ERRWARN)
    {
        failval = RETURN_OK;
    }

    cd->CurDest = cd->Destination;
    cd->DestPathSize = 0;

    if (cd->Mode == COPYMODE_COPY && !TestFileSys(name, cd))
    {
        cd->Flags |= COPYFLAG_SRCNOFILESYS;
        CopyMem(name, cd->FileName, 1 + strlen(name));
        DoWork(FilePart(name), cd);
        cd->Flags &= ~COPYFLAG_SRCNOFILESYS;

        return;
    }

    if ((APath = (struct AnchorPath *)AllocMem(sizeof(struct AnchorPath) + FILEPATH_SIZE,
                                               MEMF_PUBLIC | MEMF_CLEAR)))
    {
        int parentdirerr = 0;

        APath->ap_BreakBits = SIGBREAKF_CTRL_C;
        APath->ap_Strlen    = FILEPATH_SIZE;

        for (retval = MatchFirst(name, APath);
             !retval && cd->RetVal <= failval && !cd->RetVal2;
             retval = MatchNext(APath)
            )
        {
            if (parentdirerr)
            {
                //Printf("ParentDir() fuxored last round! Would copy next files to SYS: !\n");
                retval = IoErr();
                if (!retval)
                    SetIoErr(retval = ERROR_INVALID_LOCK);
                break;
            }

            if (doit)
            {
                DoWork(cd->Fib.fib_FileName, cd);
                doit = 0;
            }

            if (deep)         /* used for Deep checking */
            {
                ++cd->Deep;
                deep = 0;
            }

            cd->Flags &= ~COPYFLAG_ENTERSECOND;

            CopyMem(APath->ap_Buf, cd->FileName, FILEPATH_SIZE);
            CopyMem(&APath->ap_Info, &cd->Fib, sizeof(struct FileInfoBlock));

            if (first && APath->ap_Info.fib_DirEntryType > 0)
            {
#if USE_ALWAYSVERBOSE
                /* If the source is a directory, be verbose - Piru */
                cd->Flags |= COPYFLAG_VERBOSE;
#endif
                APath->ap_Flags |= APF_DODIR;
            }
            else if (APath->ap_Flags & APF_DIDDIR)
            {
                BPTR i;

                cd->Flags |= COPYFLAG_ENTERSECOND;
                APath->ap_Flags &= ~APF_DIDDIR;
                --cd->Deep;

                if (cd->Mode == COPYMODE_DELETE || cd->Mode == COPYMODE_MOVE)
                {
                    doit = 1;
                }

                if ((i = cd->CurDest))
                {
                    cd->CurDest = ParentDir(i);
                    cd->DestPathSize = 0;

                    if (i != cd->Destination)
                    {
                        UnLock(i);
                    }

                    if (!cd->CurDest)
                    {
                        parentdirerr = 1;
                        continue;
                    }
                }
            }
            else if (APath->ap_Info.fib_DirEntryType > 0)
            {
                doit = 1;

                if (cd->Flags & COPYFLAG_ALL)
                {
#if USE_SOFTLINKCHECK

                    BOOL enter = TRUE;
                    BPTR dirlock, lock;

                    dirlock = CurrentDir(APath->ap_Current->an_Lock);
                    lock = Lock(APath->ap_Info.fib_FileName, ACCESS_READ);
                    if (lock)
                        UnLock(lock);
                    else
                    {
                        struct DevProc *dvp;
                        LONG ioerr = IoErr();

                        if (ioerr == ERROR_OBJECT_NOT_FOUND &&
                            (dvp = GetDeviceProc("", NULL)))
                        {
#define BUFFERSIZE 512
                            UBYTE *buffer = AllocMem(BUFFERSIZE, MEMF_PUBLIC);

                            if (buffer)
                            {
                                if (ReadLink(dvp->dvp_Port, dvp->dvp_Lock, APath->ap_Info.fib_FileName, buffer, BUFFERSIZE - 1) > 0)
                                {
                                    if (!(cd->Flags & COPYFLAG_QUIET))
                                    {
                                        buffer[BUFFERSIZE - 1] = '\0';

                                        Printf("Warning: Skipping dangling softlink %s -> %s\n",
                                               APath->ap_Info.fib_FileName, buffer);
                                    }

                                    enter = FALSE;
                                }

                                FreeMem(buffer, BUFFERSIZE);
                            }

                            FreeDeviceProc(dvp);
                        }

                        SetIoErr(ioerr);
                    }
                    CurrentDir(dirlock);

                    if (enter)
                    {
                        APath->ap_Flags |= APF_DODIR;
                        deep = 1;
                    }

#else /* USE_SOFTLINKCHECK */

                    APath->ap_Flags |= APF_DODIR;
                    deep = 1;

#endif /* USE_SOFTLINKCHECK */
                }
            }
            else if (!cd->Pattern || MatchPatternNoCase(cd->Pattern, APath->ap_Info.fib_FileName))
            {
                doit = 1;
            }

            first = 0;
        }

        MatchEnd(APath);

        if (retval && retval != ERROR_NO_MORE_ENTRIES)
        {
            LONG ioerr = IoErr();
#if USE_ALWAYSVERBOSE
            cd->Flags |= COPYFLAG_VERBOSE;
#endif
            Printf("%s - ", name);
            PrintFault(ioerr, NULL);
            SetIoErr(ioerr);

            cd->RetVal2 = RETURN_FAIL;
        }

        if (doit)
        {
            DoWork(cd->Fib.fib_FileName, cd);
        }

        /* No need to clear the flags here, as they are cleared on next PatJoin
           call (DoWork is not called first round, as lock is zero!). */

        FreeMem(APath, sizeof(struct AnchorPath) + FILEPATH_SIZE);
    }
    else
    {
        cd->RetVal = RETURN_FAIL;

        if (!cd->Flags & COPYFLAG_QUIET)
        {
            PrintFault(ERROR_NO_FREE_STORE, NULL);
        }
    }

    if (cd->CurDest && cd->CurDest != cd->Destination)
    {
        UnLock(cd->CurDest);
    }
}


LONG IsPattern(STRPTR name, struct CopyData *cd)
{
    LONG a, ret = -1;
    STRPTR buffer;

    a = (strlen(name) << 1) + 3;

    if ((buffer = (STRPTR)AllocMem(a, MEMF_ANY)))
    {
        ret = ParsePattern(name, buffer, a);
        FreeMem(buffer, a);
    }

    if (ret == -1)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    return ret;
}


LONG IsMatchPattern(STRPTR name, struct CopyData *cd)
{
    struct AnchorPath ap;
    LONG ret = -1;

    ap.ap_BreakBits = 0;
    ap.ap_Flags     = APF_DOWILD;
    ap.ap_Strlen    = 0;

    if (MatchFirst(name, &ap) == 0)
    {
        ret = (ap.ap_Flags & APF_ITSWILD) ? TRUE : FALSE;

        MatchEnd(&ap);
    }

    return ret;
}


LONG KillFile(STRPTR name, ULONG doit, struct CopyData *cd)
{
    if (doit)
    {
        SetProtection(name, 0);
    }

    return DeleteFile(name);
}


LONG KillFileKeepErr(STRPTR name, ULONG doit, struct CopyData *cd)
{
    LONG ret, ioerr;

    ioerr = IoErr();
    ret = KillFile(name, doit, cd);
    SetIoErr(ioerr);

    return ret;
}


BPTR OpenDestDir(STRPTR name, struct CopyData *cd)
{
    LONG a, err = 0, cr = 0;
    BPTR dir;
    STRPTR ptr = name;
    UBYTE as;

    if ((cd->Mode == COPYMODE_COPY || cd->Mode == COPYMODE_MOVE) && !TestFileSys(name, cd))
    {
        cd->Flags |= COPYFLAG_DESNOFILESYS;
        CopyMem(name, cd->DestName, 1 + strlen(name));

        return Lock("", SHARED_LOCK);
    }

    while (!err && *ptr != 0)
    {
        while (*ptr && *ptr != '/')
        {
            ++ptr;
        }

        as = *ptr;
        *ptr = 0;

        if ((a = TestDest(name, 1, cd)) == TESTDEST_CANTDELETE)
        {
            if (!(cd->Flags & COPYFLAG_QUIET))
            {
                PutStr(TEXT_ERR_DEST_DIR);
            }

            err = 2;
        }
        else if (a < 0)
        {
            err = 1;
        }
        else if (a != TESTDEST_DIR_OK)
        {
            if ((dir = CreateDir(name)))
            {
                ++cr;

                if ((cd->Flags & COPYFLAG_VERBOSE))
                {
                    PrintName(name, 1, 1, 1, cd);
                    Printf("%s\n", TEXT_CREATED);
                }

                UnLock(dir);
            }
            else
            {
                if (!(cd->Flags & COPYFLAG_QUIET))
                {
                    PrintNotDone(name, TEXT_CREATED, 1, 1, cd);
                }

                err = 2;
            }
        }

        *ptr = as;

	/* 26-Oct-2003 bugfix: Don't scan past end of the string.
	 * as is the old char, if '\0' we've reached the end of the
	 * string. - Piru
	 */
        if (as)
        {
	    ptr++;
        }
    }

    if (err)
    {
        cd->RetVal = RETURN_ERROR;

        if (!(cd->Flags & COPYFLAG_QUIET) && err == 1)
        {
            PrintNotDone(name, TEXT_OPENED_FOR_OUTPUT, 1, 1, cd);
        }

        return 0;
    }

    if (cd->Mode == COPYMODE_MAKEDIR && !cr && !(cd->Flags & COPYFLAG_QUIET))
    {
        SetIoErr(ERROR_OBJECT_EXISTS);
        PrintNotDone(name, TEXT_CREATED, 1, 1, cd);
    }

    return Lock(name, SHARED_LOCK);
}


void PrintName(CONST_STRPTR name, ULONG deep, ULONG dir, ULONG txt, struct CopyData *cd)
{
#if 0
    deep %= PRINTOUT_SPACES; /* reduce number of spaces */
    /* This produces an error with MaxonC++ */
#endif

    PutStr("   ");
    if (deep)
    {
        while (--deep)
        {
            PutStr("        ");
        }
    }

    if (dir)
    {
        PutStr("     ");
    }

#if 0
    if ((deep = strlen(name)) > PRINTOUT_SIZE) /* reduce name size */
    {
        name += deep-PRINTOUT_SIZE;
        PutStr("...");
    }
#endif

    Printf((dir ? TEXT_DIRECTORY : (STRPTR) "%s"), name);

    if (!dir && txt)
    {
        PutStr("..");
    }

    Flush(Output());
}


void PrintNotDone(CONST_STRPTR name, CONST_STRPTR txt, ULONG deep, ULONG dir, struct CopyData *cd)
{
#if !USE_ALWAYSVERBOSE
    if (name)
    {
        PrintName(name, deep, dir, 1, cd);
    }
#endif

    Printf(TEXT_NOT_DONE, txt);
    PrintFault(IoErr(), NULL);
}


/* returns value, when it seems to be a filesystem */
ULONG TestFileSys(STRPTR name, struct CopyData *cd)
{
    STRPTR n = name;
    ULONG ret = 1;

    while (*n && *n != ':')
    {
        ++n;
    }

    if (*(n++) == ':')
    {
        UBYTE a;

        a = *n;
        *n = 0;
        ret = IsFileSystem(name);
        *n = a;
    }

    return ret;
}


void DoWork(STRPTR name, struct CopyData *cd)
{
    BPTR pdir, lock = 0;
    CONST_STRPTR printerr = NULL, printok = "";

#if DEBUG
    Printf("DoWork(%s, .)\n", name);
#endif

    if (cd->RetVal > (cd->Flags & COPYFLAG_ERRWARN ? RETURN_OK : RETURN_WARN) || cd->RetVal2)
    {
#if DEBUG
        Printf("DoWork(RetVal %ld)\n", cd->RetVal);
#endif
        return;
    }

    if (cd->Mode != COPYMODE_DELETE && !(cd->Flags & COPYFLAG_DESNOFILESYS))
    {
        if (!cd->DestPathSize)
        {
            if (!NameFromLock(cd->CurDest, cd->DestName, FILEPATH_SIZE))
            {
                cd->RetVal2 = RETURN_FAIL;
                UnLock(lock);

#if DEBUG
                Printf("DoWork(NameFromLock RetVal %ld)\n", cd->RetVal);
#endif

                return;
            }

            cd->DestPathSize = strlen(cd->DestName);
        }

        cd->DestName[cd->DestPathSize] = 0;
        AddPart(cd->DestName, name, FILEPATH_SIZE);
    }

    if (cd->Flags & (COPYFLAG_SRCNOFILESYS|COPYFLAG_DESNOFILESYS))
    {
        ULONG res = 0, kill = 1;
	BPTR in, out;
        CONST_STRPTR txt = TEXT_OPENED_FOR_OUTPUT;

#if DEBUG
        Printf("Partly DIRECT mode active now (%s - %s)\n", cd->FileName,
               cd->DestName);
#endif

        if ((out = Open(cd->DestName, MODE_NEWFILE)))
        {
            txt = cd->Mode == COPYMODE_MOVE ? TEXT_MOVED : TEXT_COPIED;

            if ((in = Open(cd->FileName, MODE_OLDFILE)))
            {
                ULONG h;

                h = CopyFile(in, out, cd->BufferSize, cd);
                Close(out); out = NULL;
                Close(in);

                if (!h)
                {
                    kill = 0;

                    if (cd->Mode == COPYMODE_MOVE)
                    {
                        if (KillFile(cd->FileName, cd->Flags & COPYFLAG_FORCEDELETE, cd))
                        {
                            res = 1;
                        }
                    }
                    else
                    {
                        res = 1;
                    }
                }
            }

            if (out)
            {
                Close(out);
            }

            if (kill)
            {
                KillFileKeepErr(cd->DestName, 0, cd);
            }
        }

        if (!res && !(cd->Flags & COPYFLAG_QUIET))
        {
            PrintNotDone(cd->Flags & COPYFLAG_VERBOSE ? name : 0,
                         txt, cd->Deep, cd->Fib.fib_DirEntryType > 0, cd);
        }
        else
        {
            cd->Flags |= COPYFLAG_DONE;

            if ((cd->Flags & COPYFLAG_VERBOSE))
            {
                Printf("%s\n", txt);
            }
        }

#if DEBUG
        PutStr("DoWork(done)\n");
#endif
        return;
    }

    if (!(lock = Lock(cd->FileName, SHARED_LOCK)))
    {
        cd->RetVal = RETURN_WARN;

        if (!(cd->Flags & COPYFLAG_QUIET))
        {
            PrintNotDone(cd->Fib.fib_FileName, TEXT_READ, cd->Deep,
                         cd->Fib.fib_DirEntryType > 0, cd);
        }

#if DEBUG
        Printf("DoWork(Lock RetVal %ld)\n", cd->RetVal);
#endif
        return;
    }

    if (!(pdir = ParentDir(lock)))
    {
        cd->RetVal = RETURN_ERROR;

        if (cd->Mode == COPYMODE_DELETE)
        {
            if (!(cd->Flags & COPYFLAG_QUIET))
            {
                Printf(" %s ", cd->FileName);
                Printf(TEXT_NOT_DONE, TEXT_DELETED);
                Printf("%s\n", TEXT_ERR_DELETE_DEVICE);
            }
        }

        UnLock(lock);

#if DEBUG
        Printf("DoWork(ParentDir %ld)\n", cd->RetVal);
#endif
        return;
    }

    UnLock(pdir);

    if (!(cd->Flags & COPYFLAG_QUIET))
    {
        if ((cd->Flags & COPYFLAG_VERBOSE))
        {
            PrintName(name, cd->Deep, cd->Fib.fib_DirEntryType > 0, cd->Fib.fib_DirEntryType < 0 ||
                      (cd->Flags & COPYFLAG_ALL ? cd->Mode != COPYMODE_DELETE : cd->Mode != COPYMODE_COPY) ||
                      cd->Flags & COPYFLAG_ENTERSECOND, cd);
        }
    }

    if ((cd->Flags & COPYFLAG_ENTERSECOND) || (cd->Mode == COPYMODE_DELETE &&
        (!(cd->Flags & COPYFLAG_ALL) || cd->Fib.fib_DirEntryType < 0)))
    {
        UnLock(lock);
        lock = 0;

        if (KillFile(cd->FileName, cd->Flags & COPYFLAG_FORCEDELETE, cd))
        {
            printok = TEXT_DELETED;
        }
        else
        {
            cd->RetVal = RETURN_WARN;
            printerr = TEXT_DELETED;
        }
    }
    else if (cd->Mode == COPYMODE_DELETE)
    {
        ;
    }
    else if (cd->Fib.fib_DirEntryType > 0)
    {
        ULONG a;

        if ((cd->Flags & COPYFLAG_ALL || cd->Mode == COPYMODE_LINK ||
             cd->Mode == COPYMODE_MOVE) && TestLoop(lock, cd->CurDest, cd))
        {
            printok = 0;
            cd->RetVal = RETURN_ERROR;

            if (!(cd->Flags & COPYFLAG_QUIET))
            {
                if (!(cd->Flags & COPYFLAG_VERBOSE))
                {
                    PrintName(name, cd->Deep, 1, 1, cd);
                }

                Printf(TEXT_NOT_DONE, TEXT_ENTERED);
                PutStr(TEXT_ERR_INFINITE_LOOP);
            }
        }
        else if ((a = TestDest(cd->DestName, 1, cd)) < 0)
        {
            printerr = TEXT_CREATED;
            cd->RetVal = RETURN_ERROR;
        }
        else if (cd->Flags & COPYFLAG_ALL)
        {
            BPTR i;

            i = cd->CurDest;
            cd->DestPathSize = 0;

            if (a == TESTDEST_DIR_OK)
            {
                if (!(cd->CurDest = Lock(cd->DestName, SHARED_LOCK)))
                {
                    printerr = TEXT_ENTERED;
                    cd->RetVal = RETURN_ERROR;
                }
                else
                {
#if USE_ALWAYSVERBOSE
                    printok  = "";
#else
                    printok  = TEXT_ENTERED;
#endif
                }
            }
            else if ((cd->CurDest = CreateDir(cd->DestName)))
            {
                UnLock(cd->CurDest);

                if ((cd->CurDest = Lock(cd->DestName, SHARED_LOCK)))
                {
                    printok = TEXT_CREATED;
                }
                else
                {
                    printerr = TEXT_ENTERED;
                    cd->RetVal = RETURN_ERROR;
                }
            }
            else
            {
                printerr = TEXT_CREATED;
                cd->RetVal = RETURN_ERROR;
            }

            if (!cd->CurDest)
            {
                cd->CurDest = i;
            }
            else if (i != cd->Destination)
            {
                UnLock(i);
            }
        }
        else if (cd->Mode == COPYMODE_MOVE)
        {
            if (Rename(cd->FileName, cd->DestName))
            {
                printok = TEXT_RENAMED;
            }
            else
            {
                printerr = TEXT_RENAMED;
                cd->RetVal = RETURN_WARN;
            }
        }
        else if (cd->Mode == COPYMODE_LINK)
        {
            if (!(cd->Flags & COPYFLAG_FORCELINK))
            {
                printok = 0;
                cd->RetVal = RETURN_WARN;

                if (!(cd->Flags & COPYFLAG_QUIET))
                {
                    if (!(cd->Flags & COPYFLAG_VERBOSE))
                    {
                        PrintName(name, cd->Deep, 1, 1, cd);
                    }

                    Printf(TEXT_NOT_DONE, TEXT_LINKED);
                    PutStr(TEXT_ERR_FORCELINK);
                }
            }
            else if (LinkFile(lock, cd->DestName, cd->Flags & COPYFLAG_SOFTLINK, cd))
            {
                printok = TEXT_LINKED;
            }
            else
            {
                printerr = TEXT_LINKED;
                cd->RetVal = RETURN_WARN;
            }
        }
        else /* COPY mode only displays directories, when not ALL */
        {
            printok = 0;

            if (!(cd->Flags & COPYFLAG_QUIET))
            {
                if (cd->Flags & COPYFLAG_VERBOSE)
                {
                    PutStr("\n");
                }
            }
        }
    }
    else
    {
        /* test for existing destination file */
        if (TestDest(cd->DestName, 0, cd) < 0)
        {
            printerr = TEXT_OPENED_FOR_OUTPUT;
        }
        else if (cd->Mode == COPYMODE_MOVE && Rename(cd->FileName, cd->DestName))
        {
            printok = TEXT_RENAMED;
        }
        else if (cd->Mode == COPYMODE_LINK)
        {
            if (!(cd->Flags & COPYFLAG_SOFTLINK) && LinkFile(lock, cd->DestName, 0, cd))
            {
                printok = TEXT_LINKED;
            }
            else
            {
                printerr = TEXT_LINKED;
                cd->RetVal = RETURN_WARN;

                if (cd->Flags & COPYFLAG_SOFTLINK)
                {
                    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
                }
            }
        }
        else
        {
            ULONG res = 0, h;
	    BPTR in, out;
            CONST_STRPTR txt = TEXT_OPENED_FOR_OUTPUT;

            if ((out = Open(cd->DestName, MODE_NEWFILE)))
            {
                ULONG kill = 1;

                txt = cd->Mode == COPYMODE_MOVE ? TEXT_MOVED : TEXT_COPIED;
                UnLock(lock);
                lock = 0;

                if ((in = Open(cd->FileName, MODE_OLDFILE)))
                {
                    h = CopyFile(in, out, cd->BufferSize, cd);
                    Close(out); out = NULL;
                    Close(in);

                    if (!h)
                    {
                        kill = 0;

                        if (cd->Mode == COPYMODE_MOVE)
                        {
                            if (KillFile(cd->FileName, cd->Flags & COPYFLAG_FORCEDELETE, cd))
                            {
                                res = 1;
                            }
                        }
                        else
                        {
                            res = 1;
                        }
                    }
                }

                if (out)
                {
                    Close(out);
                }

                if (kill)
                {
                    KillFileKeepErr(cd->DestName, 0, cd);
                }
            }

            if (!res)
            {
                printerr = txt;
                cd->RetVal = RETURN_WARN;
            }
            else
            {
                printok = txt;
            }
        }
    }

    if (printerr && !(cd->Flags & COPYFLAG_QUIET))
    {
        PrintNotDone(cd->Flags & COPYFLAG_VERBOSE ? name : 0,
                     printerr, cd->Deep, cd->Fib.fib_DirEntryType > 0, cd);
    }
    else if (printok)
    {
        cd->Flags |= COPYFLAG_DONE;

        if (!(cd->Flags & COPYFLAG_QUIET))
        {
            if ((cd->Flags & COPYFLAG_VERBOSE))
            {
                Printf("%s\n", printok);
            }
        }

        SetData(cd->DestName, cd);
    }

    if (lock)
    {
        UnLock(lock);
    }
}


LONG CopyFile(BPTR from, BPTR to, ULONG bufsize, struct CopyData *cd)
{
    STRPTR buffer;
    LONG s, err = 0;

    if (cd->CopyBuf)
    {
        buffer  = cd->CopyBuf;
        bufsize = cd->CopyBufLen;
    }
    else
    {
        do
        {
            buffer = (STRPTR)AllocMem(bufsize, MEMF_PUBLIC);
            if (buffer)
            {
                cd->CopyBuf    = buffer;
                cd->CopyBufLen = bufsize;
                break;
            }

            bufsize >>= 1;

        } while (bufsize >= 512);
    }

    if (buffer)
    {
#if USE_BOGUSEOFWORKAROUND
        struct FileInfoBlock *fib = (struct FileInfoBlock *) buffer; /* NOTE: bufsize is min 512 bytes */

        if (ExamineFH(from, fib))
        {
#warning "****** WARNING: No largefile support! ******"
            ULONG filesize = fib->fib_Size, copied = 0;

            /*Printf("filesize: %lu\n", filesize);*/

            do
            {
                ULONG brk = CTRL_C;
                if (brk || (s = Read(from, buffer, bufsize)) == -1 || Write(to, buffer, s) != s)
                {
                    if (brk)
                    {
                        SetIoErr(ERROR_BREAK);
                    }
                    err = RETURN_FAIL;
                    break;
                }
                else if (s == 0 && copied < filesize)
                {
                    /* premature EOF with buggy fs */
                    err = RETURN_FAIL;
                    break;
                }

                copied += s;

            } while (copied < filesize);

            /*{ LONG ioerr = IoErr();
            Printf("copied %lu/%lu\n", copied, filesize);
            SetIoErr(ioerr);}*/
        }
        else
#endif /* USE_BOGUSEOFWORKAROUND */
        {
            /* Stream or so, copy until EOF or error */
            do
            {
                ULONG brk = CTRL_C;
                if (brk || (s = Read(from, buffer, bufsize)) == -1 || Write(to, buffer, s) != s)
                {
                    if (brk)
                    {
                        SetIoErr(ERROR_BREAK);
                    }
                    err = RETURN_FAIL;
                    break;
                }
            } while (s == bufsize);
        }

        /* Freed at exit to avoid fragmentation */
        /*FreeMem(buffer, bufsize);*/
    }
    else
    {
        err = RETURN_FAIL;
    }

    return err;
}


/* Softlink's path starts always with device name! f.e. "Ram Disk:T/..." */
LONG LinkFile(BPTR from, STRPTR to, ULONG soft, struct CopyData *cd)
{
    if (soft)
    {
        LONG ret = FALSE;
        UBYTE *name;

        name = AllocMem(FILEPATH_SIZE, MEMF_ANY);
        if (name)
        {
            if (NameFromLock(from, name, FILEPATH_SIZE))
            {
                ret = MakeLink(to, name, LINK_SOFT);
            }

            FreeMem(name, FILEPATH_SIZE);
        }

        return ret;
    }
    else
    {
        return MakeLink(to, from, LINK_HARD);
    }
}


/* return 0 means no loop, return != 0 means loop found */
ULONG TestLoop(BPTR srcdir, BPTR destdir, struct CopyData *cd)
{
    ULONG loop = 0;
    BPTR par, lock;
    
    lock = destdir;

    if (SameDevice(srcdir, destdir))
    {
        do
        {
            if (!SameLock(srcdir, lock))
            {
                loop = 1;
            }
            else
            {
                par = ParentDir(lock);

                if (lock != destdir)
                {
                    UnLock(lock);
                }

                lock = par;
            }
        }
        while(!loop && lock);
    }

    if (lock != destdir)
    {
        UnLock(lock);
    }

    return loop;
}


void SetData(STRPTR name, struct CopyData *cd)
{
    if (cd->Flags & COPYFLAG_NOPRO)
    {
        /* Is already set! - Piru */
        //SetProtection(name, 0);
    }
    else if (cd->Flags & COPYFLAG_PROTECTION)
    {
        SetProtection(name, cd->Fib.fib_Protection & (ULONG) ~FIBF_ARCHIVE);
    }

    if (cd->Flags & COPYFLAG_DATES)
    {
        SetFileDate(name, &cd->Fib.fib_Date);
    }

    if (cd->Flags & COPYFLAG_COMMENT)
    {
        SetComment(name, cd->Fib.fib_Comment);
    }
}


LONG TestDest(STRPTR name, ULONG type, struct CopyData *cd)
{
    LONG ret = TESTDEST_ERROR;
    BPTR lock;

    if ((lock = Lock(name, SHARED_LOCK)))
    {
        struct FileInfoBlock *fib;

        if ((fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
        {
            if (Examine(lock, fib))
            {
                UnLock(lock);
                lock = 0;

                if (type)
                {
                    if (fib->fib_DirEntryType > 0)
                    {
                        ret = TESTDEST_DIR_OK;
                    }
                    else if (!(cd->Flags & COPYFLAG_DONTOVERWRITE))
                    {
                        if (KillFile(name, cd->Flags & COPYFLAG_FORCEOVERWRITE, cd))
                        {
                            ret = TESTDEST_DELETED;
                        }
                    }
                    else
                    {
                        ret = TESTDEST_CANTDELETE;
                    }
                }
                else if (cd->Flags & COPYFLAG_DONTOVERWRITE)
                {
                    ret = TESTDEST_CANTDELETE;
                }
                else if (KillFile(name, cd->Flags & COPYFLAG_FORCEOVERWRITE, cd))
                {
                    ret = TESTDEST_DELETED;
                }
            }

            FreeDosObject(DOS_FIB, fib);
        }

        if (lock)
        {
            UnLock(lock);
        }
    }
    else
    {
        ret = TESTDEST_NONE;
    }

    if (ret == TESTDEST_CANTDELETE)
    {
        SetIoErr(ERROR_OBJECT_EXISTS);
    }

    return ret;
}

