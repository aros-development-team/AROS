/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Join - Create a single file from several.
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosasl.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>

#define ARG_TEMPLATE "FILE/M/A,AS=TO/K/A"
#define ARG_COUNT    2
#define ARG_FILE     0
#define ARG_AS       1

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

/****** Functions *******************************************************/

int main( void )
{
    struct RDArgs     * rda             = NULL ;
    LONG                args[ARG_COUNT] = { NULL , NULL };
    STRPTR            * filenameptr;
    STRPTR              filename;
    STRPTR              destination;

    BPTR                destfile        = NULL;

    LONG                rc              = RETURN_OK;

    if( (rda = ReadArgs( ARG_TEMPLATE , args , NULL )) )
    {
	if( args[ARG_FILE] && args[ARG_AS] )
	{
	    destination = (STRPTR) args[ARG_AS];
	    filenameptr = (STRPTR *) args[ARG_FILE];

	    if( (destfile = Open( destination , MODE_NEWFILE )) )
	    {
		while( (filename = *filenameptr++) )
		{
		    if( append( destfile , filename ) != RETURN_OK )
		    {
			printf( "%s: %s" , ERROR_HEADER , getstring( STR_ABORTED ) );
			rc = RETURN_FAIL;
			break;
		    }
		}

		Close( destfile );
		if( rc == RETURN_FAIL )
		{
		    printf( ", %s.\n" , getstring( STR_REMOVINGDEST ) );
		    DeleteFile( destination );
		}
	    }
	    else
	    {
		PrintFault( IoErr() , ERROR_HEADER );
		rc = RETURN_FAIL;
	    }
	}
	else
	{
	    rc = RETURN_FAIL;
	}

	FreeArgs( rda );
    }
    else
    {
	PrintFault( IoErr() , ERROR_HEADER );
	rc = RETURN_FAIL;
    }

    return( rc );
}

LONG append( BPTR destfile , STRPTR srcfilename )
{
    BYTE * buffer       = NULL;
    LONG   actualLength = 0;
    BPTR   srcfile      = NULL;

    BOOL   rc           = RETURN_OK;

    if( (buffer = AllocMem( BUFFERSIZE , MEMF_ANY )) )
    {
	if( (srcfile = Open( srcfilename , MODE_OLDFILE )) )
	{
	    while( (actualLength = Read( srcfile , buffer , BUFFERSIZE )) != -1 )
	    {
		if( Write( destfile , buffer , actualLength ) == -1 )
		{
		    printf( "%s: %s.\n" , ERROR_HEADER , getstring( STR_ERR_WRITING ) );
		    rc = RETURN_FAIL;

		    break;
		}
		if( actualLength < BUFFERSIZE )
		{
		    break;
		}
	    }

	    Close( srcfile );
	}
	else
	{
	    printf
	    (
		"%s: %s: '%s'\n" ,
		ERROR_HEADER ,
		getstring( STR_ERR_OPENREAD ) ,
		srcfilename
	    );

	    rc = RETURN_FAIL;
	}

	FreeMem( buffer , BUFFERSIZE );
    }
    else
    {
	printf( "%s: %s.\n" , ERROR_HEADER , getstring( STR_ERR_NOMEM ) );
	rc = RETURN_FAIL;
    }

    return( rc );
}

STRPTR getstring( LONG stringid )
{
    switch( stringid )
    {
	case STR_ABORTED:
	    return( "Aborted" );
	case STR_REMOVINGDEST:
	    return( "removed incomplete destination-file" );
	case STR_ERR_OPENREAD:
	    return( "Could not open file for reading" );
	case STR_ERR_NOMEM:
	    return( "Could not allocate memory" );
	case STR_ERR_WRITING:
	    return( "Error while writing" );

	default:
	    return( "[Error: Unknown StringID]" );
    }
}

