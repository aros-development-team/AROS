/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

#define	IM_WIDTH  64
#define	IM_HEIGHT 64
#define IM_WIDBUF 84
#define IM_HEIBUF 84

#define OBJECTS 0
#define SPEED   2
#define MODE    4

typedef struct _Toaster
{
	LONG delay;
	LONG x, y;
	LONG old_x, old_y;
	LONG xspeed, yspeed;
	LONG phase;
	LONG xcol, ycol;
} Toaster;

Toaster *Toasters;
LONG NumToasters;

#include "images.h"

ULONG cmap[] = {
	0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x55, 0x55, 0x55, 0x88, 0x55, 0x22,
	0x66, 0x66, 0x66, 0x77, 0x77, 0x44, 0x55, 0x55, 0x66, 0xEE, 0x55, 0x22,
	0x88, 0x88, 0x88, 0x99, 0x99, 0x99, 0xBB, 0xBB, 0xBB, 0xCC, 0xCC, 0xCC,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xEE, 0xEE, 0xFF, 0xFF, 0xFF };

#define abs( x ) ( (x) < 0 ? -(x) : (x) )

#include "FlyingToaster_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[OBJECTS].po_Level = 3;
	Prefs[SPEED].po_Level = 6;
	Prefs[MODE].po_ModeID = getTopScreenMode();
}

LONG TestExtent( LONG Start, LONG Me, LONG x, LONG y )
{
	LONG i;
	
	for( i = Start; i < NumToasters; i++ )
		if(( i != Me )&& !Toasters[i].delay &&
		   ( abs( x - Toasters[i].x ) < IM_WIDBUF )&&
		   ( abs( y - Toasters[i].y ) < IM_HEIBUF ))
			return i;
	
	return -1L;
}

BOOL FindLaunchPos( LONG i, LONG wid, LONG hei )
{
	LONG x, y;
	
	x = wid;
	y = RangeRand( hei );
	
	FOREVER
	{
		if( x <= 2 * IM_WIDTH )
			return FALSE;
		
		if( TestExtent( 0, i, x, y ) == -1 )
		{
			Toasters[i].x = Toasters[i].old_x = x;
			Toasters[i].y = Toasters[i].old_y = y;
			Toasters[i].xspeed = RangeRand( 3 ) + 1;
			Toasters[i].yspeed = RangeRand( 2 ) + 1;
			Toasters[i].phase = RangeRand( IMAGEMAX );

			return TRUE;
		}
		
		if( y > -IM_HEIGHT )
			y -= IM_HEIGHT;
		else
			x -= IM_WIDTH;
	}
}

LONG ActXSpeed( LONG i )
{
	if( Toasters[i].xcol == -1 )
		return Toasters[i].xspeed;
	else
		return ActXSpeed( Toasters[i].xcol );
}

LONG ActYSpeed( LONG i )
{
	if( Toasters[i].ycol == -1 )
		return Toasters[i].yspeed;
	else
		return ActXSpeed( Toasters[i].ycol );
}

