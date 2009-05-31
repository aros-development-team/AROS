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

#define CELLSIZE 0
#define SOLVEDEL 2
#define COPPER   4
#define MODE     6

extern __far struct Custom custom;

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3

#define HasBeenVisited( Bob, x, y )\
( (Bob)->mz_Cells[(x)*(Bob)->mz_Height+(y)] )
#define Visit( Bob, x, y )\
( (Bob)->mz_Cells[(x)*(Bob)->mz_Height+(y)] = 1 )

LONG DirArray[] = {
	0x00010203, 0x00010302, 0x00020103, 0x00020301, 0x00030102, 0x00030201,
	0x01000203, 0x01000302, 0x01020003, 0x01020300, 0x01030002, 0x01030200,
	0x02000103, 0x02000301, 0x02010003, 0x02010300, 0x02030001, 0x02030100,
	0x03000102, 0x03000201, 0x03010002, 0x03010200, 0x03020001, 0x03020100
	};

typedef struct _Stack
{
	LONG st_x;
	LONG st_y;
	BYTE st_DirOne;
	BYTE st_DirTwo;
	BYTE st_DirThree;
	BYTE st_DirFour;
	struct _Stack *st_Prev;
} Stack;

typedef struct _Maze
{
	struct RastPort *mz_Rast;
	LONG mz_Width;
	LONG mz_Height;
	LONG mz_WidStep;
	LONG mz_HeiStep;
	BYTE *mz_Cells;
	BYTE *mz_LeftWalls;
	BYTE *mz_TopWalls;
	Stack *mz_Stack;
} Maze;

LONG StackLength, SolutionLength, NumCols, Copper;
LONG Iter, SolPeriod, SolDelay;
Triplet *ColorTable = 0L;

#include "Maze_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[CELLSIZE].po_Level = 7;
        Prefs[SOLVEDEL].po_Level = 3;
	Prefs[COPPER].po_Active = 1;
	Prefs[MODE].po_ModeID = getTopScreenMode();
	Prefs[MODE].po_Depth = 2;
}

Stack *AllocStack( LONG x, LONG y, BYTE *Directions )
{
	Stack *New = AllocVec( sizeof( Stack ), MEMF_ANY );
	
	if( New )
	{
		New->st_x = x;
		New->st_y = y;
		CopyMem( Directions, &( New->st_DirOne ), 4 * sizeof( BYTE ));
		New->st_Prev = 0L;
	}
	
	return New;
}

#define Push( b, c )\
StackLength++; (c)->st_Prev = (b)->mz_Stack; (b)->mz_Stack = (c)

Stack *Pop( Maze *Bob )
{
	Stack *Popped = Bob->mz_Stack;
	
	if( Popped )
	{
		StackLength--;
		Bob->mz_Stack = Popped->st_Prev;
	}
	
	return Popped;
}

VOID FreeMaze( Maze *FreeMe )
{
	if( FreeMe )
	{
		if( FreeMe->mz_Cells )
			FreeVec( FreeMe->mz_Cells );
		if( FreeMe->mz_LeftWalls )
			FreeVec( FreeMe->mz_LeftWalls );
		if( FreeMe->mz_TopWalls )
			FreeVec( FreeMe->mz_TopWalls );
		FreeVec( FreeMe );
	}
}

Maze *AllocMaze( struct Screen *Screen, LONG Width, LONG Height, LONG Flags )
{
	Maze *New;
	
	if( New = AllocVec( sizeof( Maze ), Flags ))
	{
		New->mz_Rast = &Screen->RastPort;
		New->mz_Width = Width;
		New->mz_Height = Height;
		New->mz_WidStep = ( Screen->Width - 1 ) / Width;
		New->mz_HeiStep = ( Screen->Height - 1 ) / Height;
		New->mz_Cells = AllocVec( sizeof( BYTE ) * Width * Height, Flags );
		New->mz_LeftWalls = AllocVec( sizeof( BYTE ) * ( Width + 1 ) * Height,
									 Flags );
		New->mz_TopWalls = AllocVec( sizeof( BYTE ) * Width * ( Height + 1 ),
									Flags );
		
		if( New->mz_Cells && New->mz_LeftWalls && New->mz_TopWalls )
			return New;
		
		FreeMaze( New );
	}
	
	return 0L;
}

