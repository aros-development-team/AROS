/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>

#include "/includes.h"

#ifdef _M68020
#define PREFOBJ LONG
#define UPREFOBJ ULONG
#else
#define PREFOBJ WORD
#define UPREFOBJ UWORD
#endif

#define HSIZE ( sizeof( PREFOBJ ) * 8 )
#define HCELLS ( 2 * HSIZE )

#define DELAY 0
#define GENS  2
#define DENS  4
#define MODE  6

PREFOBJ *Org[2], *Orgs[2], *Lookup;
LONG uw, x1pos, x2pos, ypos, offx, BWid, Hei;
Triplet *ColorTable = 0L;

#define RAND( base, offset ) (( ULONG )( RangeRand( base ) + offset ))
#define ORG( v, x, y ) ( Org[v][Lookup[y]+x] )
#define VALUE (( i < HSIZE )? 0L : 1L )

UBYTE mask[] = { 96, 240, 240, 96 };
#define RectFill( r, x1, y1, x2, y2 ) BltPattern( r, 0L, x1, y1, x2, y2, 0 )

#include "Life_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[DELAY].po_Level = 0;
	Prefs[GENS].po_Level = 150;
	Prefs[DENS].po_Level = 16;
	Prefs[MODE].po_ModeID = getTopScreenMode();
	Prefs[MODE].po_Depth = 4;
}

VOID SpewRandomShit( PREFOBJ *Org[], PREFOBJ *Orgs[], LONG v )
{
	LONG x = RAND( HCELLS-4, 2 ), y = RAND( Hei-4, 2 ), i, j;

	for( j = y; j < y+3; j++ )
		for( i = x; i < x+3; i++ )
			if( RAND( 100, 0 ) > 25 )
			{
				ORG( v, i, j ) = 1;
				if( i < HSIZE )
					Orgs[v][2*j] |= ( 1L << ( (HSIZE-1) - i ));
				else
					Orgs[v][2*j+1] |= ( 1L << ( ( HSIZE - 1 ) - i ));
			}
}

VOID FillWithRandomShit( struct RastPort *r, PREFOBJ *Org[], PREFOBJ *Orgs[],
						LONG v, LONG Density )
{
	LONG i, i1, j;
	UPREFOBJ flag;
	
	SetAPen( r, 2L );

	for( ypos = 0, j = 0; j < Hei; j++, ypos += uw )
	{
		for( x1pos = offx, x2pos = offx+(uw*HSIZE), i = 0, i1 = HSIZE,
			flag = 1L << (HSIZE-1);	i < HSIZE; i++, i1++, x1pos += uw,
			x2pos += uw, flag = ( 1L << (HSIZE-1) - i ))
		{
			if( RangeRand( 100 ) < Density )
			{
				ORG( v, i, j ) = 1;
				Orgs[v][2*j] |= flag;
				RectFill( r, x1pos, ypos, x1pos + BWid, ypos + BWid );
			}
			if( RangeRand( 100 ) < Density )
			{
				ORG( v, i1, j ) = 1;
				Orgs[v][2*j+1] |= flag;
				RectFill( r, x2pos, ypos, x2pos + BWid, ypos + BWid );
			}
		}
	}
}

