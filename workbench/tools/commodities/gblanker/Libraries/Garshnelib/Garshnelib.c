/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#define DEBUG 1
#include <aros/debug.h>

#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <graphics/videocontrol.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/mathffp.h>
#include <proto/alib.h>

#include "Garshnelib.h"
#include "Garshnelib_rev.h"
#include "../../defs.h"

const UBYTE VersTag[] = VERSTAG;

AROS_LH3(VOID, ScreenModeRequest, 
	AROS_LHA(struct Window *, Wnd, A0),
	AROS_LHA(LONG *, Mode, A1),
	AROS_LHA(LONG *, Depth, A2),
	struct Library *, library, 5, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

        struct ScreenModeRequester *smRequest;
        
        if( smRequest = AllocAslRequestTags( ASL_ScreenModeRequest, TAG_END ))
        {
                if( Depth )
                {
                        if( AslRequestTags( smRequest, ASLSM_Window, Wnd,
                                                           ASLSM_SleepWindow, TRUE,
                                                           ASLSM_TitleText, "Screen Mode",
                                                           ASLSM_InitialDisplayID, *Mode,
                                                           ASLSM_InitialDisplayDepth, *Depth,
                                                           ASLSM_DoDepth, TRUE, TAG_END ))
                        {
                                *Mode = smRequest->sm_DisplayID;
                                *Depth = smRequest->sm_DisplayDepth;
                        }
                }
                else
                {
                        if( AslRequestTags( smRequest, ASLSM_Window, Wnd,
                                                           ASLSM_SleepWindow, TRUE,
                                                           ASLSM_TitleText, "Screen Mode",
                                                           ASLSM_InitialDisplayID, *Mode, TAG_END ))
                                *Mode = smRequest->sm_DisplayID;
                }
                FreeAslRequest( smRequest );
        }

	AROS_LIBFUNC_EXIT
}

#define NUMCOLORS 45
LONG spectrum[] = {
	0x0F00, 0x0E10, 0x0D20, 0x0C30, 0x0B40, 0x0A50, 0x0960,	0x0870, 0x0780,
	0x0690, 0x05A0, 0x04B0, 0x03C0, 0x02D0,	0x01E0, 0x00F0, 0x00E1, 0x00D2,
	0x00C3, 0x00B4, 0x00A5,	0x0096, 0x0087, 0x0078, 0x0069, 0x005A, 0x004B,
	0x003C,	0x002D, 0x001E, 0x000F, 0x010E, 0x020D, 0x030C, 0x040B,	0x050A,
	0x0609, 0x0708, 0x0807, 0x0906, 0x0A05, 0x0B04,	0x0C03, 0x0D02, 0x0E01
	};

AROS_LH4(VOID, setCopperList, 
	AROS_LHA(LONG, Hei, D0),
	AROS_LHA(LONG, Col, D1),
	AROS_LHA(struct ViewPort *, VPort, A0),
	AROS_LHA(struct Custom *, Custom, A1),
	struct Library *, library, 6, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

#if 0
	struct UCopList *uCopList;

	if(( uCopList = AllocVec( sizeof( struct UCopList ), MEMF_CLEAR )))
	{
		struct TagItem uCopTags[] = {{VTAG_USERCLIP_SET,0L},{VTAG_END_CM,0L}};
		LONG i, spc;

		spc = Hei/NUMCOLORS;
		CINIT( uCopList, NUMCOLORS );
		for( i = 0; i < NUMCOLORS; ++i )
		{
			CWAIT( uCopList, i * spc, 0 );
			CMOVE( uCopList, Custom->color[Col], spectrum[i%NUMCOLORS] );
		}
		CEND( uCopList );
		Forbid();
		VPort->UCopIns = uCopList;
		Permit();
		VideoControl( VPort->ColorMap, uCopTags );
		RethinkDisplay();
	}
	
#endif
	AROS_LIBFUNC_EXIT
}

