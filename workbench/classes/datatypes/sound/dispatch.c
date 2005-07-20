/*
**
**	sound.datatype v41
**	© 1998-2004 by Stephan Rupprecht
**	all rights reserved
**
*/

#if !defined(__MAXON__) && !defined(__AROS__)
#define  CLIB_ALIB_PROTOS_H
#endif

#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/var.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <datatypes/soundclass.h>
#include <datatypes/soundclassext.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <libraries/iffparse.h>
#include <devices/audio.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuitionbase.h>
#include <intuition/cghooks.h>
#include <devices/ahi.h>
#include <gadgets/tapedeck.h>
#include <graphics/gfxmacros.h>
#include <graphics/gfxbase.h>

#ifdef __GNUC__
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/ahi.h>
#include <clib/alib_protos.h>
#else
#include <pragma/exec_lib.h>
#include <pragma/intuition_lib.h>
#include <pragma/dos_lib.h>
#include <pragma/utility_lib.h>
#include <pragma/iffparse_lib.h>
#include <pragma/graphics_lib.h>
#include <pragma/datatypes_lib.h>
#include <pragma/ahi_lib.h>
#endif

#ifndef __AROS__
#pragma header
#endif

#include "classbase.h"

#ifndef __MAXON__
#include "CompilerSpecific.h"
#ifndef __AROS__
#include "BoopsiStubs.h"
#endif
#endif

#include "aiff.h"

#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>
#endif

#if !defined(__MAXON__) && !defined(__AROS__)
#define IntuitionBase	cb->cb_IntuitionBase
#define GfxBase		cb->cb_GfxBase
#define DOSBase		cb->cb_DOSBase
#define SysBase		cb->cb_SysBase
#define DataTypesBase	cb->cb_DataTypesBase
#define UtilityBase		cb->cb_UtilityBase
#define IFFParseBase	cb->cb_IFFParseBase
#elif !defined(__AROS__)
#define REG(r,v)	register __## r v
#define __regargs
#endif

#define G(x)				((struct Gadget *)(x))
#define MAX( a, b )			((a) > (b) ? (a) : (b))
#define EXTG( x )			((struct ExtGadget *)(x))

#ifndef ID_CHAN
#define ID_CHAN		MAKE_ID( 'C', 'H', 'A', 'N' )
#endif
#ifndef ID_16SV
#define ID_16SV		MAKE_ID( '1', '6', 'S', 'V' )
#endif

#ifdef __AROS__
#include <aros/symbolsets.h>
/* Optionally open tapedeck gadget */
ADD2LIBS("gadgets/tapedeck.gadget", -40, struct Library *, TapeDeckBase)
#endif

#ifndef ID_PAN
#define ID_PAN		MAKE_ID( 'P', 'A', 'N', ' ' )
#endif
#ifndef ID_ANNO
#define ID_ANNO		MAKE_ID( 'A', 'N', 'N', 'O' )
#endif
#ifndef ID_AUTH
#define ID_AUTH		MAKE_ID( 'A', 'U', 'T', 'H' )
#endif
#ifndef ID_FVER
#define ID_FVER		MAKE_ID( 'F', 'V', 'E', 'R' )
#endif
#ifndef ID_Copyright
#define ID_Copyright	MAKE_ID( '(', 'c', ')', ' ' )
#endif

#define TEMPLATE	"VOLUME/N/K,BUFFERSIZE/N/K,AHI/S,AHIMODEID/K,FAM=FORCEAHIMODE/S,AMF=AHIMIXFREQ/N/K,AIFF16/S,C=COMPRESS/S," \
				"W=WIDTH/N/K,H=HEIGHT/N/K,BG=BACKGROUNDCOLOR/K,WF=WAVEFORMCOLOR/K,CP=CONTROLPANEL/T,NOGTSLIDER/S"

// MaxonC likes to use 16bit math even when 020+ optimiziation is turned on (BUG!)
#ifdef __MAXON__
#define UMult( x, y )	UMult32( (x), (y) )
#define SMult( x, y )	SMult32( (x), (y) )
#define UDiv( x, y )		UDivMod32( (x), (y) )
#define SDiv( x, y )		SDivMod32( (x), (y) )
#else
#define UMult( x, y )	( (x) * (y) )
#define SMult( x, y )	( (x) * (y) )
#define UDiv( x, y )		( (x) / (y) )
#define SDiv( x, y )		( (x) / (y) )
#endif

#ifdef __AROS__
#define Period2Freq( x ) 	( UDiv( UMult(709379, 5L ), (x) ) )
#else
#define Period2Freq( x ) 	( UDiv( UMult( ((struct ExecBase *)SysBase)->ex_EClockFrequency, 5L ), (x) ) )
#endif

#define Freq2Period( x )	Period2Freq( x )
#define IsStereo( x )		( (BOOL) ( ( x ) & 1 ) )

#ifndef SHRT_MAX
#define SHRT_MAX	0x7fff
#endif

//#define DEBUG
#ifdef DEBUG
#define dbug( x )	x
#else
#define dbug( x )
#endif

/****************************************************************************/

#ifndef __AROS__
IPTR Dispatcher(REG(a0,Class *cl), REG(a2,Object *o), REG(a1,Msg msg));
#endif
IPTR __regargs Sound_NEW( Class *cl, Object *o, struct opSet *ops );
IPTR __regargs Sound_GET( Class *cl, Object *o, struct opGet *ops );
IPTR __regargs Sound_SET( Class *cl, Object *o, struct opSet *ops );
IPTR __regargs Sound_UPDATE( Class *cl, Object *o, struct opUpdate *opu );
IPTR __regargs Sound_DISPOSE( Class *cl, Object *o, Msg msg );
IPTR __regargs Sound_RENDER( Class *cl, Object *o, struct gpRender *gpr );
IPTR __regargs Sound_HANDLEINPUT( Class *cl, Object *o, struct gpInput *gpi );
IPTR __regargs Sound_TRIGGER( Class *cl, Object *o, struct dtTrigger *dtt );
IPTR __regargs Sound_WRITE( Class *cl, Object *o, struct dtWrite *dtw );
IPTR __regargs Sound_LAYOUT( Class *cl, Object *o, struct gpLayout *gpl );
IPTR __regargs Sound_DOMAIN( Class *cl, Object *o, struct gpDomain *gpd );
LONG __regargs Sound_SELECT( Class *cl, Object *o, struct dtSelect *dts );
LONG __regargs Sound_CLEARSELECTED( Class *cl, Object *o, struct dtGeneral *dtg );
IPTR __regargs Sound_HITTEST( Class *cl, Object *o, struct gpHitTest *gpht );
IPTR __regargs Sound_GOINACTIVE( Class *cl, Object *o, struct gpGoInactive *gpgi );
IPTR __regargs Sound_DRAW( Class *cl, Object *o, struct dtDraw *dtd );
IPTR __regargs Sound_OBTAINDRAWINFO( Class *cl, Object *o, struct opSet *ops );
IPTR __regargs Sound_REMOVEDTOBJECT( Class *cl, Object *o, Msg msg );
IPTR __regargs Sound_RELEASEDRAWINFO( Class *cl, Object *o, Msg msg );
LONG __regargs hex2long( STRPTR hex );
BOOL __regargs parsetaglist( Class *cl, Object *o, struct opSet *ops, ULONG *cnt_p );
struct Process * __regargs CreatePlayerProc( struct ClassBase *cb, struct MsgPort **mp );
void __regargs GetSoundDTPrefs( struct ClassBase *cb );
struct IBox __regargs GetAbsGadgetBox( struct IBox *domain, struct ExtGadget *g, BOOL useBounds );
IPTR __regargs DoMemberHitTest( struct IBox *domain, Object *member, struct gpHitTest *gpht );
void PlayerProc( void );
void PlayerProcAHI( void );
void __regargs makeqtab( BYTE *qtab );
unsigned __regargs StrLen( STRPTR str );
IPTR propgdispatcher( REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg) );

/****************************************************************************/

BYTE fibtab[] = {-34,-21,-13,-8,-5,-3,-2,-1,0,1,2,3,5,8,13,21};

UBYTE bytesPerPoint[] = { 1, 2, 2, 4 };

LONG ifferr2doserr[] =
{
	0L,						// EOF
	0L,                         			// EOC
	DTERROR_INVALID_DATA,	/* No lexical scope.                             */
	ERROR_NO_FREE_STORE,	/* Insufficient memory.                          */
	ERROR_SEEK_ERROR,		/* Stream read error.                            */
	ERROR_SEEK_ERROR,		/* Stream write error.                           */
	ERROR_SEEK_ERROR,		/* Stream seek error.                            */
	DTERROR_INVALID_DATA,       /* File is corrupt.                              */
	DTERROR_INVALID_DATA,       /* IFF syntax error.                             */
	ERROR_OBJECT_WRONG_TYPE,/* Not an IFF file.                              */
	ERROR_REQUIRED_ARG_MISSING,/* Required call-back hook missing.              */
	0xDEADDEAD                  	/* Return to client. You should never see this ! */
};

struct DTMethod	TriggerMethods[] =
{
	{"Play", "PLAY", STM_PLAY},
	{"Stop", "STOP", STM_STOP},
	{"Pause", "PAUSE", STM_PAUSE},
	{(0)}
};

ULONG Methods[] = {
	OM_NEW,
	OM_DISPOSE,
	OM_SET,
	OM_GET,
	OM_UPDATE,
	GM_RENDER,
	GM_LAYOUT,
	GM_DOMAIN,
	GM_HITTEST,
	GM_HELPTEST,
	GM_GOACTIVE,
	GM_HANDLEINPUT,
	GM_GOINACTIVE,
	DTM_TRIGGER,
	DTM_WRITE,
	DTM_COPY,
	DTM_SELECT,
	DTM_CLEARSELECTED,
	DTM_PROCLAYOUT,
	DTM_REMOVEDTOBJECT,
	DTM_OBTAINDRAWINFO,
	DTM_DRAW,
	DTM_RELEASEDRAWINFO,
	~0UL
};

#if defined(__MAXON__) || defined(__AROS__)
struct Library			*AHIBase = NULL;
#endif

/****************************************************************************/

#ifdef DEBUG
#if !defined(__MAXON__) && !defined(__AROS__)
void kprintf( STRPTR FormatStr, ... )
{
	#undef SysBase
	struct Library	*SysBase = (*(struct Library **)4L);
	TEXT	PutChData[64];
	STRPTR	p = PutChData;
	
	RawDoFmt(FormatStr, ((STRPTR)(&FormatStr))+4, (void (*)())"\x16\xc0\x4e\x75", PutChData);
	
	do RawPutChar( *p );
	while( *p++ );
	#define SysBase		cb->cb_SysBase
}
#endif
#endif

/****************************************************************************/
#ifndef __AROS__
IPTR Dispatcher(REG(a0,Class *cl), REG(a2,Object *o), REG(a1,Msg msg))
{
	struct ClassBase *cb = (struct ClassBase *)cl->cl_UserData;
	IPTR  retval;

    	(void)cb;
		
	switch( msg->MethodID )
	{
		case OM_NEW:
			retval = Sound_NEW( cl, o, (struct opSet *)msg );
		break;

		case OM_GET:
			retval = Sound_GET( cl, o, (struct opGet *)msg );
		break;
		
		case OM_UPDATE:
			retval = Sound_UPDATE( cl, o, (struct opUpdate *)msg );
		break;

		case OM_SET:		
			retval = Sound_SET( cl, o, (struct opSet *)msg );
		break;		

		case OM_DISPOSE:
			retval = Sound_DISPOSE( cl, o, msg );
		break;
		
		case GM_HELPTEST:
		case GM_HITTEST:
			retval = Sound_HITTEST( cl, o, (struct gpHitTest *) msg );
		break;

		case GM_GOACTIVE:
		case GM_HANDLEINPUT:
			retval = Sound_HANDLEINPUT( cl, o, (struct gpInput *)msg );
		break;

		case GM_GOINACTIVE:
			retval = Sound_GOINACTIVE( cl, o, (struct gpGoInactive *) msg );
		break;

		case GM_DOMAIN:
			retval = Sound_DOMAIN( cl, o, (struct gpDomain *)msg );
		break;

		case GM_LAYOUT:
		case DTM_PROCLAYOUT:
			retval = Sound_LAYOUT( cl, o, (struct gpLayout *)msg );
		break;

		case GM_RENDER:
			retval = Sound_RENDER( cl, o, (struct gpRender *)msg );
		break;

		case DTM_TRIGGER:
			retval = Sound_TRIGGER( cl, o, (struct dtTrigger *)msg );
		break;

		case DTM_COPY:
		case DTM_WRITE:
			retval = Sound_WRITE( cl, o, (struct dtWrite *)msg );
		break;

		case DTM_SELECT:
			retval = Sound_SELECT( cl, o, (struct dtSelect *) msg );
		break;

		case DTM_CLEARSELECTED:
			retval = Sound_CLEARSELECTED( cl, o, (struct dtGeneral *) msg );
		break;
		
		case DTM_OBTAINDRAWINFO:
			retval = Sound_OBTAINDRAWINFO( cl, o, (struct opSet *) msg );
		break;
		
		case DTM_DRAW:
			retval = Sound_DRAW( cl, o, (struct dtDraw *) msg );
		break;
		
		case DTM_RELEASEDRAWINFO:
		
			dbug( kprintf( "DTM_RELEASEDRAWINFO\n" ); )
			
			retval = 0L;
		break;
		
		case DTM_REMOVEDTOBJECT:
			retval = Sound_REMOVEDTOBJECT(cl, o, msg);
		break;
		
		//case OM_NOTIFY:
		default:
			dbug( kprintf("METHOD: %08lx\n", msg->MethodID); )
			retval = (IPTR) DoSuperMethodA(cl, o, msg);
	}

	return(retval);
}
#endif

/****************************************************************************/

#ifdef __MAXON__
ULONG NotifyAttrs( Object *o, struct GadgetInfo *ginfo, ULONG flags, Tag tag1, ... )
{
    struct opUpdate opu;

    opu . MethodID     = OM_NOTIFY;
    opu . opu_AttrList = (struct TagItem *)(&tag1);
    opu . opu_GInfo    = ginfo;
    opu . opu_Flags    = flags;

    return( DoMethodA( o, (Msg)(&opu) ) );
}
#endif

/****************************************************************************/

LONG SendObjectMsg( struct InstanceData *id, ULONG Command, APTR Data )
{
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
	struct Library		*SysBase = (*(struct Library **)4L);
#endif
	struct ObjectMsg	*msg;

	if( ( msg = AllocVec( sizeof( *msg ), MEMF_PUBLIC|MEMF_CLEAR ) ) )
	{
		msg->Command = Command;
		msg->Data = Data;
		PutMsg( id->PlayerPort, &msg->Message );
	}
	else
	{
		dbug( kprintf( "No memory to send objmsg\n" ); )
	}
	
	return( (LONG) (msg!=NULL) );
#if !defined(__MAXON__) && !defined(__AROS__)
#define SysBase	cb->cb_SysBase
#endif
}

/****************************************************************************/

LONG __regargs Sound_CLEARSELECTED( Class *cl, Object *o, struct dtGeneral *dtg )
{
	struct RastPort		*rp;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	LONG				retval = 0L;

    	(void)cb;
		
	dbug( kprintf( "DTM_CLEARSELECTED\n"); )
	
	/* 'magic' nothing selected value */
	id->MinX =
	id->MaxX = ~0; 
	id->StartSample =
	id->EndSample = ~0L;
	
	if( ( rp = ObtainGIRPort( dtg->dtg_GInfo ) ) )
	{
		DoMethod( o, GM_RENDER, (IPTR) dtg->dtg_GInfo, (IPTR) rp, (IPTR) GREDRAW_REDRAW );
		ReleaseGIRPort( rp );
	}
	
	((struct DTSpecialInfo *)(G( o ) -> SpecialInfo)) -> si_Flags &= ~DTSIF_HIGHLIGHT;
	
	return retval;
}

/****************************************************************************/

