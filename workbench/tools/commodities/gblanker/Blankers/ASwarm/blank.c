/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "../includes.h"
#include <stdlib.h>

#define MAX_SPEED 6L

#define SPEED 0
#define COLOR 1
#define AIM   2
#define BEES  4
#define WASPS 6
#define BACC  8
#define WACC  10
#define MODE  12

#define MAXBEEVEL ( 3 * mP[BACC].po_Level )
#define MAXWASPVEL ( 4 * mP[WACC].po_Level )

#define BORDER 5

#define BEE_PEN 1
#define WASP_PEN 2

#define BEE_COL_NUM 33

#define RAND( m ) ( RangeRand( m ) - ( m ) / 2 )

#define BXVel( I ) ( SP->ss_X[3][I] )
#define BYVel( I ) ( SP->ss_Y[3][I] )
#define BeeX( P, I ) ( SP->ss_X[P][I] )
#define BeeY( P, I ) ( SP->ss_Y[P][I] )
#define MyWasp( I ) ( SP->ss_MW[I] )
#define WaXVel( I ) ( SP->ss_WX[3][I] )
#define WaYVel( I ) ( SP->ss_WY[3][I] )
#define WaspX( P, I ) ( SP->ss_WX[P][I] )
#define WaspY( P, I ) ( SP->ss_WY[P][I] )
#define SwarmSize( W, B ) ( sizeof( SwarmStruct ) + sizeof( LONG ) * ( W + B ) * 9L )

typedef struct _SwarmStruct
{
	LONG ss_Width;
	LONG ss_Height;
	LONG ss_NumWasps;
	LONG *ss_WX[4];
	LONG *ss_WY[4];
	LONG *ss_NB;
	LONG ss_NumBees;
	LONG ss_BeeAcc;
	LONG *ss_X[4];
	LONG *ss_Y[4];
	LONG *ss_MW;
} SwarmStruct;

ULONG BeeColors[] =
{
	0x000F, 0x000F, 0x004F, 0x008F, 0x00BF, 0x00FF, 0x00FB, 0x00F7, 0x00F3,
	0x00F0, 0x04F0, 0x08F0, 0x09F0, 0x0AF0, 0x0BF0, 0x0CF0, 0x0DF0, 0x0EF0,
	0x0FF0, 0x0FE0, 0x0FD0, 0x0FC0, 0x0FB0, 0x0F90, 0x0F80, 0x0F70, 0x0F60,
	0x0F50, 0x0F40, 0x0F30, 0x0F20, 0x0F10, 0x0F00
};

UWORD SwarmColors[4] = { 0x0000, 0x0000, 0x0FFF, 0x0000 };

BYTE sqrt_tab[256] =
{
	0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,14,14,14,14,14,
	14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15
	};

#include "ASwarm_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[SPEED].po_Active = 6;
	Prefs[COLOR].po_Active = 1;
	Prefs[AIM].po_Active = 1;
	Prefs[BEES].po_Level = 50;
	Prefs[WASPS].po_Level = 4;
	Prefs[BACC].po_Level = 6;
	Prefs[WACC].po_Level = 4;
	Prefs[MODE].po_ModeID = getTopScreenMode();
}

LONG FastSQRT( LONG x )
{
	LONG sr = 1L;
	
	while( x > 255L )
	{
		x /= 4L;
		sr *= 2L;
	}
	
	return sr * ( LONG )sqrt_tab[x];
}

SwarmStruct *CreateSwarms( struct Screen *Scr, PrefObject *mP )
{
	LONG Index, *Ptr;
	SwarmStruct	*SP;

	if(( SP = AllocVec( SwarmSize( mP[WASPS].po_Level, mP[BEES].po_Level ),
					   0L )) == NULL )
		return NULL;
	
	SP->ss_NumWasps = mP[WASPS].po_Level;
	SP->ss_NumBees = mP[BEES].po_Level;
	SP->ss_Width = Scr->Width;
	SP->ss_Height = Scr->Height;
	SP->ss_BeeAcc = mP[BACC].po_Level;
	
	Ptr = ( LONG * )( &SP[1] );
	for( Index = 0L; Index < 4L; Index++ )
	{
		SP->ss_WX[Index] = Ptr;
		Ptr += mP[WASPS].po_Level;
		SP->ss_WY[Index] = Ptr;
		Ptr += mP[WASPS].po_Level;
		SP->ss_X[Index] = Ptr;
		Ptr += mP[BEES].po_Level;
		SP->ss_Y[Index] = Ptr;
		Ptr += mP[BEES].po_Level;
	}
	SP->ss_NB = Ptr;
	SP->ss_MW = Ptr + mP[WASPS].po_Level;

	for( Index = 0L; Index < mP[WASPS].po_Level; Index++ )
	{
		WaspX( 1, Index ) = WaspX( 0, Index ) =	BORDER +
			RangeRand( SP->ss_Width - 2 * BORDER );
		WaspY( 1, Index ) = WaspY( 0, Index ) =	BORDER +
			RangeRand( SP->ss_Height - 2 * BORDER );
		WaXVel( Index ) = RAND( mP[WACC].po_Level );
		WaYVel( Index ) = RAND( mP[WACC].po_Level );
		SP->ss_NB[Index]=0;
	}

	for( Index = 0L; Index < SP->ss_NumBees; Index++ )
	{
		BeeX( 1, Index ) = BeeX( 0, Index ) = BORDER +
			RangeRand( SP->ss_Width - 2 * BORDER );
		BeeY( 1, Index ) = BeeY( 0, Index ) = BORDER +
			RangeRand( SP->ss_Height - 2 * BORDER );
		BXVel( Index ) = RAND( SP->ss_BeeAcc );
		BYVel( Index ) = RAND( SP->ss_BeeAcc );
		SP->ss_NB[MyWasp( Index ) = Index % SP->ss_NumWasps]++;
	}

	return SP;
}