AROS_LH1(VOID, clearCopperList, 
	AROS_LHA(struct ViewPort *, VPort, A0),
	struct Library *, library, 16, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

#if 0
        struct TagItem uCopTags[] = {{VTAG_USERCLIP_CLR,0L},{VTAG_END_CM,0L}};
        struct UCopList *uCopList;

        VideoControl( VPort->ColorMap, uCopTags );
        Forbid();
        uCopList = VPort->UCopIns;
        VPort->UCopIns = 0L;
        Permit();
        FreeVec( uCopList );

#endif
	AROS_LIBFUNC_EXIT
}

AROS_LH0(LONG, getTopScreenMode, 
	struct Library *, library, 7, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	struct Screen *pubScr;
	LONG scrMode;
	ULONG lock;

        lock = LockIBase( 0 );
        pubScr = IntuitionBase->FirstScreen;
        scrMode = GetVPModeID(&( pubScr->ViewPort));
        UnlockIBase( lock );

	return scrMode;

	AROS_LIBFUNC_EXIT
}

AROS_LH0(LONG, getTopScreenDepth, 
	struct Library *, library, 8, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

        LONG Dep = 0;
	
        struct Screen *PubScr;
        struct DrawInfo *dri;
        ULONG Lock;

        Lock = LockIBase( 0 );
        PubScr = IntuitionBase->FirstScreen;
        if( dri = GetScreenDrawInfo( PubScr ))
        {
                Dep = dri->dri_Depth;
                FreeScreenDrawInfo( PubScr, dri );
        }
        UnlockIBase( Lock );
	
	return Dep;

	AROS_LIBFUNC_EXIT
}

AROS_LH2(struct Screen *, cloneTopScreen, 
	AROS_LHA(LONG, MoreColors, D0),
	AROS_LHA(LONG, GetPublic, D1),
	struct Library *, library, 9, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	LONG sMod, sDep, i, Wid, Hei, offx, offy;
	struct Screen *Scr, *nScr = 0L;
	struct Rectangle DispRect;
	struct Library *GfxBase;
	struct DrawInfo *dri;
	UWORD *cols;
	ULONG lock;
	
        /* Lock IntuitionBase so nothing goes away */
        lock = LockIBase( 0 );
        
        /* Grab the first screen and get its attributes */
        if( GetPublic )
                Scr = LockPubScreen( 0L );
        else
                Scr = IntuitionBase->FirstScreen;
        sMod = GetVPModeID(&( Scr->ViewPort )); /* Screen Mode ID */
        offx = Scr->LeftEdge; 
        offy = Scr->TopEdge;
        Wid = Scr->Width;
        Hei = Scr->Height;
        if( dri = GetScreenDrawInfo( Scr ))
                sDep = MoreColors ? dri->dri_Depth + 1 : dri->dri_Depth;
        if( cols = AllocVec( sizeof( WORD ) * ( 1L << sDep ), MEMF_CLEAR ))
                for( i = 0; i < ( 1L << dri->dri_Depth ); ++i )
                        cols[i] = GetRGB4( Scr->ViewPort.ColorMap, i );
        
        if( GetPublic )
                UnlockPubScreen( 0L, Scr );
        
        UnlockIBase( lock );
        
        QueryOverscan( sMod, &DispRect, OSCAN_TEXT );
        
        Wid = min( Wid, DispRect.MaxX - DispRect.MinX + 1 );
        Hei = min( Hei, DispRect.MaxY - DispRect.MinY + 1 );
        
        if( sMod != INVALID_ID )
        {
                nScr = OpenScreenTags( NULL, SA_DisplayID, sMod, SA_Depth, sDep,
                                                          SA_Width, Wid, SA_Height, Hei,
                                                          SA_Behind, TRUE, SA_Quiet, TRUE, TAG_DONE );
                if( nScr )
                {
                        BltBitMap( Scr->RastPort.BitMap, offx < 0 ? -offx : 0,
                                          offy < 0 ? -offy : 0, nScr->RastPort.BitMap, 0, 0,
                                          Wid, Hei, 0x00C0, 0x00FF, NULL );
                        LoadRGB4( &(nScr->ViewPort), cols, 1L << sDep );
                        if( offx > 0 )
                                MoveScreen( nScr, offx, 0 );
                        WaitBlit();
                        ScreenToFront( nScr );
                }
        }
        
        if( cols )
                FreeVec( cols );
        
        if( dri )
                FreeScreenDrawInfo( Scr, dri );

	return nScr;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG *, GetColorTable, 
	AROS_LHA(struct Screen *, Screen, A0),
	struct Library *, library, 10, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	ULONG *ColorTable = 0L;

        LONG NumCols = ( 1L << Screen->RastPort.BitMap->Depth );

        ColorTable = AllocVec( sizeof( LONG ) * ( 3 * NumCols + 2 ),
                                                  MEMF_CLEAR );
        if( ColorTable )
        {
                GetRGB32( Screen->ViewPort.ColorMap, 0, NumCols, ColorTable + 1 );
                ColorTable[0] = ( NumCols << 16L );
        }

	return ColorTable;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AvgBitsPerGun, 
	AROS_LHA(LONG, ModeID, D0),
	struct Library *, library, 11, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	LONG BPG = 8;

        struct DisplayInfo Inf;

        if( GetDisplayInfoData( 0L, ( UBYTE * )&Inf, sizeof( Inf ), DTAG_DISP,
                                                   ModeID ))
                BPG = ( Inf.RedBits + Inf.GreenBits + Inf.BlueBits ) / 3;


	return BPG;

	AROS_LIBFUNC_EXIT
}

