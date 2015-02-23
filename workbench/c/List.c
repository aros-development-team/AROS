/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    List the contents of a directory.
*/
/*****************************************************************************

    NAME

        List

    FORMAT

        List [(dir | pattern | filename)] [ PAT (pattern)] [KEYS] [DATES]
             [NODATES] [TO (name)] [SUB (string)] [SINCE (date)] [UPTO (date)]
             [QUICK] [BLOCK] [NOHEAD] [FILES] [DIRS] [LFORMAT (string)] [ALL]

    TEMPLATE

        DIR/M,P=PAT/K,KEYS/S,DATES/S,NODATES/S,TO/K,SUB/K,SINCE/K,UPTO/K,QUICK/S,BLOCK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/K,ALL/S

    LOCATION

        C:

    FUNCTION

        Lists detailed information about the files and directories in the 
        current directory or in the directory specified by DIR.

        The information for each file or directory is presented on a separate 
        line, containing the following information:

        name
        size (in bytes)
        protection bits
        date and time

    INPUTS

        DIR           --  The directory to list. If left out, the current
                          directory will be listed.
        PAT           --  Display only files matching 'string'
        KEYS          --  Display the block number of each file or directory
        DATES         --  Always display the full modification date of files
                          and directories instead of a day name.
        NODATES       --  Don't display dates
        TO (name)     --  Write the listing to a file instead of stdout
        SUB (string)  --  Display only files, a substring of which matches
                          the substring 'string'
        SINCE (date)  --  Display only files newer than 'date'
        UPTO (date)   --  Display only files older than 'date'
        QUICK         --  Display only the names of files
        BLOCK         --  File sizes are in blocks of 512 bytes
        NOHEAD        --  Don't print any header information
        FILES         --  Display files only
        DIRS          --  Display directories only
        LFORMAT       --  Specify the list output in printf-style
        ALL           --  List the contents of directories recursively


        The following attributes of the LFORMAT strings are available

        %A  --  file attributes
        %B  --  size of file in blocks rather than bytes
        %C  --  file comment
        %D  --  modification date
        %E  --  file extension
        %F  --  absolute file path, with volume label
        %K  --  file key block number
        %L  --  size of file in bytes
        %M  --  file name without extension
        %N  --  file name
        %P  --  file path
        %S  --  superseded by %N and %P; obsolete
        %T  --  modification time


        Additionally, the following modifiers, each optional, can be used,
        in this order, following the % character:

        left-justify         --  minus sign
        field width minimum  --  value
        value width maximum  --  dot value

        Value width maximum is not available for all numeric fields.

    RESULT

        Standard DOS return codes.

    EXAMPLE

        1> List C:
        Directory "c:" on Wednesday 12/18/14:
        Assign                      6548 ---rwed Saturday    01:12:16
        Copy                       17772 ---rwed Saturday    01:12:24
        AddBuffers                  5268 ---rwed Saturday    01:14:46
        Avail                       8980 ---rwed Saturday    01:14:51
        Delete                      8756 ---rwed Saturday    01:14:59
        Install                    13024 ---rwed Saturday    01:15:09
        List                       20228 ---rwed Today       12:06:38
        Which                       7840 ---rwed Saturday    01:16:09
        8 file - 167 blocks used
        1>
        1> List C: lformat "[%10.5M] -- >-4b<"
        [     Assig] -- >13  <
        [      Copy] -- >35  <
        [     AddBu] -- >11  <
        [     Avail] -- >18  <
        [     Delet] -- >18  <
        [     Insta] -- >26  <
        [      List] -- >40  <
        [     Which] -- >16  <
        1> 

    BUGS

    SEE ALSO

        Dir

    INTERNALS

        Current lformat interpretation requires re-interpretation of the format for each entry.


******************************************************************************/

#define  DEBUG  0
#include <aros/debug.h>

#include <clib/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/datetime.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <utility/tagitem.h>

const TEXT version[] = "$VER: List 41.13 (18.12.2014)\n";

#define ARG_TEMPLATE "DIR/M,P=PAT/K,KEYS/S,DATES/S,NODATES/S,TO/K,SUB/K,SINCE/K,UPTO/K,QUICK/S,BLOCK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/K,ALL/S"

struct DirNode
{
    struct MinNode  node;
    char           *dirname;
};


