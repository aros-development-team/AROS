/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
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

	DIR/M,P=PAT/K,DATES/S,NODATES/S,TO/K,SUB/K,SINCE/K,UPTO/K,QUICK/S,BLOCK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/K,ALL/S

    LOCATION

        Workbench:C/
	   
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
        DATES         --  Display the creation date of files and directories
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
	%D  --  creation date
	%E  --  file extension
	%F  --  volume name
	%K  --  file key block number
	%L  --  size of file in bytes
	%M  --  file name without extension
	%N  --  file name
	%P  --  file path
	%S  --  superceded by %N and %P; obsolete
	%T  --  creation time
	
    RESULT

        Standard DOS return codes.

    EXAMPLE

        1> List C:
        Directory "C:" on Wednesday 12-Dec-99
        AddBuffers                  444 --p-rwed 02-Sep-99 11:51:31
        Assign                     3220 --p-rwed 02-Sep-99 11:51:31
        Avail                       728 --p-rwed 02-Sep-99 11:51:31
        Copy                       3652 --p-rwed 02-Sep-99 11:51:31
        Delete                     1972 --p-rwed 02-Sep-99 11:51:31
        Execute                    4432 --p-rwed 02-Sep-99 11:51:31
        List                       5108 --p-rwed 02-Sep-99 11:51:31
        Installer                109956 ----rwed 02-Sep-99 11:51:31
        Which                      1068 --p-rwed 02-Sep-99 11:51:31
        9 files - 274 blocks used        
        
    BUGS

    SEE ALSO

        Dir

    INTERNALS

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
#include <utility/tagitem.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <memory.h>

static const char version[] = "$VER: list 41.5 (3.12.2000)\n";

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
    ULONG nBlocks;
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


/* Possible printf-type switches

	%A  --  file attributes
	%B  --  size of file in blocks rather than bytes
	%C  --  file comment
	%D  --  file date
	%E  --  file extension
	%F  --  volume name
	%K  --  file key block number
	%L  --  size of file in bytes
	%M  --  file name without extension
	%N  --  file name
	%P  --  file path
	%S  --  the same as %N
	%T  --  creation date
*/

struct lfstruct
{
    BOOL    isdir;
    STRPTR  date;
    STRPTR  time;
    STRPTR  flags;
    STRPTR  filename;
    STRPTR  comment;
    ULONG   size;
    ULONG   key;
};


#define  roundUp(x, bSize) ((x + bSize - 1)/bSize)

int printLformat(STRPTR format, struct lfstruct *lf)
{
    STRPTR filename = FilePart(lf->filename);
    char c;
    
    while ('\0' != (c = *format++))
    {
	if ('%' == c)
	{
	    switch (toupper(*format++))
	    {
		/* File comment */
	    case 'C':
		Printf(lf->comment);
		break;
		
		/* Creation date */
	    case 'D':
		Printf(lf->date);
		break;
		
		/* Creation time */
	    case 'T':
		Printf(lf->time);
		break;
		
		/* File size in blocks of BLOCKSIZE bytes */
	    case 'B':
		if (lf->isdir)
		{
		    Printf("Dir");
		}
		else
		{
		    ULONG tmp = roundUp(lf->size, BLOCKSIZE);
		
		    /* File is 0 bytes? */
		    if (tmp == 0)
		    {
			Printf("empty");
		    }
		    else
		    {
			Printf("%lu", tmp);
		    }		    
		}

		break;

		/* Path incl. volume name*/
	    case 'F':
		{
		    STRPTR pEnd = FilePart(lf->filename);
		    UBYTE  token;
		    
		    token = pEnd[0];
		    pEnd[0] = 0;
		    
		    Printf(lf->filename);

		    /* Restore filename */
		    pEnd[0] = token;

		}

		break;

		/* File attributes (flags) */
	    case 'A':
		Printf(lf->flags);
		break;

		/* Disk block key */
	    case 'K':
		Printf("[%ld]", lf->key);
		break;
		
		/* File size */
	    case 'L':
		if (lf->isdir)
		{
		    Printf("Dir");
		}
		else
		{
		    if (lf->size == 0)
		    {
			Printf("empty");
		    }
		    else
		    {
			Printf("%lu", lf->size);
		    }
		}

		break;
		
		/* File name without extension */
	    case 'M':
		{
		    STRPTR lastPoint = strrchr(filename, '.');

		    if (lastPoint != NULL)
		    {
			*lastPoint = 0;
		    }
		    
		    Printf(filename);

		    /* Resurrect filename in case we should print it once
		       more */
		    if (lastPoint != NULL)
		    {
			*lastPoint = '.';
		    }
		}

		break;
		
		/* Filename */
	    case 'S':
		/* Fall through */
	    case 'N':
		Printf(filename);
		break;
		
		/* File extension */
	    case 'E':
		{
		    STRPTR extension = strrchr(filename, '.');

		    if (extension != NULL)
		    {
			Printf(extension);
		    }
		}

		break;

    	    	/* Path name, but without volume */
	    case 'P':
		{
		    STRPTR end = FilePart(lf->filename);
		    STRPTR start = strchr(lf->filename, ':');
		    UBYTE  token = *end;
		    
		    *end = 0;

		    if (start == NULL)
		    {
			Printf(lf->filename);
		    }
		    else
		    {
			Printf(start + 1);
		    }
		    
		    /* Restore pathname */
		    *end = token;
		}

		break;

	    case 0:
		return 0;
		break;
		
	    default:
		Printf("%%%c", *format);
		break;
	    }
	}
	else
	{
	    Printf("%c", c);
	}
    }
    
    return 0;
}


