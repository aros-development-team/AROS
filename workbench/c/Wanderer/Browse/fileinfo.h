#ifndef W_FILEINFO_H
#define W_FILEINFO_H

#include <exec/types.h>

/*** Structures **************************************************************/

struct FileInfo
{
    STRPTR fi_Name;
    UWORD  fi_NameLength;
    LONG   fi_Size;
};

/*** Prototypes **************************************************************/

struct FileInfo *allocFileInfo( APTR pool );
void             freeFileInfo( struct FileInfo *info, APTR pool );

#endif /* W_FILEINFO_H */
