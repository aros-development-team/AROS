/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include "/includes.h"

#include "LineBlanker_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Active = 0;
}

LONG RenderLines( struct Screen *LScr, USHORT Mode )
{
	struct RastPort *rp = &LScr->RastPort;
	int i, x, y, midx, midy, x2, y2;
	LONG RetVal = OK;
	
	SetAPen( rp, 1 );
	switch( Mode )
	{
	case 0:
		/* Cross
		 */
		midy = LScr->Height - 2 - ( LScr->Height - 1 ) % 2;
		midx = LScr->Width - 2 - ( LScr->Width - 1 ) % 2;
		for( i = 0; i < 4 && RetVal == OK; i += 2 )
		{
			for( x = i, x2 = midx - i;
				x < LScr->Width-1 &&(( RetVal = ContinueBlanking()) == OK );
				x += 4, x2 -= 4 )
			{
				Move( rp, x2, LScr->Height - 1 );
				Draw( rp, x, 0 );
				Move( rp, x2, 0 );
				Draw( rp, x, LScr->Height - 1 );
			}
			for( y = i, y2 = midy - i;
				y < LScr->Height-1 &&(( RetVal = ContinueBlanking()) == OK );
				y += 4, y2 -= 4 )
			{
				Move( rp, 0, y2 );
				Draw( rp, LScr->Width - 1, y );
				Move( rp, LScr->Width - 1, y2 );
				Draw( rp, 0, y );
			}
		}
		break;
	case 1:
		/* Cycle
		 */
		midx = LScr->Width / 2;
		midy = LScr->Height / 2;
		for( i = 0; i < 4; i++ )
		{
			for( x = i; x < LScr->Width; x += 4 )
			{
				Move( rp, midx, midy );
				Draw( rp, x, 0 );
			}
			if(( RetVal = ContinueBlanking()) != OK )
				break;
			for( y = i;	y < LScr->Height; y += 4 )
			{
				Move( rp, midx, midy );
				Draw( rp, LScr->Width - 1, y );
			}
			if(( RetVal = ContinueBlanking()) != OK )
				break;
			for( x = LScr->Width - 1 - i; x > 0; x -= 4 )
			{
				Move( rp, midx, midy );
				Draw( rp, x, LScr->Height - 1 );
			}
			if(( RetVal = ContinueBlanking()) != OK )
				break;
			for( y = LScr->Height - 1 - i; y > 0; y -= 4 )
			{
				Move( rp, midx, midy );
				Draw( rp, 0, y );
			}
			if(( RetVal = ContinueBlanking()) != OK )
				break;
		}
		break;
	case 2:
		/* Perimeter
		 */
		for( i = 0; i < 3 &&(( RetVal = ContinueBlanking()) == OK ); i++ )
		{
			for( x = i; x < LScr->Width; x += 3 )
			{
				Move(rp,0,LScr->Height-1);
				Draw(rp,x,0);
			}
			for( y = i; y < LScr->Height; y += 3 )
			{
				Move(rp,0,LScr->Height-1);
				Draw(rp,LScr->Width-1,y);
			}
			for( y = LScr->Height - 1 - i; y > 0; y -= 3 )
			{
				Move(rp,LScr->Width-1,LScr->Height-1);
				Draw(rp,0,y);
			}
			for( x = i; x < LScr->Width; x += 3 )
			{
				Move(rp,LScr->Width-1,LScr->Height-1);
				Draw(rp,x,0);
			}
			for( x = LScr->Width - 1 - i; x > 0; x -= 3 )
			{
				Move(rp,LScr->Width-1,0);
				Draw(rp,x,LScr->Height-1);
			}
			for( y = LScr->Height - 1 - i; y > 0; y -= 3 )
			{
				Move(rp,LScr->Width-1,0);
				Draw(rp,0,y);
			}
			for( y = i; y < LScr->Height; y += 3 )
			{
				Move(rp,0,0);
				Draw(rp,LScr->Width-1,y);
			}
			for( x = LScr->Width - 1 - i; x > 0; x -= 3 )
			{
				Move(rp,0,0);
				Draw(rp,x,LScr->Height-1);
			}
		}
		break;
	case 3:
		/* Propeller
		 */
		for( i = 0; i < 4 &&(( RetVal = ContinueBlanking()) == OK ); i++ )
		{
			for( x = i, x2 = LScr->Width - 1 - i; x < LScr->Width;
				x += 4, x2 -= 4 )
			{
				Move(rp,x2,LScr->Height-1);
				Draw(rp,x,0);
			}
			for( y = i, y2 = LScr->Height - 1 - i; y < LScr->Height;
				y += 4, y2 -= 4 )
			{
				Move(rp,0,y2);
				Draw(rp,LScr->Width-1,y);
			}
		}
		break;
	case 4:
		/* Square
		 */
		for( i = 0; i < 10 &&(( RetVal = ContinueBlanking()) == OK ); i++ )
		{
			for( x = i; x < LScr->Width; x += 20 )
			{
				Move(rp,x,0);
				Draw(rp,x,LScr->Height-1);
			}
			for( y = i; y < LScr->Height; y += 20 )
			{
				Move(rp,0,y);
				Draw(rp,LScr->Width-1,y);
			}
			for( x = 19 - i; x < LScr->Width; x += 20 )
			{
				Move(rp,x,0);
				Draw(rp,x,LScr->Height-1);
			}
			for( y = 19 - i; y < LScr->Height; y += 20 )
			{
				Move(rp,0,y);
				Draw(rp,LScr->Width-1,y);
			}
		}
		break;
	}

	return RetVal;
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *LScr;
	struct Window *Wnd;
	LONG RetVal;
	
	if( LScr = cloneTopScreen( FALSE, 0 ))
	{
		Wnd = BlankMousePointer( LScr );
		RetVal = RenderLines( LScr, Prefs[0].po_Active );
		while( RetVal == OK )
		{
			RetVal = ContinueBlanking();
			Delay( 5 );
		}
		UnblankMousePointer( Wnd );
		CloseScreen( LScr );
	}
	else
		RetVal = FAILED;

	return RetVal;
}
