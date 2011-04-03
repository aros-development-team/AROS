
/*
** icon.c
**
** (c) 1998-2011 Guido Mersmann
*/

/***************************************************************************/

#define SOURCENAME "icon.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <dos/dos.h>
#include <workbench/startup.h>

/***************************************************************************/

extern struct WBStartup *wbmessage; /* see wbstartup.c */

/***************************************************************************/

/* /// Icon_GetPutDiskObject()
**
*/

/***************************************************************************/

struct DiskObject *Icon_GetPutDiskObject( struct DiskObject *diskobj )
{
#define ds_SIZEOF 0x200
char ds[ds_SIZEOF];
STRPTR name;
BPTR lock;
struct DiskObject *result = 0L;
struct WBArg *wbarg;

    if( wbmessage ) {

        wbarg = wbmessage->sm_ArgList;

        lock = (BPTR) wbarg->wa_Lock;
        name = wbarg->wa_Name;

        if( wbmessage->sm_NumArgs >= 2) {
            wbarg++;

			if( ( name = wbarg->wa_Name ) ) {
                lock = (BPTR) wbarg->wa_Lock;
                name = wbarg->wa_Name;
            }
        }
		if( !name ) {
			return( NULL );
        }

    } else {

		if( GetProgramName( ds, ds_SIZEOF ) ) {

            name = (STRPTR) FilePart( (APTR) &ds );
            lock = GetProgramDir();

		} else {
			return( NULL );
		}
    }
	lock = CurrentDir( lock );
	if( !diskobj ) {
		result = GetDiskObjectNew( name );
	} else {
		result  = (APTR)(ULONG) PutDiskObject( name, diskobj );
	}
	CurrentDir( lock );

	return( result );
}
/* \\\ */
/* /// Icon_ToolTypeGetBool()
**
*/

/***************************************************************************/

BOOL Icon_ToolTypeGetBool( struct DiskObject *o, STRPTR tooltype, BOOL defvalue )
{
STRPTR toolstring;

#if defined(__MORPHOS__)
	if( ( toolstring = (STRPTR) FindToolType( (CONST STRPTR *) o->do_ToolTypes, (CONST STRPTR) tooltype ) ) ) {
#elif defined( __amigaos4__)
	if( ( toolstring = (STRPTR) FindToolType( (STRPTR *) o->do_ToolTypes, (CONST_STRPTR) tooltype ) ) ) {
#else
	if( ( toolstring = (STRPTR) FindToolType( (const STRPTR *) o->do_ToolTypes, (const STRPTR) tooltype ) ) ) {
#endif
		return( ( ( !Stricmp( toolstring, "YES" ) ) || (!Stricmp( toolstring, "ON" ) ) || (!Stricmp( toolstring, "TRUE" ) ) ) ? TRUE : FALSE );

    }
	return( defvalue );
}
/* \\\ */
/* /// Icon_ToolTypeGetInteger()
**
*/

/***************************************************************************/
long Icon_ToolTypeGetInteger( struct DiskObject *o, STRPTR tooltype, long defvalue )
{
STRPTR toolstring;
long value;

#if defined(__MORPHOS__)
	if( ( toolstring = (STRPTR) FindToolType( (CONST STRPTR *) o->do_ToolTypes, (CONST STRPTR) tooltype ) ) ) {
#elif defined( __amigaos4__)
	if( ( toolstring = (STRPTR) FindToolType( (STRPTR *) o->do_ToolTypes, (CONST_STRPTR) tooltype ) ) ) {
#else
	if( ( toolstring = (STRPTR) FindToolType( (const STRPTR *) o->do_ToolTypes, (const STRPTR) tooltype ) ) ) {
#endif

		if( -1 != StrToLong(toolstring, &value ) ) {
			return( value );
        }
    }
	return( defvalue );
}
/* \\\ */
/* /// Icon_ToolTypeGetString()
**
*/

/***************************************************************************/

STRPTR Icon_ToolTypeGetString( struct DiskObject *o, STRPTR tooltype, STRPTR deftooltype )
{

STRPTR result;

#if defined(__MORPHOS__)
	result =  (STRPTR) FindToolType( (CONST STRPTR *) o->do_ToolTypes, (CONST STRPTR) tooltype );
#elif defined( __amigaos4__)
	result =  (STRPTR) FindToolType( (STRPTR *) o->do_ToolTypes, (CONST_STRPTR) tooltype );
#else
	result =  (STRPTR) FindToolType( (const STRPTR *) o->do_ToolTypes, (const STRPTR) tooltype );
#endif

	if( !result ) {
		return( deftooltype );
    } else {
		return( result );
    }
}
/* \\\ */

