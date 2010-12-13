
#include <config.h>

#include "library.h"
#include "DriverData.h"

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  VoidBase->dosbase = OpenLibrary( DOSNAME, 37 );

  if( VoidBase->dosbase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

#ifdef __AMIGAOS4__
  if ((IDOS = (struct DOSIFace *) GetInterface((struct Library *) DOSBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IDOS interface!\n");
    return FALSE;
  }
#endif
  
  // Fail if no hardware is present (this check prevents the audio
  // modes from being added to the database if the driver cannot be
  // used).

/*
  if( unable_to_find_hardware )
  {
    Req( "No sound card present.\n" );
    return FALSE;
  }
*/
  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

#ifdef __AMIGAOS4__
  DropInterface( (struct Interface *) IDOS);
#endif

  CloseLibrary( (struct Library*) DOSBase );
}
