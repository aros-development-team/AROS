/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VAR
#define VAR extern
#endif

VAR struct IntuitionBase        *IntuitionBase;
VAR struct GfxBase              *GfxBase;

#ifdef __AROS__
VAR struct UtilityBase          *UtilityBase;
VAR struct LocaleBase           *LocaleBase;
#else
VAR struct Library              *UtilityBase;
VAR struct Library              *LocaleBase;
#endif

VAR struct Library              *KeymapBase;
VAR struct Library              *LayersBase;
VAR struct Library              *DataTypesBase;
VAR struct Library              *AslBase;
VAR struct Library              *GadToolsBase;
VAR struct Library              *DiskfontBase;

VAR struct Screen               *scr;
VAR struct Window               *win;
VAR struct Rectangle            wincoords;
VAR struct DrawInfo             *dri;
VAR STRPTR                      filename;
VAR BPTR                        cd;     /* saved current directory */
VAR APTR                        vi;
VAR struct Catalog              *catalog;
VAR Object                      *dto;
VAR Object                      *old_dto;
VAR Object                      *model_obj;
VAR Object                      *dto_to_vert_ic_obj;
VAR Object                      *dto_to_horiz_ic_obj;
VAR Object                      *vert_to_dto_ic_obj;
VAR Object                      *horiz_to_dto_ic_obj;
VAR Object                      *model_to_dto_ic_obj;
VAR Object                      *gad[NUM_GADGETS];
VAR Object                      *img[NUM_IMAGES];
VAR UBYTE                       filenamebuffer[300];
VAR UBYTE                       objnamebuffer[300];
VAR UBYTE                       s[300];
/* current dt's group ID */
VAR ULONG                       dto_subclass_gid;
/* basic menus and menu dependent on current dt's group ID */
VAR struct Menu                 *menus;
VAR struct Menu                 *pictmenus;
VAR struct Menu                 *textmenus;
/* methods supported by current dt */
VAR UBYTE                       dto_supports_write;
VAR UBYTE                       dto_supports_write_iff;
VAR UBYTE                       dto_supports_print;
VAR UBYTE                       dto_supports_copy;
VAR UBYTE                       dto_supports_selectall;
VAR UBYTE                       dto_supports_clearselected;
/* triggers supported by current dt */
VAR UBYTE                       dto_supports_activate_field;
VAR UBYTE                       dto_supports_next_field;
VAR UBYTE                       dto_supports_prev_field;
VAR UBYTE                       dto_supports_retrace;
VAR UBYTE                       dto_supports_browse_prev;
VAR UBYTE                       dto_supports_browse_next;
VAR UBYTE                       dto_supports_search;
VAR UBYTE                       dto_supports_search_prev;
VAR UBYTE                       dto_supports_search_next;
/* variables for picture-dt scaling */
VAR UWORD                       pdt_origwidth;
VAR UWORD                       pdt_origheight;
VAR WORD                        pdt_zoom;
VAR BOOL                        pdt_fit_win;
VAR BOOL                        pdt_keep_aspect;
VAR BOOL                        pdt_force_map;
VAR BOOL                        pdt_pict_dither;
/* variable for text-dt word-wraping */
VAR BOOL                        tdt_text_wordwrap;
/* variables for AppWindow handling */
VAR struct MsgPort             *msgport;
VAR struct AppWindow           *appwindow;
VAR ULONG                       winmask;
VAR ULONG                       msgmask;
/* variables for Intuition's ScreenNotify */
VAR struct MsgPort             *isnport;
VAR APTR                        isnstarted;
VAR ULONG                       isnmask;
/* variable for using separate screen */
VAR BOOL                        separate_screen;