int printFileData(STRPTR filename, BOOL isDir, struct DateStamp *ds, 
                  ULONG protection, ULONG size, STRPTR filenote, LONG diskKey,
                  BOOL showFiles, BOOL showDirs, STRPTR parsedPattern,
                  ULONG *files, ULONG *dirs, ULONG *nBlocks, STRPTR lFormat,
		  BOOL quick, BOOL dates, BOOL noDates, BOOL block,
		  struct DateStamp *sinceDate, struct DateStamp *uptoDate,
		  BOOL doSince, BOOL doUpto, STRPTR subpatternStr,
		  BOOL keys)
{
    int error = RETURN_OK;

    UBYTE date[LEN_DATSTRING];
    UBYTE time[LEN_DATSTRING];
    UBYTE flags[8];

    struct DateTime dt;

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
    dt.dat_Flags   = DTF_SUBST;
    dt.dat_StrDay  = NULL;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt); /* returns 0 if invalid */

    /* Convert the protection bits to a string */
    flags[0] = protection & FIBF_SCRIPT  ? 's' : '-';
    flags[1] = protection & FIBF_PURE    ? 'p' : '-';
    flags[2] = protection & FIBF_ARCHIVE ? 'a' : '-';

    /* The following flags are high-active! */
    flags[3] = protection & FIBF_READ    ? '-' : 'r';
    flags[4] = protection & FIBF_WRITE   ? '-' : 'w';
    flags[5] = protection & FIBF_EXECUTE ? '-' : 'e';
    flags[6] = protection & FIBF_DELETE  ? '-' : 'd';
    flags[7] = 0x00;

    if (isDir)
    {
	if (showDirs)
	{
      	    if (lFormat != NULL)
	    {
		struct lfstruct lf = { isDir, date, time, flags, filename,
				       filenote, size, diskKey};

		printLformat(lFormat, &lf);
		Printf("\n");
		
		*dirs += 1;
	    }
	    else
	    {
		D(bug("Found file %s\n", filename));

		Printf("%-25s ", FilePart(filename));

		if (!quick)
		{
		    Printf("  <Dir> %7s ", flags);
		}
		
		if (!noDates && (!quick || dates))
		{
		    Printf("%-11s %s", date, time);
		}
			
		Printf("\n");
		
		*dirs += 1;
	    }
	}
    }
    else if (showFiles)
    {
	if (lFormat != NULL)
        {
	    struct lfstruct lf = { isDir, date, time, flags, filename,
				   filenote, size, diskKey };

	    printLformat(lFormat, &lf);
	    Printf("\n");
	    
	    *files += 1;
	    *nBlocks += roundUp(size, BLOCKSIZE);
        }
        else
        {
	    Printf("%-25s ", FilePart(filename));

	    if (!quick)
	    {
		if(keys)
		{
		    char key[16];
		    int  fill;
		    int  i;	/* Loop variable */

		    sprintf(key, "%ld", diskKey);
		    fill = 7 - strlen(key) - 2;

		    for (i = 0; i < fill; i++)
		    {
			Printf(" ");
		    }

		    Printf("[%ld] ", diskKey);
		}
		else
		{
		    if (0 != size)
		    {
			Printf("%7ld ", 
			       block ? roundUp(size, BLOCKSIZE) : size);
		    }
		    else
		    {
			Printf("  empty ");
		    }
		}
		
		Printf("%7s ", flags);
	    }
	    
	    if (!noDates && (!quick || dates))
	    {
		Printf("%-11s %s", date, time);
	    }

	    if (!quick && (*filenote != 0))
	    {
		Printf("\n: %s", filenote);
	    }
	    
	    Printf("\n");
	    *files  += 1;
	    *nBlocks += roundUp(size, BLOCKSIZE);
        }
    }
    
    return error;
}


