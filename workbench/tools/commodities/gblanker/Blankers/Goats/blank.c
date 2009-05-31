/*
 *  Copyright (c) 1994 Steve "Goatboy" Akers
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

#define DELAY   0
#define HERDERS 2
#define GOATS   4
#define REPRO   6
#define SCREEN  8
#define MODE    10

#define GOAT 1
#define HERDER 2
#define GRASS 3

typedef struct _Position
{
	LONG x;
	LONG y;
} Position;

LONG myBlank(struct Screen *LScr, UWORD Width, UWORD Height);
void doGoats(struct RastPort *r);
void doHerders(struct RastPort *r);
LONG neighbor(struct RastPort *r, LONG dir);

Position	herders[512];
LONG		herderQ[512] = {0};
LONG		numHerders, herderClr;
Position	goats[512];
LONG		goatQ[512] = {0};
LONG		grassEaten[512] = {0};
LONG		numGoats, goatClr, reproduction;
LONG		x,y,tx,ty, backgroundClr, grassClr;
LONG		Width, Height;

#include "Goats_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[DELAY].po_Level = 0;
	Prefs[HERDERS].po_Level = 40;
	Prefs[GOATS].po_Level = 60;
	Prefs[REPRO].po_Level = 2;
	Prefs[SCREEN].po_Active = 0;
	Prefs[MODE].po_ModeID = getTopScreenMode();
}

LONG myBlank(struct Screen *LScr, UWORD Width, UWORD Height)
{
	struct RastPort *r;
	
	r = &( LScr->RastPort );
	doHerders(r);
	doGoats(r);
	
	return 0;
}

void doHerders(struct RastPort *r)
{
	LONG	ptr = numHerders-1;
	LONG	i,n,newX,newY,startDir,tmpDir;
	LONG	flag = 0;
	
	while( ptr >= 0 ) {
		if( herderQ[ptr] ) {
			flag = 1;
			x = herders[ptr].x;
			y = herders[ptr].y;
			SetAPen(r, grassClr);
			WritePixel(r, x, y);
			startDir = tmpDir  = (LONG)RangeRand(8);
			while (1) {
				n = neighbor(r, tmpDir);
				if (n == grassClr || n == -1) {
					tmpDir = (tmpDir+1) % 8;
					if (tmpDir == startDir) {
						herderQ[ptr] = 0;
						break;
					}
				}
				else {
					herders[ptr].x = newX = tx;
					herders[ptr].y = newY = ty;
					i = 0;
					while(herderQ[i] && i < numHerders)
						++i;
					if (i != numHerders) {
						herders[i].x = newX;
						herders[i].y = newY;
						herderQ[i] = 1;
					}
					SetAPen(r,herderClr);
					WritePixel(r,newX,newY);
					break;
				}
			}
		}
		--ptr;
	}
	if (!flag) {
		herderQ[0] = 1;
		herders[0].x = (LONG)RangeRand(Width-1);
		herders[0].y = (LONG)RangeRand(Height-1);
	}
}

void doGoats(struct RastPort *r)
{
	LONG	ptr = numGoats-1;
	LONG	i,n,newX,newY,startDir,tmpDir;
	LONG	flag = 0;

	while (ptr >= 0) {
		if (goatQ[ptr] ) {
			flag = 1;
			x = goats[ptr].x;
			y = goats[ptr].y;
			SetAPen(r, backgroundClr);
			WritePixel(r, x, y);
			startDir = tmpDir  = (LONG)RangeRand(8);
			while (1) {
				n = neighbor(r, tmpDir);
				if (n != grassClr || n == -1) {
					tmpDir = (tmpDir+1) % 8;
					if (tmpDir == startDir) {
						goatQ[ptr] = 0;
						break;
					}
				}
				else {
					goats[ptr].x = newX = tx;
					goats[ptr].y = newY = ty;
					++grassEaten[ptr];
					if (grassEaten[ptr] >= reproduction) {
						grassEaten[ptr] = 0;
						i = 0;
						while(goatQ[i] != 0 && i < numGoats)
							++i;
						if (i != numGoats) {
							goats[i].x = newX;
							goats[i].y = newY;
							goatQ[i] = 1;
							grassEaten[i] = 0;
						}
					}
					SetAPen(r,goatClr);
					WritePixel(r,newX,newY);
					break;
				}
			}
		}
		--ptr;
	}
	if (!flag) {
		goatQ[0] = 1;
		goats[0].x = (LONG)RangeRand(Width-1);
		goats[0].y = (LONG)RangeRand(Height-1);
		grassEaten[0] = 0;
	}
}

LONG neighbor(struct RastPort *r, LONG dir)
{
	switch (dir) {
	case 0:tx = x;ty = y+1;break;
	case 1:tx = x+1;ty = y+1;break;
	case 2:tx = x+1;ty = y;break;
	case 3:tx = x+1;ty = y-1;break;
	case 4:tx = x;ty = y-1;break;
	case 5:tx = x-1;ty = y-1;break;
	case 6:tx = x-1;ty = y;break;
	case 7:tx = x-1;ty = y+1;break;
	default: return -1;break;
	}
	if (tx < 0 || tx >= Width || ty < 0 || ty >= Height)
		return -1; 
	
	return (LONG)ReadPixel(r, tx, ty);
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *LScr;
	struct Window *Wnd;
	LONG RetVal = OK,i,goatFlag = 0, ToFrontCount = 0;
	
	if (Prefs[SCREEN].po_Active )
		LScr = cloneTopScreen( FALSE, TRUE );
	else
		LScr = OpenScreenTags( 0l, SA_DisplayID, Prefs[MODE].po_ModeID,
							  SA_Depth, 2, SA_Overscan, OSCAN_STANDARD,
							  SA_Quiet, TRUE, SA_Behind, TRUE, TAG_DONE );
	if( LScr )
	{
		if( GarshnelibBase->lib_Version < 39 || !Prefs[SCREEN].po_Active )
		{
			SetRGB4(&(LScr->ViewPort),0,0x0,0x0,0x0);
			SetRGB4(&(LScr->ViewPort),1,0x8,0x8,0x8);
			SetRGB4(&(LScr->ViewPort),2,0x7,0x4,0x2);
			SetRGB4(&(LScr->ViewPort),3,0x0,0xa,0x0);
			backgroundClr = 0;
			grassClr = GRASS;
			herderClr = HERDER;
			goatClr = GOAT;
		}
		else
		{
			backgroundClr = FindColor( LScr->ViewPort.ColorMap, 0, 0, 0, -1 );
			grassClr = FindColor( LScr->ViewPort.ColorMap, 0, 0x9L<<28, 0,
								 -1 );
			goatClr = FindColor( LScr->ViewPort.ColorMap, 0xAL<<28, 0xAL<<28,
								0xAL<<28, -1 );
			herderClr = FindColor( LScr->ViewPort.ColorMap, 0xBL<<28, 0x2L<<28,
								  0x4L<<28, -1 );
		}
	
		numGoats = Prefs[GOATS].po_Level;
		numHerders = Prefs[HERDERS].po_Level;
		reproduction = Prefs[REPRO].po_Level;
		Width = LScr->Width;
		Height = LScr->Height;
		
		for (i=0;i<numHerders;++i)
			herderQ[i] = 0;
		herderQ[0] = 1;
		herders[0].x = Width/2;
		herders[0].y = Height/2;
		for (i=0;i<numGoats;++i)
			goatQ[i] = 0;
		
		Wnd = BlankMousePointer( LScr );
		ScreenToFront( LScr );
		i = 0;
		while( RetVal == OK )
		{
			WaitTOF();

			if(!( ++ToFrontCount % 60 ))
				ScreenToFront( LScr );
			
			if( !Prefs[DELAY].po_Level ||
			   !( ToFrontCount % Prefs[DELAY].po_Level ))
			{
				myBlank( LScr, LScr->Width, LScr->Height );
				if (!goatFlag && i == 20)
				{
					goatQ[0] = 1;
					goats[0].x = Width/2;
					goats[0].y = Height/2;
					grassEaten[0] = 0;
					goatFlag = 1;
				}
				else
					++i;
			}

			RetVal = ContinueBlanking();
		}

		UnblankMousePointer( Wnd );
		CloseScreen( LScr );
	}
	else
		RetVal = FAILED;
	
	return RetVal;
}

