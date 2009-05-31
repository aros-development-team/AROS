#ifndef GARSHNELIB_PROTOS_H
#define GARSHNELIB_PROTOS_H

#ifndef min
#define min( x, y ) ( (x) < (y) ? (x) : (y) )
#endif

typedef struct _Triplet
{
	ULONG Red;
	ULONG Green;
	ULONG Blue;
} Triplet;

#define SASM __saveds __asm
#define DREG(x) register __d ## x
#define AREG(x) register __a ## x

#define CastAndShift( x ) (( SPFix( SPMul( x, SPFlt( Colors )))) << Shift )

VOID SASM ScreenModeRequest( AREG(0) struct Window *, AREG(1) LONG *,
							AREG(2) LONG * );
VOID SASM setCopperList( DREG(0) LONG, DREG(1) LONG, AREG(0) struct ViewPort *,
						AREG(1) struct Custom * );
VOID SASM clearCopperList( AREG(0) struct ViewPort * );
LONG SASM getTopScreenMode( VOID );
LONG SASM getTopScreenDepth( VOID );
struct Screen *SASM cloneTopScreen( DREG(0) LONG, DREG(1) LONG );
ULONG *SASM GetColorTable( AREG(0) struct Screen * );
LONG SASM AvgBitsPerGun( DREG(0) LONG );
VOID SASM FadeAndLoadTable( AREG(0) struct Screen *, DREG(0) LONG,
						   AREG(1) ULONG *, DREG(1) LONG );
struct Window *SASM BlankMousePointer( AREG(0) struct Screen * );
VOID SASM UnblankMousePointer( AREG(0) struct Window * );
Triplet *SASM RainbowPalette( AREG(0) struct Screen *, AREG(1) Triplet *,
							 DREG(0) LONG, DREG(1) LONG );
	 
#endif /* GARSHNELIB_PROTOS_H */