VOID DrawSwarms( struct RastPort *RP, SwarmStruct *SP, PrefObject *mP )
{
	LONG Index, Aimmode = mP[AIM].po_Active;

	for( Index = 0L; Index < SP->ss_NumWasps; Index++ )
	{
		WaspX( 2, Index ) = WaspX( 1, Index );
		WaspX( 1, Index ) = WaspX( 0, Index );
		WaspY( 2, Index ) = WaspY( 1, Index );
		WaspY( 1, Index ) = WaspY( 0, Index );
 
		WaXVel( Index ) += RAND( mP[WACC].po_Level );
		WaYVel( Index ) += RAND( mP[WACC].po_Level );

		if( WaXVel( Index ) > MAXWASPVEL )
			WaXVel( Index ) = MAXWASPVEL;
		if( WaXVel( Index ) < -MAXWASPVEL )
			WaXVel( Index ) = -MAXWASPVEL;
		if( WaYVel( Index ) > MAXWASPVEL )
			WaYVel( Index ) = MAXWASPVEL;
		if( WaYVel( Index ) < -MAXWASPVEL )
			WaYVel( Index ) = -MAXWASPVEL;

		WaspX( 0, Index ) = WaspX( 1, Index ) + WaXVel( Index );
		WaspY( 0, Index ) = WaspY( 1, Index ) + WaYVel( Index );

		if(( WaspX( 0, Index ) < BORDER )||
		   ( WaspX( 0, Index ) > SP->ss_Width-BORDER-1 ))
		{
			WaXVel( Index ) = -WaXVel( Index );
			if( WaspX( 0, Index ) < BORDER )
				WaspX( 0, Index ) = BORDER;
			else
				WaspX( 0, Index ) = SP->ss_Width-BORDER-1;
		}

		if(( WaspY( 0, Index ) < BORDER )||
		   ( WaspY( 0, Index ) > SP->ss_Height-BORDER-1 ))
		{
			WaYVel( Index ) = -WaYVel( Index );
			if( WaspY( 0, Index ) < BORDER )
				WaspY( 0, Index ) = BORDER;
			else
				WaspY(0, Index ) = SP->ss_Height-BORDER-1;
		}
	}

	for( Index = 0L; Index < SP->ss_NumBees; Index++ )
	{
		LONG DX, DY, ChkIndex;
		LONG Distance, NewDistance;

		BeeX( 2, Index ) = BeeX( 1, Index );
		BeeX( 1, Index ) = BeeX( 0, Index );
		BeeY( 2, Index ) = BeeY( 1, Index );
		BeeY( 1, Index ) = BeeY( 0, Index );

		DX = WaspX( 1, MyWasp( Index )) - BeeX( 1, Index );
		DY = WaspY( 1, MyWasp( Index )) - BeeY( 1, Index );
		Distance = FastSQRT( DX * DX + DY * DY );
		if( !Distance )
			Distance = 1L;

		if( Aimmode )
		{
			for( ChkIndex = 0; ChkIndex <= SP->ss_NumWasps; ChkIndex++ )
			{
				if( ChkIndex != MyWasp( Index ))
				{
					LONG NewDX, NewDY;

					NewDX = WaspX( 1, ChkIndex ) - BeeX( 1, Index );
					NewDY = WaspY( 1, ChkIndex ) - BeeY( 1, Index );
					NewDistance = FastSQRT( NewDX * NewDX + NewDY * NewDY );
					if( Distance > NewDistance )
					{
						DX = NewDX;
						DY = NewDY;
						if(!( NewDistance ))
							Distance = 1L;
						else
							Distance = NewDistance;
						SP->ss_NB[MyWasp( Index )]--;
						SP->ss_NB[MyWasp( Index ) = ChkIndex]++;
					}
				}
			}
		}

		BXVel( Index ) +=( DX * SP->ss_BeeAcc ) / Distance + RAND( 3 );
		BYVel( Index ) +=( DY * SP->ss_BeeAcc ) / Distance + RAND( 3 );

		if( BXVel( Index ) > MAXBEEVEL )
			BXVel( Index ) = MAXBEEVEL;
		if( BXVel( Index ) < -MAXBEEVEL )
			BXVel( Index ) = -MAXBEEVEL;
		if( BYVel( Index ) > MAXBEEVEL )
			BYVel( Index ) = MAXBEEVEL;
		if( BYVel( Index ) < -MAXBEEVEL )
			BYVel( Index ) = -MAXBEEVEL;

		BeeX( 0, Index ) = BeeX( 1, Index ) + BXVel( Index );
		BeeY( 0, Index ) = BeeY( 1, Index ) + BYVel( Index );

		if(( BeeX( 0, Index ) < BORDER )||
		   ( BeeX( 0, Index ) > SP->ss_Width-BORDER-1 ))
		{
			BXVel( Index ) = -BXVel( Index );
			BeeX( 0, Index ) = BeeX( 1, Index ) + BXVel( Index );
		}

		if(( BeeY( 0, Index ) < BORDER )||
		   ( BeeY( 0, Index ) > SP->ss_Height-BORDER-1 ))
		{
			BYVel( Index ) = -BYVel( Index );
			BeeY( 0, Index ) = BeeY( 1, Index ) + BYVel( Index );
		}
	}

	for( Index = 0L; Index < SP->ss_NumWasps; Index++ )
	{
		SetAPen( RP, 0 );
		Move( RP, WaspX( 2, Index ), WaspY( 2, Index ));
		Draw( RP, WaspX( 1, Index ), WaspY( 1, Index ));
		SetAPen( RP, WASP_PEN );
		Draw( RP, WaspX( 0, Index ), WaspY( 0, Index ));
	}

	for( Index = 0L; Index < SP->ss_NumBees; Index++ )
	{
		SetAPen( RP, 0 );
		Move( RP, BeeX( 2, Index ), BeeY( 2, Index ));
		Draw( RP, BeeX( 1, Index ), BeeY( 1, Index ));
		SetAPen( RP, BEE_PEN );
		Draw( RP, BeeX( 0, Index ), BeeY( 0, Index ));
	}
}