typedef struct _Statistics
{
    ULONG nFiles;
    ULONG nDirs;
    UQUAD nBlocks;
} Statistics;


enum
{
    ARG_DIR = 0,
    ARG_PAT,
    ARG_KEYS,
    ARG_DATES,
    ARG_NODATES,
    ARG_TO,
    ARG_SUB,
    ARG_SINCE,
    ARG_UPTO,
    ARG_QUICK,
    ARG_BLOCK,
    ARG_NOHEAD,
    ARG_FILES,
    ARG_DIRS,
    ARG_LFORMAT,
    ARG_ALL,
    NOOFARGS
};

#define  MAX_PATH_LEN  1024

#define  BLOCKSIZE  512

#define  DIRTEXT   "Dir"
#define  EMPTYTEXT "empty"


/* UQUAD2string: Helper function to generate a decimal string representation for a UQUAD value.
   Arguments:    Value; a UBYTE buffer large enough to hold the string; length of the buffer.
   Returns:      Pointer to string inside the buffer. (String is end-aligned in the buffer!)
   Note:         Just a helper: Not safe with incorrect input!
*/

UBYTE *UQUAD2string( UQUAD value, UBYTE *buffer, int buflen)
{
    buffer[ --buflen] = '\0';

    do
    {
        buffer[ --buflen] = '0' + (value % 10);
        value /= 10;
    } while (value);

    return buffer +buflen;
}



int printDirHeader(STRPTR dirname, BOOL noHead)
{
    struct DateTime dt;

    char  datestr[LEN_DATSTRING];
    char  dow[LEN_DATSTRING];
    
    if (!noHead)
    {
        DateStamp((struct DateStamp *)&dt);
        dt.dat_Format = FORMAT_DEF;
        dt.dat_Flags = 0;
        dt.dat_StrDay = dow;
        dt.dat_StrDate = datestr;
        dt.dat_StrTime = NULL;
        DateToStr(&dt);
        
        Printf("Directory \"%s\" on %s %s:\n", dirname, dow, datestr);
    }

    return RETURN_OK;
}


/* Possible lformat switches

        %A  --  file attributes
        %B  --  size of file in blocks rather than bytes
        %C  --  file comment
        %D  --  file date
        %E  --  file extension
        %F  --  absolute file path, with volume label
        %K  --  file key block number
        %L  --  size of file in bytes
        %M  --  file name without extension
        %N  --  file name
        %P  --  file path
        %S  --  file name or file path
        %T  --  file time

%F used to be documented as just volume name. Though not Amiga-compatible, volume and path separate might indeed be useful.
*/

struct lfstruct
{
    struct AnchorPath *ap;
    BOOL               isdir;
    STRPTR             date;
    STRPTR             time;
    STRPTR             flags;
    STRPTR             filename;
    STRPTR             comment;
    UQUAD              size;
    ULONG              key;
};


#define  roundUp(x, bSize) ((x + bSize - 1)/bSize)


