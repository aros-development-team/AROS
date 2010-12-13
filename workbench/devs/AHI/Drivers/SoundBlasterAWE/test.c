
#include <resources/isapnp.h>

#include <proto/exec.h>
#include <proto/isapnp.h>

#include <unistd.h>
#include <stdio.h>

struct Library* ISAPNPBase;

void 
play( UWORD r1, 
      UWORD r2,
      UWORD r3 );

int main( void )
{
  struct ISAPNP_Device*     dev[2];
  APTR                      lock;
  UWORD                     ioregs[ 3 ] = { 0, 0, 0 };
  struct ISAPNP_IOResource* r;
  int                       o;

  ISAPNPBase = OpenResource( ISAPNPNAME );

  if( ISAPNPBase == NULL )
  {
    printf( "No " ISAPNPNAME ".\n" );
    exit( 20 );
  }

  dev[ 0 ] = ISAPNP_FindDevice( NULL, 
                                ISAPNP_MAKE_ID( 'C', 'T', 'L' ),
                                2, 1 );
                           
  if( dev[ 0 ] == NULL )
  {
    printf( "SB AWE Wavetable device found.\n" );
    exit( 20 );
  }

  dev[ 1 ] = NULL;

  lock = ISAPNP_LockDevicesA( ISAPNP_LOCKF_NONE, dev );

  if( lock == NULL )
  {
    printf( "Unable to lock device?\n" );
    exit( 20 );
  }


  for( r = (struct ISAPNP_IOResource*) dev[ 0 ]->isapnpd_Resources.mlh_Head, o = 0;
       r->isapnpior_MinNode.mln_Succ != NULL;
       r = (struct ISAPNP_IOResource*) r->isapnpior_MinNode.mln_Succ, ++o )
  {
    if( o > 2 )
    {
      printf( "Unexpected number of io registers!\n" );
      break;
    }
    
    ioregs[ o ] = r->isapnpior_MinBase;
  }

  if( ioregs[ 2 ] != 0 )
  {
    play( ioregs[ 0 ], ioregs[ 1 ], ioregs[ 2 ] );
  }

  ISAPNP_UnlockDevices( lock );

  return 0;
};

void 
play( UWORD r1, 
      UWORD r2,
      UWORD r3 )
{
  printf( "1: %04lx; 2: %04lx; 3: %04lx\n", r1, r2, r3 );
}
