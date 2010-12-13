
#include <config.h>

#include "library.h"
#include "DriverData.h"

#define AHI_AROSQUIET
#define DEBUG 1
#include <aros/debug.h>

struct DosLibrary *DOSBase = NULL;
struct Library *OSSBase = NULL;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

  DOSBase = OpenLibrary( DOSNAME, 37 );

  if( DOSBase == NULL )
  {
#ifndef AHI_AROSQUIET
    Req( "Unable to open 'dos.library' version 37.\n" );
#else
    D(bug("Unable to open 'dos.library' version 37.\n" ));
#endif
    return FALSE;
  }

  OSSBase = OpenLibrary( "oss.library", 0 );

  if( OSSBase == NULL )
  {
#ifndef AHI_AROSQUIET
    Req( "Unable to open 'oss.library'.\n" );
#endif
    return FALSE;
  }
  
  // Fail if no hardware is present (this check prevents the audio
  // modes from being added to the database if the driver cannot be
  // used).

  if( ! OSS_Open( "/dev/dsp", FALSE, TRUE, FALSE ) )
  {
#ifndef AHI_AROSQUIET
    Req( "No sound card present.\n" );
#else
    D(bug( "No sound card present.\n" ));
#endif
    return FALSE;
  }

  OSS_Close();

  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

  CloseLibrary( (struct Library*) DOSBase );
  CloseLibrary( OSSBase );
}