int printLformat(STRPTR format, struct lfstruct *lf)
{
    STRPTR filename       = FilePart(lf->filename);
    STRPTR temp           = format;
    LONG   substitutePath = 0;
    char   c, cu;
    UBYTE  fbuf[ 260]; // 256 plus 3 characters format string plus '\0'.
    int    fbufindex, dot;

    /*
        Whether the path or the filename is substituted for an occurrence
        of %S depends on how many occurrences are in the LFORMAT line, and
        their order, as follows:

        Occurrences of %S   1st         2nd         3rd         4th
        1                   filename
        2                   path        filename
        3                   path        filename    filename
        4                   path        filename    path        filename

        For 5 or more occurences: As with 4, with occurences beyond the 4th the filename.
    */

    while ( ( substitutePath < 4 ) && ( '\0' != (c = *temp++) ) )
    {
        if ( '%' == c )
            if ( 'S' == ToUpper(*temp++) )
                substitutePath++;
    }
    if ( substitutePath == 3 )
        substitutePath = 2;


    while ('\0' != (c = *format++))
    {
        if ('%' == c) // Character introducing a format switch in lformat.
        {
            /* Try for modifiers */
            fbufindex= 0;
            dot= 0;
            fbuf[ fbufindex++]= '%';  // Introducing a format type for PrintF.

            while ((c = *format++))
            {
                if      (c == '-')            // Left align
                {
                    fbufindex= 1;                  // Only the last one counts
                    dot= 0;                        // Reset max value width as well
                }
                else if (c == '.')            // Max value width.
                {
                    if (dot)
                    {
                        fbufindex= dot;            // Only the last one counts.
                    }
                    else
                    {
                        dot= fbufindex;            // Max value width starts after the dot
                    }
                }
                else if ( c < '0' || '9' < c) // It's not a digit either ==> end of modifiers
                {
                    break;
                }
                if (fbufindex < 256)          // Leave room for a three character format string plus \0.
                {
                    fbuf[ fbufindex++]= c;
                } // Squeezes out any overflow silently. Not a likely event, but is it acceptable? Desperado 20141217
            }


            /* Interpret argument */
            switch (cu= ToUpper(c))
            {
                /* File comment */
            case 'C':
                strcpy( fbuf +fbufindex, "s");
                D(bug("[List] rawFormat = [%s]", fbuf));
                Printf( fbuf, lf->comment);
                break;
                
                /* Modification date */
            case 'D':
                strcpy( fbuf +fbufindex, "s");
                D(bug("[List] rawFormat = [%s]", fbuf));
                Printf( fbuf, lf->date);
                break;
                
                /* Modification time */
            case 'T':
                strcpy( fbuf +fbufindex, "s");
                D(bug("[List] rawFormat = [%s]", fbuf));
                Printf( fbuf, lf->time);
                break;

                /* File size */
            case 'L':
                /* File size in blocks of BLOCKSIZE bytes */               
            case 'B':

                if (lf->isdir)
                {
                    strcpy( fbuf +fbufindex, "s");
                    D(bug("[List] rawFormat = [%s]", fbuf));
                    Printf( fbuf, DIRTEXT);
                }
                else
                {
                    UQUAD size= ( cu == 'B' ? roundUp(lf->size, BLOCKSIZE) : lf->size); // Blocks or bytes.

                    /* File has no content? */
                    if (size == 0)
                    {
                        strcpy( fbuf +fbufindex, "s");
                        D(bug("[List] rawFormat = [%s]", fbuf));
                        Printf( fbuf, EMPTYTEXT);
                    }
                    else
                    {
                        UBYTE buf[ 256]; // Should be UQUADSTRSIZE +1, but this will suffice.
                        UBYTE *quadstr= UQUAD2string( size, buf, 256);

                        strcpy( fbuf +fbufindex, "s"); // Should we implement a '%q' type?
                        D(bug("[List] rawFormat = [%s]", fbuf));
                        Printf( fbuf, quadstr);
                    } // Side effect of converting uquad to string: User can even maxsize numbers, if wanted.
                }

                break;

                /* Path incl. volume name*/
            case 'F':
                {
                    UBYTE buf[257]; // 256 + room for an extra '/'.
                    
                    if (NameFromLock(lf->ap->ap_Current->an_Lock, buf, 256))
                    {
                        int len = strlen(buf); // For checking the end of the string

                        if ((len > 0) && (buf[len - 1] != ':') && (buf[len - 1] != '/'))  // We need a separator:
                        {
                            strcpy( buf +len, "/");  // Add an /.
                        }

                        strcpy( fbuf +fbufindex, "s");
                        D(bug("[List] rawFormat = [%s]", fbuf));
                        Printf( fbuf, buf);
                    }
                }

                break;

                /* File attributes (flags) */
            case 'A':
                strcpy( fbuf +fbufindex, "s");
                D(bug("[List] rawFormat = [%s]", fbuf));
                Printf( fbuf, lf->flags);
                break;

                /* Disk block key */
            case 'K':
                strcpy( fbuf +fbufindex, "lu");
                D(bug("[List] rawFormat = [%s]", fbuf));
                Printf( fbuf, lf->key);
                break;
                
                /* File name without extension */
            case 'M':
                {
                    STRPTR lastPoint = strrchr(filename, '.');

                    if (lastPoint != NULL)
                    {
                        *lastPoint = 0;
                    }
                    
                    strcpy( fbuf +fbufindex, "s");
                    D(bug("[List] rawFormat = [%s]", fbuf));
                    Printf( fbuf, filename);

                    /* Resurrect filename in case we need to print it again */
                    if (lastPoint != NULL)
                    {
                        *lastPoint = '.';
                    }
                }

                break;
                
                /* Filename or Path name */
            case 'S':
                D(bug("[List] substitutePath = %d\n", substitutePath));
                if ( (--substitutePath == 3) || (substitutePath == 1) )
                {
                    STRPTR end = FilePart(lf->filename);
                    UBYTE  token = *end;

                    *end = '\0';

                    strcpy( fbuf +fbufindex, "s");
                    D(bug("[List] rawFormat = [%s]", fbuf));
                    Printf( fbuf, lf->filename);

                    /* Restore pathname */
                    *end = token;

                    break;
                }
                /* Fall through */
            case 'N':
                strcpy( fbuf +fbufindex, "s");
                D(bug("[List] rawFormat = [%s]", fbuf));
                Printf( fbuf,  filename);
                break;
                
                /* File extension */
            case 'E':
                {
                    STRPTR extension = strrchr(filename, '.');

                    if (extension != NULL)
                    {
                        strcpy( fbuf +fbufindex, "s");
                        D(bug("[List] rawFormat = [%s]", fbuf));
                        Printf( fbuf,  ++extension); // Skip the dot.
                    }
                }

                break;

                /* File path as specified */
            case 'P':
                {
                    STRPTR end = FilePart(lf->filename);
                    UBYTE  token = *end;
                    
                    *end = 0;

                    strcpy( fbuf +fbufindex, "s");
                    D(bug("[List] rawFormat = [%s]", fbuf));
                    Printf( fbuf,  lf->filename);
                    
                    /* Restore pathname */
                    *end = token;
                }

                break;

                /* Unexpected end of format */
            case 0:
                fbuf[ fbufindex]= '\0'; // Just add end to the string.
                Printf( "%s", fbuf);    // We never found the switch, so print as text.
                return 0;
                break;
                
                /* Unrecognised %-sequence */
            default:
                fbuf[ fbufindex]= '\0';   // Just add end to the string.
                Printf("%s%lc", fbuf, c); // Print interpreted format part as text.
                break;
            }
        }
        else
        {
            Printf("%lc", c);
        }
    }
    
    return 0;
}


