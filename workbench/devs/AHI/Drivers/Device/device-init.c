
#include <config.h>

#include "library.h"
#include "DriverData.h"

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

  DOSBase = (struct DosLibrary*) OpenLibrary( "dos.library", 37 );

  if( DOSBase == NULL )
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
  
  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

#ifdef __AMIGAOS4__
  DropInterface( (struct Interface *) IDOS);
#endif

  CloseLibrary( (struct Library*) DOSBase );
}
