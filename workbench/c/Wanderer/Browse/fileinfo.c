#include <exec/types.h>
#include <proto/exec.h>

#include "fileinfo.h"

struct FileInfo *allocFileInfo( APTR pool )
{
    return AllocPooled( pool, sizeof( struct FileInfo ) );
}

void freeFileInfo( struct FileInfo *info, APTR pool )
{
    if( info )
    {
        if( info->fi_Name ) FreePooled( pool, info->fi_Name, info->fi_NameLength + 1 );
        
        FreePooled( pool, info, sizeof( struct FileInfo ) );
    }
}
