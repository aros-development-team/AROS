/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
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
VAR struct DrawInfo             *dri;
VAR struct Menu                 *menus;
VAR STRPTR                      filename;
VAR APTR                        vi;
VAR struct Catalog              *catalog;
VAR Object                      *dto;
VAR Object                      *old_dto;
VAR Object                      *model_obj;
VAR Object                      *dto_to_vert_ic_obj;
VAR Object                      *dto_to_horiz_ic_obj;
VAR Object                      *vert_to_dto_ic_obj;
VAR Object                      *horiz_to_dto_ic_obj;
VAR Object  	    	    	*model_to_dto_ic_obj;
VAR Object                      *gad[NUM_GADGETS];
VAR Object                      *img[NUM_IMAGES];
VAR WORD                        prog_exitcode;
VAR UBYTE                       filenamebuffer[300];
VAR UBYTE                       objnamebuffer[300];
VAR UBYTE                       s[300];
VAR UBYTE                       dto_supports_copy;
VAR UBYTE                       dto_supports_clearselected;
VAR UBYTE                       dto_supports_activate_field;
VAR UBYTE                       dto_supports_next_field;
VAR UBYTE                       dto_supports_prev_field;
