/*
    Copyright © 1995-2004 The AROS Development Team. All rights reserved.
    $Id$

    Desc: Memory Dump util functions.
    Lang: english
*/

#define  DEBUG  1
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

static const char version[] = "$VER: dumpmem.c 45.0 (10.2.2004)\n";

#define ARG_TEMPLATE "ADDRESS/A,SIZE/N/A,SERIAL/S,QUIET/S"

enum 
{
    ARG_ADDRESS,
    ARG_SIZE,
    ARG_SERIAL,
    ARG_QUIET,
    NOOFARGS
};

/* unsigned int hextoint( char **value ); */
void clean_string( char *dirtystring );

int main(int argc, char **argv)
{
    IPTR args[NOOFARGS] = { (STRPTR)NULL,         // ARG_ADDRESS
	                    NULL,                    // ARG_SIZE
                            (IPTR)FALSE,             // ARG_SERIAL
                            (IPTR)FALSE              // ARG_QUIET
    };

    struct RDArgs           *rda;
    char                    outpstring[sizeof(unsigned int)*6];
    unsigned int            addr_value,offset,start_address,dump_size;
    BOOL                    line_end;
    int                     PROGRAM_ERROR = RETURN_OK;
    char                    *ERROR_TEXT,*HELPTXT;

    HELPTXT =
        "ADDRESS  The start address to dump from (in hex)\n"
	"SIZE     The number of bytes to dump\n"
	"SERIAL   if specified, output will be using serial debugging rather than stdout\n"
	"QUIET    do not display warnings\n";

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (NULL != rda)
    {
	if (args[ARG_ADDRESS]!=0)
	{
	
            if (args[ARG_SIZE]!=0)
	    {
	        ULONG  *sizptr = (ULONG *)args[ARG_SIZE];
                char   * pEnd;

	        if ( sizptr != NULL ) dump_size = *sizptr;
                else PROGRAM_ERROR = RETURN_FAIL;
                
                start_address = strtoul( (STRPTR *)args[ARG_ADDRESS], &pEnd, 16 );
                dump_size = ( dump_size / sizeof(unsigned int));

                BOOL serial_out = (BOOL)args[ARG_SERIAL];
                BOOL quiet_out = (BOOL)args[ARG_QUIET];

                if ( dump_size <= 0) 
                {
                    ERROR_TEXT = "Size must be a positive value\n";
                    PROGRAM_ERROR = RETURN_FAIL;
                }
                
                /* a quick warning */
                if ( ( !quiet_out  ) && ( PROGRAM_ERROR != RETURN_FAIL ) )
                {
                    char szInput [256];                         
                    printf( "\n  *Some memory areas should NOT be read\n  use this tool carefully!\n  would you like to proceed? ");

                    gets ( szInput );

                    printf( "\n" );

                    if ( ( strncmp( &szInput, "n", 1) )||( strncmp( &szInput, "N", 1) ) )
                    {
                        ERROR_TEXT = "User canceled..\n";
                        PROGRAM_ERROR = RETURN_FAIL;
                    }
                }

                
                if ( PROGRAM_ERROR != RETURN_FAIL )
                {
                    if (serial_out) kprintf("dumpmem - Memory Dump tool.\n© Copyright the AROS Dev Team.\n-----------------------------\n\nDumping From [%p] for %d bytes..\n\n", start_address, (dump_size * sizeof(unsigned int))); /* use kprintf so it is output on serial.. */
                    else printf("dumpmem - Memory Dump tool.\n© Copyright the AROS Dev Team.\n-----------------------------\n\nDumping From [%p] for %d bytes..\n\n", start_address, (dump_size * sizeof(unsigned int))); /* use kprintf so it is output on serial.. */

                    for ( offset = 0 ; offset < dump_size ; offset += sizeof(unsigned int) )
                    {
                        if ( SetSignal(0L,0L) & SIGBREAKF_CTRL_C )
                        {
	                    SetIoErr( ERROR_BREAK );
	                    PROGRAM_ERROR = RETURN_WARN;
                            break;
                        }
                        
                        if ( ( (offset/sizeof(unsigned int) ) % 6) == 0 ) 
                        {
                            if (serial_out) kprintf("0x%8.8P        ",start_address+offset);
                            else printf("0x%8.8P        ",start_address+offset);
                        }
                        
                        if (serial_out) kprintf("%8.8X", (unsigned int *)((IPTR *)start_address+offset)[0]); /* use kprintf so it is output on serial.. */
                        else printf("%8.8X", (unsigned int *)((IPTR *)start_address+offset)[0]); /* use kprintf so it is output on serial.. */
                        sprintf( &outpstring + ((((offset/sizeof(unsigned int)) % 6) * 4)) ,"%4.4s\0", (unsigned int *)((IPTR *)start_address+offset)[0]);

                        if ( ((offset/sizeof(unsigned int)) % 6) == 5 ) 
                        {
                            line_end = TRUE;
                            
                            clean_string( &outpstring );

                            if (serial_out) kprintf("       '%24.24s'                         \t\n",&outpstring);
                            else printf("       '%24.24s'                         \t\n",&outpstring);
                        }
                        else
                        {   
                            line_end = FALSE;
     
                            if (serial_out)  kprintf(" ");
                            else printf(" ");
                        }

                    }

                    if (serial_out) kprintf("\n\nDump Complete. ");
                    else printf("\n\nDump Complete. ");
                }
	    }
            else ERROR_TEXT = HELPTXT;
	}
        else ERROR_TEXT = HELPTXT;
	FreeArgs(rda);
    }	
    else ERROR_TEXT = HELPTXT;

    if (ERROR_TEXT) printf( ERROR_TEXT );
    return PROGRAM_ERROR;
}

