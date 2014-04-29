/*
    Copyright © 1995-2014 The AROS Development Team. All rights reserved.
    $Id$

    Desc: Memory Dump util functions.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int (*outfunc)(const char *, ...);

static int HexDump(const UBYTE *data, ULONG count)
{
    ULONG t, end;
    int   i;

    end = (count + 15) & -16;

    for (t=0; t<end; t++)
    {
        if ( SetSignal(0L,0L) & SIGBREAKF_CTRL_C )
        {
	    SetIoErr( ERROR_BREAK );
	    return RETURN_WARN;
        }

	if ((t&15) == 0)
	    outfunc("%p:", data + t);

	if ((t&3) == 0)
	    outfunc(" ");

	if (t < count)
	    outfunc("%02x", ((UBYTE *)data)[t]);
	else
	    outfunc("  ");

	if ((t&15) == 15)
	{
	    outfunc(" ");

	    for (i=15; i>=0; i--)
	    {
	    	UBYTE c = data[t-i];

	    	if (isprint(c))
		    outfunc("%c", c);
		else
		    outfunc(".");
	    }
	    outfunc("\n");
	}
    }
    return RETURN_OK;
} /* HexDump */

static const char version[] = "$VER: dumpmem 45.1 (10.4.2014)\n";

#define ARG_TEMPLATE "ADDRESS/A,SIZE/N/A,SERIAL/S,QUIET/S"

enum 
{
    ARG_ADDRESS,
    ARG_SIZE,
    ARG_SERIAL,
    ARG_QUIET,
    NOOFARGS
};

int main(int argc, char **argv)
{
    IPTR args[NOOFARGS] =
    {
        0,         // ARG_ADDRESS
	0,         // ARG_SIZE
        FALSE,     // ARG_SERIAL
        FALSE      // ARG_QUIET
    };

    struct RDArgs *rda;
    IPTR           start_address,dump_size = 0;
    int            PROGRAM_ERROR = RETURN_OK;
    char          *ERROR_TEXT = NULL;
    char          *HELPTXT;

    HELPTXT =
        "ADDRESS  The start address to dump from (in hex)\n"
	"SIZE     The number of bytes to dump\n"
	"SERIAL   If specified, output will use serial debugging instead of stdout\n"
	"QUIET    Do not display warnings\n";

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (NULL != rda)
    {
	if (args[ARG_ADDRESS]!=0)
	{
	
            if (args[ARG_SIZE]!=0)
	    {
                BOOL serial_out = (BOOL)args[ARG_SERIAL];
                BOOL quiet_out = (BOOL)args[ARG_QUIET];
	        ULONG  *sizptr = (ULONG *)args[ARG_SIZE];
                char   * pEnd;
                
	        if ( sizptr != NULL ) dump_size = *sizptr;
                else PROGRAM_ERROR = RETURN_FAIL;
                
                start_address = strtoul((CONST_STRPTR) args[ARG_ADDRESS], &pEnd, 16 );
                
                if ( dump_size <= 0) 
                {
                    ERROR_TEXT = "Size must be a positive value\n";
                    PROGRAM_ERROR = RETURN_FAIL;
                }
                
                /* a quick warning */
                if ( ( !quiet_out  ) && ( PROGRAM_ERROR != RETURN_FAIL ) )
                {
                    TEXT szInput[256];                         
                    printf( "\n  *Some memory areas should NOT be read\n  use this tool carefully!\n  would you like to proceed? ");
                    
                    gets ( szInput );
                    
                    printf( "\n" );
                    
                    if ( ( strncmp( szInput, "n", 1) == 0 )
                        || ( strncmp( szInput, "N", 1) == 0 ) )
                    {
                        ERROR_TEXT = "User canceled...\n";
                        PROGRAM_ERROR = RETURN_FAIL;
                    }
                }

		outfunc = serial_out ? (APTR)kprintf : (APTR)printf;
		
                if ( PROGRAM_ERROR != RETURN_FAIL )
                {
                    outfunc("dumpmem - Memory Dump tool.\n"
                        "© Copyright the AROS Dev Team.\n"
                         "-----------------------------\n\n"
                         "Dumping From [%p] for %d bytes...\n\n",
                         (void *)start_address, dump_size);

		    PROGRAM_ERROR = HexDump((const UBYTE *)start_address, dump_size);

                    outfunc("\n\nDump Complete.\n");
                }
	    }
            else ERROR_TEXT = HELPTXT;
	}
        else ERROR_TEXT = HELPTXT;
	FreeArgs(rda);
    }	
    else ERROR_TEXT = HELPTXT;

    if (ERROR_TEXT) puts( ERROR_TEXT );
    return PROGRAM_ERROR;
}