#define ActivateLeftWall( x, y )\
( Bob->mz_LeftWalls[(x)*Bob->mz_Height+(y)] = 1 )
#define ActivateTopWall( x, y )\
( Bob->mz_TopWalls[(x)*Bob->mz_Height+(y)] = 1 )

#define LeftWallActive( x, y ) ( Bob->mz_LeftWalls[(x)*Bob->mz_Height+(y)] )
#define TopWallActive( x, y ) ( Bob->mz_TopWalls[(x)*Bob->mz_Height+(y)] )

VOID AddHorizWall( Maze *Bob, LONG x, LONG y )
{
	Move( Bob->mz_Rast, x * Bob->mz_WidStep, y * Bob->mz_HeiStep );
	Draw( Bob->mz_Rast, ( x + 1 ) * Bob->mz_WidStep, y * Bob->mz_HeiStep );
}

VOID AddVertWall( Maze *Bob, LONG x, LONG y )
{
	Move( Bob->mz_Rast, x * Bob->mz_WidStep, y * Bob->mz_HeiStep );
	Draw( Bob->mz_Rast, x * Bob->mz_WidStep, ( y + 1 ) * Bob->mz_HeiStep );
}

VOID DrawSolution( Maze *Bob, LONG x, LONG y, LONG Pen, LONG Trailer )
{
	SetAPen( Bob->mz_Rast, Pen );
	switch( Trailer )
	{
	case NORTH:
		RectFill( Bob->mz_Rast, x * Bob->mz_WidStep + 1,
				 y * Bob->mz_HeiStep + 1, ( x + 1 ) * Bob->mz_WidStep - 1,
				 ( y + 1 ) * Bob->mz_HeiStep );
		break;
	case SOUTH:
		RectFill( Bob->mz_Rast, x * Bob->mz_WidStep + 1, y * Bob->mz_HeiStep,
				 ( x + 1 ) * Bob->mz_WidStep - 1,
				 ( y + 1 ) * Bob->mz_HeiStep - 1 );
		break;
	case EAST:
		RectFill( Bob->mz_Rast, x * Bob->mz_WidStep + 1,
				 y * Bob->mz_HeiStep + 1, ( x + 1 ) * Bob->mz_WidStep,
				 ( y + 1 ) * Bob->mz_HeiStep - 1 );
		break;
	case WEST:
		RectFill( Bob->mz_Rast, x * Bob->mz_WidStep, y * Bob->mz_HeiStep + 1,
				 ( x + 1 ) * Bob->mz_WidStep - 1,
				 ( y + 1 ) * Bob->mz_HeiStep - 1 );
		break;
	}

	if(!( ++Iter % SolPeriod ))
		Delay( SolDelay );
}

BYTE *RemoveDir( BYTE *Dir, BYTE RemMe )
{
	static BYTE Dirs[4];
	int i;
	
	for( i = 0; i < 4; i++ )
	{
		if( Dir[i] == RemMe )
			Dirs[i] = 4;
		else
			Dirs[i] = Dir[i];
	}
	
	return Dirs;
}

#define DefRemDir( x ) RemoveDir(( BYTE * )&( DirArray[RangeRand( 24 )] ), x )