LONG __regargs Sound_SELECT( Class *cl, Object *o, struct dtSelect *dts )
{
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	WORD				minX = id->MinX, maxX = id->MaxX, oldx, 
						x = G(o)->LeftEdge, w = G(o)->Width;
	ULONG				samplelength = id->SampleLength,
						startSample = ~0L, endSample = ~0L;
	LONG				dy = samplelength, dx, error, add, sub, step = 1L, i,
						retval = 0L;

    	(void)cb;
	
	dbug( kprintf("DTM_SELECT\n"); )

	if( minX > maxX )
	{
		maxX = minX;
		minX = id->MaxX;
	}
	/* get abs xpos/width */
	if( G(o)->Flags & GFLG_RELWIDTH )
	{
		w += dts->dts_GInfo->gi_Domain.Width;
	}
	
	if( G(o)->Flags & GFLG_RELRIGHT )
	{
		x += dts->dts_GInfo->gi_Domain.Left;
	}
	
	oldx = x+=4; w-=8;
	
	dx = w;
	/* a big sample */
	if( samplelength >> 17 )
	{
		dy >>= 7;
		step = 128;
	}
	/* get first and last sample of the marked area by
	** comparing the coordinates */
	if( samplelength <= w )
	{
		LONG	c = 0L;

		error = dy-2*dx-1;	
		sub = 2*(dx+1); add = 2*(dy+1); 
	
		for( i = w; i; i-- ) 
		{
			if( x >= minX && startSample == ~0L )
			{
				startSample = c;
			} 
			else if( x++ >= maxX )
			{
				endSample = c;
				break;
			}
			
			if( error > 0 ) 
			{
				c += step;
				error -= sub;
			}
		
			error += add;
		}
	}
	else
	{		
		error = dx-2*dy-1;   	
		sub = 2*(dy+1); add = 2*(dx+1);
	
		for( i = 0; i < samplelength; i+=step )
		{
			if( error > 0 )
			{
				if( x >= minX && startSample == ~0L )
				{
					startSample = i;
				}
				else if( x++ >= maxX )
				{
					endSample = i;
					break;
				}
				
				error -= sub;
			} 
		
			error += add;
		}
	}
	/* some corrections */
	if( endSample == ~0L )
	{
		endSample = samplelength;
	}
	
	if( minX == oldx)
	{
		startSample = 0L;
	}
	
	id->StartSample = startSample;
	id->EndSample = endSample;
	
	dbug( kprintf( "%ld, %ld\n", startSample, endSample ); )

	return retval;
}

/****************************************************************************/

void __regargs GetSoundDTPrefs( struct ClassBase *cb )
{
	TEXT			buf[256];
	LONG			len;

	dbug( kprintf( "GetSoundDTPrefs\n" ); )

	ObtainSemaphore( &cb->cb_LibLock );
	
	/* set default settings */
	dbug( kprintf( "Setting default values\n" ); )
#ifdef __AROS__
	cb->cb_AHI = TRUE;
#else
	cb->cb_AHI = FALSE;
#endif
	cb->cb_AHIModeID = AHI_DEFAULT_ID;
	cb->cb_ForceAHIMode = FALSE;
	cb->cb_AHIMixFrequency = AHI_DEFAULT_FREQ;
	
	cb->cb_Compress	= FALSE;
	cb->cb_AIFF		= FALSE;

	cb->cb_NomWidth	= 160;
	cb->cb_NomHeight	= 100;
	
	cb->cb_BgCol[0] =
	cb->cb_BgCol[1] =
	cb->cb_BgCol[2] = 0;
	
	cb->cb_WfCol[0] = 35 << 24L;
	cb->cb_WfCol[1] = 204 << 24L;
	cb->cb_WfCol[2] = 23 << 24L;
	
	cb->cb_BufferSize = 65536L;
	cb->cb_Volume = 64L;
	cb->cb_ControlPanel = FALSE;
	cb->cb_NoGTSlider = FALSE;
	
	if( ( len = GetVar( "datatypes/sounddt41.prefs", buf, sizeof( buf )-1, GVF_GLOBAL_ONLY ) ) > 0 )
	{
		struct RDArgs	*rdargs;
		
		dbug( kprintf( "Prefs: %s\n", buf ); )

		if( ( rdargs = AllocDosObject( DOS_RDARGS, NULL ) ) )
		{
			struct RDArgs	*freeargs;
			struct {
				LONG	*volume;
				LONG	*buffersize;
				LONG	ahi;
				STRPTR	ahimodeid;
				LONG	forceahimode;
				LONG	*mixfreq;
				LONG	aiff;
				LONG	compress;
				LONG	*width, *height;
				STRPTR	bg, wf;
				LONG	cp;
				LONG	nogtsl;			
			} args = { };
		
			/* prepare rdargs */
			buf[len++] = '\n';
			rdargs->RDA_Source.CS_Buffer = buf;
			rdargs->RDA_Source.CS_Length = len;
			rdargs->RDA_Flags = RDAF_NOPROMPT;
		
			if( ( freeargs = ReadArgs( TEMPLATE, (LONG *)&args, rdargs ) ) )
			{
				dbug( kprintf( "List of options:\n" ); )
				
				if( args.ahi )
				{
					cb->cb_AHI = TRUE;
					dbug( kprintf( "AHI\n" ); )
				}
				
				if( args.ahimodeid )
				{
					cb->cb_AHIModeID = hex2long( args.ahimodeid );
					dbug( kprintf( "AHIMODEID = %lx\n", cb->cb_AHIModeID ); )
				}
				
				if( args.forceahimode )
				{
					cb->cb_ForceAHIMode = TRUE;
					dbug( kprintf( "FAM\n" ); )
				}
				
				if( args.mixfreq )
				{
					cb->cb_AHIMixFrequency = *args.mixfreq;
					dbug( kprintf( "AHIMIXFREQ = %ld\n", *args.mixfreq ); )
				}
				
				if( args.aiff )
				{
					cb->cb_AIFF = TRUE;
					dbug( kprintf( "AIFF16\n" ); )
				}
				
				if( args.compress )
				{
					cb->cb_Compress = TRUE;
					dbug( kprintf( "COMPRESS\n" ); )
				}
				
				if( args.width )
				{
					cb->cb_NomWidth = *args.width;
					dbug( kprintf( "WIDTH = %ld\n", *args.width ); )
				}
				
				if( args.height )
				{
					cb->cb_NomHeight = *args.height;
					dbug( kprintf( "HEIGHT = %ld\n", *args.height ); )
				}
				
				if( args.wf )
				{
					LONG	num = hex2long( args.wf );
					
					cb->cb_WfCol[0] = (num>>16L)<<24;
					cb->cb_WfCol[1] = (num>>8L)<<24L;
					cb->cb_WfCol[2] = (num)<<24L;
					dbug( kprintf( "WFCOL = %08lx\n", num ); )
				}
				
				if( args.bg )
				{
					LONG	num = hex2long( args.bg );
					
					cb->cb_BgCol[0] = (num>>16L)<<24L;
					cb->cb_BgCol[1] = (num>>8L)<<24L;
					cb->cb_BgCol[2] = (num)<<24L;
					dbug( kprintf( "BGCOL = %08lx\n", num ); )
				}
				
				if( args.buffersize )
				{
					LONG	bufsize = *args.buffersize;
					
					if( bufsize > 1023 && bufsize < 131073 )
					{
						cb->cb_BufferSize = bufsize;
					}
					
					dbug( kprintf( "BUFFERSZ = %ld\n", bufsize ); )
				}
				
				if( args.cp )
				{
					cb->cb_ControlPanel = TRUE;
					dbug( kprintf( "CP=YES\n" ); )
				}
				else
				{
					cb->cb_ControlPanel = FALSE;
					dbug( kprintf( "CP=NO\n" ); )
				}
				
				if( args.volume )
				{
					cb->cb_Volume = *args.volume;
					dbug( kprintf( "VOLUME = %ld\n", *args.volume ); )
				}
				
				if( args.nogtsl )
				{
					cb->cb_NoGTSlider = TRUE;
					dbug( kprintf( "NOGTSLIDER\n" ); )
				}
				
				FreeArgs( freeargs );
			}
			else
			{
				struct EasyStruct	es = { sizeof(struct EasyStruct), 0, "sound.datatype", buf, "Okay" };
				
				if( Fault( IoErr(), "Error in prefs file", buf, sizeof( buf ) ) )
				{
					dbug( kprintf( "Prefserr: %s\n" , buf ); )
					EasyRequestArgs( NULL, &es, NULL, NULL );
				}
			}
							
			FreeDosObject( DOS_RDARGS, rdargs );
		}
		else
		{
			dbug( kprintf( "AllocDosObject() failed\n" ); )
		}
	}
	else
	{
		dbug( kprintf( "GetVar failed\n" ); )
	}
	
	ReleaseSemaphore( &cb->cb_LibLock );
}

/****************************************************************************/

LONG __regargs hex2long(STRPTR s)
{ ULONG a;
  BYTE c;

  if (s && (c=*s++) && (c=='$' || (c == '0' && *s++ == 'x'))) {
    a=0;
    while ((c=*s++)) {
      if (c>'9')
        c&=0x5f;
      if ((c-='0')<0 || (c>9 && (c-=7)>15))
        return 0;
      a<<=4,a|=c;
    }
    return a;
  }
  return 0;
}

/****************************************************************************/

void CreateTapeDeck( struct ClassBase *cb, struct InstanceData *id, Object *o )
{
#if defined(__MAXON__) || defined(__AROS__)
	extern struct Library	*TapeDeckBase;
#else
#define TapeDeckBase		cb->cb_TapeDeckBase
#endif
	ULONG				cp;
		
	dbug( kprintf( "Creating panel\n" ); )
	
	GetDTAttrs( o, DTA_ControlPanel, (IPTR) &cp, TAG_DONE );

	if( TapeDeckBase && cp )
	{
		STATIC struct TagItem	prop2vol[] = { {PGA_Top, SDTA_Volume}, {TAG_END} };
		
		ObtainSemaphoreShared( &cb->cb_LibLock );
		
		if( ( id->VolumeSlider = NewObject( NULL, PROPGCLASS,
			PGA_Top, id->Volume,
			PGA_Visible, 1L,
			PGA_Total, 64L,
			PGA_Freedom, FREEHORIZ,
			PGA_NewLook, cb->cb_NoGTSlider,
			! cb->cb_NoGTSlider ? PGA_Borderless : TAG_IGNORE, TRUE,
			ICA_TARGET, (IPTR) o,
			ICA_MAP, (IPTR) prop2vol,
			TAG_DONE ) ) )
		{
			if( ( id->TapeDeckGadget = NewObject( NULL, "tapedeck.gadget",
				TDECK_Tape, TRUE,
				TAG_DONE ) ) )
			{
				ReleaseSemaphore( &cb->cb_LibLock );
				return;
			}
			
			DisposeObject( id->VolumeSlider );
			id->VolumeSlider = NULL;
		}
		
		ReleaseSemaphore( &cb->cb_LibLock );
	}
	else
	{
		dbug( kprintf( "Failed or turned off\n" ); )
	}
	
	id->ControlPanel = FALSE;
}

/****************************************************************************/

IPTR __regargs Sound_NEW( Class *cl, Object *o, struct opSet *ops )
{
	struct ClassBase	*cb = (struct ClassBase *)cl->cl_UserData;
	struct TagItem	ti[6];

	dbug( kprintf( "OM_NEW\n" ); )
	
	GetSoundDTPrefs( cb );

	ObtainSemaphoreShared( &cb->cb_LibLock );
	ti[0].ti_Tag = DTA_NominalHoriz;
	ti[0].ti_Data = cb->cb_NomWidth;
	ti[1].ti_Tag = DTA_NominalVert;
	ti[1].ti_Data = cb->cb_NomHeight;
	ti[2].ti_Tag = DTA_VertUnit;
	ti[2].ti_Data = 1L;
	ti[3].ti_Tag = DTA_HorizUnit;
	ti[3].ti_Data = 1L;
	ti[4].ti_Tag = DTA_ControlPanel;
	ti[4].ti_Data = cb->cb_ControlPanel;
	ti[5].ti_Tag = TAG_MORE;
	ti[5].ti_Data = (IPTR) ops->ops_AttrList;
	ReleaseSemaphore( &cb->cb_LibLock );
		
	ops->ops_AttrList = ti;
	
	if( ( o = (Object *) DoSuperMethodA( cl, o, (Msg) ops ) ) )
	{
		struct InstanceData	*id = INST_DATA( cl, o );
		struct ObjectMsg		*msg;
		struct MsgPort		*replyport;

		id->Volume = cb->cb_Volume;
		id->Frequency = Period2Freq( 394 );
		id->Cycles = 1;
		id->SignalBit = -1L;
//		id->SampleType = SDTST_M8S;
		id->Panning = 0x8000;
		/* 'magic' nothing selected value */
		id->MinX = 
		id->MaxX = ~0L;
		id->StartSample =
		id->EndSample = ~0L;
		id->TapeDeckHeight = 15L; // tapedeck.gadget is currently limited to 15 pixels
		id->BackgroundPen =
		id->WaveformPen = -1;
		id->ClassBase = cb;
		id->FreeSampleData = TRUE;
//		id->SyncSampleChange = FALSE;
		InitSemaphore( &id->Lock );
		
		/* create process */
		replyport = CreateMsgPort();
	
		if( ( msg = (struct ObjectMsg *) CreateIORequest( replyport, sizeof( *msg ) ) ) )
		{
			ObtainSemaphoreShared( &cb->cb_LibLock );

			if( ( id->PlayerProc = CreateNewProcTags(
				NP_Name, (IPTR) "sound.datatype",
				NP_Entry, (IPTR) (cb->cb_AHI) ? (IPTR) PlayerProcAHI : (IPTR) PlayerProc,
				NP_Priority, 19L,
				TAG_DONE ) ) )
			{
				msg->Data = (APTR) id;
				msg->Command = COMMAND_INIT;
				PutMsg( &id->PlayerProc->pr_MsgPort, &msg->Message );
				WaitPort( replyport );
				GetMsg( replyport );
			
				if( msg->Data )
				{
					id->PlayerPort = (struct MsgPort *) msg->Data;
				}
				else
				{
					id->PlayerProc = NULL;
				}
			}
		
			ReleaseSemaphore( &cb->cb_LibLock );
		
			DeleteIORequest( (struct IORequest *) msg );
			
			if( id->PlayerProc )
			{				
				parsetaglist( cl, o, ops, NULL );	
				CreateTapeDeck( cb, id, o );
			}
			else
			{
				CoerceMethod( cl, o, OM_DISPOSE );
				o = NULL;
			}
		}
		else
		{
			dbug( kprintf( "No memory for ioreq or msgport\n" ); )
		}
	
		DeleteMsgPort( replyport );
	}
	else
	{
		dbug( kprintf( "OM_NEW failed\n" ); )
	}
	
	ops->ops_AttrList = (struct TagItem *) ti[5].ti_Data;
	
	dbug( if( !o ) kprintf("Object creation failed\n"); )
	
	return( (IPTR) o );
}

/****************************************************************************/

void __regargs SetVSlider( struct ClassBase *cb, struct InstanceData *id )
{
	/* this function works aroung a "spilled register" error */
	if( id->ControlPanel )
	{
		IPTR	top;
		GetAttr( PGA_Top, (Object *)id->VolumeSlider, &top );
		if( top != id->Volume ) // avoid loops
		{
			SetGadgetAttrs( id->VolumeSlider, id->Window, id->Requester,
				PGA_Top, id->Volume, TAG_DONE );
		}
	}
}

/****************************************************************************/

