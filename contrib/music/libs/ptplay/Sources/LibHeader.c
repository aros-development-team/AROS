/* Project started 11-Aug-2003	*/

#include	<exec/initializers.h>
#include	<exec/nodes.h>
#include	<exec/resident.h>

#include	"LibHeader.h"

#define	COMPILE_VERSION	2
#define	COMPILE_REVISION	6
#define	COMPILE_DATE		"(23.3.2007)"
#define	PROGRAM_VER			"2.6"

static const char 					VerString[];
static const struct MyInitData	InitData;
static const ULONG					InitTable[];
static const char 					LibName[];

const struct Resident RomTag	=
{
	RTC_MATCHWORD,
	(struct Resident *)&RomTag,
	(struct Resident *)&RomTag+1,
	RTF_AUTOINIT | RTF_PPC | RTF_EXTENDED,
	COMPILE_VERSION,
	NT_LIBRARY,
	0,
	(char *)&LibName[0],
	(char *)&VerString[7],
	(APTR)&InitTable[0]

#ifdef	__MORPHOS__
	, COMPILE_REVISION, NULL
#endif
};

static const APTR FuncTable[] =
{
#ifdef __MORPHOS__
	(APTR)	FUNCARRAY_32BIT_NATIVE, 
#endif

	(APTR)	LibOpen,
	(APTR)	LibClose,
	(APTR)	LibExpunge,
	(APTR)	LibReserved,
	(APTR)	Init,
	(APTR)	PtRender,
	(APTR)	PtTest,
	(APTR)	PtCleanup,
	(APTR)	PtSetAttrs,
	(APTR)	PtGetAttr,
	(APTR)	PtSeek,
	(APTR)	-1
};

static const ULONG InitTable[] =
{
	sizeof(struct PtPlayLibrary),
	(ULONG)	FuncTable,
	(ULONG)	&InitData,
	(ULONG)	LibInit
};

static const struct MyInitData InitData	=
{
	0xa0,8,		NT_LIBRARY,0,
	0xa0,9,		-5,0,
	0x80,10,		(ULONG)&LibName[0],
	0xa0,14,		LIBF_SUMUSED|LIBF_CHANGED,0,
	0x90,20,		COMPILE_VERSION,
	0x90,22,		COMPILE_REVISION,
	0x80,24,		(ULONG)&VerString[7],
	0
};

static const char VerString[]	= "\0$VER: ptplay.library " PROGRAM_VER " "COMPILE_DATE;
static const char LibName[]	= "ptplay.library";

const char Authors[]	= "Ronald Hof, Timm S. Müller, Per Johansson, Ilkka Lehtoranta";

/**********************************************************************
	Globals
**********************************************************************/

#ifdef	__MORPHOS__
const ULONG __abox__	= 1;
#endif