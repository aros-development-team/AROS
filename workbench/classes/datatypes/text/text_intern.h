
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif

#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif

#ifndef GRAPHICS_RPATTR_H
#   include <graphics/rpattr.h>
#endif

#ifndef INTUITION_IMAGECLASS_H
#   include <intuition/imageclass.h>
#endif

#ifndef INTUITION_ICCLASS_H
#   include <intuition/icclass.h>
#endif

#ifndef INTUITION_GADGETCLASS_H
#   include <intuition/gadgetclass.h>
#endif

#ifndef DATATYPES_DATATYPESCLASS_H
#   include <datatypes/datatypesclass.h>
#endif

#ifndef DATATYPES_TEXTCLASS_H
#   include <datatypes/textclass.h>
#endif

#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

#ifndef COMPILERSPECIFIC_H
#   include "compilerspecific.h"
#endif


#ifndef CLIB_ALIB_PROTOS_H
#   include <clib/alib_protos.h>
#endif

#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

#ifndef PROTO_DOS_H
#   include <proto/dos.h>
#endif

#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif

#ifndef PROTO_GRAPHICS_H
#   include <proto/graphics.h>
#endif

#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif

#ifndef PROTO_IFFPARSE_H
#   include <proto/iffparse.h>
#endif

#ifndef PROTO_LAYERS_H
#   include <proto/layers.h>
#endif

#ifdef COMPILE_DATATYPE

#ifndef PROTO_DATATYPES_H
#   include <proto/datatypes.h>
#endif

#endif

/**************************************************************************************************/

struct Text_Data
{
    /* Gadget stuff */

    LONG 		left, top;		/* Offsets of the gadget 			*/
    LONG 		width, height;		/* Dimensions of the gadget			*/

    struct Screen 	*screen;		/* Screen on which the gadget lies 		*/
														    // Wird benötigt!!
    struct DrawInfo 	*drinfo;		/* Resulting from screen			*/

#ifndef COMPILE_DATATYPE
    struct RastPort 	*rp;
    APTR 		line_pool;
#else
    LONG 		update_type;
    LONG 		update_arg;
    LONG 		mouse_pressed;
#endif

    STRPTR 		title;
    UBYTE 		*buffer_allocated;
    ULONG 		buffer_allocated_len;
    struct List 	line_list;		/* double linked list of the lines		*/
    
    struct TextFont 	*font;
    struct TextAttr 	attr;

    LONG 		horiz_visible;
    LONG 		vert_visible;

    LONG 		vert_top;
    LONG 		horiz_top;

    LONG 		vert_diff;		/* For optimized Scrolling			*/
    LONG 		use_vert_diff;
    LONG 		horiz_diff;
    LONG 		use_horiz_diff;

    LONG 		oldmarkactivation;
    LONG 		mark_x1;
    LONG 		mark_x2;
    LONG 		mark_y1;
    LONG 		mark_y2;
    struct Line 	*mark_line1;
    struct Line 	*mark_line2;
    LONG 		pressed;

    LONG 		doubleclick;		/* 1 if doubleclicked, 2 if trippleclicked 	*/
    LONG 		lastsecs;		/* For Doubleclick check			*/
    LONG 		lastmics;
};

/**************************************************************************************************/

/* Global variables */

extern struct ExecBase 		*SysBase;
extern struct IntuitionBase 	*IntuitionBase;
extern struct GfxBase 		*GfxBase;
#ifdef _AROS
extern struct UtilityBase	*UtilityBase;
#else
extern struct Library		*UtilityBase;
#endif
extern struct DosLibrary	*DOSBase;
extern struct Library 		*LayersBase;
extern struct Library 		*DiskfontBase;
extern struct Library 		*DataTypesBase;
extern struct Library 		*IFFParseBase;

extern struct IClass		*dt_class;

/**************************************************************************************************/

/* Protos: support.c */

struct MinNode *	Node_Next(APTR node);
struct MinNode *	Node_Prev(APTR node);
struct MinNode *	List_First(APTR list);
struct MinNode *	List_Last(APTR list);
ULONG 			List_Length(APTR list);
struct MinNode *	List_Find(APTR list, ULONG num);
STRPTR 			StrCopy( const STRPTR str );
STRPTR 			StrCopyPool( APTR pool, const STRPTR str );
STRPTR 			StrNCopyPool( APTR pool, const STRPTR str, LONG len);
LONG 			GetFileSize( BPTR fileh );
struct IFFHandle *	PrepareClipboard(void);
VOID 			FreeClipboard(struct IFFHandle *iff);

/**************************************************************************************************/
