
#include <config.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "emu10kx-ac97.h"
#include "linuxsupport.h"

#define VERSION 6

int
main( void )
{
  struct Library* EMU10kxBase;
  struct EMU10kxAC97* EMU10kxAC97;
  ULONG value;

  EMU10kxBase = OpenLibrary( "DEVS:AHI/emu10kx.audio", VERSION );
  
  if( EMU10kxBase == NULL )
  {
    Printf( "Unable to open DEVS:AHI/emu10kx.audio version %ld.\n", VERSION );
    return RETURN_FAIL;
  }

  Forbid();
  EMU10kxAC97 = (struct EMU10kxAC97*) FindSemaphore( EMU10KX_AC97_SEMAPHORE );
  if( EMU10kxAC97 != NULL )
  {
    ObtainSemaphore( &EMU10kxAC97->Semaphore );
  }
  Permit();

  if( EMU10kxAC97 == NULL )
  {
    CloseLibrary( EMU10kxBase );
    Printf( "Unable to find semaphore '%s'.\n", (ULONG) EMU10KX_AC97_SEMAPHORE );
    return RETURN_FAIL;
  }

  Printf( "%ld EMU10kx cards found.\n", EMU10kxAC97->Cards );
  
  value = CallHook( &EMU10kxAC97->GetFunc, (Object*) EMU10kxBase,
		    0, AC97_CD_VOL );

  Printf( "CD volume on card 0 is 0x%04lx\n", value );

  Printf( "Setting it to 0x0000.\n" );
  CallHook( &EMU10kxAC97->SetFunc, (Object*) EMU10kxBase, 0, AC97_CD_VOL, 0 );

  Delay( 3 * 50 );

  Printf( "Restoring it.\n" );
  CallHook( &EMU10kxAC97->SetFunc, (Object*) EMU10kxBase, 0, AC97_CD_VOL, value );

  Printf( "Exiting.\n" );
  
  if( EMU10kxAC97 != NULL )
  {
    ReleaseSemaphore( &EMU10kxAC97->Semaphore );
  }

  CloseLibrary( EMU10kxBase );

  return RETURN_OK;
}