int printFileData(struct AnchorPath *ap,
                  BOOL showFiles, BOOL showDirs, STRPTR parsedPattern,
                  ULONG *files, ULONG *dirs, ULONG *nBlocks, STRPTR lFormat,
                  BOOL quick, BOOL dates, BOOL noDates, BOOL block,
                  struct DateStamp *sinceDate, struct DateStamp *uptoDate,
                  BOOL doSince, BOOL doUpto, STRPTR subpatternStr,
                  BOOL keys)
{
    STRPTR             filename = ap->ap_Buf;
    BOOL               isDir = (ap->ap_Info.fib_DirEntryType >= 0);
    struct DateStamp  *ds = &ap->ap_Info.fib_Date;
    ULONG              protection = ap->ap_Info.fib_Protection;
    UQUAD              size = ap->ap_Info.fib_Size;
    STRPTR             filenote = ap->ap_Info.fib_Comment;
    ULONG              diskKey = ap->ap_Info.fib_DiskKey;
    
    int error = 0;

    UBYTE date[LEN_DATSTRING];
    UBYTE time[LEN_DATSTRING];
    UBYTE flags[8];

    struct DateTime dt;

#if defined(ACTION_GET_FILE_SIZE64)
    if (ap->ap_Info.fib_Size >= 0x7FFFFFFF)
    {
        BPTR flock = BNULL;
        flock = Lock(filename, ACCESS_READ);

        if (flock)
        {
            UQUAD *size_ptr = (UQUAD *)DoPkt(((struct FileLock *)flock)->fl_Task, ACTION_GET_FILE_SIZE64, (IPTR)flock, 0, 0, 0, 0);
            if (size_ptr)
            {
                size = *size_ptr;
            }
            UnLock(flock);
        }
    }
#endif
    /* Do the file match the time interval we are looking for?
       (ARG_SINCE and ARG_UPTO) -- any combination of these may be
       specified */
    if ((doSince && (CompareDates(sinceDate, ds) < 0)) ||
        (doUpto && (CompareDates(uptoDate, ds) > 0)))
    {
        return 0;
    }

    /* Does the filename match a certain pattern? (ARG_PAT) */
    if (parsedPattern != NULL &&
        !MatchPatternNoCase(parsedPattern, FilePart(filename)))
    {
        return 0;
    }
    
    /* Does a substring of the filename match a certain pattern? (ARG_SUB) */
    if (subpatternStr != NULL && 
        !MatchPatternNoCase(subpatternStr, FilePart(filename)))
    {
        return 0;
    }

    CopyMem(ds, &dt.dat_Stamp, sizeof(struct DateStamp));
    dt.dat_Format  = FORMAT_DOS;
    if (dates)
        dt.dat_Flags = 0;
    else
        dt.dat_Flags = DTF_SUBST;
    dt.dat_StrDay  = NULL;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt);

    /* Convert the protection bits to a string */
    // Active when set
    flags[0] = protection & FIBF_SCRIPT  ? 's' : '-';
    flags[1] = protection & FIBF_PURE    ? 'p' : '-';
    flags[2] = protection & FIBF_ARCHIVE ? 'a' : '-';

    // Active when unset!
    flags[3] = protection & FIBF_READ    ? '-' : 'r';
    flags[4] = protection & FIBF_WRITE   ? '-' : 'w';
    flags[5] = protection & FIBF_EXECUTE ? '-' : 'e';
    flags[6] = protection & FIBF_DELETE  ? '-' : 'd';
    flags[7] = 0x00;

    if (isDir)
    {
        if (showDirs)
        {
            ++*dirs;
            ++*nBlocks; /* dir entry uses 1 block on AROS, 2 on OS31) */

            if (lFormat != NULL)
            {
                struct lfstruct lf = { ap, isDir, date, time, flags, filename,
                                       filenote, size, diskKey};

                printLformat(lFormat, &lf);
                Printf("\n");
            }
            else
            {
                D(bug("Found file %s\n", filename));

                Printf("%-24s ", FilePart(filename)); // Entry name field width

                if (!quick)
                {
                    Printf("%7s %7s ", DIRTEXT, flags); // Size field width, flags field width
                }
                
                if (!noDates && (!quick || dates))
                {
                    Printf("%-11s %s", date, time); // Date field width
                }
                        
                Printf("\n");
            }
        }
    }
    else if (showFiles)
    {
        ++*files;
        *nBlocks += roundUp(size, BLOCKSIZE);

        if (lFormat != NULL)
        {
            struct lfstruct lf = { ap, isDir, date, time, flags, filename,
                                   filenote, size, diskKey };

            printLformat(lFormat, &lf);
            Printf("\n");
        }
        else
        {
            Printf("%-24s ", FilePart(filename)); // Entryname field width

            if (!quick)
            {
                if(keys)
                {
                    char key[16];

                    __sprintf(key, "%lu", (unsigned long)diskKey);

                    Printf("[%5lu] ", diskKey); // Key field width
                }
                else
                {
                    if (0 != size)
                    {
                        UQUAD filesize = block ? roundUp(size, BLOCKSIZE) : size;

                        UBYTE buf[ 256]; // Should be UQUADSTRSIZE +1, but this will suffice.
                        UBYTE *quadstr= UQUAD2string( filesize, buf, 256);

                        Printf("%7s ", quadstr); // Size field width
                    }
                    else
                    {
                        Printf("%7s ", EMPTYTEXT); // Size field width
                    }
                }
                
                Printf("%7s ", flags); // Flags field width
            }
            
            if (!noDates && (!quick || dates))
            {
                Printf("%-11s %s", date, time); // Date field width
            }

            if (!quick && (*filenote != 0))
            {
                Printf("\n: %s", filenote);
            }
            
            Printf("\n");
        }
    }
    
    return error;
}