BOOL __regargs parsetaglist( Class *cl, Object *o, struct opSet *ops, ULONG *cnt_p )
{
	struct ClassBase	*cb = (struct ClassBase *) cl->cl_UserData;
	struct TagItem	*ti, *tstate = ops->ops_AttrList;
	struct InstanceData *id = INST_DATA( cl, o );
	BOOL			pervol = FALSE, newSample = FALSE;
	LONG			cnt = 0L;

	ObtainSemaphore( &id->Lock );
dbug( kprintf( "NextTagItem\n" ); )
	while( ( ti = NextTagItem( (const struct TagItem **)&tstate ) ) )
	{
		IPTR	data = ti->ti_Data;
		
		switch( ti->ti_Tag )
		{
			case SDTA_VoiceHeader:
				id->VoiceHeader = *(struct VoiceHeader *) data;
				dbug( kprintf("SDTA_VoiceHeader\n"); )
				cnt++;
			break;
			
			case SDTA_Sample:
				id->Sample = (BYTE *) data;
				id->LeftSample = FALSE;
				dbug( kprintf("SDTA_Sample = %08lx\n", data); )
				newSample = TRUE;
				cnt++;
			break;
			
			case SDTA_SampleLength:
				id->SampleLength = data;
				dbug( kprintf("SDTA_SampleLength = %ld\n", data); )
				cnt++;
			break;
			
			case SDTA_Period:
				id->Frequency = (UWORD) Period2Freq( data );
				pervol = TRUE;
				dbug( kprintf("SDTA_Period = %ld (%lDHz)\n", data, Period2Freq(data) ); )
				cnt++;
			break;
			
			case SDTA_Volume:
				id->Volume = (UWORD) data;
				pervol = TRUE;
				
				SetVSlider( cb, id );
				
				dbug( kprintf("SDTA_Volume = %ld\n", data); )
				
				cnt++;
			break;
			
			case SDTA_Cycles:
				id->Cycles = (UWORD) data;
				dbug( kprintf("SDTA_Cycles = %ld\n", data); )
				cnt++;
			break;
			
			case SDTA_SignalTask:
				id->SignalTask = (struct Task *) data;
				dbug( kprintf("SDTA_SignalTask = %08lx\n", data); )
				cnt++;
			break;
			
			case SDTA_SignalBitMask: // aka: SDTA_SignalBit
				dbug( kprintf("SDTA_SignalBit(Mask) = %08lx\n", data); )
				if( data != 0L )
				{
					LONG	i;
					
					for( i = 0; i < 32; i++ )
					{
						if( ( 1L << i ) & data )
						{
							id->SignalBit = i;
							break;
						}
					}
				}
				else id->SignalBit = -1;
				cnt++;
			break;
			
			case SDTA_Continuous:
				id->Continuous = (BOOL) data;
				dbug( kprintf("SDTA_Continuous = %ld\n", data); )
				cnt++;
			break;

			case SDTA_SampleType:
				id->SampleType = data;
				dbug( kprintf("SDTA_SampleType = %ld\n", data ); )
				cnt++;
			break;
			
			case SDTA_Pan:
			case SDTA_Panning:
				id->Panning = data;
				pervol = TRUE;
				dbug( kprintf("SDTA_Panning = %05lx\n", data ); )
				cnt++;
			break;
			
			case SDTA_SamplesPerSec:
			case SDTA_Frequency:
				id->Frequency = data;
				pervol = TRUE;
				dbug( kprintf("SDTA_Frequency/SamplesPerSec =%ld\n", data); )
				cnt++;
			break;
			
			case DTA_ControlPanel:
				if( data != id->ControlPanel )
				{
					id->ForceRefresh = TRUE;
				}
			
				id->ControlPanel = (BOOL) data;
				dbug( kprintf("DTA_ControlPanel =%ld\n", data); )
				cnt++;
			break;
			
			case DTA_Immediate:
				if( ! ( id->Immediate = data ) )
				{
					CoerceMethod( cl, o, DTM_TRIGGER, 0, STM_STOP, 0 );
				}
				else
				{
					newSample = ( id->Sample != NULL );
					id->DelayedImmed = TRUE;					
				}
				dbug( kprintf("DTA_Immediate = %ld\n", data ); )
				cnt++;
			break;
			
			case DTA_Repeat:
				id->Repeat = (BOOL) data;
				dbug( kprintf("DTA_Repeat = %ld\n", data ); )
				cnt++;
			break;
						
			case SDTA_SyncSampleChange:
				id->SyncSampleChange = data;
				dbug( kprintf("SDTA_SyncSampleChange = %ld\n", data ); )
				cnt++;
			break;
			
			case SDTA_FreeSampleData:
				id->FreeSampleData = data;
				dbug( kprintf("SDTA_FreeSampleData = %ld\n", data ); )
				cnt++;
			break;

			case SDTA_LeftSample:
				id->LeftSample = TRUE;
				id->Sample = (BYTE *) data;
				newSample = TRUE;
				dbug( kprintf("SDTA_LeftSample = %08lx\n", data ); )
				cnt++;
			break;
			
			case SDTA_RightSample:
				id->RightSample = (BYTE *) data;
				newSample = TRUE;
				dbug( kprintf("SDTA_RightSample = %08lx\n", data ); )
				cnt++;
			break;
			
			case SDTA_SignalBitNumber:
				id->SignalBit = data;
				dbug( kprintf("SDTA_SignalBitNumber = %ld\n", data ); )
				cnt++;
			break;
			
			default:
				dbug( kprintf("Attr: %08lx\n", ti->ti_Tag); )
				break;
		}
	}
	
	ReleaseSemaphore( &id->Lock );
dbug( kprintf( "NextTagItem done.\n" ); )	
	/*- inform sound object handler about that change -*/
	if( pervol && id->PlayerProc )
	{dbug( kprintf( "NextTagItem 2\n" ); )
		SendObjectMsg( id, COMMAND_PERVOL, NULL );
	}
	
	if( newSample )
	{dbug( kprintf( "NextTagItem 3\n" ); )
		/* continuous stream of data ? */
		if( id->Continuous )
		{
			/* inform player process that we got a new buffer */
			SendObjectMsg( id, COMMAND_NEXT_BUFFER, NULL );
			
			if( id->SyncSampleChange )
			{
			}
		}
		else if( id->Immediate && id->DelayedImmed )
		{
			id->DelayedImmed = FALSE;
			CoerceMethod( cl, o, DTM_TRIGGER, 0, STM_PLAY, 0 );
		}
	}
	dbug( kprintf( "NextTagItem4\n" ); )
	
	if( cnt_p ) *cnt_p = cnt;
	
	return( pervol );
}

/****************************************************************************/

IPTR __regargs Sound_GET( Class *cl, Object *o, struct opGet *opg )
{
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	IPTR				retval = TRUE, data;
	
	dbug( kprintf("OM_GET\n"); )
	
	switch( opg->opg_AttrID )
	{
		case SDTA_VoiceHeader:
			data = (IPTR) &id->VoiceHeader;
			dbug( kprintf("SDTA_VoiceHeader\n"); )
		break;
		
		case SDTA_Sample:
			data = (IPTR) ( ( id->LeftSample ) ? NULL : id->Sample );
			dbug( kprintf("SDTA_Sample\n"); )
		break;
		
		case SDTA_SampleLength:
			data = id->SampleLength;
			dbug( kprintf("SDTA_SampleLength\n"); )
		break;
		
		case SDTA_Period:
			data = (IPTR) Freq2Period( id->Frequency );
			dbug( kprintf("SDTA_Period\n"); )
		break;
		
		case SDTA_Volume:
			data = (IPTR) id->Volume;
			dbug( kprintf("SDTA_Volume\n"); )
		break;
		
		case SDTA_Cycles:
			data = (IPTR) id->Cycles;
			dbug( kprintf("SDTA_Cycles\n"); )
		break;
		
		case SDTA_SignalTask:
			data = (IPTR) id->SignalTask;
			dbug( kprintf("SDTA_SignalTask\n"); )
		break;
		
		case SDTA_SignalBitMask: // aka SDTA_SignalBit
			data = (IPTR) ( id->SignalBit == -1L ) ? 0L : ( 1L << id->SignalBit );
			dbug( kprintf("SDTA_SignalBit(Mask)\n"); )
		break;
		
		case SDTA_SignalBitNumber:
			data = id->SignalBit;
			dbug( kprintf("SDTA_SignalBitNumber\n"); )
		break;
		
		case SDTA_Continuous:
			data = (IPTR) id->Continuous;
			dbug( kprintf("SDTA_Continuous\n"); )
		break;

		case SDTA_Pan:
		case SDTA_Panning:
			data = id->Panning;
			dbug( kprintf("SDTA_Pan(ning)\n"); )
		break;
		
		case SDTA_SampleType:
			data = id->SampleType;
			dbug( kprintf("SDTA_SampleType\n"); )
		break;
		
		case SDTA_SamplesPerSec:
		case SDTA_Frequency:
			data = id->Frequency;
			dbug( kprintf("SDTA_Frequency/SamplesPerSec\n"); )
		break;
		
		case DTA_TriggerMethods:
			data = (IPTR) TriggerMethods;
			dbug( kprintf("SDTA_TriggerMethods\n"); )
		break;
		
		case DTA_ControlPanel:
			data = (IPTR) id->ControlPanel;
			dbug( kprintf("DTA_ControlPanel\n"); )
		break;
		
		case DTA_Repeat:
			data = (IPTR) id->Repeat;
			dbug( kprintf("DTA_Repeat\n"); )
		break;

		case DTA_Immediate:
			data = (IPTR) id->Immediate;
			dbug( kprintf("DTA_Immediate\n"); )
		break;

		case DTA_Methods:
		{
			dbug( kprintf( "DTA_Methods\n"); )
#ifndef __GNUC__ 
			ObtainSemaphore( &cb->cb_LibLock );
			/* first request of DTA_Methods ? */
			if( ! ( data = (IPTR) cb->cb_Methods ) )
			{
				ULONG	cntSuper, cntMethods, *superMethods, *newMethods;
				/* create Methods array */
				DoSuperMethod( cl, o, OM_GET, DTA_Methods, (IPTR) &superMethods );
				
				for( cntSuper = 0; superMethods[ cntSuper ] != ~0L; cntSuper++ ) {}
				for( cntMethods = 0; Methods[ cntMethods ] != ~0L; cntMethods++ ) {}
				
				cb->cb_Methods = // For any reasons, GNUC seems to have problems here !
				newMethods = (ULONG *) AllocVec( ( cntSuper+cntMethods+1UL ) * sizeof( ULONG ), MEMF_PUBLIC );
				
				if( data = (ULONG) newMethods )
				{
					ULONG	num = 0L;

					CopyMem( superMethods, newMethods, sizeof( ULONG ) * cntSuper );
					newMethods += cntSuper;
				
					for( num = 0; num < cntMethods; num++ )
					{
						ULONG	i, method = Methods[ num ];
						
						for( i = 0; i < cntSuper; i++ )
						{
							if( superMethods[ i ] == method )
							{
								break;
							}
						}
						
						if( i == cntSuper )
						{
							*newMethods++ = method;
						}
					}
					
					*newMethods = ~0L;
				}
				else 
				{
					data = (ULONG) Methods;
				}
			}
			
			ReleaseSemaphore( &cb->cb_LibLock );
#else
			ObtainSemaphore( &cb->cb_LibLock );
			
			if( ! ( data = (IPTR) cb->cb_Methods ) )
			{
				if( DataTypesBase->lib_Version >= 45L )
				{
					ULONG	*superMethods = NULL;
					
					DoSuperMethod( cl, o, OM_GET, DTA_Methods, (ULONG) &superMethods );
					
					data = (ULONG) ( cb->cb_Methods = CopyDTMethods( superMethods, Methods, NULL ) );
					
					dbug( kprintf( "CopyDTMethods returned %08lx\n", data ); )
				}
			}
			
			if( ! data )
			{
				data = (IPTR) Methods;
			}
			
			ReleaseSemaphore (&cb->cb_LibLock );
#endif
		}
		break;
		
		case SDTA_SyncSampleChange:
			data = id->SyncSampleChange;
			dbug( kprintf("SDTA_SyncSampleChange\n"); )
		break;
		
		case SDTA_FreeSampleData:
			data = id->FreeSampleData;
			dbug( kprintf("SDTA_FreeSampleData\n"); )
		break;

		case SDTA_LeftSample:
			data = (IPTR) ( ( id->LeftSample ) ? id->Sample : NULL );
			dbug( kprintf("SDTA_LeftSample\n"); )
		break;
		
		case SDTA_RightSample:
			data = (IPTR) id->RightSample;
			dbug( kprintf("SDTA_RightSample\n"); )
		break;
		
		case SDTA_ReplayPeriod:
		{
			ULONG	secs, micro;
						
			data = (IPTR) &id->ReplayPeriod;
			
			if( id->Continuous )
			{
				secs = 
				micro = -1L;
			}
			else if( id->SampleLength )
			{				
				secs = UDiv( id->SampleLength, id->Frequency );				
				micro = UMult( UDiv( 1000000, id->Frequency ), ( id->SampleLength % id->Frequency ) );
			}
			else
			{
				secs =
				micro = 0L;
			}
			
			id->ReplayPeriod.tv_secs  = secs;
			id->ReplayPeriod.tv_micro = micro;
			
			dbug( kprintf("SDTA_ReplayPeriod %ld:%ld\n", secs, micro ); )
		}
		break;
		
		default:
			retval = FALSE;
 			dbug( kprintf("Attr: %08lx\n",  opg->opg_AttrID); )
	}
	
	if( retval )
	{
		*opg->opg_Storage = data;
	}
	else
	{		
		retval = DoSuperMethodA( cl, o, (Msg) opg );
		dbug( kprintf("SuperAttr: %08lx, %08lx\n",  opg->opg_AttrID, *opg->opg_Storage); )
	}
	
	return( retval );
}

/****************************************************************************/

IPTR __regargs Sound_SET( Class *cl, Object *o, struct opSet *ops )
{
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	IPTR				retval, cnt;

    	(void)cb;
			
	dbug( kprintf("OM_SET\n"); )

	retval = DoSuperMethodA( cl, o, (Msg) ops );
	dbug( kprintf("parsing list\n"); )
	parsetaglist( cl, o, ops, &cnt );
	dbug( kprintf("refreshing\n"); )
	if( retval || ( ( id->ForceRefresh ) && ( id->Gadget ) ) )
	{
		struct RastPort	*rp;
		
		if( ( rp = ObtainGIRPort( ops->ops_GInfo ) ) )
		{
			DoMethod( o, GM_RENDER, (IPTR) ops->ops_GInfo, (IPTR) rp, GREDRAW_REDRAW );
			ReleaseGIRPort( rp );
			
			id->ForceRefresh = FALSE;
		}
	}
	dbug( kprintf("leaving OM_SET\n"); )
	return( retval+cnt );
}

/****************************************************************************/

IPTR __regargs Sound_UPDATE( Class *cl, Object *o, struct opUpdate *opu )
{
	struct ClassBase	*cb = (struct ClassBase *)cl->cl_UserData;
	STATIC IPTR		 methodID = ICM_CHECKLOOP;	

    	(void)cb;
	
	dbug( kprintf("OM_UPDATE\n"); )
	
	if( DoSuperMethodA( cl, o, (Msg) &methodID ) )
	{
		return FALSE;
	}	

	dbug( kprintf("no loop %08lx\n", (ULONG)opu); )

	return Sound_SET( cl, o, (struct opSet *) opu );
}

/****************************************************************************/

IPTR __regargs Sound_DISPOSE( Class *cl, Object *o, Msg msg )
{
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct SignalSemaphore	*lock = &((struct DTSpecialInfo *)G( o )->SpecialInfo)->si_Lock;
	
	(void)cb;
	
	dbug( kprintf("OM_DISPOSE\n"); )
	
	/* free playerproc stuff */
	if( id->PlayerProc )
	{
		struct MsgPort	*replyport;
		struct ObjectMsg	*msg;
		
		while( ! ( replyport = CreateMsgPort() ) )
		{
			Delay( 25L );
		}
		
		while( ! ( msg = (struct ObjectMsg *) CreateIORequest( replyport, sizeof( *msg ) ) ) )
		{
			Delay( 25L );
		}
		
		msg->Command = COMMAND_EXIT;
		PutMsg( id->PlayerPort, &msg->Message );
		WaitPort( replyport );
		GetMsg( replyport );
		
		DeleteIORequest( (struct IORequest *) msg );
		DeleteMsgPort( replyport );
	}
	/* free controls */
	dbug( kprintf( "freeing controls\n" ); )
	DisposeObject( id->TapeDeckGadget );
	DisposeObject( id->VolumeSlider );
	
	/* free sample memory, obtain semaphore in case that there's a write in progess */
	dbug( kprintf( "releasing memory\n" ); )

	ObtainSemaphore( lock );
	if( id->FreeSampleData && ! id->Continuous )
	{			
		FreeVec( id->Sample );		
		if( id->Sample != id->RightSample ) FreeVec( id->RightSample );
	}
	ReleaseSemaphore( lock );

	return( DoSuperMethodA( cl, o, msg ) );
}

/****************************************************************************/

IPTR __regargs Sound_DOMAIN( Class *cl, Object *o, struct gpDomain *gpd )
{
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	struct InstanceData 	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct IBox			*gbox = &gpd->gpd_Domain;

	dbug( kprintf("GDOMAIN\n"); )
	
	*gbox = GetAbsGadgetBox( &gpd->gpd_GInfo->gi_Domain, EXTG( o ), FALSE );

	switch( gpd->gpd_Which )
	{
		case GDOMAIN_MINIMUM:
			gbox->Width = 32;
			gbox->Height = id->TapeDeckHeight + 10;
		break;
		
		case GDOMAIN_NOMINAL:
			ObtainSemaphoreShared( &cb->cb_LibLock );
			gbox->Width = cb->cb_NomWidth;
			gbox->Height = cb->cb_NomHeight;
			ReleaseSemaphore( &cb->cb_LibLock );
		break;
		
		case GDOMAIN_MAXIMUM:
			gbox->Width = 
			gbox->Height = SHRT_MAX;
		break;
	}
	
	return TRUE;
}

/****************************************************************************/

