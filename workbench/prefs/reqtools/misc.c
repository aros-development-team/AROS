#include <libraries/gadtools.h>
#include <libraries/reqtools.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/reqtools.h>
#include "rtstrings.h"

#ifdef __AROS__
#include <libraries/locale.h>
extern struct ReqToolsBase 	*ReqToolsBase;
#endif

extern struct Window		*WindowPtr;
extern struct Hook		IntuiHook;

#ifdef __AROS__
struct LocaleBase		*LocaleBase;
#define LOCALECAST (struct LocaleBase *)
#else
struct Library			*LocaleBase;
#define LOCALECAST
#endif
struct Catalog			*Catalog;


VOID
InitLocale( VOID )
{
    if( ( LocaleBase = LOCALECAST OpenLibrary( "locale.library", 0 ) ) )
    {
	Catalog = OpenCatalogA( NULL, "System/Prefs/ReqTools.catalog", NULL );
    }
}


VOID
FreeLocale( VOID )
{
    if( LocaleBase )
    {
	CloseCatalog( Catalog );
	CloseLibrary( (struct Library *)LocaleBase );
    }
}


STRPTR
GetString( STRPTR idstr )
{
    STRPTR local;

    local = idstr + 2;

    if( LocaleBase )
    {
	return( ( STRPTR ) GetCatalogStr( Catalog, ( ( UBYTE ) idstr[ 0 ] << 8 ) | idstr[ 1 ], local ) );
    }

    return( local );
}

VOID
LocalizeMenus( struct NewMenu *nm )
{
    STRPTR local;

    while( nm->nm_Type != NM_END )
    {
	if( nm->nm_Label && ( nm->nm_Label != NM_BARLABEL ) )
	{
	    local = GetString( nm->nm_Label );

	    if( nm->nm_Type != NM_TITLE )
	    {
		if( *local != ' ' )
		{
		    nm->nm_CommKey = local;
		}

		local += 2;
	    }

	    nm->nm_Label = local;
	}

	++nm;
    }
}


VOID
LocalizeLabels( STRPTR *labels )
{
    STRPTR local;

    while( *labels )
    {
	local = GetString( *labels );
	*labels++ = local;
    }
}



static struct EasyStruct EZ;


ULONG
EasyReq( STRPTR str, STRPTR gadtxt, APTR args )
{
    if( ReqToolsBase )
    {
	struct TagItem tags[] =
	{
	    {RT_Window		, (IPTR)WindowPtr	},
	    {RT_LockWindow	, WindowPtr != NULL	},
	    {RT_ShareIDCMP	, WindowPtr != NULL	},
	    {RT_IntuiMsgFunc	, (IPTR)&IntuiHook	},
	    {TAG_DONE					}

	};

	return( rtEZRequestA( str, gadtxt, NULL, args, tags) );
    }

    EZ.es_StructSize = sizeof( struct EasyStruct );
    EZ.es_Title = GetString( MSG_INFORMATION );
    EZ.es_TextFormat = str;
    EZ.es_GadgetFormat = gadtxt;
    
    return( ( ULONG ) EasyRequestArgs( NULL, &EZ, NULL, args ) );
}


ULONG
LocEZReq( STRPTR str, STRPTR gadtxt, ... )
{
    return( EasyReq( GetString( str ), GetString( gadtxt ), &gadtxt + 1 ) );
}

ULONG
EZReq( STRPTR str, STRPTR gadtxt, ... )
{
    return( EasyReq( str, gadtxt, &gadtxt + 1 ) );
}