/* Print directory summary information */
void printSummary(CONST_STRPTR dirname, int files, int dirs, int nBlocks,
                    BOOL noHead, BOOL PrintEmpty)
{

    if (noHead) return;

    if (files || dirs)
    {

        if (files > 1)
            Printf("%ld files", files);
        else if (files > 0)
            PutStr("1 file");
        
        if( files && (dirs || nBlocks) ) PutStr(" - ");

        if (dirs > 1)
            Printf("%ld directories", dirs);
        else if (dirs > 0)
            PutStr("1 directory");

        if( dirs && nBlocks ) PutStr(" - ");

        if (nBlocks > 1)
            Printf("%ld blocks used\n", nBlocks);
        else if (nBlocks > 0)
            PutStr("1 block used\n");
        else PutStr("\n");

    }
    else if (PrintEmpty)
        Printf("Directory \"%s\" is empty\n", dirname);
}


int listFile(CONST_STRPTR filename, BOOL showFiles, BOOL showDirs,
             STRPTR parsedPattern, BOOL noHead, STRPTR lFormat, BOOL quick,
             BOOL dates, BOOL noDates, BOOL block, struct DateStamp *sinceDate,
             struct DateStamp *uptoDate, BOOL doSince, BOOL doUpto,
             STRPTR subpatternStr, BOOL all, BOOL keys, Statistics *stats)
{
    struct AnchorPath *ap;
    struct List DirList, FreeDirNodeList;
    struct DirNode *dirnode, *prev_dirnode = NULL;
    
    ULONG  files = 0;
    ULONG  dirs = 0;
    ULONG  nBlocks = 0;
    LONG  error;

    NewList(&DirList);
    NewList(&FreeDirNodeList);
     
    do 
    {
        ap = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN, MEMF_CLEAR);

        if (ap == NULL)
        {
            return 0;
        }

        ap->ap_Strlen = MAX_PATH_LEN;

        error = MatchFirst(filename, ap);

        /* Explicitly named directory and not a pattern? --> enter dir */

        if (0 == error)
        {
            if (!(ap->ap_Flags & APF_ITSWILD))
            {
                if (ap->ap_Info.fib_DirEntryType >= 0)
                {
                    //error = printDirHeader(filename, noHead);
                    ap->ap_Flags |= APF_DODIR;

                    if (0 == error)
                    {
                        error = MatchNext(ap);
                    }
                }
            }    
        }

        if (0 == error)
        {
            BOOL first = TRUE;

            ap->ap_BreakBits = SIGBREAKF_CTRL_C;
            if (FilePart(ap->ap_Buf) == ap->ap_Buf)
            {
                ap->ap_Flags &= ~APF_DirChanged;
            }
            
            do
            {
                /*
                ** There's something to show.
                */
                if (!(ap->ap_Flags & APF_DIDDIR))
                {
                    if (ap->ap_Flags & APF_DirChanged)
                    {
                        STRPTR p;
                        UBYTE c;

                        if (!first) printSummary(filename, files, dirs, nBlocks, noHead, TRUE);

                        /* Update global statistics for (possible) ALL option */
                        stats->nFiles += files;
                        stats->nDirs += dirs;
                        stats->nBlocks += nBlocks;

                        files = 0;
                        dirs = 0;
                        nBlocks = 0;

                        p = PathPart(ap->ap_Buf);
                        c = *p;
                        *p = 0;

                        error = printDirHeader(ap->ap_Buf, noHead);

                        *p = c;

                    }

                    error = printFileData(ap,
                                          showFiles,
                                          showDirs,
                                          parsedPattern,
                                          &files,
                                          &dirs,
                                          &nBlocks,
                                          lFormat,
                                          quick,
                                          dates,
                                          noDates,
                                          block,
                                          sinceDate,
                                          uptoDate,
                                          doSince,
                                          doUpto,
                                          subpatternStr,
                                          keys);
                
                    if (all && (ap->ap_Info.fib_DirEntryType >= 0))
                    {
                        if ((dirnode = AllocMem(sizeof(struct DirNode), MEMF_ANY)))
                        {
                            if ((dirnode->dirname = StrDup(ap->ap_Buf)))
                            {
                                Insert(&DirList, (struct Node *)dirnode,
                                       (struct Node *)prev_dirnode);
                                
                                prev_dirnode = dirnode;
                            }
                            else
                            {
                                FreeMem(dirnode, sizeof(struct DirNode));
                            }
                        }
                    }
                }

                error = MatchNext(ap);
                
                first = FALSE;

            } while (0 == error);
        }

        MatchEnd(ap);

        FreeVec(ap);
        
        if (error == ERROR_BREAK)
        {
            PrintFault(error, NULL);
        }

        if (error == ERROR_NO_MORE_ENTRIES)
        {
            error = 0;
        }

        if ((error == 0) || (error == ERROR_BREAK))
        {
            BOOL printEmpty = !(ap->ap_Flags & APF_ITSWILD);
            printSummary(filename, files, dirs, nBlocks, noHead, printEmpty);
        }

        /* Update global statistics for (possible) ALL option */
        stats->nFiles += files;
        stats->nDirs += dirs;
        stats->nBlocks += nBlocks;

        files = 0;
        dirs = 0;
        nBlocks = 0;


        if (error) break;
        
        dirnode = (struct DirNode *)RemHead(&DirList);

        if (dirnode != NULL)
        {
            filename = dirnode->dirname;
            
            prev_dirnode = NULL;
            
            /* do not free() dirnode, as we reference dirnode->dirname! */
            
            AddTail(&FreeDirNodeList, (struct Node *)dirnode);
        }
    } while (dirnode);
        
    while ((dirnode = (struct DirNode *)RemHead(&FreeDirNodeList)))
    {
        FreeVec(dirnode->dirname);
        FreeMem(dirnode, sizeof(struct DirNode));
    }
    
    return error;
}