IPTR __regargs Sound_LAYOUT( Class *cl, Object *o, struct gpLayout *gpl )
{
	struct ClassBase		*cb = (struct ClassBase *)cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct DTSpecialInfo	*si = (struct DTSpecialInfo *)(G( o ) -> SpecialInfo);
	struct GadgetInfo		*gi = gpl->gpl_GInfo;
	struct IBox			*domain = NULL;
	STRPTR				name = NULL;
	IPTR				retval;

	dbug( kprintf( "GM_LAYOUT\n" ); )

	retval = DoSuperMethodA( cl, o, (Msg) gpl );

	si->si_Flags |= DTSIF_LAYOUT;
	ObtainSemaphore( &si->si_Lock );
	
    #ifndef __AROS__	
	NotifyAttrs( o, gi, 0L,
		GA_ID, G( o )->GadgetID,
		DTA_Busy, TRUE,
		TAG_DONE );
    #else
    	{
	    struct TagItem tags[] =
	    {
	    	{GA_ID, G( o )->GadgetID},
		{DTA_Busy, TRUE},
		{TAG_DONE}
	    };
	    
	    DoMethod(o, OM_NOTIFY, (IPTR)tags, (IPTR)gi, 0); 
	}
    #endif
    
	if( GetDTAttrs( o, 
		DTA_ObjName, (IPTR) &name,
		DTA_Domain, (IPTR) &domain,
		TAG_DONE ) == 2L )
	{	
		ULONG	vunit = si->si_VertUnit,
				hunit = si->si_HorizUnit,
				totalw = si->si_TotHoriz,
				totalh = si->si_TotVert;
	
		si->si_VertUnit = vunit;
		si->si_VisVert = domain->Height / vunit;
		si->si_TotVert = totalh;
		
		si->si_HorizUnit = hunit;
		si->si_VisHoriz = domain->Width / hunit;
		si->si_TotHoriz = totalw;

		if( ! name )
		{
			if( ! GetDTAttrs( o, DTA_Name, (IPTR) &name, TAG_DONE ) || ! name )
			{
				name = "Untitled";
			}
			else
			{
				name = FilePart( name );
			}
		}

		if( gpl->gpl_Initial )
		{
			ObtainSemaphore( &id->Lock );
			id->Window = gi->gi_Window;
			id->Requester = gi->gi_Requester;
			id->Gadget = (struct Gadget *) o;
			ReleaseSemaphore( &id->Lock );
			/* obtain pens */
			id->ColorMap = gi->gi_Screen->ViewPort.ColorMap;
			
			ObtainSemaphoreShared( &cb->cb_LibLock );
			id->BackgroundPen = ObtainBestPenA( id->ColorMap, 
				cb->cb_BgCol[0], cb->cb_BgCol[1], cb->cb_BgCol[2], NULL );
			id->WaveformPen = ObtainBestPenA( id->ColorMap, 
				cb->cb_WfCol[0], cb->cb_WfCol[1], cb->cb_WfCol[2], NULL );
			ReleaseSemaphore( &cb->cb_LibLock );
		}
	
		if( id->TapeDeckGadget )
		{	
			struct gpLayout	member_gpl;				
			struct TagItem	ti[] = {
				{GA_Left, domain->Left},
				{GA_Top, domain->Top + domain->Height - id->TapeDeckHeight},
				{GA_Width, 201},
				{GA_Height, id->TapeDeckHeight},
				{TAG_DONE }
			};
			
			if( ( gpl->gpl_Initial ) && ( G( o )->GadgetType & GTYP_REQGADGET ) )
			{	
				id->TapeDeckGadget->GadgetType |= GTYP_REQGADGET;
				id->VolumeSlider->GadgetType |= GTYP_REQGADGET; 
			}
			
			ObtainSemaphoreShared( &cb->cb_LibLock );
		
			member_gpl = *gpl;
			member_gpl.MethodID = GM_LAYOUT;
			
			SetAttrsA( id->TapeDeckGadget, ti );
				
			DoMethodA( (Object *) id->TapeDeckGadget, (Msg) &member_gpl );
			
			ti[0].ti_Data += 205;
			ti[2].ti_Data = domain->Width - 205;
	
			if( ! cb->cb_NoGTSlider )
			{
				ti[0].ti_Data += 4;
				ti[1].ti_Data += 2;
				ti[2].ti_Data -= 8;
				ti[3].ti_Data -= 4;
			}
			
			SetAttrsA( id->VolumeSlider, ti );
			
			DoMethodA( (Object *) id->VolumeSlider, (Msg) &member_gpl );
		
			ReleaseSemaphore( &cb->cb_LibLock );
		}
		
    	    #ifndef __AROS__		
		NotifyAttrs( o, gi, 0L,
			GA_ID, G(o)->GadgetID,
			DTA_VisibleVert, si->si_VisVert,
			DTA_TotalVert, totalh,
			DTA_VertUnit, vunit,
			DTA_VisibleHoriz, si->si_VisHoriz,
			DTA_TotalHoriz, totalw,
			DTA_HorizUnit, hunit,
			DTA_Title, (IPTR) name,
			DTA_Busy, FALSE,
			DTA_Sync, TRUE,
			TAG_DONE );
    	    #else
	    {
		struct TagItem tags[] =
		{
		    {GA_ID, G(o)->GadgetID},
		    {DTA_VisibleVert, si->si_VisVert},
		    {DTA_TotalVert, totalh},
		    {DTA_VertUnit, vunit},
		    {DTA_VisibleHoriz, si->si_VisHoriz},
		    {DTA_TotalHoriz, totalw},
		    {DTA_HorizUnit, hunit},
		    {DTA_Title, (IPTR) name},
		    {DTA_Busy, FALSE},
		    {DTA_Sync, TRUE},
		    {TAG_DONE}
		};

		DoMethod(o, OM_NOTIFY, (IPTR)tags, (IPTR)gi, 0); 
	    }

	    #endif					
	}

	ReleaseSemaphore( &si->si_Lock );
	si->si_Flags &= ~DTSIF_LAYOUT;	

	return retval + si->si_TotVert;
}

/****************************************************************************/

void __regargs DrawWaveform( struct ClassBase *cb, struct RastPort *rp, struct InstanceData *id, UWORD x, UWORD y, UWORD w, UWORD h )
{
	ULONG	sampleStart = id->StartSample, sampleEnd = id->EndSample;
	LONG	samplelength = id->SampleLength, error, dx = w, dy = samplelength, sub, add, i, 
			step = 1L, k=0L, oldx=x, shift = ( id->SampleType >= SDTST_M16S ? 1 : 0 );
	BOOL	stereo = IsStereo( id->SampleType );
	WORD	minX = ~0, maxX = ~0, oldy = y, oldh = h;
	/* scaling limit is 25500% */
	if( w > (samplelength<<8) )
	{
		return;
	}
	/* a big sample, speed up rendering */
	if( samplelength >> 17 )
	{
		dy >>= 7;
		step = 128;
	}
	/* adjust ypos and height for stereo sample */
	if( stereo )
	{
		h /= 2;
		y += h/2;
		shift++;
	}
	else
	{
		y+= h/2;
	}
	
	for( k = 0; k < (stereo?2:1); k++ )
	{
		BYTE	*sample = &id->Sample[ k ];
		
		if( k && id->SampleType == SDTST_S16S )
		{
			sample++;
		}
		
		Move( rp, x, y );
		
		if( samplelength <= w )
		{
			LONG	c = 0L;
	
			error = dy-2*dx-1;	
			sub = 2*(dx+1); add = 2*(dy+1); 
			
			for( i = 0; i < w; i++ ) 
			{
				/* get minX and maxX for highlighting */
				if( sampleStart != ~0L )
				{
					if( i >= sampleStart && minX == ~0 )
					{
						minX = x;
					}
					else if( i >= sampleEnd && maxX == ~0 )
					{
						maxX = x;
						sampleStart = ~0L; // speed up
					}
				}
					
				Draw( rp, x++, y + ( SMult( sample[ c << shift ], h ) >> 8 ) );
		
				if( error > 0 ) 
				{
					c += step;
					error -= sub;
				}
				
				error += add;
			}
		}
		else
		{		
			error = dx-2*dy-1;   	
			sub = 2*(dy+1); add = 2*(dx+1);
			
			for( i = 0; i < samplelength; i+=step )
			{
				if( error > 0 )
				{
					/* get minX and maxX for highlighting */
					if( sampleStart != ~0L )
					{
						if( i >= sampleStart && minX == ~0 )
						{
							minX = x;
						}
						else if( i >= sampleEnd && maxX == ~0 )
						{
							maxX = x;
							sampleStart = ~0L; // speed up
						}
					}
						
					Draw( rp, x++, y + ( SMult( sample[ i << shift ], h ) >> 8 ) );
					error -= sub;
				} 
				
				error += add;
			}
		}
	
		y += h;
		x = oldx;
	}
	/* something marked ? */
	if( minX != ~0 )
	{
		if( maxX == ~0 )
		{
			maxX = oldx+w-1;
		}

		SetDrMd( rp, COMPLEMENT );
		RectFill( rp, minX, oldy, maxX, oldy+oldh-1 );
		/* fix tapedeck.gadget bug (?) */
		SetDrMd( rp, JAM1 );
	}
}

/****************************************************************************/

IPTR __regargs Sound_RENDER( Class *cl, Object *o, struct gpRender *gpr )
{
	struct ClassBase		*cb = (struct ClassBase *)cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	
	dbug( kprintf( "GM_RENDER\n" ); )

//	if( gpr->gpr_Redraw == GREDRAW_REDRAW )
	{
		struct RastPort		*rp = gpr->gpr_RPort;
		struct ColorMap		*cm = NULL;
		struct IBox			gbox;
		LONG				bgpen=id->BackgroundPen, wfpen=id->WaveformPen;
		UWORD				*pens = gpr->gpr_GInfo->gi_DrInfo->dri_Pens;
		WORD				x,y, w,h;
			
		gbox = GetAbsGadgetBox( &gpr->gpr_GInfo->gi_Domain, EXTG( o ), FALSE );
		x = gbox.Left; y = gbox.Top;
		w = gbox.Width; h = gbox.Height;
		
		if( id->ControlPanel )
		{
			h -= ( id->TapeDeckHeight + 2 );
		}
		
		if( ( x != SHRT_MAX && y != SHRT_MAX ) && ( w >=16 && h >= 8 ) )
		{
			if( gpr->gpr_GInfo->gi_Screen )
			{
				cm = gpr->gpr_GInfo->gi_Screen->ViewPort.ColorMap;
			
			}
				
			SetAPen( rp, ( bgpen == -1L ) ? pens[TEXTPEN] : bgpen );
			RectFill( rp, x,y, w+x-1, h+y-1 );

			x += 4; y += 2;
			w -= 8; h -= 4;
				
			SetAPen( rp, ( wfpen == -1L ) ? pens[HIGHLIGHTTEXTPEN] : wfpen );		
			
			DrawWaveform( cb, rp, id, x, y, w, h );
		}
		
		if( id->ControlPanel )
		{
/*			SetAPen( rp, 4L );
			RectFill( rp, gbox.Left, gbox.Top+=(gbox.Height-id->TapeDeckHeight-3),
				gbox.Left+gbox.Width-1, gbox.Top+id->TapeDeckHeight+1 );*/
			
			if( gbox.Height >= ( id->TapeDeckHeight + 2 ) )
			{
				if( gbox.Width >= 201)
				{
					DoMethodA( (Object *)id->TapeDeckGadget, (Msg) gpr );
					
					if( gbox.Width >= 220 )
					{
						DoMethodA( (Object *) id->VolumeSlider, (Msg) gpr );
						
						if( ! cb->cb_NoGTSlider )
						{
							Object	*img;
							
							if( ( img = NewObject( NULL, FRAMEICLASS,
								IA_Left, id->VolumeSlider->LeftEdge - 4,
								IA_Top,  id->VolumeSlider->TopEdge - 2,
								IA_Width, id->VolumeSlider->Width + 8,
								IA_Height, id->TapeDeckHeight,
								IA_FrameType, FRAME_BUTTON,
								IA_EdgesOnly, TRUE,
								TAG_DONE ) ) )
							{
								DrawImageState( rp, (struct Image *)img, 0,0, IDS_NORMAL, gpr->gpr_GInfo->gi_DrInfo );
								DisposeObject( img );
							}	
						}
					}
				}
			}
		}
		
		if( G( o )->Flags & GFLG_DISABLED )
		{
			ULONG patt = 0x11114444;
			
			SetAfPt(rp, (UWORD *)&patt, 1);
			SetAPen( rp, pens[ SHADOWPEN ] );
			RectFill( rp, gbox.Left, gbox.Top, gbox.Left+gbox.Width-1, gbox.Top+gbox.Height-1 );
			SetAfPt( rp, NULL, 0L );
		}
	}/*
	else if( gpr->gpr_Redraw == GREDRAW_UPDATE )
	{
	}*/

	return( TRUE );
}

/****************************************************************************/

IPTR __regargs Sound_DRAW( Class *cl, Object *o, struct dtDraw *dtd )
{
	struct InstanceData	*id = INST_DATA( cl, o );
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	struct RastPort		*rp = dtd->dtd_RPort;
	struct DrawInfo		*dri;
	struct Screen			*scr;
	struct ColorMap		*cm = NULL;
	UWORD				x = dtd->dtd_Left, y = dtd->dtd_Top, *pens,
						w = dtd->dtd_Width, h = dtd->dtd_Height;
	WORD				bgpen = -1, wfpen = -1;
	BOOL				freedri;
	
	dbug( kprintf( "DTM_DRAW\n" ); )

	if( ( scr = id->Screen ) )
	{
		dri = GetScreenDrawInfo( scr );
		freedri = TRUE;
	}
	else 
	{
		dri = id->DrawInfo;
		freedri = FALSE;
	}
	
	if( dri )
	{
		pens = dri->dri_Pens;
		
		if( scr )
		{
			cm = scr->ViewPort.ColorMap;
			
			ObtainSemaphoreShared( &cb->cb_LibLock );
			bgpen = ObtainBestPenA( cm, 
				cb->cb_BgCol[0], cb->cb_BgCol[1], cb->cb_BgCol[2], NULL );
			wfpen = ObtainBestPenA( cm, 
				cb->cb_WfCol[0], cb->cb_WfCol[1], cb->cb_WfCol[2], NULL );
			ReleaseSemaphore( &cb->cb_LibLock );
		}
				
		SetAPen( rp, ( bgpen == -1L ) ? pens[TEXTPEN] : bgpen );
		RectFill( rp, x,y, w+x-1, h+y-1 );
	
		x += 4; y += 2;
		w -= 8; h -= 4;
				
		SetAPen( rp, ( wfpen == -1L ) ? pens[HIGHLIGHTTEXTPEN] : wfpen );		
			
		DrawWaveform( cb, rp, id, x, y, w, h );
			
		if( cm )
		{
			ReleasePen( cm, bgpen );
			ReleasePen( cm, wfpen );
		}
			
		if( freedri )
		{
			FreeScreenDrawInfo( scr, dri );
		}
	}
	
	return( dri != NULL );
}

/****************************************************************************/

IPTR __regargs Sound_HITTEST( Class *cl, Object *o, struct gpHitTest *gpht )
{
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	IPTR				retval = GMR_GADGETHIT;
	WORD				h = G(o)->Height;

	dbug( kprintf( "GM_HITTEST\n" ); )
	
	if( ! ( G( o )->Flags & GFLG_DISABLED ) )
	{	
		/* calculate absolute height */
		if( G(o)->Flags & GFLG_RELHEIGHT )
		{
			h += gpht->gpht_GInfo->gi_Domain.Height;
		}
		/* check if one of our members was hit */
		if( gpht->gpht_Mouse.Y > ( h - ( id->TapeDeckHeight + 3 ) ) )
		{
			if( id->ControlPanel )
			{
				struct IBox	domain;
		
				domain = GetAbsGadgetBox( &gpht->gpht_GInfo->gi_Domain, EXTG( o ), FALSE );
				
				if( ! DoMemberHitTest( &domain, (Object *) id->TapeDeckGadget, gpht ) )
				{
					if( ! DoMemberHitTest( &domain, (Object *) id->VolumeSlider, gpht ) )
					{
						retval = 0L;
					}
				}
			}
		}
		else
		{
			/* We were hit, change return code if it was GM_HELPTEST */
			if( gpht->MethodID == GM_HELPTEST )
			{
				retval = GMR_HELPHIT;
			}
		}
	}
	else
	{
		retval = 0L;
	}
	
	return retval;
}

/****************************************************************************/