Stack *GoDirection( Maze *Bob, Stack *Cell, BYTE DirNum )
{
	LONG x = Cell->st_x, y = Cell->st_y;
	BYTE Direction;
	
	switch( DirNum )
	{
	case 0:
		Direction = Cell->st_DirOne;
		Cell->st_DirOne = 4;
		break;
	case 1:
		Direction = Cell->st_DirTwo;
		Cell->st_DirTwo = 4;
		break;
	case 2:
		Direction = Cell->st_DirThree;
		Cell->st_DirThree = 4;
		break;
	case 3:
		Direction = Cell->st_DirFour;
		Cell->st_DirFour = 4;
		break;
	}
	
	switch( Direction )
	{
	case EAST:
		if( x < Bob->mz_Width - 1 )
		{
			if( !HasBeenVisited( Bob, x + 1, y ))
			{
				Push( Bob, Cell );
				return AllocStack( x + 1, y, DefRemDir( WEST ));
			}
			else
			{
				AddVertWall( Bob, x + 1, y );
				ActivateLeftWall( x + 1, y );
			}
		}
		break;
	case NORTH:
		if( y < Bob->mz_Height - 1 )
		{
			if( !HasBeenVisited( Bob, x, y + 1 ))
			{
				Push( Bob, Cell );
				return AllocStack( x, y + 1, DefRemDir( SOUTH ));
			}
			else
			{
				AddHorizWall( Bob, x, y + 1 );
				ActivateTopWall( x, y + 1 );
			}
		}
		break;
	case WEST:
		if( x > 0 )
		{
			if( !HasBeenVisited( Bob, x - 1, y ))
			{
				Push( Bob, Cell );
				return AllocStack( x - 1, y, DefRemDir( EAST ));
			}
			else
			{
				AddVertWall( Bob, x, y );
				ActivateLeftWall( x, y );
			}
		}
		break;
	case SOUTH:
		if( y > 0 )
		{
			if( !HasBeenVisited( Bob, x, y - 1 ))
			{
				Push( Bob, Cell );
				return AllocStack( x, y - 1, DefRemDir( NORTH ));
			}
			else
			{
				AddHorizWall( Bob, x, y );
				ActivateTopWall( x, y );
			}
		}
		break;
	default:
		break;
	}
	
	return 0L;
}

LONG ConstructMaze( Maze *Bob, LONG x, LONG y, LONG sx, LONG sy )
{
	Stack *Cell, *NewCell;
	LONG RetVal;
	
	Cell = AllocStack( x, y, ( BYTE * )&( DirArray[RangeRand( 24 )] ));
	
	do
	{
		RetVal = ContinueBlanking();
		
		if( Cell->st_x == sx && Cell->st_y == sy && !SolutionLength )
			SolutionLength = StackLength;

		Visit( Bob, Cell->st_x, Cell->st_y );
		
		if( NewCell = GoDirection( Bob, Cell, 0 ))
		{
			Cell = NewCell;
			continue;
		}
		
		if( NewCell = GoDirection( Bob, Cell, 1 ))
		{
			Cell = NewCell;
			continue;
		}
		
		if( NewCell = GoDirection( Bob, Cell, 2 ))
		{
			Cell = NewCell;
			continue;
		}
		
		if( NewCell = GoDirection( Bob, Cell, 3 ))
		{
			Cell = NewCell;
			continue;
		}
		
		FreeVec( Cell );
		Cell = Pop( Bob );
	}
	while( Cell &&( RetVal == OK ));
	
	if( Cell )
	{
		do
			FreeVec( Cell );
		while( Cell = Pop( Bob ));
	}
	
	return RetVal;
}

Stack *SolveDirection( Maze *Bob, Stack *Cell, BYTE DirNum )
{
	LONG x = Cell->st_x, y = Cell->st_y;
	BYTE Direction;
	
	switch( DirNum )
	{
	case 0:
		Direction = Cell->st_DirOne;
		Cell->st_DirOne = 4;
		break;
	case 1:
		Direction = Cell->st_DirTwo;
		Cell->st_DirTwo = 4;
		break;
	case 2:
		Direction = Cell->st_DirThree;
		Cell->st_DirThree = 4;
		break;
	case 3:
		Direction = Cell->st_DirFour;
		Cell->st_DirFour = 4;
		break;
	}
	
	switch( Direction )
	{
	case EAST:
		if( x < Bob->mz_Width - 1 )
		{
			if( !LeftWallActive( x + 1, y ))
			{
				Push( Bob, Cell );
				DrawSolution( Bob, x + 1, y, Copper == 2 || Copper == 3 ?
							 (( StackLength * NumCols / SolutionLength ) %
							  NumCols ) + 1 : 3, WEST );
				return AllocStack( x + 1, y, DefRemDir( WEST ));
			}
		}
		break;
	case NORTH:
		if( y < Bob->mz_Height - 1 )
		{
			if( !TopWallActive( x, y + 1 ))
			{
				Push( Bob, Cell );
				DrawSolution( Bob, x, y + 1, Copper == 2 || Copper == 3 ?
							 (( StackLength * NumCols / SolutionLength ) %
							  NumCols ) + 1 : 3, SOUTH );
				return AllocStack( x, y + 1, DefRemDir( SOUTH ));
			}
		}
		break;
	case WEST:
		if( x > 0 )
		{
			if( !LeftWallActive( x, y ))
			{
				Push( Bob, Cell );
				DrawSolution( Bob, x - 1, y, Copper == 2 || Copper == 3 ?
							 (( StackLength * NumCols / SolutionLength ) %
							  NumCols ) + 1 : 3, EAST );
				return AllocStack( x - 1, y, DefRemDir( EAST ));
			}
		}
		break;
	case SOUTH:
		if( y > 0 )
		{
			if( !TopWallActive( x, y ))
			{
				Push( Bob, Cell );
				DrawSolution( Bob, x, y - 1, Copper == 2 || Copper == 3 ?
							 (( StackLength * NumCols / SolutionLength ) %
							  NumCols ) + 1 : 3, NORTH );
				return AllocStack( x, y - 1, DefRemDir( NORTH ));
			}
		}
		break;
	default:
		break;
	}
	
	return 0L;
}