AROS_LH4(VOID, FadeAndLoadTable, 
	AROS_LHA(struct Screen *, Screen, A0),
	AROS_LHA(LONG, BPG, D0),
	AROS_LHA(ULONG *, ColorTable, A1),
	AROS_LHA(LONG, SavePlanes, D1),
	struct Library *, library, 12, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	if( !ColorTable )
		return;

        LONG NumCols = ( 1L << ( Screen->RastPort.BitMap->Depth-SavePlanes ));
        LONG i;
        
        for( i = 0; i < 3 * NumCols; ++i )
        {
                if( ColorTable[i+1] > 1L << ( 32 - BPG ))
                        ColorTable[i+1] -= ( 1L << ( 32 - BPG ));
                else
                        ColorTable[i+1] = 0;
        }
                
        LoadRGB32(&( Screen->ViewPort ), ColorTable );

	AROS_LIBFUNC_EXIT
}

AROS_LH1(struct Window *, BlankMousePointer, 
	AROS_LHA(struct Screen *, Scr, A0),
	struct Library *, library, 13, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	struct Window *Wnd;
	ULONG oldmodes;
	
        oldmodes = SetPubScreenModes( 0 );
        if( Wnd = OpenWindowTags( 0L, WA_Activate, TRUE, WA_Left, 0, WA_Top, 0,
                                                         WA_Width, 1, WA_Height, 1, WA_Borderless,
                                                         TRUE, WA_CustomScreen, Scr, TAG_END ))
        {
                if( Wnd->UserData = AllocVec( 3 * 2 * sizeof( UWORD ),
                                                                         MEMF_CHIP|MEMF_CLEAR ))
                {
                        /* Set a pointer sprite of size 16*1 (alloc and clear data for
                           sprite def which is 2 control words, 2 words 4-color data
                           for each line and 2 empty words at the end (as suggested in
                           Hardware Reference Manual)). */
                        SetPointer( Wnd, ( UWORD * )Wnd->UserData, 1, 16, 0, 0 );
                }
        }
        SetPubScreenModes( oldmodes );

	return Wnd;

	AROS_LIBFUNC_EXIT
}

AROS_LH1(VOID, UnblankMousePointer, 
	AROS_LHA(struct Window *, Wnd, A0),
	struct Library *, library, 14, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

        if( Wnd )
        {
                if( Wnd->UserData )
                {
                        ClearPointer( Wnd );
                        FreeVec( Wnd->UserData );
                }
                CloseWindow( Wnd );
        }

	AROS_LIBFUNC_EXIT
}


