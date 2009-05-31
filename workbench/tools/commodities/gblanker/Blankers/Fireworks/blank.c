/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <hardware/custom.h>
#include "/includes.h"

#define SCALE 100
#define ACCEL 4

#define	MAX_FIRE       200
#define	MAX_EXPLOSIONS 25

#define	NONE      0
#define	FIRE      1
#define	EXPLODING 2

extern __far struct Custom custom;

struct Fire
{
	BYTE type;
	BYTE time;
	ULONG x;
	ULONG y;
	LONG vx;
	LONG vy;
	LONG color;
};

#define NUMBER 0
#define POWER  2
#define RADIUS 4
#define MODE   6

struct Fire *FireTable, *LastFire;
LONG MaxFires, NumFires, Power, Radius, Wid, Hei;

#include "Fireworks_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[NUMBER].po_Level = 16;
	Prefs[POWER].po_Level = 5;
	Prefs[RADIUS].po_Level = 6;
	Prefs[MODE].po_ModeID = getTopScreenMode();
	Prefs[MODE].po_Depth = 2;
}

LONG CheckFire( struct Fire *f )
{
	if( f->x/SCALE >= Wid )
		return 1;
	
	if( f->y/SCALE >= Hei )
		return 1;
	
	return 0 ;
}

VOID IterateFire( struct RastPort *Rast )
{
	LONG l;
	struct Fire *f, *tf;
	
	if(( NumFires < MaxFires )&&( RangeRand( 100 ) > 90 ))
	{
		for( tf = FireTable; tf->type; tf++ );
		if( tf != LastFire )
		{
			tf->type = FIRE;
			tf->x = ( Wid / 4 + RangeRand( Wid / 2 )) * SCALE;
			tf->y = ( Hei - 5 ) * SCALE;
			tf->color = RangeRand(( 1L << Rast->BitMap->Depth ) - 1 ) + 1;
			tf->vx = RangeRand( SCALE * Hei / 100 ) - SCALE;
			tf->vy = -1 * ( Power * SCALE * Hei / 400 );
			tf->time = RangeRand( 100 ) + 100;
			NumFires++;
		}
	}
	
	for( f = FireTable; f != LastFire; f++ )
	{
		switch( f->type )
		{
		case NONE:
			break;
		case FIRE:
			if( !f->time )
			{
				for( l = 0, tf = FireTable; ( l < MAX_EXPLOSIONS )&&
					( tf != LastFire ); tf++ )
				{
					if( !tf->type )
					{
						CopyMem( f, tf, sizeof( struct Fire ));
						tf->type = EXPLODING;
						tf->vx = ( RangeRand( 100 ) - 50 ) *
							( Radius * Wid / 1000 );
						tf->vy = ( RangeRand( 100 ) - 50 ) *
							( Radius * Hei / 1000 );
						tf->time = RangeRand( 10 ) + 50;
						l++;
					}
				}
				NumFires--;
				f->type = NONE;
			}
			else
			{
				SetAPen( Rast, 0 );
				WritePixel( Rast, f->x/SCALE, f->y/SCALE );
				f->x += f->vx;
				f->y += f->vy;
				if( CheckFire( f ))
				{
					f->type = NONE;
					NumFires--;
				}
				else
				{
					SetAPen( Rast, f->color );
					WritePixel( Rast, f->x/SCALE, f->y/SCALE );
					f->vy += ACCEL;
					f->time--;
				}
			}
			break;
		case EXPLODING:
			SetAPen( Rast, 0 );
			WritePixel( Rast, f->x/SCALE, f->y/SCALE );
			if( !f->time )
				f->type = NONE;
			else
			{
				f->x += f->vx;
				f->y += f->vy;
				if( CheckFire( f ))
					f->type = NONE;
				else
				{
					SetAPen( Rast, f->color );
					WritePixel( Rast, f->x/SCALE, f->y/SCALE );
					f->vy += ACCEL;
					f->time--;
				}
			}
			break;
		}
	}
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *Scr;
	struct Window *Wnd;
	LONG ToFrontCount = 0, RetVal = OK;
	
	MaxFires = Prefs[NUMBER].po_Level;
	Power = Prefs[POWER].po_Level;
	Radius = Prefs[RADIUS].po_Level;
	
	FireTable = AllocVec( sizeof( struct Fire ) * ( MAX_FIRE + 1 ),
						 MEMF_CLEAR );
	
	if( FireTable )
	{
		Scr = OpenScreenTags( 0L, SA_Depth, Prefs[MODE].po_Depth,
							 SA_Overscan, OSCAN_STANDARD, SA_Quiet, TRUE,
							 SA_DisplayID, Prefs[MODE].po_ModeID,
							 SA_Behind, TRUE, TAG_DONE );
		if( Scr )
		{
			LastFire = &( FireTable[MAX_FIRE] );
			
			Wid = Scr->Width;
			Hei = Scr->Height;
			
			if( Prefs[MODE].po_Depth < 3 )
				setCopperList( Hei, 1, &( Scr->ViewPort ), &custom );
			
			SetRGB4(&( Scr->ViewPort ), 0, 0, 0, 0 );
			SetRGB4(&( Scr->ViewPort ), 1, 0x0F, 0x0F, 0x0F );
			SetRast(&( Scr->RastPort ), 0 );
			
			NumFires = 0;
			Wnd = BlankMousePointer( Scr );
			ScreenToFront( Scr );
			
			while( RetVal == OK )
			{
				WaitTOF();
				IterateFire(&( Scr->RastPort ));
				if(!( ToFrontCount++ % 60 ))
					ScreenToFront( Scr );
				if(!( ToFrontCount % 5 ))
					RetVal = ContinueBlanking();
			}
			
			UnblankMousePointer( Wnd );
			if( Prefs[MODE].po_Depth < 3 )
				clearCopperList( &Scr->ViewPort );
			CloseScreen( Scr );
		}
		else
			RetVal = FAILED;

		FreeVec( FireTable );
	}
	else
		RetVal = FAILED;
	
	return RetVal;
}