IPTR __regargs Sound_HANDLEINPUT( Class *cl, Object *o, struct gpInput *gpi )
{
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct InputEvent		*ie = gpi->gpi_IEvent;
	struct DTSpecialInfo	*si = (struct DTSpecialInfo *)G( o )->SpecialInfo;
	struct IBox			gbox = GetAbsGadgetBox( &gpi->gpi_GInfo->gi_Domain, EXTG( o ), FALSE );
	IPTR				retval = GMR_MEACTIVE;
	WORD				mx = gpi->gpi_Mouse.X, my = gpi->gpi_Mouse.Y;
    	
	(void)cb;
	
	dbug( kprintf("GM_GOACTIVE / GM_HANDLEINPUT\n"); )
	/* no input processing during layout */
	if( si->si_Flags & DTSIF_LAYOUT )
	{
		return GMR_NOREUSE;
	}
	
	/* check if we're hit or one of our members */
	if( gpi->MethodID == GM_GOACTIVE )
	{
		G( o )->Activation |= GACT_ACTIVEGADGET;
		G( o )->Flags |= GFLG_SELECTED;
		/* check if mousepointer is over one of our members */
		if( id->ControlPanel )
		{
			if( my > ( gbox.Height - id->TapeDeckHeight - 3 ) )
			{
				struct gpHitTest	gpht = { GM_HITTEST, gpi->gpi_GInfo, {mx, my} };
				
				if( gbox.Width >= 201 && DoMemberHitTest( &gbox, (Object *) id->TapeDeckGadget, &gpht ) )
				{
					id->ActiveMember = id->TapeDeckGadget;
				}
				else if( gbox.Width >= 220 && DoMemberHitTest( &gbox, (Object *) id->VolumeSlider, &gpht ) )
				{
					id->ActiveMember = id->VolumeSlider;
				}
				/* nothing hit ? */
				if( ! id->ActiveMember )
				{
					retval = GMR_NOREUSE;
				}
			}
		}
	}
	/* pass input to the active member ( if there's one ) */
	if( id->ActiveMember )
	{
		struct IBox	domain = gbox;		
		
		gbox = GetAbsGadgetBox( &gpi->gpi_GInfo->gi_Domain, EXTG( id->ActiveMember ), FALSE );
		
		(gpi -> gpi_Mouse . X) -= ((gbox . Left) - (domain . Left));
		(gpi -> gpi_Mouse . Y) -= ((gbox . Top) - (domain . Top));
		
		if( ( retval = DoMethodA( (Object *) id->ActiveMember, (Msg) gpi ) ) & GMR_VERIFY )
		{	/* tapedeck.gadget doesn't send OM_UPDATE :( */
			if( id->ActiveMember == id->TapeDeckGadget )
			{
				if( ie->ie_Code == IECODE_LBUTTON )
				{
					ULONG			stm = 0L;
					
					dbug( kprintf( "TapeDeck ICSPECIAL_CODE: %04lx\n", *gpi->gpi_Termination ); )
					
					if( *gpi->gpi_Termination & 0x1000 )
					{
						stm = STM_PAUSE;
					}
					else
					{
						switch( *gpi->gpi_Termination )
						{
							case BUT_PLAY:
								stm = STM_PLAY;
							break;
							
							case BUT_STOP:
								stm = STM_STOP;
							break;
						}
					}
					
					if( stm )
					{	
						CoerceMethod( cl, o, DTM_TRIGGER, 0, stm, 0 );
					}
				}
			}
		}
		
		(gpi -> gpi_Mouse . X) += ((gbox . Left) - (domain . Left));
		(gpi -> gpi_Mouse . Y) += ((gbox . Top) - (domain . Top));
	}
	/* we're hit */
	else if( retval == GMR_MEACTIVE )
	{
		while( ie )
		{
			if( ie->ie_Class == IECLASS_RAWMOUSE )
			{
				/* test for mark mode */
				if( si->si_Flags & DTSIF_DRAGSELECT )
				{
					struct RastPort 	*rp;
					WORD			mx = gpi->gpi_Mouse.X, x, y, w, h;
		
					x = gbox.Left; y = gbox.Top + 2;
					w = gbox.Width - 8; h = gbox.Height - 4;
					
					if( id->ControlPanel )
					{
						h -= ( id->TapeDeckHeight + 2 );
					}
					
					if( mx > (w+3) )
					{
						mx = w+3;
					}
					else if( mx < 4 )
					{
						mx = 4;
					}
					
					if( id->MarkMode )
					{					
						if( id->MaxX != (x+mx) )
						{
							if( ( rp = ObtainGIRPort( gpi->gpi_GInfo ) ) )
							{
								UWORD	minX, maxX;
								
								SetDrMd( rp, COMPLEMENT );
								
								minX = id->MinX;
								maxX = id->MaxX;
								
								if( minX > maxX )
								{
									maxX = minX;
									minX = id->MaxX;
								}
								//WaitTOF();
								RectFill( rp, minX, y, maxX, y+h-1 );
								
								minX = id->MinX;
								maxX =
								id->MaxX = x + mx;
								
								if( minX > maxX )
								{
									maxX = minX;
									minX = id->MaxX;						
								}
								//WaitTOF();
								RectFill( rp, minX, y, maxX, y+h-1 );
								/* fixes a bug (?) of tapedeck.gadget */
								SetDrMd( rp, JAM1 );
								
								ReleaseGIRPort( rp );
							}
						}
					}
					/* start mark mode */
					else if( ie->ie_Code == IECODE_LBUTTON )
					{	
						if( id->MinX != ~0L )
						{
							DoMethod( o, DTM_CLEARSELECTED, (IPTR) gpi->gpi_GInfo );
						}
						
						id->MarkMode = TRUE;
						
						id->MinX =
						id->MaxX = x + mx;
					}
				}
				/* not in mark mode - start or stop playing */
				else if( ie->ie_Code == IECODE_LBUTTON )
				{
					struct timeval		tv = gpi->gpi_IEvent->ie_TimeStamp;
					STATIC ULONG	stm[] = { STM_PLAY, STM_STOP };
					
					CoerceMethod( cl, o, DTM_TRIGGER, 0, stm[ DoubleClick( id->LastClick.tv_secs, id->LastClick.tv_micro, tv.tv_secs, tv.tv_micro ) ], 0L );
					
					id->LastClick = tv;
				}
			
				if( ie->ie_Code == SELECTUP )
				{
					retval = GMR_NOREUSE;
					break;
				}
			}
		
			ie = ie->ie_NextEvent;
		}
	}
	
	return( retval );
}

/****************************************************************************/

IPTR __regargs Sound_GOINACTIVE( Class *cl, Object *o, struct gpGoInactive *gpgi )
{
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct ClassBase		*cb = (struct ClassBase *) cl->cl_UserData;
	
	(void)cb;
	
	DoSuperMethodA( cl, o, (Msg) gpgi );
	
	dbug( kprintf("GOINACTIVE\n"); )
	
	if( id->ActiveMember )
	{
		DoMethodA( (Object *) id->ActiveMember, (Msg) gpgi );
		id->ActiveMember = NULL;
	}
	
	id->MarkMode = FALSE;
	
	G( o )->Activation &= ~GACT_ACTIVEGADGET;
	G( o )->Flags &= ~GFLG_SELECTED;
	
	return 0L;
}

/****************************************************************************/

IPTR __regargs Sound_TRIGGER( Class *cl, Object *o, struct dtTrigger *dtt )
{
	struct ClassBase		*cb = (struct ClassBase *)cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	APTR				data = NULL;
	ULONG				cmd = ~0L;

    	(void)cb;
	
	dbug( kprintf("DTM_TRIGGER\n"); )

	switch( dtt->dtt_Function )
	{
		case STM_PLAY:
			dbug( kprintf( "STM_PLAY\n" ); )
			cmd = COMMAND_PLAY;
			data = (APTR) id;
		break;
		
		case STM_STOP:
			dbug( kprintf( "STM_STOP\n" ); )
			cmd = COMMAND_STOP;
		break;
		
		case STM_PAUSE:
			dbug( kprintf( "STM_PAUSE\n" ); )
			cmd = COMMAND_PAUSE;
		break;
	}
		
	if( cmd != ~0L )
	{
		SendObjectMsg( id, cmd, data );
	}
	
	return 0L;
}

/****************************************************************************/
// "stolen" from embeddt
IPTR __regargs DoMemberHitTest( struct IBox *domain, Object *member, struct gpHitTest *gpht )
{
	struct GadgetInfo 	*gi = gpht -> gpht_GInfo;
	struct IBox		gbox;
	IPTR              	retval = 0L;

	gbox = GetAbsGadgetBox( &gi->gi_Domain, EXTG( member ), (BOOL) ( gpht -> MethodID == GM_HELPTEST ) );

	(gpht->gpht_Mouse.X) -= ((gbox.Left) - (domain->Left));
	(gpht->gpht_Mouse.Y) -= ((gbox.Top)  - (domain->Top));
	
	/* Mouse coordinates must be inside members's select box */
	if(	((gpht->gpht_Mouse . X) >= 0) &&
		((gpht->gpht_Mouse . Y) >= 0) &&
		((gpht->gpht_Mouse . X) <= (gbox.Width)) &&
		((gpht->gpht_Mouse . Y) <= (gbox.Height)) )
	{
		retval = DoMethodA( member, (Msg)gpht );
	}
	
	(gpht->gpht_Mouse.X) += ((gbox.Left) - (domain->Left));
	(gpht->gpht_Mouse.Y) += ((gbox.Top)  - (domain->Top));
	
	return( retval );
}

/****************************************************************************/

struct IBox __regargs GetAbsGadgetBox( struct IBox *domain, struct ExtGadget *g, BOOL useBounds )
{
	struct IBox retbox = { 0, 0, 0, 0 };

	if( domain && g )
	{
		if( ((g->MoreFlags) & GMORE_BOUNDS) && useBounds )
		{
			retbox = *((struct IBox *)(&(g->BoundsLeftEdge)));
		}
		else
		{
			retbox = *(GADGET_BOX( g ));
		}
	
		if( (g->Flags) & GFLG_RELBOTTOM )
		{
			(retbox.Top) += ((domain->Top) + (domain->Height));
		}
	
		if( (g->Flags) & GFLG_RELRIGHT )
		{
			(retbox.Left) += ((domain->Left) + (domain->Width));
		}
	
		if( (g->Flags) & GFLG_RELWIDTH )
		{
			(retbox.Width) += (domain->Width);
		}
	
		if( (g->Flags) & GFLG_RELHEIGHT )
		{
			(retbox.Height) += (domain->Height);
		}
	}
	
	return( retbox );
}

/****************************************************************************/

void __regargs makeqtab( BYTE *qtab )
{
	LONG i;
	
	for( i=0; i < 512; i++ )
	{
		qtab[i] = 0;
	}
	
	for ( i = 1; i < 16; i++ )
	{
		LONG	j = 256 + fibtab[i],
				k = 256 + fibtab[i-1];
				
		if( fibtab[i] < 0 ) 
		{
			while( j > k )
			{
				qtab[j--] = i;
			}
		} 
		else 
		{
			while( j < 512 )
			{
				qtab[j++] = i;
			}
		}
	}
}

/****************************************************************************/

LONG __regargs WriteChunk( struct ClassBase *cb, struct IFFHandle *iff, ULONG type, ULONG id, APTR data, ULONG size )
{
	LONG	err;
	
	if( ! ( err = PushChunk( iff, type, id, size ) ) )
	{
		if( ( err = WriteChunkBytes( iff, data, size ) ) == size )
		{
			err = PopChunk( iff );
		}
	}
	
	return err;
}

/****************************************************************************/

LONG __regargs WriteAIFF( struct ClassBase *cb, Object *o, struct IFFHandle *iff, struct InstanceData *id )
{
		LONG	err;
	
	if( ! ( err = PushChunk( iff, ID_AIFF, ID_FORM, IFFSIZE_UNKNOWN ) ) )
	{
		CommonChunk	common;
		UBYTE			sampleType = id->SampleType;
		ULONG			sampleLength = id->SampleLength;
		BYTE			*sample = id->Sample;

		if( id->StartSample != ~0L )
		{
			sample = &sample[ UMult32( id->StartSample, bytesPerPoint[ sampleType ] ) ];
			sampleLength = id->EndSample - id->StartSample;
		}

		common.numChannels = ( sampleType & 1 ) + 1;
		common.numSampleFrames = sampleLength;
		common.sampleSize = ( sampleType >= SDTST_M16S ) ? 16 : 8;
		ulong2extended( id->Frequency, &common.sampleRate );
			
		if( ! ( err = WriteChunk( cb, iff, ID_AIFF, ID_COMM, &common, sizeof( common ) ) ) )
		{		
			SampledSoundHeader	ssnd = { 0L, 0L };

			if( ! ( err = PushChunk( iff, ID_AIFF, ID_SSND, IFFSIZE_UNKNOWN ) ) )
			{
				if( ( err = WriteChunkBytes( iff, &ssnd, sizeof( ssnd ) ) ) == sizeof( ssnd ) )
				{				
					ULONG	bytes = UMult32( sampleLength, bytesPerPoint[ sampleType ] );

					if( ( err = WriteChunkBytes( iff, sample, bytes ) ) == bytes )
					{
						err = PopChunk( iff );
					}
				}
			}
		}
		
		if( ! err )
		{
			err = PopChunk( iff );
		}
	}
	
	return( err );
}

/****************************************************************************/

LONG __regargs WriteSVX( struct ClassBase *cb, Object *o, struct IFFHandle *iff, struct InstanceData *id )
{
	UBYTE	sampletype = id->SampleType, bps = ( sampletype >= SDTST_M16S ? 2 : 1 );
	BOOL	stereo = IsStereo( id->SampleType );
	LONG	err, type = ( sampletype >= SDTST_M16S ? ID_16SV : ID_8SVX );
	ULONG	sampleLength = id->SampleLength;
	BYTE	*sample = id->Sample;
	struct VoiceHeader	vhdr;

	CopyMem( &id->VoiceHeader, &vhdr, sizeof( vhdr ) );
	vhdr.vh_Compression = ( ( id->VoiceHeader.vh_Compression == CMP_FIBDELTA ) || ( cb->cb_Compress ) ) && ( sampletype <= SDTST_S8S );
							
	if( id->StartSample != ~0L )
	{
		sample = &sample[ UMult( id->StartSample, bytesPerPoint[ sampletype ] ) ];
		sampleLength = id->EndSample - id->StartSample;
		vhdr.vh_OneShotHiSamples = sampleLength;
		vhdr.vh_RepeatHiSamples = 0L;
	}
	
	if( ! ( err = PushChunk( iff, type, ID_FORM, IFFSIZE_UNKNOWN ) ) )
	{
		if( ! ( err = WriteChunk( cb, iff, type, ID_VHDR, &vhdr, sizeof( vhdr ) ) ) )
		{
			struct ObjectData
			{
				ULONG	attr;
				ULONG	id;
				STRPTR	data;
			} ObjData[] = { 
				{DTA_ObjName, ID_NAME, NULL}, 
				{DTA_ObjAnnotation, ID_ANNO, NULL},
				{DTA_ObjAuthor, ID_AUTH, NULL},
				{DTA_ObjCopyright, ID_Copyright, NULL},
				{DTA_ObjVersion, ID_FVER, NULL},
				{0}
			};
			LONG	i;
			
			for( i = 0; ObjData[i].attr; i++ )
			{
				struct ObjectData	*od = &ObjData[i];
				
				if( GetDTAttrs( o, od->attr, (IPTR) &od->data, TAG_DONE ) && od->data )
				{
					if( ( err = WriteChunk( cb, iff, type, od->id, od->data, StrLen( od->data ) ) ) )
					{
						break;
					}
				}
			}
			
			if( ! err )
			{
				if( stereo )
				{
					ULONG	chan = 6L;
					err = WriteChunk( cb, iff, type, ID_CHAN, &chan, sizeof( ULONG ) );
				}
				else
				{
					ULONG	pan = 0x10000 - id->Panning;
					err = WriteChunk( cb, iff, type, ID_PAN, &pan, sizeof( ULONG ) );
				}
			}
			
			if( ! err )
			{
				if( vhdr.vh_Compression == CMP_FIBDELTA )
				{
					BYTE	*qtab, *buf;
					LONG	buffersize;
					
					buffersize = sampleLength;
											
					while( ! ( buf = AllocVec( buffersize + 512L, MEMF_PUBLIC ) ) )
					{
						if( ( buffersize /= 2 ) < 1024L )
						{
							break;
						}
					}
					
					if( buf )
					{
						if( ! ( err = PushChunk( iff, type, ID_BODY, IFFSIZE_UNKNOWN ) ) )
						{
							LONG	i;

							qtab = &buf[buffersize];
							makeqtab( qtab );
							
							for( i = 0; i < (stereo?2:1) && !err; i++ )
							{
								LONG	length = sampleLength, samples = 0L;
								BYTE	c0 = 0, *src = &sample[i], add = stereo?2:1;
								UBYTE	o, x = 1; 
								
								if( length & 1 )
								{
									c0 = *src++;
									length--;
								}

								buf[ 0 ] = 0;
								buf[ 1 ] = c0;
								samples += 2;
								
								while( length-- )
								{	
									UBYTE	n;
										
									c0 += fibtab[ n = qtab[256+(*src-c0)] ];
									
									src+=add;
									
									if( x^=1 )
									{
										buf[samples++] = o | n;
									}
									else
									{
										o = n << 4;
									}
										
									if( samples == buffersize || length == 0L )
									{
										if( ( err = WriteChunkBytes( iff, buf, samples ) ) != samples )
										{
											break;
										}
										err = samples = 0L;
									}
								}
							}
							
							if( ! err )
							{
								err = PopChunk( iff );
							}
						}
						
						FreeVec( buf );
					}
					else
					{
						err = ERROR_NO_FREE_STORE;
					}
				}					
				/*
				** write uncompressed svx file
				*/					
				else
				{
					/*
					**	create a stereo BODY
					*/
					if( stereo )
					{
						if( ! ( err = PushChunk( iff, type, ID_BODY, IFFSIZE_UNKNOWN ) ) )
						{								
							BYTE	*buf;
							LONG	bufsize = sampleLength;
							
							while( ! ( buf = AllocVec( UMult( bufsize, bps ), MEMF_PUBLIC ) ) )
							{
								if( ( bufsize /= 2 ) < 1024L )
								{
									break;
								}
							}
							
							if( buf )
							{									
								LONG	i;
								
								for( i = 0; i < 2 && ! err; i++, sample+=bps )
								{									
									LONG	samples = bufsize, length = sampleLength;
									BYTE	*src = sample;
									
									while( length )
									{
										ULONG	bytes = UMult( samples, bps );
										
										if( sampletype == SDTST_S16S )
										{
											WORD	*dst = (WORD *)buf,
													*src_p = (WORD *)src;
											LONG	c;
											
											for( c = samples; c; c--, src_p+=2 )
											{
												*dst++ = *src_p;
											}
											
											src = (BYTE *) src_p;
										}
										else
										{
											BYTE	*dst = buf;
											LONG	c;
											
											for( c = samples; c; c--, src+=2 )
											{
												*dst++ = *src;
											}
										}
										
										if( ( err = WriteChunkBytes( iff, buf, bytes ) ) != bytes )
										{
											break;
										}
										
										if( (length-=samples) < samples )
										{
											samples = length;
										}
										
										err = 0L;
									}
								}
								
							}
							else
							{
								err = ERROR_NO_FREE_STORE;
							}
							
							if( ! err )
							{
								err = PopChunk( iff );
							}
						}
					}
					/*
					**	write uncompressed mono sample
					*/
					else
					{
						err = WriteChunk( cb, iff, type, ID_BODY, sample, UMult( sampleLength, bps ) );
					}
				}
			}
		}
			
		if( ! err )
		{
			err = PopChunk( iff );
		}
	}
	
	return( err );
}

