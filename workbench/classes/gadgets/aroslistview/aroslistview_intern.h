/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSLISTVIEW_INTERN_H
#define AROSLISTVIEW_INTERN_H

#undef	AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE


#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_CGHOOKS_H
#   include <intuition/cghooks.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#   include <intuition/gadgetclass.h>
#endif

/* Predeclaration */
struct LVBase_intern;

#define GLOBAL_INTUIBASE

#define TURN_OFF_DEBUG

struct LVData
{
    struct Hook *lvd_DisplayHook;
    struct Hook *lvd_RenderHook;
    Object	*lvd_List;
    STRPTR	*lvd_DHArray;
    struct ColumnAttrs *lvd_ColAttrs;
    struct TextFont *lvd_Font;
    LONG	lvd_First; /* The nuber of the first item showed in list */
    UWORD	lvd_EntryHeight;

    /* Damage area */

    UWORD	lvd_DamageOffset;
    UWORD	lvd_NumDamaged;

    UBYTE	lvd_HorSpacing;
    UBYTE	lvd_VertSpacing;
    UBYTE	lvd_Flags;

    /* Number of columns that the displayhook returns items for */
    UBYTE	lvd_MaxColumns;
    /* Number of colomns to view. Depends on AROSA_List_Format */
    UBYTE	lvd_ViewedColumns;

    UBYTE	lvd_BackPen;
    UBYTE	lvd_FrontPen;

    /* To prevent 'echo' OM_UPDATEs (double redraws) */
    UBYTE	lvd_NotifyCount;

    /* For testting for doubleclicks */
    ULONG	lvd_StartSecs;
    ULONG	lvd_StartMicros;

};



/* The minwitdh of one or more colums is as large as the biggest entry */
#define LVFLG_SPECIALCOLWIDTH	(1 << 0)

#define LVFLG_READONLY		(1 << 1)
#define LVFLG_MULTISELECT	(1 << 2)
#define LVFLG_DOUBLECLICK	(1 << 3)

/* Macros */
#undef LVD
#define LVD(x) ((struct LVData *)x)

#undef UB
#define UB(x) ((UBYTE *)x)

#undef EG
#define EG(o) ((struct ExtGadget *)o)

#undef MIN
#define MIN(a,b) ((a < b) ? a : b)

#define ReCalcEntryHeight(data) \
		data->lvd_EntryHeight =   data->lvd_Font->tf_YSize  \
					+ data->lvd_VertSpacing;

#undef NumVisible
#define NumVisible(ibox, eh) \
	(((ibox)->Height - LV_BORDERWIDTH_Y * 2) / eh)




/* Constants */
#define LV_BORDERWIDTH_X 2
#define LV_BORDERWIDTH_Y 2

#define LV_DEFAULTHORSPACING 2
#define LV_DEFAULTVERTSPACING 1

struct ColumnAttrs
{
    UBYTE ca_Pen; /* Background pen for the entry */
    UBYTE ca_Flags;

    UWORD ca_Delta;
    WORD ca_MinWidth; /* A value of -1 makes minwidth as small as the largest text */

    /* computed before each render */

    UWORD ca_Left;
    UWORD ca_Right;
    UWORD ca_Width;

    /* Index into the array filled by displayhook for getting text for this column */
    UBYTE ca_DHIndex;

};

#define CA_ALIGN_MASK (3)

#define CA_ALIGN_LEFT	0
#define CA_ALIGN_RIGHT	1
#define CA_ALIGN_CENTRE 2

#define CAFLG_BAR		(1 << 3)
#define CAFLG_SPECIALCOLWIDTH	(1 << 4)


/* Prototypes */
BOOL ParseFormatString(STRPTR, struct LVData *, struct LVBase_intern *);

VOID RenderEntries(Class *, Object *, struct gpRender *, LONG, UWORD, BOOL, struct LVBase_intern *);

VOID GetGadgetIBox(Object *, struct GadgetInfo *, struct IBox *);
VOID DrawListBorder(struct RastPort *, UWORD *, struct IBox *, BOOL, struct LVBase_intern *);
VOID ComputeColumnWidths(UWORD, struct LVData *, struct LVBase_intern *);
VOID ComputeColLeftRight(UWORD, struct LVData *);
UWORD ShownEntries(struct LVData *, struct IBox *, struct LVBase_intern *);
VOID NotifyAttrs(Class *, Object *, struct opSet *, struct TagItem *);



/* Library stuff */
struct LVBase_intern
{
    struct Library	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;
    struct Library	*dosbase;

    #ifndef GLOBAL_INTUIBASE
    struct IntuitionBase *intuitionbase;
    #endif
    struct GfxBase	*gfxbase;
    struct Library	*utilitybase;

    struct IClass	*classptr;

};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

#undef LVB
#define LVB(b) ((struct LVBase_intern *)b)
#undef UtilityBase
#define UtilityBase	LVB(AROSListviewBase)->utilitybase


#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	LVB(AROSListviewBase)->intuitionbase
#endif

#undef GfxBase
#define GfxBase 	LVB(AROSListviewBase)->gfxbase

/* On Linux-M68k SysBase has to be global! */
extern struct ExecBase * SysBase;

/*
#undef SysBase
#define SysBase 	LVB(AROSListviewBase)->sysbase
*/
#undef DOSBase
#define DOSBase 	LVB(AROSListviewBase)->dosbase


#define expunge() \
AROS_LC0(BPTR, expunge, struct LVBase_intern *, AROSListviewBase, 3, AROSListview)

#endif /* ASL_INTERN_H */