LONG SolveMaze( Maze *Bob, LONG x, LONG y, LONG sx, LONG sy )
{
	Stack *Cell, *NewCell;
	LONG RetVal;
	
	Cell = AllocStack( x, y, ( BYTE * )&( DirArray[RangeRand( 24 )] ));
	
	do
	{
		RetVal = ContinueBlanking();

		if( Cell->st_x == sx && Cell->st_y == sy )
			break;

		if( NewCell = SolveDirection( Bob, Cell, 0 ))
		{
			Cell = NewCell;
			continue;
		}
		
		if( NewCell = SolveDirection( Bob, Cell, 1 ))
		{
			Cell = NewCell;
			continue;
		}
		
		if( NewCell = SolveDirection( Bob, Cell, 2 ))
		{
			Cell = NewCell;
			continue;
		}
		
		if( NewCell = SolveDirection( Bob, Cell, 3 ))
		{
			Cell = NewCell;
			continue;
		}
		
		x = Cell->st_x;
		y = Cell->st_y;

		FreeVec( Cell );
		Cell = Pop( Bob );

		if( Cell->st_x > x )
			DrawSolution( Bob, x, y, 0, EAST );
		if( Cell->st_x < x )
			DrawSolution( Bob, x, y, 0, WEST );
		if( Cell->st_y > y )
			DrawSolution( Bob, x, y, 0, NORTH );
		if( Cell->st_y < y )
			DrawSolution( Bob, x, y, 0, SOUTH );
	}
	while( Cell &&( RetVal == OK ));
	
	if( Cell )
	{
		do
			FreeVec( Cell );
		while( Cell = Pop( Bob ));
	}
	
	return RetVal;
}

