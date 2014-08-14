##rem $Id: C++_CatalogF.cc 253 2014-02-18 11:15:58Z damato $

//   Multipurpose CatalogF class implementation
//   Written by Antonio J. Gomez Glez. on 20.6.94
//   You MUST include CatalogF.h in your code. 
//   If you use FlexCat, you only have to include the file generated
//   with it.

#include "CatalogF.h"

extern "C" {
#include <clib/locale_protos.h>
#include <inline/locale.h>
#include <clib/exec_protos.h>
};

unsigned CatalogF::counter = 0;     // counter of open catalogs
struct LocaleBase* LocaleBase = 0;  // We will try to open locale.library

// Constructor:
// Needs:- filename of the catalog to be used. NEEDED!!
//       - built in language of the catalog descriptor. defaults to "english"
//       - language requested. Otherwise, it will use the user defined one.
//       - requested version number for the catalog. defaults to any.
//       - Locale (as returned by OpenLocale). defaults to user defined one.
//
// Try to open locale.library for the first CatalogF object declared. And
// keeps with this with following objects.
// Counts the number of defined objects.
// And opens the catalog if avaible.

CatalogF::CatalogF( const STRPTR   catalogFileName,
                    const STRPTR   builtInLanguage,
                    const LONG     versionNumber,
                    const STRPTR   languageName,
                    struct Locale* loc )
{
  if ( counter == 0 )  // means that this is the first object
  {
     LocaleBase = (struct LocaleBase* )OpenLibrary("locale.library", 38L );
  }
  counter++;

  if ( LocaleBase != 0 )  
  {                  // locale.library is avaible
    LONG tag = TAG_IGNORE;  // in case language not provided use default
    
    if (languageName != 0)      // if language specified, use that
    { 
      tag = OC_Language;
    }
    thecatalog = OpenCatalog( loc,
                              catalogFileName,
                              OC_BuiltInLanguage, builtInLanguage,
                              tag, languageName,
                              OC_Version, versionNumber,
                              TAG_DONE );
  } // else use built-in strings                       
}

// Destructor:
// If locale.library was opened, try to close the catalog, even if no catalog
// was opened (this is supported by CloseCatalog()).
// When the counter of avaible objects reaches 0, it try to close locale.library

CatalogF::~CatalogF()
{
  counter--;
  if ( LocaleBase != 0 )
  { 
    CloseCatalog( thecatalog );
    if ( counter == 0 )
    {
      struct LocaleBase* lb = LocaleBase;
      CloseLibrary( (struct Library* )lb );
      LocaleBase = 0;
    }
  }
}

// Retrive the string.
// If there is locale.library and a opened catalog returns the catalog
// string, else returns the built-in string.
//
// Needs a struct of type catMessage that contains the value ID and the
// string itself. The name of this constant struct is the ID name. This way
// we avoid the use of #define's or const, or any type of search ...
// it returns a const pointer (STRPTR) to the string.
// This method is constant so CatalogFs object can be declared to be const

const STRPTR
CatalogF::GetStr(const CatMessage& mess) const
{
  if ( LocaleBase == 0 )
  {
     return( mess.textstring );
  }
  else
  {
     return( GetCatalogStr(thecatalog, mess.ID, mess.textstring) );
  }
}