/* convert a hex string to an int - deprecated */
/*
unsigned int hextoint(char **value)
{
    struct CHexMap
    {
        char chr;
        int value;
    };
    const int HexMapL = 16;
    struct CHexMap HiHexMap[] =
    {
        {"0", 0},   {"1", 1},
        {"2", 2},   {"3", 3},
        {"4", 4},   {"5", 5},
        {"6", 6},   {"7", 7},
        {"8", 8},   {"9", 9},
        {"A", 10},  {"B", 11},
        {"C", 12},  {"D", 13},
        {"E", 14},  {"F", 15},
        {""}
    };
    struct CHexMap LoHexMap[] =
    {
        {"0", 0},   {"1", 1},
        {"2", 2},   {"3", 3},
        {"4", 4},   {"5", 5},
        {"6", 6},   {"7", 7},
        {"8", 8},   {"9", 9},
        {"a", 10},  {"b", 11},
        {"c", 12},  {"d", 13},
        {"e", 14},  {"f", 15},
        {""}
    };
    unsigned int result = 0,start=0;

    printf(" converting %s to an int..\n",value);

    if ((char *)value[0] == '0' && (((char *)value[1] == 'X')||((char *)value[1] == 'x') ) ) start = 2;

    BOOL        firsttime = TRUE;

    int vallen;

    for (vallen = start; vallen < strlen(value) ; vallen++)
    {
        BOOL        found = FALSE;
        int         i;
        
        printf(" Parsing $s\n",(char *)value[vallen]);

        for ( i = 0 ; i < HexMapL; i++)
        {
            if (( (char *)value[vallen] == HiHexMap[i].chr )||( (char *)value[vallen] == LoHexMap[i].chr ))
            {
                if (!firsttime) result <<= 4;
                result |= HiHexMap[i].value;
                found = TRUE;
                break;
            }
        }
        if (!found) break;
        firsttime = FALSE;
    }
    return result;
}*/

/* Replace all non printable characters with a '.' */

void clean_string( char *dirtystring )
{
    int             i;

    for (i = 0 ; i < (sizeof(unsigned int) * 6) ; i++)
    {
        if (( (UBYTE *)((IPTR *)dirtystring)[i] < 32 )||( (UBYTE *)((IPTR *)dirtystring)[i] > 126 )) (char *)((IPTR *)dirtystring)[i] = ".";
    }

}