/****************************************************************************/

IPTR __regargs Sound_WRITE( Class *cl, Object *o, struct dtWrite *dtw )
{
	struct ClassBase		*cb = (struct ClassBase *)cl->cl_UserData;
	struct InstanceData	*id = (struct InstanceData *) INST_DATA( cl, o );
	struct IFFHandle		*iff;
	struct SignalSemaphore	*lock = &((struct DTSpecialInfo *)G( o )->SpecialInfo)->si_Lock;
	LONG				err = 0L;
	
	dbug( kprintf( "DTM_WRITE / DTM_COPY\n" ); )
	
	ObtainSemaphoreShared( &cb->cb_LibLock );
	
	if( id->Sample )
	{
		ObtainSemaphoreShared( lock );
			
		if( ( iff = AllocIFF() ) )
		{
			if( dtw->MethodID == DTM_WRITE )
			{
				dbug( kprintf( "DTWM_RAW\n" ); )
				
				if( ( iff->iff_Stream = (IPTR)dtw->dtw_FileHandle ) )
				{
					InitIFFasDOS( iff );
				}
				else
				{
					dbug( kprintf( "NULL filehandle\n"); )
					err = ERROR_INVALID_LOCK;
				}
			}
			else
			{
				dbug( kprintf( "DTM_COPY\n" ); )
				
				if( ( iff->iff_Stream = (IPTR) OpenClipboard( PRIMARY_CLIP ) ) )
				{
					InitIFFasClip( iff );
				}
				else
				{
					dbug( kprintf( "Cant open clipboard\n" ); )
					err = ERROR_OBJECT_IN_USE;
				}
			}
	
			if( ! err )
			{
				if( ( err = OpenIFF( iff, IFFF_WRITE ) ) )
				{
					err = ifferr2doserr[-err-1];
					dbug( kprintf( "OpenIFF() failed\n" ); )
				}
			}
		}
		else
		{
			err = ERROR_NO_FREE_STORE;
			dbug( kprintf( "AllocIFF failed\n" ); )
		}

		if( ! err )
		{
			dbug( kprintf( "Everything fine so far ...\n" ); )
			
			if( id->SampleType >= SDTST_M16S && cb->cb_AIFF )
			{
				err = WriteAIFF( cb, o, iff, id );
			}
			else
			{
				err = WriteSVX( cb, o, iff, id );
			}
		
			if( err < 0L )
			{
				err = ifferr2doserr[-err-1];
			}
		}
		else
		{
			dbug( kprintf( "Something failed\n" ); )
		}
		
		dbug( kprintf( "closing iff stream\n" ); )
			
		CloseIFF( iff );
			
		if( dtw->MethodID == DTM_COPY )
		{
			CloseClipboard( (struct ClipboardHandle *)iff->iff_Stream );
		}
			
		FreeIFF( iff );

		ReleaseSemaphore( lock );
	}
	else
	{
		err = ERROR_REQUIRED_ARG_MISSING;
	}
	
	ReleaseSemaphore( &cb->cb_LibLock );
	
	SetIoErr( err );
	return( err ? FALSE : TRUE );
}

/****************************************************************************/

/*
**	test if dblscan screenmode
**	taken from the ahi paula driver by Martin Blom
*/
#if 0
BOOL Is31Khz( struct ClassBase *cb )
{
	BOOL	ret = FALSE;
	
	/* check for ocs */
#ifdef __MAXON__
	extern struct Library	*GfxBase, *IntuitionBase;

	if( ((struct GfxBase *)GfxBase)->ChipRevBits0 & ( GFXF_HR_DENISE|GFXF_AA_LISA ) )
#else
#undef GfxBase
	if( ((struct GfxBase *)cb->cb_GfxBase)->ChipRevBits0 & ( GFXF_HR_DENISE|GFXF_AA_LISA ) )
#define GfxBase	cb->cb_GfxBase
#endif
	{
		ULONG	lock, modeid;
		
		lock = LockIBase( 0L );
#ifdef __MAXON__
		modeid = GetVPModeID( & ((struct IntuitionBase *)IntuitionBase)->FirstScreen->ViewPort );
#else
#undef IntuitionBase
		modeid = GetVPModeID( & ((struct IntuitionBase *)cb->cb_IntuitionBase)->FirstScreen->ViewPort );
#define IntuitionBase	cb->cb_IntuitionBase
#endif
		UnlockIBase( lock );
		
		/* gfxboard  */
		if( modeid & 0x40000000 )
		{
			Forbid();
			if( FindTask( "Picasso96" ) )
			{
				UBYTE	buf[16];
				
				Permit();
				
				if( GetVar( "Picasso96/AmigaVideo", buf, sizeof( buf ), GVF_GLOBAL_ONLY ) )
				{
					if( ! Strnicmp( buf, "31Khz" , 5L ) )
					{
						ret = TRUE;
					}
				}
			}
			else
			{
#ifdef __MAXON__
				struct copinit	*ci = ((struct GfxBase *)GfxBase)->copinit;
#else
#undef GfxBase
				struct copinit	*ci = ((struct GfxBase *)cb->cb_GfxBase)->copinit;
#define GfxBase	cb->cb_GfxBase
#endif
				Permit();
				
				if( ( ci->fm0[ 0 ] != 0x01fc ) || ( ci->fm0[ 1 ] & 0xc000 ) )
				{
					ret = TRUE;
				}
			}
		}
		/* native screen */
		else
		{
			struct MonitorInfo	moni;
			
			if( GetDisplayInfoData( NULL, (UBYTE *) &moni, sizeof( moni ), DTAG_MNTR, modeid ) )
			{
				if( moni.TotalColorClocks * moni.TotalRows / ( 2 * ( moni.TotalRows - moni.MinRow + 1 ) ) <= 64 )
				{
					ret = TRUE;
				}
			}
		}
	}
	
	return ret;
}
#endif
/****************************************************************************/

void __regargs CalcPanVol( UWORD overall, ULONG pan, UWORD *leftvol, UWORD *rightvol )
{
	pan = ( pan << 6 ) >> 16;
	*rightvol = pan ? overall / ( 64 / pan ) : 0;
	*leftvol = overall - *rightvol;
}

/****************************************************************************/

void __regargs RemoveRequest( struct IORequest *ior )
{
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
struct Library	*SysBase = (*(struct Library **)4L);
#endif
	struct Node	*ln, *succ;
	
	Disable();
	for(	ln = ior->io_Message.mn_ReplyPort->mp_MsgList.lh_Head;
			(succ = ln->ln_Succ); ln = succ )
	{
		if( ln == &ior->io_Message.mn_Node )
		{
			Remove( ln );
			SetSignal( 0L, 1L << ior->io_Message.mn_ReplyPort->mp_SigBit );
			break;
		}
	}
	Enable();
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
#define SysBase	cb->cb_SysBase
#endif
}

/****************************************************************************/

#define LEFT_CHANNEL_0	1
#define RIGHT_CHANNEL_1	2
#define RIGHT_CHANNEL_2	4
#define LEFT_CHANNEL_3	8
#define MAX_SAMPLE		131072
#define MAX_SAMPLE_RATE	28867
#define LEFT_MASK		(LEFT_CHANNEL_0|LEFT_CHANNEL_3)
#define RIGHT_MASK		(RIGHT_CHANNEL_1|RIGHT_CHANNEL_2)

enum {
	AUDIO_CONTROL,
	AUDIO_LEFT,
	AUDIO_LEFT2,
	AUDIO_RIGHT,
	AUDIO_RIGHT2,
	NUM_REQUESTS
};

/****************************************************************************/

void __regargs SetTDMode( struct ClassBase *cb, struct InstanceData *id, ULONG mode )
{
	if( id->ControlPanel )
	{
		IPTR	oldmode = mode;
		
		ObtainSemaphoreShared( &id->Lock );

		/* avoid deadlocks */
		GetAttr( TDECK_Mode, (Object *)id->TapeDeckGadget, &oldmode );
		if( oldmode != mode )
		{
			SetGadgetAttrs( id->TapeDeckGadget, id->Window, id->Requester,
				TDECK_Mode, mode, TAG_DONE );
		}
		
		ReleaseSemaphore( &id->Lock );
	}
}

/****************************************************************************/