LONG Blank( PrefObject *Prefs )
{
	LONG ToFrontCount = 0, i, RetVal, StartY, SolutionY;
	struct RastPort *Rast;
	struct Screen *Scr;
	struct Window *Wnd;
	Maze *Bob;
	
	Scr = OpenScreenTags( 0L, SA_Depth, Prefs[MODE].po_Depth, SA_Behind, TRUE,
						 SA_Overscan, OSCAN_STANDARD, SA_Quiet, TRUE,
						 SA_DisplayID, Prefs[MODE].po_ModeID, TAG_DONE );
	
	if( Scr )
		Bob = AllocMaze( Scr, ( Scr->Width - 1 ) / Prefs[CELLSIZE].po_Level,
						( Scr->Height - 1 ) / Prefs[CELLSIZE].po_Level,
						MEMF_CLEAR );
	
	if( Scr && Bob )
	{
		SetRGB4( &Scr->ViewPort, 0, 0L, 0L, 0L );
		
		if( !Prefs[SOLVEDEL].po_Level )
		{
			SolDelay = 0;
			SolPeriod = 100;
		}
		else if( Prefs[SOLVEDEL].po_Level < 12 )
		{
			SolDelay = 1;
			SolPeriod = 12 - Prefs[SOLVEDEL].po_Level;
		}
		else
		{
			SolDelay = Prefs[SOLVEDEL].po_Level - 10;
			SolPeriod = 1;
		}
		
		Rast = &( Scr->RastPort );
		NumCols = ( 1L << Rast->BitMap->Depth ) - 1;
		
		SetRGB4( &Scr->ViewPort, 2, 0x0FL, 0x02L, 0x02L );

		switch( Copper = Prefs[COPPER].po_Active )
		{
		case 0:
			SetRGB4( &Scr->ViewPort, 3, 0x0DL, 0x0DL, 0x0DL );
			setCopperList(( LONG )Scr->Height, 1, &Scr->ViewPort, &custom );
			break;
		case 1:
			SetRGB4( &Scr->ViewPort, 3, 0x02L, 0x02L, 0x0DL );
			setCopperList(( LONG )Scr->Height, 3, &Scr->ViewPort, &custom );
			break;
		case 2:
			ColorTable = RainbowPalette( Scr, 0L, 1L, 0L );
			break;
		case 3:
			ColorTable = RainbowPalette( Scr, 0L, 1L, 0L );
			setCopperList(( LONG )Scr->Height, 1, &Scr->ViewPort, &custom );
			break;
		default:
			break;
		}

		Wnd = BlankMousePointer( Scr );
		ScreenToFront( Scr );
		
		do
		{
			SetRast( Rast, 0 );

			if( Copper != 2 )
				SetRGB4( &Scr->ViewPort, 1, 0x02L + RangeRand( 3 ),
						0x0DL - RangeRand( 5 ), 0x02L + RangeRand( 3 ));

			if(!( ++ToFrontCount % 60 ))
				ScreenToFront( Scr );
			
			if( Copper != 2 && Copper != 4 )
				SetAPen( Rast, 1 );
			else
				SetAPen( Rast, RangeRand( NumCols ) + 1L );

			Move( Rast, 0, 0 );
			Draw( Rast, 0, Bob->mz_Height * Bob->mz_HeiStep );
			Draw( Rast, Bob->mz_Width * Bob->mz_WidStep,
				 Bob->mz_Height * Bob->mz_HeiStep );
			Draw( Rast, Bob->mz_Width * Bob->mz_WidStep, 0 );
			Draw( Rast, 0, 0 );
			
			StartY = RangeRand( Bob->mz_Height );
			SolutionY = RangeRand( Bob->mz_Height );
			
			StackLength = SolutionLength = 0;

			RetVal = ConstructMaze( Bob, 0, StartY, Bob->mz_Width - 1,
								   SolutionY );

			if( RetVal == OK )
			{
				if( Copper != 2 )
				{
					DrawSolution( Bob, 0, StartY, 2, WEST );
					DrawSolution( Bob, Bob->mz_Width - 1, SolutionY, 2, EAST );
				}
				else
				{
					DrawSolution( Bob, 0, StartY, 1, WEST );
					DrawSolution( Bob, Bob->mz_Width - 1, SolutionY, 1, EAST );
				}
				
				RetVal = SolveMaze( Bob, 0, StartY, Bob->mz_Width - 1,
								   SolutionY );

				if( RetVal == OK )
				{
					for( i = 0; i < Bob->mz_Width * Bob->mz_Height; i++ )
						Bob->mz_Cells[i] = 0;

					for( i = 0; i < ( Bob->mz_Width+1 ) * Bob->mz_Height; i++ )
						Bob->mz_LeftWalls[i] = 0;
					
					for( i = 0; i < Bob->mz_Width * ( Bob->mz_Height+1 ); i++ )
						Bob->mz_TopWalls[i] = 0;
				}
			}
		}
		while( RetVal == OK );
		
		UnblankMousePointer( Wnd );

		switch( Copper )
		{
		case 0:
		case 1:
			clearCopperList( &Scr->ViewPort );
			break;
		case 3:
			clearCopperList( &Scr->ViewPort );
		case 2:
			RainbowPalette( 0L, ColorTable, 1L, 0L );
			break;
		default:
			break;
		}

	}
	else
		RetVal = FAILED;
	
	if( Bob )
		FreeMaze( Bob );
	
	if( Scr )
		CloseScreen( Scr );
	
	return RetVal;
}
