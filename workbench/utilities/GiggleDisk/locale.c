
/*
** locale.c
**
** (c) 1998-2011 Guido Mersmann
**
** This file contains full locale support for this application.
*/

/*************************************************************************/

#define SOURCENAME "locale.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>
#include <proto/locale.h>

#include <exec/types.h>
#include <libraries/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

/*************************************************************************/

#ifdef __amigaos4__
struct LocaleIFace *ILocale;
#endif
#if defined( __amigaos4__ ) || defined( __MORPHOS__ )
struct Library *LocaleBase;
#else
struct LocaleBase *LocaleBase;
#endif

struct Locale *locale_locale;
struct Catalog *locale_catalog;

extern char CatCompBlock[];

/*************************************************************************/

/* /// Locale_Open()
**
*/

/*************************************************************************/

BOOL Locale_Open( STRPTR catname, ULONG version, ULONG revision )
{
	if( (LocaleBase = (APTR) OpenLibrary( "locale.library",0 ) ) ) {
#ifdef __amigaos4__
		if( (ILocale = (struct LocaleIFace *) GetInterface( LocaleBase, "main", 1, NULL ) ) ) {
#endif
			if( (locale_locale = OpenLocale(NULL)) ) {
				if( (locale_catalog = OpenCatalogA(locale_locale, catname, TAG_DONE ) ) ) {
					if(    locale_catalog->cat_Version  ==  version &&
						   locale_catalog->cat_Revision == revision ) {
	                    return(TRUE);

	                }
					CloseCatalog( locale_catalog );           /* so close catalog */
	                locale_catalog = NULL;                  /* and use default (if present) */
	            }
				CloseLocale( locale_locale );
	            locale_locale = NULL;
			}
#ifdef __amigaos4__
			DropInterface( (struct Interface *) ILocale );
        }
#endif
		CloseLibrary( (APTR) LocaleBase );
    }

#ifdef ENABLE_INTERNALCATALOG
	return( TRUE );
#else
	return( FALSE );          /* FIXME: We should include an error requester for such case */
#endif
}
/* \\\ */
/* /// Locale_Close()
**
*/

/*************************************************************************/

void Locale_Close( void )
{
	if(  LocaleBase ) {
		if( locale_catalog ) {
			CloseCatalog( locale_catalog );
            locale_catalog = NULL;
        }
		if( locale_locale ) {
			CloseLocale( locale_locale );
            locale_locale = NULL;
        }
#ifdef __amigaos4__
		DropInterface( (struct Interface *) ILocale );
#endif
		CloseLibrary( (APTR) LocaleBase );
		LocaleBase = NULL;
    }
}
/* \\\ */

/* /// Locale_GetString()
**
*/

/*************************************************************************/

STRPTR Locale_GetString( long id )
{
    const struct CatCompArrayType *a;
    STRPTR  builtin = NULL;

    for (a = CatCompArray; a->cca_Str; a++)
    {
	if (a->cca_ID == id)
	{
	    builtin = a->cca_Str;
	    break;
	}
    }

    if ( locale_catalog && LocaleBase ) {
		return( (APTR) GetCatalogStr( locale_catalog, id, builtin ) );
    }
    return(builtin);
}
/* \\\ */
