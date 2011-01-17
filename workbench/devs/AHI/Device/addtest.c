
#include "addroutines.h"


/*
** Samples          Number of samples to calculate.
** ScaleLeft        Left volume multiplier.
** ScaleRight       Right volume multiplier (not used for mono sounds).
** StartPointLeft   Sample value from last session, for interpolation. Update!
** StartPointRight  Sample value from last session, for interpolation. Update!
** Src              Pointer to source samples.
** Dst              Pointer to pointer to destination buffer. Update!
** FirstOffsetI     The offset value of the first sample (when StartPoint* 
**                  should be used).
** Offset           The offset (fix-point). Update!
** Add              Add value (fix-point).
** StopAtZero       If true, abort at next zero-crossing.
*/

#define ADDARGS LONG      Samples,\
                LONG      ScaleLeft,\
                LONG      ScaleRight,\
                LONG	 *StartPointLeft,\
                LONG	 *StartPointRight,\
                void     *Src,\
                void    **Dst,\
                LONG	  FirstOffsetI,\
                Fixed64   Add,\
                Fixed64  *Offset,\
                BOOL      StopAtZero

/*
            processed = ((ADDFUNC *) cd->cd_AddRoutine)( try_samples,
                                                         cd->cd_ScaleLeft,
                                                         cd->cd_ScaleRight,
                                                        &cd->cd_TempStartPointL,
                                                        &cd->cd_TempStartPointR,
                                                         cd->cd_DataStart,
                                                        &dstptr,
                                                         cd->cd_FirstOffsetI,
                                                         cd->cd_Add,
                                                        &cd->cd_Offset, 
                                                         TRUE );
*/

//#include <stdio.h>
//#include <string.h>

ULONG __amigappc__=1;

static long outbuffer[ 4096 ];
static char sample[16] =
{
  0, 16, 32, 48, 64, 48, 32, 16, 0, -16, -32, -48, -64, -48, -32, -16
};

int
main( void )
{
  int       i;
  int       num              = 10;
  long      startpointleft   = 0;
  long      startpointright  = 0;
  void*     dst              = outbuffer;
  long long offset           = 0x00000000;

  ADDFUNC* af = AddByteMono;

//  memset( outbuffer, 0x00, sizeof( outbuffer ) );

//  printf( "spl: %08x, spr: %08x, dst: %08lx, offset: %ld\n",
//          startpointleft, startpointright, dst, offset );

  num = (*af)( num, 0x1, 0x00000,
         &startpointleft,
         &startpointright,
         sample,
         &dst,
         0,
         0x080000000,
         &offset,
         FALSE );

//  printf( "Iterations: %d\n", num );

//  for( i = 0; i < num * 2; i++ )
//  {
//    printf( "%3d: %08x\n", i, outbuffer[ i ] );
//  }

//  printf( "spl: %08x, spr: %08x, dst: %08lx, offset: %ld\n",
//          startpointleft, startpointright, dst, offset );

  startpointleft = startpointright = 0;
  dst            = outbuffer;
  offset         = 0;

  num = (*af)( num, 0x0, 0x00000,
         &startpointleft,
         &startpointright,
         sample,
         &dst,
         0,
         0x080000000,
         &offset,
         TRUE );

//  printf( "Iterations: %d\n", num );

//  for( i = 0; i < num * 2; i++ )
//  {
//    printf( "%3d: %08x\n", i, outbuffer[ i ] );
//  }

//  printf( "spl: %08x, spr: %08x, dst: %08lx, offset: %ld\n",
//          startpointleft, startpointright, dst, offset );

  return 0;
}