void PlayerProc( void )
{
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
	struct Library		*SysBase = *(struct Library **)4L;
#endif
	struct Process	*pr = (struct Process *) FindTask( NULL );
	struct MsgPort	*mp, *mpaudio = NULL;
	struct IOAudio	*ioaudio[NUM_REQUESTS] = { };
	struct ClassBase	*cb;
	struct InstanceData *id;
	LONG			numChAllocated, samples, buffersize, length, cycles, 
					loops, audiompmsk = 0, mpmsk;
	BOOL			releaseAudio = FALSE, restart, paused = FALSE;
	BYTE			*sample, *buffer[4] = {};
	
	mp = &pr->pr_MsgPort;
	mpmsk = 1L << mp->mp_SigBit;
	
	dbug( kprintf("child process launched\n"); )
	
	for( ;; )
	{
		struct ObjectMsg	*msg;
		LONG			rcv;
		
		rcv = Wait( mpmsk | audiompmsk );
		
		if( rcv & mpmsk )
		{
			while( ( msg = (struct ObjectMsg *) GetMsg( mp ) ) )
			{
				switch( msg->Command )
				{
					case COMMAND_NEXT_BUFFER:
						if( releaseAudio )
						{
							ObtainSemaphoreShared( &id->Lock );
							sample = id->Sample;
							length = id->SampleLength;
							samples = ( length > buffersize ) ? buffersize : length;
							rcv = audiompmsk;
							ReleaseSemaphore( &id->Lock );
						}
					break;
					
					case COMMAND_INIT:
						dbug( kprintf( "child/COMMAND_INIT\n"); )
					
						id = (struct InstanceData *) msg->Data;
						cb = id->ClassBase;
					
						if( ! ( msg->Data = mp = CreateMsgPort() ) )
						{
							ReplyMsg( &msg->Message );
							return;
						}

						mpmsk = 1L << mp->mp_SigBit;
					break;
					
					case COMMAND_EXIT:
					case COMMAND_STOP:
						dbug( kprintf( "child/COMMAND_EXIT or STOP\n"); )
					
						if( releaseAudio )
						{
							LONG	i;
							
							SetTDMode( cb, id, BUT_STOP );
							
							for( i = AUDIO_LEFT; i < NUM_REQUESTS; i++ )
							{
								struct IORequest	*ioa = (struct IORequest *) ioaudio[ i ];
								
								if( CheckIO( ioa ) )
								{
									RemoveRequest( ioa );
								}
								else
								{
									AbortIO( ioa );
									WaitIO( ioa );
								}
							}
							
							CloseDevice( (struct IORequest *) ioaudio[ AUDIO_CONTROL ] );
							DeleteMsgPort( mpaudio );
							audiompmsk = 0L;
							FreeVec( ioaudio[ AUDIO_CONTROL ] );
							FreeVec( buffer[0] ); buffer[0] = NULL;
							releaseAudio = FALSE;							
						}
						
						if( msg->Command == COMMAND_EXIT )
						{
							Forbid();
							ReplyMsg( &msg->Message );
							DeleteMsgPort( mp );
							return;
						}
					break;
					
					case COMMAND_PERVOL:
					{
						dbug( kprintf( "child/COMMAND_PERVOL\n"); )
						
						if( releaseAudio )
						{
							struct IOAudio *ioa = ioaudio[ AUDIO_CONTROL ];
							
							ObtainSemaphoreShared( &id->Lock );
							
							if( numChAllocated == 1L || IsStereo( id->SampleType ) )
							{
								ioa->ioa_Period	= Freq2Period( id->Frequency );
								ioa->ioa_Volume	= id->Volume;
								ioa->ioa_Request.io_Flags 	= 0;
								ioa->ioa_Request.io_Command = ADCMD_PERVOL;
								DoIO( &ioa->ioa_Request );
								
								Forbid();
								ioaudio[ AUDIO_LEFT ]->ioa_Volume =
								ioaudio[ AUDIO_LEFT2 ]->ioa_Volume = 
								ioaudio[ AUDIO_RIGHT ]->ioa_Volume =
								ioaudio[ AUDIO_RIGHT2 ]->ioa_Volume = id->Volume;
								Permit();
							}
							else
							{
								UWORD	leftvol, rightvol;
								ULONG	oldmask = (ULONG) ioa->ioa_Request.io_Unit;
								
								CalcPanVol( id->Volume, id->Panning, &leftvol, &rightvol );
								/*
								ioa->ioa_Request.io_Command = CMD_STOP;
								DoIO( &ioa->ioa_Request );*/
								
								ioa->ioa_Request.io_Unit = (struct Unit *) ( oldmask & LEFT_MASK );
								ioa->ioa_Period	= Freq2Period( id->Frequency );
								ioa->ioa_Volume	= leftvol;
								ioa->ioa_Request.io_Flags 	= 0;
								ioa->ioa_Request.io_Command = ADCMD_PERVOL;
								DoIO( &ioa->ioa_Request );
								
								ioa->ioa_Request.io_Unit = (struct Unit *) ( oldmask & RIGHT_MASK );
								ioa->ioa_Period	= Freq2Period( id->Frequency );
								ioa->ioa_Volume	= rightvol;
								ioa->ioa_Request.io_Flags 	= 0;
								ioa->ioa_Request.io_Command = ADCMD_PERVOL;
								DoIO( &ioa->ioa_Request );
								
								ioa->ioa_Request.io_Unit = (struct Unit *) oldmask;
								
								Forbid();
								ioaudio[ AUDIO_LEFT ]->ioa_Volume =
								ioaudio[ AUDIO_LEFT2 ]->ioa_Volume = leftvol;
								ioaudio[ AUDIO_RIGHT ]->ioa_Volume =
								ioaudio[ AUDIO_RIGHT2 ]->ioa_Volume = rightvol;
								Permit();

								/*
								ioa->ioa_Request.io_Command = CMD_START;
								DoIO( &ioa->ioa_Request );*/
							}
							
							Forbid();
							ioaudio[ AUDIO_LEFT ]->ioa_Period =
							ioaudio[ AUDIO_LEFT2 ]->ioa_Period =
							ioaudio[ AUDIO_RIGHT ]->ioa_Period =
							ioaudio[ AUDIO_RIGHT2 ]->ioa_Period = Freq2Period( id->Frequency );
							Permit();
							
							ReleaseSemaphore( &id->Lock );
						}
					}
					break;
					
					case COMMAND_PAUSE:
					{
						dbug( kprintf( "child/COMMAND_PAUSE\n"); )
						
						if( releaseAudio )
						{
							if( ! paused )
							{
								ioaudio[ AUDIO_CONTROL ]->ioa_Request.io_Command = CMD_STOP;
								DoIO( &ioaudio[ AUDIO_CONTROL ]->ioa_Request );
								paused = TRUE;
							}
						}
					}
					break;
					
					case COMMAND_PLAY:
					{
						dbug( kprintf( "child/COMMAND_PLAY\n"); )

						ObtainSemaphoreShared( &id->Lock );
						
						if( releaseAudio )
						{
							if( paused )							
							{	
								rcv = audiompmsk;
								restart = TRUE;
								paused = FALSE;
							}
							else
							{
								LONG	i;
								
								for( i = AUDIO_LEFT; i < NUM_REQUESTS; i++ )
								{
									struct IORequest	*ioa = &ioaudio[ i ]->ioa_Request;
									
									if( CheckIO( ioa ) )
									{
										RemoveRequest( ioa );
									}
									else
									{
										AbortIO( ioa );
										WaitIO( ioa );
									}
								}
								
								sample = id->Sample;
								length = id->SampleLength;
								samples = buffersize;
								if( length <= buffersize )
								{
									loops = 1L;
									cycles = id->Cycles;
								}
								else
								{
									loops = id->Cycles;
									cycles = 1L;
								}
							}
						}						
						else											
						{
							UBYTE	ChMap[] = {
								LEFT_CHANNEL_0 | RIGHT_CHANNEL_1,
								LEFT_CHANNEL_3 | RIGHT_CHANNEL_2,
								LEFT_CHANNEL_0, LEFT_CHANNEL_3,
								RIGHT_CHANNEL_1, RIGHT_CHANNEL_2
							};
							struct IOAudio	*ioa;
							LONG			i;
							
							if( ! id->Sample || ! id->SampleLength )
							{
								break;
							}
							
							dbug( kprintf( "child/allocating requests\n" ); )
							
							/*- allocate requests and msgport -*/
							if(	! ( ioa = AllocVec( sizeof( *ioa ) * NUM_REQUESTS, MEMF_PUBLIC|MEMF_CLEAR ) ) || 
								! ( mpaudio = CreateMsgPort() ) )
							{
								FreeVec( ioa );
								SetTDMode( cb, id, BUT_STOP );
								break;
							}
							
							for( i = 0; i < NUM_REQUESTS; i++ )
							{
								ioaudio[i] = ioa++;
							}
							
							audiompmsk = 1L << mpaudio->mp_SigBit;
							
							/*- Open audio.device and intialize requests -*/
							ioa = ioaudio[ AUDIO_CONTROL ];
							ioa->ioa_Request.io_Message.mn_ReplyPort = mpaudio;
							ioa->ioa_Request.io_Message.mn_Length = sizeof( *ioa );
													
							ioa->ioa_Data = ChMap;
							ioa->ioa_Length = sizeof( ChMap );							
							
							if( ! OpenDevice( "audio.device", 0L, (struct IORequest *) ioa, 0L ) )
							{
								/*- make CheckIO() work -*/
								ioa->ioa_Request.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
								
								for( i = 1; i < NUM_REQUESTS; i++ )
								{
									CopyMem( ioa, ioaudio[i], sizeof( *ioa ) );
								}
								
								for( i = 0; i < sizeof( ChMap ); i++ )
								{
									if( (ULONG) ioa->ioa_Request.io_Unit == (ULONG) ChMap[i] )
									{
										break;
									}
								}
								
								if( i < 2 )
								{
									dbug( kprintf( "child/got two stereo channels\n" ); )
									numChAllocated = 2L;							
									/*- stop device in order to sync stereo channels -*/
									ioa->ioa_Request.io_Command = CMD_STOP;
									DoIO( &ioa->ioa_Request );
									restart = TRUE;
								}
								else
								{
									dbug( kprintf( "child/got only one channel\n" ); )
									numChAllocated = 1L;
									restart = FALSE;
								}
								
								ioaudio[ AUDIO_LEFT   ]->ioa_Request.io_Unit =
								ioaudio[ AUDIO_LEFT2 ]->ioa_Request.io_Unit = (struct Unit *) ( (ULONG) ioa->ioa_Request.io_Unit & LEFT_MASK );
								ioaudio[ AUDIO_RIGHT   ]->ioa_Request.io_Unit =
								ioaudio[ AUDIO_RIGHT2 ]->ioa_Request.io_Unit = (struct Unit *) ( (ULONG) ioa->ioa_Request.io_Unit & RIGHT_MASK );
								
								sample = id->Sample;
								length = id->SampleLength;
					
								if( ( id->SampleType == SDTST_M8S ) && ( TypeOfMem( sample ) & MEMF_CHIP ) )
								{
									buffersize = samples = ( length > MAX_SAMPLE ) ? MAX_SAMPLE : length;
									dbug( kprintf( "child/chip sample\n" ); )
								}
								/*- allocate buffers -*/
								else
								{	
									ObtainSemaphoreShared( &cb->cb_LibLock ); /* determine buffersize, don't go below 1k (heavy system load) */
									buffersize =  ( length > cb->cb_BufferSize ) ? cb->cb_BufferSize : ( ((length+1)/2) < 1024L ? length : (length+1)/2 );
									ReleaseSemaphore( &cb->cb_LibLock );
									
									while( ! ( buffer[0] = AllocVec( buffersize << numChAllocated, MEMF_CHIP ) ) )
									{
										dbug( kprintf( "child/failed to allocate %ld bytes of buffer\n", buffersize ); )
										if( (buffersize /= 2) < 1024L )
										{
											break;
										}
									}
									
									samples = buffersize;
									
									dbug( kprintf( "child/Buffer at %08lx, size %ld, num %ld\n", buffer[0], buffersize, numChAllocated*2 ); )
									
									if( buffer[0] == NULL )
									{
										CloseDevice( (struct IORequest *) ioa );
										DeleteMsgPort( mpaudio );
										audiompmsk = 0L;
										FreeVec( ioa );
										SetTDMode( cb, id, BUT_STOP );
										break;
									}
									
									for( i = 1; i < 4; i++ )
									{							
										buffer[ i ] = buffer[ i-1 ] + buffersize;
									}
								}
								
								if( buffersize <= length )
								{
									cycles = id->Cycles;
									loops = 1L;
								}
								else
								{
									cycles = 1L;
									loops = id->Cycles;
								}
								
								dbug( kprintf( "child/Sample: %08lx, Length: %ld, Cycles: %ld, Loops: %ld, Freq: %ld\n", sample, length, cycles, loops, id->Frequency ); )
								
								releaseAudio = TRUE;
								rcv = audiompmsk;
								SetTDMode( cb, id, BUT_PLAY );
							}
							else
							{
								dbug( kprintf("child/can't open audio.device\n"); )
								DeleteMsgPort( mpaudio );
								audiompmsk = 0L;
								FreeVec( ioa );
								SetTDMode( cb, id, BUT_STOP );
							}
						}
					
						ReleaseSemaphore( &id->Lock );
					}
					break;
				} // switch
			
				if( msg->Message.mn_ReplyPort )
				{
					ReplyMsg( &msg->Message );
				}
				else // NT_FREEMSG
				{
					FreeVec( msg );
				}
			} // while
		} // if
		
		if( rcv & audiompmsk )
		{
			ObtainSemaphoreShared( &id->Lock );
			
			if( length == 0L )
			{
				LONG	i;
				BOOL	Done = TRUE;
					
				dbug( kprintf( "child/reachend end of sample\n" ); )
				
				/*- wait for any outstanding request -*/
				for( i = AUDIO_LEFT; i < NUM_REQUESTS; i++ )
				{
					struct IORequest	*ioa = &ioaudio[i]->ioa_Request;
						
					if( ! CheckIO( ioa ) )
					{
						/* don't block doing WaitIO() */
						Done = FALSE;
					}
					else
					{
						RemoveRequest( ioa );
					}
				}
				
				if( ! Done || id->Continuous )
				{
					ReleaseSemaphore( &id->Lock );
					continue;
				}
				
				if(  id->Repeat || ( --loops > 0L ) )
				{
					dbug( kprintf( "child/%ld loops to go\n", loops ); )
								
					sample = id->Sample;
					length = id->SampleLength;
					samples = buffersize;
					/*- stop device in order to sync stereo channels -*/
					if( numChAllocated == 2L )
					{
						ioaudio[ AUDIO_CONTROL ]->ioa_Request.io_Command = CMD_STOP;
						DoIO( &ioaudio[ AUDIO_CONTROL ]->ioa_Request );
						restart = TRUE;
					}
					else
					{
						restart = FALSE;
					}
				}
				else
				{
					dbug( kprintf("child/cleanup\n"); )
					
					/*- cleanup -*/
					CloseDevice( &ioaudio[AUDIO_CONTROL]->ioa_Request );
					FreeVec( buffer[0] ); buffer[0] = NULL;
					FreeVec( ioaudio[ AUDIO_CONTROL ] );
					DeleteMsgPort( mpaudio ); audiompmsk = 0L;
					releaseAudio = FALSE;
					
					SetTDMode( cb, id, BUT_STOP );
					
					if( id->SignalTask && id->SignalBit != -1L )
					{
						Signal( id->SignalTask, 1L << id->SignalBit );
					}
				}
			}
			
			if( length )
			{			
				LONG	i;
				UWORD	volleft, volright;
				
				if( numChAllocated == 1L || IsStereo( id->SampleType ) )
				{
					volright =
					volleft = id->Volume;
				}
				else
				{
					CalcPanVol( id->Volume, id->Panning, &volleft, &volright );
				}
				
				/*- we work doublebuffered -*/
				for( i = 0; i < 2; i++ )
				{
					struct IOAudio	*ioa_left = ioaudio[ i + AUDIO_LEFT ],
									*ioa_right = ioaudio[ i + AUDIO_RIGHT ];
					
					/*- get free audiorequest -*/
					if( CheckIO( (struct IORequest *) ioa_left ) && CheckIO( (struct IORequest *) ioa_right ) )
					{	
						BYTE	*data_left, *data_right;
						
						dbug( kprintf( "child/Request pair #%ld is free\n", i ); )
						
						RemoveRequest( (struct IORequest *) ioa_left );
						RemoveRequest( (struct IORequest *) ioa_right );
						
						if( buffer[ 0 ] )
						{
							if( IsStereo( id->SampleType ) )
							{
								LONG	c, add = ( id->SampleType == SDTST_S16S ) ? 2 : 1;
									
								if( numChAllocated == 2L )
								{
									BYTE	*buf_left = buffer[ i * 2 ],
											*buf_right = buffer[ i * 2 + 1 ];

									data_left = buf_left;
									data_right = buf_right;
								
									for( c = samples; c; c-- )
									{
										*buf_left++ = *sample; sample += add;
										*buf_right++ = *sample; sample += add;
									}
								}
								else
								{
									BYTE	*buf = buffer[ i ];

									data_left = 
									data_right = buf;
								
									for( c = samples; c; c--, sample+=(add<<1) )
									{
										*buf++ = ( sample[0] + sample[add] ) >> 1;
									}
								}
							}
							else
							{
								BYTE	*buf = buffer[ i ];

								data_left = 
								data_right = buf;
								
								if( id->SampleType == SDTST_M8S )
								{
									if( (ULONG) sample&3 || (ULONG) buf&3 || samples&3 )
									{
										CopyMem( sample, buf, samples );
									}
									else
									{
										CopyMemQuick( sample, buf, samples );
									}
									
									sample += samples;
								}
								else
								{
									LONG	c;
									
									for( c = samples; c; c--, sample+=2 )
									{
										*buf++ = *sample;
									}
								}
							}
						}
						else
						{
							data_left =
							data_right = sample;
							sample += samples;
						}
						
						ioa_left->ioa_Data = data_left;
						ioa_left->ioa_Length = samples;
						ioa_left->ioa_Cycles = cycles;
						ioa_left->ioa_Period = Freq2Period( id->Frequency );
						ioa_left->ioa_Volume = volleft;
						ioa_left->ioa_Request.io_Flags = ADIOF_PERVOL;
						ioa_left->ioa_Request.io_Command = CMD_WRITE;
						BeginIO( (struct IORequest *) ioa_left );
						
						if( numChAllocated == 2L )
						{
							ioa_right->ioa_Data = data_right;
							ioa_right->ioa_Length = samples;
							ioa_right->ioa_Cycles = cycles;
							ioa_right->ioa_Period = Freq2Period( id->Frequency );
							ioa_right->ioa_Volume = volright;
							ioa_right->ioa_Request.io_Flags = ADIOF_PERVOL;
							ioa_right->ioa_Request.io_Command = CMD_WRITE;
							BeginIO( (struct IORequest *) ioa_right );
						}
						
						if( (length-=samples) < samples )
						{
							samples = length;
						} // if
					} // if
					else
					{
						dbug( kprintf( "child/Req #%ld in use\n", i ); )
					}
				
					if( ! length )
					{
						/* a continuous stream of data? */
						if( id->Continuous )
						{
							/* notify */
							if( id->SignalTask && id->SignalBit != -1L )
							{
								Signal( id->SignalTask, 1L << id->SignalBit );
							}
						}
						
						break;
					}
				} // for
				
				if( restart )
				{
					dbug( kprintf( "child/starting stereo channels\n" ); )
					ioaudio[ AUDIO_CONTROL ]->ioa_Request.io_Command = CMD_START;
					DoIO( &ioaudio[ AUDIO_CONTROL ]->ioa_Request );
					restart = FALSE;
				} // if
			} // if
		
			ReleaseSemaphore( &id->Lock );
		} // if
	} // while
	
#if !defined(__MAXON__) && !defined(__AROS__)
#define SysBase	cb->cb_SysBase
#endif
}

/****************************************************************************/

void __regargs CloseAHI( struct MsgPort *mp, struct AHIRequest *ahir )
{
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
	struct Library	*SysBase = (*(struct Library **)4L);
#endif	
	CloseDevice( (struct IORequest *) ahir );
	DeleteIORequest( (struct IORequest *) ahir );
	DeleteMsgPort( mp );
#if !defined(__MAXON__) && !defined(__AROS__)
#define SysBase	cb->cb_SysBase
#endif
}

/****************************************************************************/

struct Library * __regargs OpenAHI( struct MsgPort **mpp, struct AHIRequest **iop )
{
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
	struct Library	*SysBase = (*(struct Library **)4L);
#endif
	struct MsgPort	*mp;
	struct AHIRequest	*ahir;
	
	mp = CreateMsgPort();
	if( ( ahir = (struct AHIRequest *) CreateIORequest( mp, sizeof( struct AHIRequest ) ) ) )
	{
		ahir->ahir_Version = 4;
		if( ! OpenDevice( "ahi.device", AHI_NO_UNIT, (struct IORequest *)ahir, 0L ) )
		{
			*mpp = mp;
			*iop = ahir;
			return( (struct Library *) ahir->ahir_Std.io_Device );
		}
		else
		{
			dbug( kprintf( "Can't open ahi.device v4\n" ); )
		}
		
		DeleteIORequest( (struct IORequest *) ahir );
	}
	else
	{
		dbug( kprintf( "no memory for ahirequest or msgport\n" ); )
	}
	DeleteMsgPort( mp );
	
	return FALSE;
#if !defined(__MAXON__) && !defined(__AROS__)
#define SysBase	cb->cb_SysBase
#endif
}

/****************************************************************************/


struct SoundFuncData {
	UWORD		CyclesLeft;
	UWORD		Cycles;
	UWORD		Channels;
	UWORD		Sound;
	BOOL		Continuous;
	BOOL		Valid[2];
	BOOL		Restart;
	BOOL		*Repeat;
	struct Task	*SigTask;
#if !defined(__MAXON__) && !defined(__AROS__)
#undef SysBase
	struct Library	*AHIBase;
	struct Library	*SysBase;
#define AHIBase	sfd->AHIBase
#define SysBase	sfd->SysBase
#endif
};

