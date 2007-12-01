/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  AmigaOS4 Specific funcctions - Provides interface aware functions
**
**  Written by Alexandre Balaban <alexandre@balaban.name>
*/

#undef __USE_INLINE__

#include "lib.h"

#if !defined(__amigaos4__)
#  error This file is AmigaOS4 specific and should not be used/compiled on other platforms
#endif

#include <proto/exec.h>

struct Library * SAVEDS ASM initLib (REG(a0,ULONG segList), REG(a6,struct ExecBase *sys), REG(d0, struct Library *base));
struct Library * SAVEDS ASM openLib (REG(a6,struct Library *base));
ULONG SAVEDS ASM expungeLib (REG(a6,struct Library *base));
ULONG SAVEDS ASM closeLib(REG(a6,struct Library *base));

/* amigaos4 *****************************************************************/

#include <stdarg.h>

ULONG VARARGS68K OS4_URL_OpenA ( struct OpenURLIFace * Self, STRPTR url, struct TagItem *attrs )
{
    return URL_OpenA( url, attrs );
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_Open ( struct OpenURLIFace * Self, STRPTR url, ... )
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, url);
    tags = va_getlinearva(ap, struct TagItem *);

    return URL_OpenA( url, tags );
}

/* amigaos4 *****************************************************************/

struct URL_Prefs * VARARGS68K OS4_URL_GetPrefsA ( struct OpenURLIFace * Self, struct TagItem *attrs )
{
    return URL_GetPrefsA( attrs );
}

/* amigaos4 *****************************************************************/

struct URL_Prefs * VARARGS68K OS4_URL_GetPrefs ( struct OpenURLIFace * Self, ... )
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, Self);
    tags = va_getlinearva(ap, struct TagItem *);

    return URL_GetPrefsA( tags );
}

/* amigaos4 *****************************************************************/

struct URL_Prefs * VARARGS68K OS4_URL_OldGetPrefs ( struct OpenURLIFace * Self )
{
    return URL_OldGetPrefs();
}

/* amigaos4 *****************************************************************/

void VARARGS68K OS4_URL_FreePrefsA ( struct OpenURLIFace * Self, struct URL_Prefs *up , struct TagItem *attrs )
{
    URL_FreePrefsA( up, attrs );
}

/* amigaos4 *****************************************************************/

void VARARGS68K OS4_URL_FreePrefs ( struct OpenURLIFace * Self, struct URL_Prefs *up, ... )
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, up);
    tags = va_getlinearva(ap, struct TagItem *);

    URL_FreePrefsA( up, tags );
}

/* amigaos4 *****************************************************************/

void VARARGS68K OS4_URL_OldFreePrefs ( struct OpenURLIFace * Self, struct URL_Prefs *up )
{
    URL_OldFreePrefs( up );
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_SetPrefsA ( struct OpenURLIFace * Self, struct URL_Prefs *p, struct TagItem *attrs )
{
    return URL_SetPrefsA( p, attrs );
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_SetPrefs ( struct OpenURLIFace * Self, struct URL_Prefs *p, ... )
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, p);
    tags = va_getlinearva(ap, struct TagItem *);

    return URL_SetPrefsA( p, tags );
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_OldSetPrefs ( struct OpenURLIFace * Self, struct URL_Prefs *p , ULONG permanent )
{
    return URL_OldSetPrefs( p, permanent );
}

/* amigaos4 *****************************************************************/

struct URL_Prefs * VARARGS68K OS4_URL_OldGetDefaultPrefs ( struct OpenURLIFace * Self )
{
    return URL_OldGetDefaultPrefs();
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_LaunchPrefsAppA ( struct OpenURLIFace * Self, struct TagItem *attrs )
{
    return URL_LaunchPrefsAppA( attrs );
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_LaunchPrefsApp ( struct OpenURLIFace * Self, ... )
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, Self);
    tags = va_getlinearva(ap, struct TagItem *);

    return URL_LaunchPrefsAppA( tags );
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_OldLaunchPrefsApp ( struct OpenURLIFace * Self )
{
    return URL_OldLaunchPrefsApp();
}

/* amigaos4 *****************************************************************/

ULONG VARARGS68K OS4_URL_GetAttr ( struct OpenURLIFace * Self, ULONG attr , ULONG *storage )
{
    return URL_GetAttr( attr, storage );
}

/* amigaos4 *****************************************************************/

LONG VARARGS68K OS4_dispatch ( struct OpenURLIFace * Self, struct RexxMsg *msg, UBYTE ** resPtr )
{
	 return dispatch( msg, resPtr );
}

/* amigaos4 *****************************************************************/

struct Library * mgr_Init (struct Library *base, BPTR segList, struct ExecIFace *ISys)
{
	base->lib_Node.ln_Type = NT_LIBRARY;
	base->lib_Node.ln_Pri = 0;
	base->lib_Node.ln_Name = lib_name;
	base->lib_Flags = LIBF_SUMUSED|LIBF_CHANGED;
	base->lib_Version = lib_version;
	base->lib_Revision = lib_revision;
	base->lib_IdString = lib_ver;

	IExec = ISys;
	SysBase = (struct ExecBase*)ISys->Data.LibBase;

	return initLib( segList, SysBase, base );
}

/* amigaos4 *****************************************************************/

struct Library *mgr_Open( struct LibraryManagerInterface *Self, uint32 version )
{
	return openLib( Self->Data.LibBase );
}

/* amigaos4 *****************************************************************/

APTR mgr_Close( struct LibraryManagerInterface *Self )
{
	return (APTR)closeLib( Self->Data.LibBase );
}

/* amigaos4 *****************************************************************/

APTR mgr_Expunge( struct LibraryManagerInterface *Self )
{
	return (APTR)expungeLib( Self->Data.LibBase );
}

/* amigaos4 *****************************************************************/

uint32 mgr_Obtain( struct LibraryManagerInterface *Self ) {
   return( Self->Data.RefCount++ );
}

/* amigaos4 *****************************************************************/

uint32 mgr_Release( struct LibraryManagerInterface *Self ) {
   return( Self->Data.RefCount-- );
}

/* amigaos4 *****************************************************************/

uint32 VARARGS68K OS4_URL_Obtain( struct OpenURLIFace *Self ) {
   return( Self->Data.RefCount++ );
}

/* amigaos4 *****************************************************************/

uint32 VARARGS68K OS4_URL_Release( struct OpenURLIFace *Self ) {
   return( Self->Data.RefCount-- );
}

/* amigaos4 *****************************************************************/

/// _start
/*
 * The system (and compiler) rely on a symbol named _start which marks
 * the beginning of execution of an ELF file. To prevent others from
 * executing this library, and to keep the compiler/linker happy, we
 * define an empty _start symbol here.
 *
 * On the classic system (pre-AmigaOS4) this was usually done by
 * moveq #0,d0
 * rts
 *
 */

int32 _start( STRPTR argstring, int32 arglen, APTR SysBase )
{

    /* If you feel like it, open DOS and print something to the user */

    return( RETURN_OK );
}

/* amigaos4 *****************************************************************/