int __nocommandline;

int main(void)
{
    IPTR args[NOOFARGS] =
    {
        (IPTR) NULL,    // ARG_DIR
        (IPTR) NULL,    // ARG_PAT
               FALSE,   // ARG_KEYS
               FALSE,   // ARG_DATES
               FALSE,   // ARG_NODATES
        (IPTR) NULL,    // ARG_TO
        (IPTR) NULL,    // ARG_SUB
        (IPTR) NULL,    // ARG_SINCE
        (IPTR) NULL,    // ARG_UPTO
               FALSE,   // ARG_QUICK
               FALSE,   // ARG_BLOCK
               FALSE,   // ARG_NOHEAD
               FALSE,   // ARG_FILES
               FALSE,   // ARG_DIRS
               FALSE,   // ARG_LFORMAT
               FALSE    // ARG_ALL
    };
    static CONST_STRPTR default_directories[] = {(CONST_STRPTR)"", 0};
    struct RDArgs *rda;       

    LONG     result = RETURN_OK;
    LONG     error = 0;
    STRPTR   parsedPattern = NULL;
    STRPTR   subpatternStr = NULL;
    BPTR     oldOutput = BNULL;

    Statistics stats = { 0, 0, 0 };

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if (rda != NULL)
    {
        CONST_STRPTR *directories = (CONST_STRPTR *)args[ARG_DIR];
        STRPTR  lFormat = (STRPTR)args[ARG_LFORMAT];
        STRPTR  pattern = (STRPTR)args[ARG_PAT];
        STRPTR  toFile = (STRPTR)args[ARG_TO];
        STRPTR  subStr = (STRPTR)args[ARG_SUB];
        STRPTR  since = (STRPTR)args[ARG_SINCE];
        STRPTR  upto = (STRPTR)args[ARG_UPTO];
        BOOL    files = (BOOL)args[ARG_FILES];
        BOOL    dirs  = (BOOL)args[ARG_DIRS];
        BOOL    noDates = (BOOL)args[ARG_NODATES];
        BOOL    dates = (BOOL)args[ARG_DATES];
        BOOL    quick = (BOOL)args[ARG_QUICK];
        BOOL    noHead = (BOOL)args[ARG_NOHEAD];
        BOOL    block = (BOOL)args[ARG_BLOCK];
        BOOL    all = (BOOL)args[ARG_ALL];
        BOOL    keys = (BOOL)args[ARG_KEYS];

        struct DateTime  sinceDatetime;
        struct DateTime  uptoDatetime;

        ULONG   i;              /* Loop variable */    

        if (since != NULL)
        {
            sinceDatetime.dat_StrDate = since;
            sinceDatetime.dat_StrTime = NULL;
            sinceDatetime.dat_Format = FORMAT_DEF;
            sinceDatetime.dat_Flags = 0;
            if (StrToDate(&sinceDatetime) == DOSFALSE)
            {
                FreeArgs(rda);
                Printf("*** Illegal 'SINCE' parameter\n");

                return RETURN_FAIL;
            }
            sinceDatetime.dat_Stamp.ds_Minute = 0;
            sinceDatetime.dat_Stamp.ds_Tick = 0;
        }

        if (upto != NULL)
        {
            uptoDatetime.dat_StrDate = upto;
            uptoDatetime.dat_StrTime = NULL;
            uptoDatetime.dat_Format = FORMAT_DEF;
            uptoDatetime.dat_Flags = 0;

            if (StrToDate(&uptoDatetime) == DOSFALSE)
            {
                FreeArgs(rda);
                Printf("*** Illegal 'UPTO' parameter\n");

                return RETURN_FAIL;
            }
            uptoDatetime.dat_Stamp.ds_Minute = 1439;
            uptoDatetime.dat_Stamp.ds_Tick = 2999;
        }

        if (subStr != NULL)
        {
            STRPTR  subStrWithPat;
            ULONG   length = (strlen(subStr) + sizeof("#?#?"))*2 + 2;

            subStrWithPat = AllocVec(length, MEMF_ANY);

            if (subStrWithPat == NULL)
            {
                error = IoErr();
                FreeArgs(rda);
                PrintFault(error, "List");

                return RETURN_FAIL;
            }

            strcpy(subStrWithPat, "#?");
            strcat(subStrWithPat, subStr);
            strcat(subStrWithPat, "#?");

            subpatternStr = AllocVec(length, MEMF_ANY);

            if (subpatternStr == NULL || 
                ParsePatternNoCase(subStrWithPat, subpatternStr, length) == -1)
            {
                error = IoErr();
                FreeVec(subStrWithPat);
                FreeArgs(rda);
                PrintFault(error, "List");
                
                return RETURN_FAIL;
            }

            FreeVec(subStrWithPat);
        }


        if (pattern != NULL)
        {
            ULONG length = strlen(pattern)*2 + 2;

            parsedPattern = AllocVec(length, MEMF_ANY);

            if (parsedPattern == NULL ||
                ParsePatternNoCase(pattern, parsedPattern, length) == -1)
            {
                FreeVec(subpatternStr);
                FreeArgs(rda);

                return RETURN_FAIL;
            }
        }

        if (toFile != NULL)
        {
            BPTR file = Open(toFile, MODE_NEWFILE);

            if (file == BNULL)
            {
                error = IoErr();
                FreeVec(subpatternStr);
                FreeVec(parsedPattern);
                FreeArgs(rda);
                PrintFault(error, "List");

                return RETURN_FAIL;
            }
            oldOutput = SelectOutput(file);
        }

        if (!files && !dirs)
        {
            files = TRUE;
            dirs = TRUE;
        }

/*      if (!dates && !noDates)
        {
            dates = TRUE;
        }*/

        if (lFormat)
        {
            noHead = TRUE;
        }
        
        if ((directories == NULL) || (*directories == NULL))
        {
            directories = default_directories;
        }
        
        for (i = 0; directories[i] != NULL; i++)
        {
            error = listFile(directories[i], files, dirs, parsedPattern,
                             noHead, lFormat, quick, dates, noDates,
                             block, &sinceDatetime.dat_Stamp,
                             &uptoDatetime.dat_Stamp, since != NULL,
                             upto != NULL, subpatternStr, all, keys,
                             &stats);

            if (error != 0)
            {
                break;
            }

//          Printf("\n");
        } 
        
        FreeArgs(rda);
    } 
    else
    {
        error = IoErr();
    }

    if ((BOOL)args[ARG_NOHEAD] == FALSE &&
        (BOOL)args[ARG_LFORMAT] == FALSE &&
        (BOOL)args[ARG_ALL] &&
        (stats.nFiles || stats.nDirs))
    {
        Printf("\nTOTAL: %ld files - %ld directories - %ld blocks used\n",
               stats.nFiles, stats.nDirs, stats.nBlocks);
    }


    if (error != 0)
    {
        if (error == ERROR_BREAK)
        {
            result = RETURN_WARN;
        }
        else
        {
            PrintFault(error, "List");
            result = RETURN_FAIL;
        }
    }
    
    if (parsedPattern != NULL)
    {
        FreeVec(parsedPattern);
    }

    if (subpatternStr != NULL)
    {
        FreeVec(subpatternStr);
    }

    if (oldOutput != BNULL)
    {
        Close(SelectOutput(oldOutput));
    }
    
    return result;
}
