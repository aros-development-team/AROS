#ifndef ICON_H
#define ICON_H 1

/***************************************************************************/

#include <dos/dos.h>
#include <workbench/startup.h>

/***************************************************************************/

struct DiskObject *Icon_GetPutDiskObject( struct DiskObject *diskobj );
BOOL               Icon_ToolTypeGetBool( struct DiskObject *o, STRPTR tooltype, BOOL defvalue );
long               Icon_ToolTypeGetInteger( struct DiskObject *o, STRPTR tooltype, long defvalue );
STRPTR             Icon_ToolTypeGetString( struct DiskObject *o, STRPTR tooltype, STRPTR deftooltype );

/***************************************************************************/

#endif /* ICON_H */

