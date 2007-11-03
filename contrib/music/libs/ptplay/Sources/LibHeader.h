#ifndef	PTPLAY_LIBHEADER_H
#define	PTPLAY_LIBHEADER_H

#ifndef	DOS_DOS_H
#include	<dos/dos.h>
#endif

#ifndef	EXEC_LIBRARIES_H
#include	<exec/libraries.h>
#endif

#ifndef	__DECLGATE_H__
#include	"declgate.h"
#endif

#ifndef	UTILITY_UTILITY_H
#include	<utility/utility.h>
#endif

#ifndef	RTF_PPC
#define	RTF_PPC			0
#endif

#ifndef	RTF_EXTENDED
#define	RTF_EXTENDED	0
#endif

#ifdef __MORPHOS__
#define REG(reg,arg) arg
#else
#define REG(reg,arg) MREG(reg,arg)
#endif

#ifndef	__GNUC__
#define	__attribute__(x)
#endif

#include	"ptplay_priv.h"

/*********************************************************************
 * @Structures																			*
 ********************************************************************/

struct MyInitData
{
	UBYTE ln_Type_Init[4];
	UBYTE ln_Pri_Init[4];
	UBYTE ln_Name_Init[2];
	ULONG __attribute__((aligned(2)))	ln_Name_Content;
	UBYTE lib_Flags_Init[4];
	UBYTE lib_Version_Init[2]; UWORD lib_Version_Content;
	UBYTE lib_Revision_Init[2]; UWORD lib_Revision_Content;
	UBYTE lib_IdString_Init[2];
	ULONG  __attribute__((aligned(2)))	lib_IdString_Content;
	UWORD EndMark;
} __attribute__((packed));

struct PtPlayLibrary
{
	struct Library			Library;
	UWORD						Pad;
	BPTR						SegList;
	struct ExecBase		*MySysBase;
	struct UtilityBase	*MyUtilBase;
};

/*********************************************************************
 * @Prototypes																			*
 ********************************************************************/

ULONG					LibReserved(void);
struct Library	*	LibInit(struct PtPlayLibrary *LibBase, BPTR LibSegList, struct ExecBase *MySysBase);
BPTR					NATDECLFUNC_1(LibExpunge, a6, struct PtPlayLibrary *, LibBase);
BPTR					NATDECLFUNC_1(LibClose, a6, struct PtPlayLibrary *, LibBase);
struct Library	*	NATDECLFUNC_1(LibOpen, a6, struct PtPlayLibrary *, LibBase);

pt_mod_s *			NATDECLFUNC_5(Init, a1, UBYTE *, buf, d0, LONG, bufsize, d1, LONG, freq, d2, ULONG, modtype, a6, struct PtPlayLibrary *, LibBase);
VOID					NATDECLFUNC_8(PtRender, a0, pt_mod_s *, mod, a1, BYTE *, buf, a2, BYTE *, buf2, d0, LONG, bufmodulo, d1, LONG, numsmp, d2, LONG, scale, d3, LONG, depth, d4, LONG, channels);
ULONG					NATDECLFUNC_3(PtTest, a0, STRPTR, filename, a1, UBYTE *, buf, d0, LONG, bufsize);
VOID					NATDECLFUNC_2(PtCleanup, a0, pt_mod_s *, mod, a6, struct PtPlayLibrary *, LibBase);
VOID					NATDECLFUNC_3(PtSetAttrs, a0, pt_mod_s *, mod, a1, struct TagItem *, taglist, a6, struct PtPlayLibrary *, LibBase);
ULONG					NATDECLFUNC_3(PtGetAttr, a0, pt_mod_s *, mod, d0, ULONG, tagitem, a1, ULONG *, StoragePtr);
ULONG					NATDECLFUNC_2(PtSeek, a0, pt_mod_s *, mod, d0, ULONG, time);

#endif	/* PTPLAY_LIBHEADER_H */
