#ifndef LNF_MARKED
#define LNF_MARKED  (1<<15)
#endif


struct Text_Data
{
    LONG 	left, top;		/* Offsets of the gadget 			*/
    LONG 	width, height;		/* Dimensions of the gadget			*/
    LONG	fillpen, filltextpen;	/* pens for marking */

#ifndef COMPILE_DATATYPE
    struct Screen 	*screen;	/* Screen on which the gadget lies 		*/
    struct DrawInfo 	*drinfo;	/* Resulting from screen			*/

    struct RastPort 	*rp;
    APTR 	line_pool;
#else
    LONG 	update_type;
    LONG 	update_arg;
    LONG 	mouse_pressed;
    LONG	redraw;
#endif

    STRPTR 		title;
    UBYTE 		*buffer_allocated;
    ULONG 		buffer_allocated_len;
    struct List 	line_list;	/* double linked list of the lines		*/
    char *word_delim;
    LONG word_wrap;

    struct TextFont 	*font;
    struct TextAttr 	attr;

    LONG 	horiz_visible;
    LONG 	vert_visible;

    LONG 	vert_top;
    LONG 	horiz_top;

    LONG	horiz_unit;
    LONG	vert_unit;

    LONG 	vert_diff;		/* For optimized Scrolling			*/
    LONG 	use_vert_diff;
    LONG 	horiz_diff;
    LONG 	use_horiz_diff;

    LONG 	mark_x1;
    LONG 	mark_x2;
    LONG 	mark_y1;
    LONG 	mark_y2;
    struct Line *mark_line1;
    struct Line *mark_line2;
    LONG 	pressed;
    LONG 	copy_text;		/* if mb is released, copy the text into the clipboard */

    LONG 	doubleclick;		/* 1 if doubleclicked, 2 if trippleclicked 	*/
    LONG 	lastsecs;		/* For Doubleclick check			*/
    LONG 	lastmics;

    struct	TextExtent te;
    struct	RastPort font_rp;

    char	search_buffer[128];
    struct Process	*search_proc;	/* the search requester process */
    struct GadgetInfo	search_ginfo;	/* for the search process */
    int		search_line;
    int		search_pos;		/* x position */
    int		search_case;

#ifdef MORPHOS_AG_EXTENSION
    LONG        links;
    struct Line *marked_line;
    struct Line *selected_line;
    struct Line *last_marked_line;
    LONG        shinepen, shadowpen;
    BOOL        link_pressed;
    Object      *obj;
    UBYTE       word[128];     /* double clicked word */
    struct GadgetInfo *ginfo;
#endif
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