LONG Blank( PrefObject *Prefs )
{
	LONG delay_rate, i, j, counter = 0, RetVal = OK;
	struct RastPort *Rast;
	struct Screen *FTScr;
	struct Window *FTWin;
	struct Window *Wnd;
	
	NumToasters = Prefs[OBJECTS].po_Level;
	delay_rate = 11 - Prefs[SPEED].po_Level;
	
	Toasters = AllocVec( sizeof( Toaster ) * NumToasters, MEMF_CLEAR );
	FTScr = OpenScreenTags( NULL, SA_DisplayID, Prefs[MODE].po_ModeID,
						   SA_Depth, 4, SA_Overscan, OSCAN_STANDARD,
						   SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
						   SA_Behind, TRUE, TAG_DONE );
	if( Toasters && FTScr )
	{
		for( i = 0; i < 48; i += 3 )
			if( GfxBase->lib_Version < 39 )
				SetRGB4(&( FTScr->ViewPort ), i/3, ( cmap[i] * 16 ) / 256,
						( cmap[i+1] * 16 ) / 256, ( cmap[i+2] * 16 ) / 256 );
			else
				SetRGB32(&( FTScr->ViewPort ), i/3, cmap[i]<<24, cmap[i+1]<<24,
						 cmap[i+2]<<24 );
		FTWin = OpenWindowTags( 0L, WA_Width, FTScr->Width, WA_Height,
							   FTScr->Height, WA_IDCMP, 0L, WA_Flags,
							   WFLG_SIMPLE_REFRESH|WFLG_BORDERLESS,
							   WA_CustomScreen, FTScr, TAG_DONE );
		if( FTWin )
		{
			Rast = FTWin->RPort;
			SetAPen( Rast, 0 );
			
			for( i = 0; i < NumToasters; i++ )
				if( !FindLaunchPos( i, ( LONG )FTWin->Width,
								   ( LONG )FTWin->Height ))
					Toasters[i].delay = 30;
			
			Wnd = BlankMousePointer( FTScr );
			ScreenToFront( FTScr );
			
			while( RetVal == OK )
			{
				WaitTOF();
				
				if(!( ++counter % 60 ))
					ScreenToFront( FTScr );
				
				if(!( counter % delay_rate ))
				{
					for( i = 0; i < NumToasters; i++ )
					{
						if( !Toasters[i].delay )
						{
							Toasters[i].old_x = Toasters[i].x;
							Toasters[i].old_y = Toasters[i].y;
							Toasters[i].x -= Toasters[i].xspeed;
							Toasters[i].y += Toasters[i].yspeed;
							Toasters[i].xcol = -1;
							Toasters[i].ycol = -1;
						}
					}

					for( i = 0; i < NumToasters; i++ )
					{
						if( !Toasters[i].delay )
						{
							j = -1;
							while(( j = TestExtent( j+1, i, Toasters[i].x,
												   Toasters[i].y )) >= 0 )
							{
								if( abs( Toasters[j].old_x -
										Toasters[i].old_x ) < IM_WIDBUF )
								{
									if( Toasters[i].y < Toasters[j].y )
										Toasters[i].ycol = j;
									if( Toasters[i].xspeed ==
									   Toasters[j].xspeed )
										Toasters[i].xspeed++;
								}
								else
								{
									if( Toasters[i].x > Toasters[j].x )
										Toasters[i].xcol = j;
									if( Toasters[i].yspeed ==
									   Toasters[j].yspeed )
										Toasters[i].yspeed++;
								}
								if( abs( Toasters[j].old_y -
										Toasters[i].old_y ) < IM_HEIBUF )
								{
									if( Toasters[i].x > Toasters[j].x )
										Toasters[i].xcol = j;
									if( Toasters[i].yspeed ==
									   Toasters[j].yspeed )
										Toasters[i].yspeed++;
								}
							}
						}
					}

					for( i = 0; i < NumToasters; i++ )
					{
						if( !Toasters[i].delay )
						{
							Toasters[i].x = Toasters[i].old_x - ActXSpeed( i );
							Toasters[i].y = Toasters[i].old_y + ActYSpeed( i );
						}
					}
					
					for( i = 0; i < NumToasters; i++ )
					{
						if( !Toasters[i].delay )
						{
							j = -1;
							while(( j = TestExtent( j+1, i, Toasters[i].x,
												   Toasters[i].y )) >= 0 )
							{
								if( abs( Toasters[j].old_x -
										Toasters[i].old_x ) < IM_WIDBUF )
									if( Toasters[i].x > Toasters[j].x )
										Toasters[i].x = Toasters[i].old_x;
								else
									if( Toasters[i].y < Toasters[j].y )
										Toasters[i].y = Toasters[i].old_y;
								if( abs( Toasters[j].old_y -
										Toasters[i].old_y ) < IM_HEIBUF )
									if( Toasters[i].y < Toasters[j].y )
										Toasters[i].y = Toasters[i].old_y;
							}
						}
					}

					for( i = 0; i < NumToasters; i++ )
					{
						if( !Toasters[i].delay )
						{
							Toasters[i].phase =
								( Toasters[i].phase + 1 ) % IMAGEMAX;
							EraseRect( Rast, Toasters[i].x + IM_WIDTH,
									  Toasters[i].old_y, Toasters[i].x +
									  IM_WIDTH + Toasters[i].xspeed,
									  Toasters[i].old_y + IM_HEIGHT );
							EraseRect( Rast, Toasters[i].old_x,
									  Toasters[i].old_y,
									  Toasters[i].old_x + IM_WIDTH,
									  Toasters[i].old_y + Toasters[i].yspeed );
							DrawImage( Rast, img[Toasters[i].phase],
									  Toasters[i].x, Toasters[i].y );
							if(( Toasters[i].x < -IM_WIDTH-1 )||
							   ( Toasters[i].y > FTWin->Height ))
								Toasters[i].delay = RangeRand( 50 );
						}
						else
							if(!( --Toasters[i].delay ))
								Toasters[i].delay =
									FindLaunchPos( i, ( LONG )FTWin->Width,
												  ( LONG )FTWin->Height )
										? 0 : 30;
					}
				}
				RetVal = ContinueBlanking();
			}

			SetSignal( 0L, SIGBREAKF_CTRL_C );
			UnblankMousePointer( Wnd );
			CloseWindow( FTWin );
		}
		else
			RetVal = FAILED;
		CloseScreen( FTScr );
	}
	else
		RetVal = FAILED;
	
	FreeVec( Toasters );

	return RetVal;
}
