/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Join - Create a single file from several.
*/

/******************************************************************************


    NAME

    Join [FILE] {(file | pattern)} AS|TO (filename)

    SYNOPSIS

    FILE/M/A,AS=TO/K/A

    LOCATION

    Workbench:C

    FUNCTION

    Join makes one big file of all listed files by putting them together
    in the order given. The destination file may not have the same name 
    as any of input files. You must supply a destination file name. The
    original files remian unchanged. Any number of files can be Join:ed in
    one operation.

    INPUTS

    FILE   --  files to join
    TO=AS  --  the name of the combined file

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#define  DEBUG  0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosasl.h>

#include <proto/exec.h>
#include <proto/dos.h>

#define ARG_TEMPLATE "FILE/M/A,AS=TO/K/A"
#define ARG_FILE     0
#define ARG_AS       1
#define ARG_COUNT    2

#define BUFFERSIZE   32768    /* Buffersize used when reading & writing */

/****** StringID's for getstring() **************************************/

#define STR_ABORTED       1   /* Aborted                                */
#define STR_REMOVINGDEST  2   /* Removing destination-file              */
#define STR_ERR_OPENREAD  3   /* Error opening file for reading         */
#define STR_ERR_NOMEM     4   /* Error allocating memory                */
#define STR_ERR_WRITING   5   /* Error while writing to file            */

/****** Version- and error-header ***************************************/

static const char version[] = "$VER: Join 41.1 (07.09.1999)";
static char ERROR_HEADER[]  = "Join";

/****** Prototypes for local functions **********************************/

LONG   append( BPTR destfile , STRPTR srcfilename );
STRPTR getstring( LONG stringid );
int doJoin(STRPTR *files, BPTR destfile);

/****** Functions *******************************************************/

int main( void )
{
    struct RDArgs  *rda = NULL ;

    IPTR    args[ARG_COUNT] = { (IPTR) NULL , (IPTR) NULL };
    STRPTR *files;
    STRPTR  destination;
    BPTR    destfile = NULL;
    LONG    rc = RETURN_OK;

    if( (rda = ReadArgs( ARG_TEMPLATE , args , NULL )) )
    {
	if( args[ARG_FILE] && args[ARG_AS] )
	{
	    destination = (STRPTR)args[ARG_AS];
	    files = (STRPTR *)args[ARG_FILE];
	    
	    if( (destfile = Open( destination , MODE_NEWFILE )) )
	    {
		rc = doJoin(files, destfile);

		if (rc == RETURN_OK)
		{
		    Close(destfile);
		}
		else
		{
		    Printf(", %s.\n", getstring(STR_REMOVINGDEST));
		    DeleteFile(destination);
		}
	    }
	    else
	    {
		PrintFault(IoErr() , ERROR_HEADER);
		rc = RETURN_FAIL;
	    }
	}
	else
	{
	    rc = RETURN_FAIL;
	}

	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), ERROR_HEADER);
	rc = RETURN_FAIL;
    }

    return rc;
}


#define  MAX_PATH_LEN  512


int doJoin(STRPTR *files, BPTR destfile)
{
    struct AnchorPath *ap;

    LONG  i;			/* Loop variable over patterns */
    LONG  match;		/* Loop variable over files */
    LONG  rc = RETURN_OK;
    
    ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) +
				       MAX_PATH_LEN, MEMF_CLEAR);

    if (ap == NULL)
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return RETURN_FAIL;
    }

    ap->ap_Strlen = MAX_PATH_LEN;

    /* Loop over the arguments */
    for (i = 0; files[i] != NULL; i++)
    {
	for (match = MatchFirst(files[i], ap); match == 0;
	     match = MatchNext(ap))
	{	
	    if(append(destfile, ap->ap_Buf) != RETURN_OK )
	    {
		Printf("%s: %s", ERROR_HEADER, getstring(STR_ABORTED));
		rc = RETURN_FAIL;
		break;
	    }
	}
	
	MatchEnd(ap);
    }

    FreeVec(ap);

    return rc;
}


LONG append(BPTR destfile, STRPTR srcfilename)
{
    BYTE  *buffer       = NULL;
    LONG   actualLength = 0;
    BPTR   srcfile      = NULL;
    
    BOOL   rc           = RETURN_OK;
    
    if ( (buffer = AllocMem( BUFFERSIZE , MEMF_ANY )) )
    {
	if ( (srcfile = Open( srcfilename , MODE_OLDFILE )) )
	{
	    while( (actualLength = Read(srcfile, buffer, BUFFERSIZE)) != -1 )
	    {
		if (Write(destfile, buffer, actualLength) == -1 )
		{
		    Printf("%s: %s.\n", ERROR_HEADER, getstring(STR_ERR_WRITING));
		    rc = RETURN_FAIL;
		    
		    break;
		}
		
		if (actualLength < BUFFERSIZE)
		{
		    break;
		}
	    }
	    
	    Close(srcfile);
	}
	else
	{
	    Printf("%s: %s: '%s'\n",
		ERROR_HEADER,
		getstring(STR_ERR_OPENREAD),
		srcfilename);
	    
	    rc = RETURN_FAIL;
	}
	
	FreeMem(buffer, BUFFERSIZE);
    }
    else
    {
	Printf("%s: %s.\n", ERROR_HEADER, getstring(STR_ERR_NOMEM));
	rc = RETURN_FAIL;
    }
    
    return rc;
}


STRPTR getstring(LONG stringid)
{
    switch(stringid)
    {
    case STR_ABORTED:
	return "Aborted";

    case STR_REMOVINGDEST:
	return "removed incomplete destination-file";
	    
    case STR_ERR_OPENREAD:
	return "Could not open file for reading";

    case STR_ERR_NOMEM:
	return "Could not allocate memory";

    case STR_ERR_WRITING:
	return "Error while writing";

    default:
	return "[Error: Unknown StringID]";
    }
}
