#define DTTM_GET_STRING     0x20000
#define DTTM_SEARCH_NEXT    0x20001
#define DTTM_SEARCH_PREV    0x20002

struct dttGetString
{
  ULONG MethodID;
  struct GadgetInfo dttgs_GInfo;
/*  LONG dttgs_SearchMethod;*/
};

struct dttSearchText
{
   ULONG MethodID;
   struct GadgetInfo *dttst_GInfo;
   STRPTR dttst_Text;
   LONG dttst_TextLen;
};

struct Text_Data
{
    LONG 		left, top;		/* Offsets of the gadget 			*/
    LONG 		width, height;		/* Dimensions of the gadget			*/

    struct Screen 	*screen;		/* Screen on which the gadget lies 		*/
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

    char		search_buffer[128];
    struct Process	*search_proc;
    struct dttGetString msg;
    int		search_line;
};


#ifdef __cplusplus

extern "C"
{
#endif

APTR Text_Create(void);
VOID Text_SetFrameBox( APTR mem, struct Screen *scr, struct RastPort *rp, LONG left, LONG top, LONG width, LONG height);
VOID Text_Load(APTR mem, STRPTR);
VOID Text_ChangeDimension( APTR mem, LONG left, LONG top, LONG width, LONG height);
VOID Text_Redraw( APTR mem );
VOID Text_Free(APTR mem);
ULONG Text_PageHeight( APTR mem );
ULONG Text_PageWidth( APTR mem );
ULONG Text_VisibleHeight( APTR mem );
ULONG Text_VisibleTop( APTR mem );
ULONG Text_VisibleHoriz( APTR mem );
VOID Text_SetVisibleTop( APTR mem, ULONG newy );
VOID Text_SetVisibleLeft( APTR mem, ULONG newx );
VOID Text_HandleMouse( APTR mem, LONG x, LONG y, LONG code, ULONG secs, ULONG mics);
VOID Text_Print( APTR mem );

#ifdef __cplusplus
}
#endif