#define TOP (Hei*2-2)
VOID Grow( PREFOBJ *Orgs, PREFOBJ *OrgsN )
{
	LONG i, j;
	UPREFOBJ mask = 1L << (HSIZE-1), one = 1;
	
	CopyMemQuick( Orgs, OrgsN, 2 * Hei * sizeof( PREFOBJ ));

	if( Orgs[0] & one )
		OrgsN[1] |= mask;
	Orgs[0] |= ( Orgs[0] >> 1 );
		if( Orgs[1] & one )
			OrgsN[0] |= mask;
	Orgs[1] |= ( Orgs[1] >> 1 );
	if( Orgs[0] & mask )
		OrgsN[1] |= one;
	Orgs[0] |= ( Orgs[0] << 1 );
	if( Orgs[1] & mask )
		OrgsN[0] |= one;
	OrgsN[1] |= ( Orgs[1] << 1 );
	OrgsN[0] |= Orgs[TOP];
	OrgsN[0] |= Orgs[2];
	OrgsN[1] |= Orgs[TOP+1];
	OrgsN[1] |= Orgs[3];

	for( i = 2, j = 3; i < TOP; i += 2, j += 2 )
	{
		OrgsN[i] |= ( Orgs[i] >> 1 );
		OrgsN[j] |= ( Orgs[j] >> 1 );
		OrgsN[i] |= ( Orgs[i] << 1 );
		OrgsN[j] |= ( Orgs[j] << 1 );
		if( Orgs[i] & one )
			OrgsN[j] |= mask;
		if( Orgs[j] & one )
			OrgsN[i] |= mask;
		if( Orgs[i] & mask )
			OrgsN[j] |= one;
		if( Orgs[j] & mask )
			OrgsN[i] |= one;
		OrgsN[i] |= Orgs[i-2];
		OrgsN[i] |= Orgs[i+2];
		OrgsN[j] |= Orgs[j-2];
		OrgsN[j] |= Orgs[j+2];
	}

	if( Orgs[TOP] & one )
		OrgsN[TOP+1] |= mask;
	OrgsN[TOP] |= ( Orgs[TOP] >> 1 );
	if( Orgs[TOP+1] & one )
		OrgsN[TOP] |= mask;
	OrgsN[TOP+1] |= ( Orgs[TOP+1] >> 1 );
	if( Orgs[TOP] & mask )
		OrgsN[TOP+1] |= one;
	OrgsN[TOP] |= ( Orgs[TOP] << 1 );
	if( Orgs[TOP+1] & mask )
		OrgsN[TOP] |= one;
	OrgsN[TOP+1] |= ( Orgs[TOP+1] << 1 );
	OrgsN[TOP] |= Orgs[TOP-2];
	OrgsN[TOP] |= Orgs[0];
	OrgsN[TOP+1] |= Orgs[TOP-1];
	OrgsN[TOP+1] |= Orgs[1];
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *LScr;
	struct Window *Wnd;
	struct RastPort *r;
	LONG Gens, RetVal = OK, colrand, maxcol, iter = 1, nbrs, n = 0, m = 1;
	LONG tofront = 0, i, i1, gi, li, j, gj, lj;
	UPREFOBJ flag;
	
	colrand = ( 1L << Prefs[MODE].po_Depth ) - 1;
	if( Prefs[MODE].po_Depth > 2 )
		maxcol = colrand - (1L<<Prefs[MODE].po_Depth)/6;
	else
		maxcol = colrand;
	
	Gens = Prefs[GENS].po_Level;
	
	LScr = OpenScreenTags( 0l, SA_DisplayID, Prefs[MODE].po_ModeID,
						  SA_Depth, Prefs[MODE].po_Depth,
						  SA_Quiet, TRUE, SA_Behind, TRUE,
						  SA_Overscan, OSCAN_TEXT, TAG_DONE );
	if( LScr )
	{
		r = &( LScr->RastPort );
		uw = LScr->Width / HCELLS;
		offx = ( LScr->Width - ( uw * HCELLS ))/ 2;
		Hei = LScr->Height / uw;
		BWid = uw-2;
		
		Org[0] = AllocVec( sizeof( PREFOBJ ) * HCELLS * Hei, MEMF_CLEAR );
		Org[1] = AllocVec( sizeof( PREFOBJ ) * HCELLS * Hei, MEMF_CLEAR );
		Orgs[0] = AllocVec( sizeof( PREFOBJ ) * 2 * Hei, MEMF_CLEAR );
		Orgs[1] = AllocVec( sizeof( PREFOBJ ) * 2 * Hei, MEMF_CLEAR );
		Lookup = AllocVec( sizeof( PREFOBJ ) * Hei, MEMF_CLEAR );
		
		if( Org[0] && Org[1] && Orgs[0] && Orgs[1] && Lookup )
		{
			SetRGB4(&( LScr->ViewPort ), 0, 0, 0, 0 );
			if( Prefs[MODE].po_Depth > 1 )
				ColorTable = RainbowPalette( LScr, 0L, 1L, 0L );
			else
				SetRGB4(&( LScr->ViewPort ), 1, 0xF, 0xF, 0xF );
			Wnd = BlankMousePointer( LScr );
			ScreenToFront( LScr );
			
			for( i = 1; i < Hei; i++ )
				Lookup[i] = Lookup[i-1] + HCELLS;
			
			FillWithRandomShit( r, Org, Orgs, n, Prefs[DENS].po_Level );
			
			while( RetVal == OK )
			{
				WaitTOF();

				if(!( tofront++ % 60 ))
					ScreenToFront( LScr );

				if( !Prefs[DELAY].po_Level ||
				   !( tofront % Prefs[DELAY].po_Level ))
				{
					if( !iter )
						SpewRandomShit( Org, Orgs, n );
					
					Grow( Orgs[n], Orgs[m] );

					CopyMemQuick( Org[n], Org[m],
								 HCELLS * Hei * sizeof( PREFOBJ ));
					
					for( ypos = 0, j = 0; j < Hei; j++, ypos += uw )
					{
						for( x1pos = offx, x2pos = offx+(uw*HSIZE), i = 0,
							i1 = HSIZE; i < HSIZE;
							i++, i1++, x1pos += uw, x2pos += uw )
						{
							flag = ( UPREFOBJ )( 1L << ( HSIZE - 1 - i ));
							if( Orgs[m][2*j] & flag )
							{
								gi = i + 1;
								li = ( i + HCELLS - 1 )%HCELLS;
								gj = ( j + 1 )%Hei;
								lj = ( j + Hei - 1 )%Hei;
								nbrs = 0;
								if( ORG( n, gi, gj )) nbrs++;
								if( ORG( n, i, gj )) nbrs++;
								if( ORG( n, li, gj )) nbrs++;
								if( ORG( n, gi, j )) nbrs++;
								if( ORG( n, li, j )) nbrs++;
								if( ORG( n, gi, lj )) nbrs++;
								if( ORG( n, i, lj )) nbrs++;
								if( ORG( n, li, lj )) nbrs++;
								switch( nbrs )
								{
								case 3:
									if( ORG( m, i, j ) < maxcol ) {
										SetAPen( r, ( LONG )++ORG( m, i, j ));
										RectFill( r, x1pos, ypos, x1pos + BWid,
												 ypos + BWid );
									} break;
								case 2:
									if( ORG( m, i, j )) {
										if( ORG( m, i, j ) < maxcol ) {
											SetAPen( r,
													( LONG )++ORG( m, i, j ));
											RectFill( r, x1pos, ypos, x1pos +
													 BWid, ypos + BWid );
										}
									} else
										Orgs[m][2*j] &= ~flag;
									break;
								default:
									ORG( m, i, j ) = 0;
									Orgs[m][2*j] &= ~flag;
									if( ORG( n, i, j ))	{
										SetAPen( r, 0L );
										RectFill( r, x1pos, ypos, x1pos + BWid,
												 ypos + BWid );
									}
								}
							}
							else
								ORG( m, i, j ) = 0;
							if( Orgs[m][2*j+1] & flag )
							{
								gi = ( i1 + 1 )%HCELLS;
								li = ( i1 - 1 )%HCELLS;
								gj = ( j + 1 )%Hei;
								lj = ( j + Hei - 1 )%Hei;
								nbrs = 0;
								if( ORG( n, gi, gj )) nbrs++;
								if( ORG( n, i1, gj )) nbrs++;
								if( ORG( n, li, gj )) nbrs++;
								if( ORG( n, gi, j )) nbrs++;
								if( ORG( n, li, j )) nbrs++;
								if( ORG( n, gi, lj )) nbrs++;
								if( ORG( n, i1, lj )) nbrs++;
								if( ORG( n, li, lj )) nbrs++;
								switch( nbrs )
								{
								case 3:
									if( ORG( m, i1, j ) < maxcol )
									{
										SetAPen( r, ( LONG )++ORG( m, i1, j ));
										RectFill( r, x2pos, ypos, x2pos + BWid,
												 ypos + BWid );
									} break;
								case 2:
									if( ORG( m, i1, j ))
									{
										if( ORG( m, i1, j ) < maxcol )
										{
											SetAPen( r,
													( LONG )++ORG( m, i1, j ));
											RectFill( r, x2pos, ypos, x2pos +
													 BWid, ypos + BWid );
										}
									}
									else
									Orgs[m][2*j+1] &= ~flag;
									break;
								default:
									ORG( m, i1, j ) = 0;
									Orgs[m][2*j+1] &= ~flag;
									if( ORG( n, i1, j ))
									{
										SetAPen( r, 0L );
										RectFill( r, x2pos, ypos, x2pos + BWid,
												 ypos + BWid );
									}
								}
							}
							else
								ORG( m, i1, j ) = 0;
						}
					}
					
					n = m;
					m = 1 - m;
					iter = (iter+1) % Gens;
				}
				RetVal = ContinueBlanking();
			}
			if( Prefs[MODE].po_Depth )
				RainbowPalette( 0L, ColorTable, 1L, 0L );
			UnblankMousePointer( Wnd );
			CloseScreen( LScr );
		}
		else
			RetVal = FAILED;

		if( Org[0] )
			FreeVec( Org[0] );
		if( Org[1] )
			FreeVec( Org[1] );
		if( Orgs[0] )
			FreeVec( Orgs[0] );
		if( Orgs[1] )
			FreeVec( Orgs[1] );
		if( Lookup )
			FreeVec( Lookup );
	}
	else
		RetVal = FAILED;
	
	return RetVal;
}