#ifdef __MAXON__
void SoundFunc( struct Hook *h, struct AHIAudioCtrl *actrl, struct AHISoundMessage *ahism )
#else
void __interrupt SoundFunc( REG(a0, struct Hook *h),  REG(a2, struct AHIAudioCtrl *actrl), REG(a1, struct AHISoundMessage *ahism) )
#endif
{
	struct SoundFuncData	*sfd = (struct SoundFuncData *) h->h_Data;
	
	dbug( kprintf( "SoundFunc\n" ); )
	
	if( ! ( *sfd->Repeat ) )
	{
		if( ! ( sfd->CyclesLeft-- ) )
		{
			LONG	i, snd = sfd->Sound;
		
			sfd->Valid[ snd ] = FALSE;
		
			sfd->Sound = snd = ( sfd->Continuous ) ? ! snd : AHI_NOSOUND;
			/* no echos please */
			if( ! sfd->Valid[ snd ] )
			{
				snd = AHI_NOSOUND;
				sfd->Restart = TRUE;
			}
	
			for( i = 0; i < sfd->Channels; i++ )
			{
				AHI_SetSound( i, snd, 0L, 0L, actrl, 0L );
			}
			
			sfd->CyclesLeft = sfd->Cycles;
			
			Signal ( sfd->SigTask, SIGBREAKF_CTRL_C );
		}
	}
}

#if !defined(__MAXON__) && !defined(__AROS__)
#undef AHIBase
#undef SysBase
#define SysBase	cb->cb_SysBase
#endif

/****************************************************************************/

void PlayerProcAHI( void )
{
#if !defined(__MAXON__) && !defined(__AROS__)
	struct Library			*SysBase = (*(struct Library **)4L);
	struct Library			*AHIBase = NULL;
#endif
	struct Process		*pr = (struct Process *) FindTask( NULL );
	struct MsgPort		*mp, *AHImp;
	struct AHIRequest		*ahir;
	struct AHIAudioCtrl 	*actrl;
	struct SoundFuncData	*sfd;
	struct Hook			*SoundHook = NULL;
	struct ClassBase		*cb;
	struct InstanceData	*id;
	ULONG				mpmsk, numCh, buffersize;
	BOOL				stereo, paused = FALSE, failed;
	static UBYTE			sdtst2ahist[] = { AHIST_M8S, AHIST_S8S, AHIST_M16S, AHIST_S16S };
	BYTE				*buffer = NULL;
	
	mp = &pr->pr_MsgPort;
	mpmsk = 1L << mp->mp_SigBit;
	
	dbug( kprintf( "child launched\n" ); )
	
	for( ;; )
	{
		LONG			rcv;
		struct ObjectMsg	*msg;
		
		rcv = Wait( mpmsk | SIGBREAKF_CTRL_C );
		
		if( rcv & mpmsk )
		{
			while( ( msg = (struct ObjectMsg *) GetMsg( mp ) ) )
			{
				switch( msg->Command )
				{
					case COMMAND_NEXT_BUFFER:
					{
						dbug( kprintf( "child: COMMAND_NEXT_BUFFER\n"); )

						ObtainSemaphoreShared( &id->Lock );

						if( AHIBase )
						{
							BYTE	*dst = ( sfd->Sound ) ? &buffer[ buffersize ] : buffer;
							ULONG	transfer = UMult( id->SampleLength, bytesPerPoint[ id->SampleType ] ), i;
							
							if( transfer > buffersize )
							{
								transfer = buffersize;
							}
							
							if( (ULONG)id->Sample&3 || (ULONG)dst&3 || transfer&3 )
							{
								CopyMem( id->Sample, dst, transfer );
							}
							else
							{
								CopyMemQuick( id->Sample, dst, transfer );
							}
							
							for( i = transfer; i < buffersize; i++ )
							{
								dst[ i ] = 0;
							}
							
							sfd->Valid[ sfd->Sound ] = TRUE;
							
							/* new sample wasn't available fast enough */
							if( sfd->Restart )
							{
								sfd->Restart = FALSE;
								
								dbug( kprintf( "restart playback\n" ); )
								
								for( i = 0; i < numCh; i++ )
								{
									AHI_SetSound( i, sfd->Sound, 0,0, actrl, AHISF_IMM );
								}
							}
						}
					
						ReleaseSemaphore( &id->Lock );
					}
					break;
					
					case COMMAND_INIT:
						dbug( kprintf("child: COMMAND_INIT\n"); )
					
						id = (struct InstanceData *) msg->Data;
						cb = id->ClassBase;
					
						if( ! ( mp = msg->Data = CreateMsgPort() ) )
						{
							ReplyMsg( &msg->Message );
							return;
						}
					
						mpmsk = 1L << mp->mp_SigBit;
					break;
					
					case COMMAND_EXIT:
					case COMMAND_STOP:
					{
						dbug( kprintf("child: COMMAND_STOP/EXIT\n"); )
						
						if( AHIBase )
						{
							AHI_FreeAudio( actrl );
							FreeVec( SoundHook );
							FreeVec( buffer );
							buffer = NULL;
							CloseAHI( AHImp, ahir );
							AHIBase = NULL;
							
							SetTDMode( cb, id, BUT_STOP );
						}
							
						if( msg->Command == COMMAND_EXIT )
						{
							Forbid();
							ReplyMsg( &msg->Message );
							DeleteMsgPort( mp );
							return;
						}
					}
					break;
					
					case COMMAND_PERVOL:
						dbug( kprintf("child: COMMAND_PERVOL\n"); )
					
						if( AHIBase )
						{
							ULONG			freq, vol, pan, i;
							
							ObtainSemaphoreShared( &id->Lock );
							pan = id->Panning;
							vol = ( id->Volume << 10 );
							freq = id->Frequency;
							ReleaseSemaphore( &id->Lock );
							
							for( i = 0; i < numCh; i++ )
							{
								if( ! paused ) 
								{
									AHI_SetFreq( i, freq, actrl, AHISF_IMM );
								}
								AHI_SetVol( i, vol, pan, actrl, AHISF_IMM );
							}
						}
					break;
					
					case COMMAND_PAUSE:
					{
						if( AHIBase )
						{
							if( ! paused )
							{
								LONG	i;
								
								for( i = 0; i < numCh; i++ )
								{
									AHI_SetFreq( i, 0L, actrl, AHISF_IMM );
								}
							 
								paused = TRUE;
							}
						}
					}
					break;
					
					case COMMAND_PLAY:
						dbug( kprintf("child: COMMAND_PLAY\n"); )
					
						// ObtainSemaphoreShared( &id->Lock );
					
						if( AHIBase )
						{
							if( paused )
							{
								LONG	i;
								
								for( i = 0; i < numCh; i++ )
								{
									AHI_SetFreq( i, id->Frequency, actrl, AHISF_IMM );
								}
									
								paused = FALSE;
							}
							else
							{
								LONG	i;

								AHI_ControlAudio( actrl,
									AHIC_Play, FALSE,
									TAG_DONE );
								
								for( i = 0; i < numCh; i++ )
								{
									AHI_SetSound( i, sfd->Sound, 0,0, actrl, AHISF_IMM );
								}

								sfd->CyclesLeft = sfd->Cycles;
								
								AHI_ControlAudio( actrl,
									AHIC_Play, TRUE,
									TAG_DONE );
							}
						}
						else					
						{
							failed = TRUE;
							
							if( ( AHIBase = OpenAHI( &AHImp, &ahir ) ) )
							{							
								stereo = IsStereo( id->SampleType );
							
								if( ( SoundHook = (struct Hook *) AllocVec( sizeof( *SoundHook ) + sizeof( *sfd ), MEMF_CLEAR|MEMF_PUBLIC ) ) )
								{
									ULONG	audioID, mixfreq;
								#ifndef __AROS__
									extern void HookEntry( void );
								#endif
																
									numCh = stereo ? 2 : 1;

									sfd = (struct SoundFuncData *) (SoundHook+1);
#if defined(__MAXON__) || defined(__AROS__)								
									SoundHook->h_Entry = (HOOKFUNC) HookEntry;
									SoundHook->h_SubEntry = (HOOKFUNC) SoundFunc;
#else
									SoundHook->h_Entry = (HOOKFUNC) SoundFunc;
#endif
									SoundHook->h_Data = (APTR) sfd;
									
									sfd->SigTask = &pr->pr_Task;
									sfd->Channels = numCh;
									sfd->Cycles = 
									sfd->CyclesLeft = UMult( id->Cycles, numCh );
//									sfd->Sound = 0;
									sfd->Continuous = id->Continuous;
									sfd->Valid[ 0 ] = TRUE;
									sfd->Repeat = (BOOL *) &id->Repeat;
//									sfd->Valid[ 1 ] = FALSE;
//									sfd->Restart = FALSE;
#if !defined(__MAXON__) && !defined(__AROS__)
									sfd->AHIBase = AHIBase;
									sfd->SysBase = SysBase;
#endif
									dbug( kprintf("child: ahi is open\n"); )
									
									ObtainSemaphoreShared( &cb->cb_LibLock );
									
									mixfreq = cb->cb_AHIMixFrequency;
									
									if( cb->cb_ForceAHIMode )
									{
										audioID = cb->cb_AHIModeID;
									}
									else
									{									
										if( ( audioID = AHI_BestAudioID( 
											( cb->cb_AHIModeID ? AHIDB_AudioID : TAG_IGNORE ), cb->cb_AHIModeID,
											AHIDB_Volume, TRUE,
											( ! stereo ? TAG_IGNORE : AHIDB_Stereo ), TRUE,
											( ! stereo ? AHIDB_Panning : TAG_IGNORE ), TRUE,
											TAG_DONE ) ) == AHI_INVALID_ID )
										{
											audioID = AHI_DEFAULT_ID;
											dbug( kprintf("child: bestaid failed\n"); )
										}
									}
									
									ReleaseSemaphore( &cb->cb_LibLock );
									
									if( ( actrl = AHI_AllocAudio(
										AHIA_AudioID, audioID,
										AHIA_Channels, numCh,
										AHIA_Sounds, ( id->Continuous ) ? 2L : 1L,
										AHIA_MixFreq, mixfreq,
										( id->Cycles ? AHIA_SoundFunc : TAG_IGNORE), (IPTR) SoundHook,
										TAG_DONE ) ) )						
									{
										struct AHISampleInfo	sample;
										ULONG				freq, vol;
										
										freq = id->Frequency;
										vol = ( id->Volume << 10 );
			
										dbug( kprintf("child: alloc audio okay\n"); )
										
										sample.ahisi_Type = sdtst2ahist[ id->SampleType ];
										sample.ahisi_Length = id->SampleLength;
										
										if( id->Continuous )
										{
											ULONG playsamples;
										
											AHI_GetAudioAttrs( AHI_INVALID_ID, actrl, AHIDB_MaxPlaySamples,(IPTR) &playsamples, TAG_DONE );
											AHI_ControlAudio( actrl, AHIC_MixFreq_Query, (IPTR) &mixfreq, TAG_DONE );
										
											buffersize = UDiv( UMult( playsamples,  id->Frequency ), mixfreq );
										
											if( buffersize < id->SampleLength )
											{
												buffersize = id->SampleLength;
											}
											
											buffersize = UMult( buffersize, bytesPerPoint[ id->SampleType ] );
											
											if( ( buffer = AllocVec( buffersize*2, MEMF_PUBLIC|MEMF_CLEAR ) ) )
											{												
												sample.ahisi_Address	= buffer;
												
												if( ! AHI_LoadSound( 0, AHIST_DYNAMICSAMPLE, &sample, actrl ) )
												{
													sample.ahisi_Address	= &buffer[ buffersize ];
													
													failed = (BOOL) AHI_LoadSound( 1, AHIST_DYNAMICSAMPLE, &sample, actrl );
												}
												
												CopyMem( id->Sample, buffer, UMult( id->SampleLength, bytesPerPoint[ id->SampleType ] ) );
											}
											
										}
										else
										{
											sample.ahisi_Address	= id->Sample;
											
											failed = (BOOL) AHI_LoadSound( 0, AHIST_SAMPLE, &sample, actrl );
										}
										
										dbug( kprintf("child: loading sample\n"); )
										
										if( ! failed )
										{
											failed = TRUE;
											
											if( ! AHI_ControlAudio( actrl,
												AHIC_Play, TRUE,
												TAG_DONE ) )
											{
												AHI_Play( actrl, 
													AHIP_BeginChannel, 0,
													AHIP_Freq, freq,
													AHIP_Vol, vol,
													AHIP_Pan, id->Panning,
													AHIP_Sound, 0,
													AHIP_EndChannel, 0,
													( stereo ? TAG_IGNORE : TAG_END ), 0L,
													AHIP_BeginChannel, 1,
													AHIP_Freq, freq,
													AHIP_Vol, vol,
													AHIP_Pan, id->Panning,
													AHIP_Sound, 0,
													AHIP_EndChannel, 0,
													TAG_DONE );
												
												failed = FALSE;
												
												dbug( kprintf( "child: playback started, freq: %ld\n", freq ); )
											}
											else
											{
												dbug( kprintf( "child: CtrlAudio failed\n" ); )
											}
										}
										else
										{
											dbug( kprintf( "child: loadsample failed\n" ); )
										}
									}
									else
									{
										dbug( kprintf( "child: AllocAudio failed\n"); )
									}
								}
								else
								{
									dbug( kprintf( "child: No free store\n" ); )
								}
							}
							
							if( failed )
							{
								AHI_FreeAudio( actrl );
								FreeVec( SoundHook );
								FreeVec( buffer );
								buffer = NULL;
								CloseAHI( AHImp, ahir );
								AHIBase = NULL;
								SetTDMode( cb, id, BUT_STOP );
							}
							else
							{
								SetTDMode( cb, id, BUT_PLAY );
							}
						}
						
						// ReleaseSemaphore( &id->Lock );
					break;
				}

				if( msg->Message.mn_ReplyPort )
				{
					ReplyMsg( &msg->Message );
				}
				else // NT_FREEMSG
				{
					FreeVec( msg );
				}
			}
		}
		
		if( rcv & SIGBREAKF_CTRL_C )
		{
			dbug( kprintf( "child: end of sample\n" ); )
			
			if( AHIBase )
			{
				ObtainSemaphoreShared( &id->Lock );
				if( id->SignalTask && id->SignalBit != -1 )
				{
					Signal( id->SignalTask, 1L << id->SignalBit );
				}
				ReleaseSemaphore( &id->Lock );
			
				if( ! id->Continuous )
				{
					AHI_FreeAudio( actrl );
					FreeVec( SoundHook );
					FreeVec( buffer );
					buffer = NULL;
					CloseAHI( AHImp, ahir );
					AHIBase = NULL;
					SetTDMode( cb, id, BUT_STOP );
				}
			}
		}
	}
	
#if !defined(__MAXON__) && !defined(__AROS__)
#define SysBase	cb->cb_SysBase
#endif
}

/****************************************************************************/

unsigned __regargs StrLen( STRPTR str )
{
	STRPTR p = str;
	
	while( *str++ );
	
	return( ~(unsigned) (p - str) );
}

/****************************************************************************/

IPTR __regargs Sound_OBTAINDRAWINFO( Class *cl, Object *o, struct opSet *ops )
{
    struct InstanceData	*id = INST_DATA( cl, o );
			
    dbug( kprintf( "DTM_OBTAINDRAWINFO\n" ); )
			
    return	(IPTR)(	( id->Screen = (struct Screen *) GetTagData( PDTA_Screen, 0, ops->ops_AttrList ) ) ||
			( id->DrawInfo = (struct DrawInfo *) GetTagData( GA_DrawInfo, 0, ops->ops_AttrList ) )
    );
}

/****************************************************************************/

IPTR __regargs Sound_RELEASEDRAWINFO( Class *cl, Object *o, Msg msg )
{
    return (IPTR)0;
}

/****************************************************************************/

IPTR __regargs Sound_REMOVEDTOBJECT( Class *cl, Object *o, Msg msg )
{
    struct InstanceData	*id = INST_DATA( cl, o );
    /* prevent other tasks (cursor- or playertask) from reading this */
    dbug( kprintf( "DTM_REMOVEDTOBJECT\n" ); )
			
    ObtainSemaphore( &id->Lock );
    id->Window = (struct Window *)NULL;
    id->Requester = NULL;
    ReleaseSemaphore( &id->Lock );

    if( id->ColorMap )
    {
	ReleasePen( id->ColorMap, id->BackgroundPen );
	ReleasePen( id->ColorMap, id->WaveformPen );
    }
    
    return (IPTR) DoSuperMethodA(cl, o, msg);
}

    