Triplet *AllocTable( LONG Colors, LONG Dep, LONG Offset )
{
	FLOAT incr, p2, p3, f, h;
	LONG i, Shift, ih;
	Triplet *Table;
	
	Table = AllocVec( 2*( Colors + Offset ) * sizeof( Triplet ), MEMF_CLEAR );
	
	if( Table )
	{
		incr = SPDiv( SPFlt( Colors ), SPFlt( 360 ));
		Shift = 32 - Dep;
		
		for( i = Offset, h = SPFlt( 0 ); i < Colors + Offset;
			i++, h = SPAdd( h, incr ))
		{
			ih = SPFix( SPDiv( SPFlt( 60 ), h ));
			f = SPSub( SPFlt( ih ), SPDiv( SPFlt( 60 ), h ));
			p2 = SPSub( f, SPFlt( 1 ));
			p3 = f;
			switch( ih )
			{
			case 0:
				Table[i].Red = Colors << Shift;
				Table[i].Green = CastAndShift( p3 );
				Table[i].Blue = 0;
				break;
			case 1:
				Table[i].Red = CastAndShift( p2 );
				Table[i].Green = Colors << Shift;
				Table[i].Blue = 0;
				break;
			case 2:
				Table[i].Red = 0;
				Table[i].Green = Colors << Shift;
				Table[i].Blue = CastAndShift( p3 );
				break;
			case 3:
				Table[i].Red = 0;
				Table[i].Green = CastAndShift( p2 );
				Table[i].Blue = Colors << Shift;
				break;
			case 4:
				Table[i].Red = CastAndShift( p3 );
				Table[i].Green = 0;
				Table[i].Blue = Colors << Shift;
				break;
			case 5:
				Table[i].Red = Colors << Shift;
				Table[i].Green = 0;
				Table[i].Blue = CastAndShift( p2 );
				break;
			}
		}
		CopyMem(&( Table[Offset] ), &( Table[Colors+Offset] ),
				Colors * sizeof( Triplet ));
	}
	
        FreeVec( Table );
        Table = 0L;
	
	return Table;
}

AROS_LH4(Triplet *, RainbowPalette, 
	AROS_LHA(struct Screen *, Screen, A0),
	AROS_LHA(Triplet *, Table, A1),
	AROS_LHA(LONG, Offset, D0),
	AROS_LHA(LONG, EP, D1),
	struct Library *, library, 15, Garshnelib
)
{
	AROS_LIBFUNC_INIT

        ALIVE

	static LONG ColorPos = 0L;
	ULONG tmpColorPos;

	if( Screen )
	{
                LONG Colors = ( 1L << Screen->BitMap.Depth ) - Offset;
                LONG PDep = ( Screen->BitMap.Depth < ( 9 - EP )) ?
                        Screen->BitMap.Depth + EP : 8;
                LONG PColors = ( 1L << PDep ) - Offset;
                
                if( !Table )
                {
                        Table = AllocTable( PColors, PDep, Offset );
                        if( Offset )
                                SetRGB32(&(Screen->ViewPort), 0, 0, 0, 0 );
                }
                
                // This give warning: operation on 'ColorPos' may be undefined
                // ColorPos = ( ++ColorPos % PColors );
                tmpColorPos = ( ++ColorPos % PColors );
                ColorPos = tmpColorPos;
                
                if( Table )
                {
                        LONG Temp1, Temp2;
                        
                        Temp1 = Table[ColorPos].Blue;
                        Temp2 = Table[ColorPos+Colors+1].Red;
                        
                        Table[ColorPos].Blue = ( Colors << 16L ) + Offset;
                        Table[ColorPos+Colors+1].Red = 0;
                        
                        LoadRGB32(&(Screen->ViewPort), &( Table[ColorPos].Blue ));
                        
                        Table[ColorPos].Blue = Temp1;
                        Table[ColorPos+Colors+1].Red = Temp2;
                }
	}
	else
	{
		if( Table )
			FreeVec( Table );
	}
	
	return Table;

	AROS_LIBFUNC_EXIT
}