/* Print directory summary information */
void printSummary(int files, int dirs, int nBlocks, BOOL noHead)
{
    if (noHead)
    {
	return;
    }

    if ((files == 0) && (dirs == 0))
    {
	Printf("Directory is empty\n");
    }
    else
    {
	if (files != 0)
	{
	    Printf("%ld files - ", files);
	}
	
	if (dirs != 0)
	{
	    Printf("%ld directories - ", dirs);
	}
	
	Printf("%ld bytes used\n", nBlocks);
    }
}


int listFile(STRPTR filename, BOOL showFiles, BOOL showDirs,
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
    ULONG  error;

    NewList(&DirList);
    NewList(&FreeDirNodeList);
     
    do 
    {
	ap = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN, MEMF_CLEAR);

	if (ap == NULL)
	{
	    return RETURN_ERROR;
	}

	ap->ap_Strlen = MAX_PATH_LEN;

	error = MatchFirst(filename, ap);

	/* Explicitely named directory and not a pattern? --> enter dir */

	if (0 == error)
	{
	    if (!(ap->ap_Flags & APF_ITSWILD))
	    {
		if (ap->ap_Info.fib_DirEntryType >= 0)
		{
		    error = printDirHeader(filename, noHead);
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
    	    ap->ap_BreakBits = SIGBREAKF_CTRL_C;

	    do
	    {
		/*
		** There's something to show.
		*/
		if (!(ap->ap_Flags & APF_DIDDIR))
		{
		    error = printFileData(ap->ap_Buf,
					  ap->ap_Info.fib_DirEntryType >= 0,
					  &ap->ap_Info.fib_Date,
					  ap->ap_Info.fib_Protection,
					  ap->ap_Info.fib_Size,
					  ap->ap_Info.fib_Comment,
					  ap->ap_Info.fib_DiskKey,
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
		    	if ((dirnode = malloc(sizeof(struct DirNode))))
			{
			    if ((dirnode->dirname = strdup(ap->ap_Buf)))
			    {
			    	Insert(&DirList, (struct Node *)dirnode,
				       (struct Node *)prev_dirnode);
				
				prev_dirnode = dirnode;
			    }
			    else
			    {
			    	free(dirnode);
			    }
			}
		    }
		}

		error = MatchNext(ap);

	    } while (0 == error);
	}

	MatchEnd(ap);

	FreeVec(ap);
	
	if (error == ERROR_BREAK)
	{
	    PrintFault(error, NULL);
	}

	printSummary(files, dirs, nBlocks, noHead);

	/* Update global statistics for (possiblr) ALL option */
	stats->nFiles += files;
	stats->nDirs += dirs;
	stats->nBlocks += nBlocks;

	files = 0;
	dirs = 0;
	nBlocks = 0;

	if (error == ERROR_NO_MORE_ENTRIES)
	{
	    error = 0;
	}

    	if (error) break;
	
    	dirnode = (struct DirNode *)RemHead(&DirList);

	if (dirnode != NULL)
	{
	    filename = dirnode->dirname;
	    puts("");
	    
	    prev_dirnode = NULL;
	    
	    /* do not free() dirnode, as we reference dirnode->dirname! */
	    
	    AddTail(&FreeDirNodeList, (struct Node *)dirnode);
	}
    } while (dirnode);
        
    while ((dirnode = (struct DirNode *)RemHead(&FreeDirNodeList)))
    {
    	free(dirnode->dirname);
	free(dirnode);
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
        
    struct RDArgs *rda;		       

    LONG     error = RETURN_OK;
    STRPTR   parsedPattern = NULL;
    STRPTR   subpatternStr = NULL;
    BPTR     oldOutput = NULL;

    Statistics stats = { 0, 0, 0 };

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    if (rda != NULL)
    {
	STRPTR *directories = (STRPTR *)args[ARG_DIR];
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

	ULONG   i;		/* Loop variable */	    

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
	}

	if (subStr != NULL)
	{
	    STRPTR  subStrWithPat;
	    ULONG   length = (strlen(subStr) + sizeof("#?#?"))*2 + 2;

	    subStrWithPat = AllocVec(length, MEMF_ANY);

	    if (subStrWithPat == NULL)
	    {
		FreeArgs(rda);
		PrintFault(IoErr(), "List");

		return RETURN_FAIL;
	    }

	    strcpy(subStrWithPat, "#?");
	    strcat(subStrWithPat, subStr);
	    strcat(subStrWithPat, "#?");

	    subpatternStr = AllocVec(length, MEMF_ANY);

	    if (subpatternStr == NULL || 
		ParsePatternNoCase(subStrWithPat, subpatternStr, length) == -1)
	    {
		FreeVec(subStrWithPat);
		FreeArgs(rda);
		PrintFault(IoErr(), "List");
		
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

	    if (file == NULL)
	    {
		FreeVec(subpatternStr);
		FreeVec(parsedPattern);
		FreeArgs(rda);
		PrintFault(IoErr(), "List");

		return RETURN_FAIL;
	    }
	    oldOutput = SelectOutput(file);
	}

	if (!files && !dirs)
	{
	    files = TRUE;
	    dirs = TRUE;
	}

	if (!dates && !noDates)
	{
	    dates = TRUE;
	}

	if ((directories == NULL) || (*directories == NULL))
	{
	    /* No file to list given. Just list the current directory */
	    error = listFile("#?", files, dirs, parsedPattern, noHead,
			     lFormat, quick, dates, noDates, block,
			     &sinceDatetime.dat_Stamp, &uptoDatetime.dat_Stamp,
			     since != NULL, upto != NULL, subpatternStr, all,
			     keys, &stats);
	}
	else
	{
	    for (i = 0; directories[i] != NULL; i++)
	    {
		error = listFile(directories[i], files, dirs, parsedPattern,
				 noHead, lFormat, quick, dates, noDates,
				 block, &sinceDatetime.dat_Stamp,
				 &uptoDatetime.dat_Stamp, since != NULL,
				 upto != NULL, subpatternStr, all, keys,
				 &stats);
		
		if (error != RETURN_OK)
		{
		    break;
		}
		    
//		Printf("\n");
	    } 
	}
	
	FreeArgs(rda);
    } 
    else
    {
	error = IoErr();;
    }

    if ((BOOL)args[ARG_ALL])
    {
	Printf("\nTOTAL: %ld files - %ld directories - %ld blocks used\n",
	       stats.nFiles, stats.nDirs, stats.nBlocks);
    }


    if (error != RETURN_OK)
    {
	if (error == ERROR_BREAK)
	{
	    error = RETURN_WARN;
	}
	else
	{
	    PrintFault(error, "List");
	    error = RETURN_FAIL;
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

    if (oldOutput != NULL)
    {
	Close(SelectOutput(oldOutput));
    }
    
    return error;
}