LONG Blank( PrefObject *Prefs )
{
	LONG Count, Color, DColor, ScrToFrontCnt = 0, RetVal = OK;
	struct Screen *SwarmScreen;
	struct Window *Wnd;
	SwarmStruct *Swarms;

	SwarmScreen = OpenScreenTags( 0L, SA_Depth, 2, SA_Overscan, OSCAN_STANDARD,
								 SA_DisplayID, Prefs[MODE].po_ModeID,
								 SA_Quiet, TRUE, SA_Behind, TRUE, TAG_DONE );
	if( SwarmScreen )
	{
		SetRGB4( &( SwarmScreen->ViewPort ), 0, 0, 0, 0 );
		SetRast( &( SwarmScreen->RastPort ), 0 );
		Wnd = BlankMousePointer( SwarmScreen );
		ScreenToFront( SwarmScreen );
		Swarms = CreateSwarms( SwarmScreen, Prefs );
		if( Swarms )
		{
			Color = BEE_COL_NUM-1;
			DColor = Prefs[COLOR].po_Active ? 1 : 0;
			Count = Prefs[SPEED].po_Level + 1;
			
			while( RetVal == OK )
			{
				SwarmColors[BEE_PEN] = BeeColors[Color];
				LoadRGB4(&( SwarmScreen->ViewPort ), SwarmColors, 4 );
				Color += DColor;
				if(( Color == -1 )||( Color == BEE_COL_NUM ))
				{
					DColor = -DColor;
					Color += 2 * DColor;
				}
				
				WaitTOF();
				
				if(!( ScrToFrontCnt++ % 60 ))
					ScreenToFront( SwarmScreen );
					
				if( ++Count > MAX_SPEED )
				{
					Count = Prefs[SPEED].po_Level + 1;
					DrawSwarms(&( SwarmScreen->RastPort ), Swarms, Prefs );

				}

				RetVal = ContinueBlanking();
			}
			FreeVec( Swarms );
		}
		else
			RetVal = FAILED;
		
		UnblankMousePointer( Wnd );
		CloseScreen( SwarmScreen );
	}
	else
		RetVal = FAILED;

	return RetVal;
}
