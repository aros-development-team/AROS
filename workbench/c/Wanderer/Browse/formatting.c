#include <exec/types.h>
#include <stdio.h>

#include "formatting.h"

void formatSize( STRPTR buffer, ULONG bufferSize, LONG size )
{
    char *suffix;

#   define GB 1000000000
#   define MB 1000000
#   define KB 1000
    
         if( size >= GB ) { suffix = "GB"; size /= GB; }
    else if( size >= MB ) { suffix = "MB"; size /= MB; }
    else if( size >= KB ) { suffix = "KB"; size /= KB; }
    else suffix = "B";
    
    snprintf( buffer, bufferSize, "\33r%ld %s", size, suffix );
    
#   undef GB
#   undef MB
#   undef KB
}
