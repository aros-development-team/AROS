/*
    Copyright (C) 1997-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

#include "compilerspecific.h"
#include "debug.h"
#include "arossupport.h"

/*********************************************************************************************/

#define ARG_TEMPLATE    "FILE,CLIPBOARD/S,CLIPUNIT/K/N,SCREEN/S,PUBSCREEN/K,REQUESTER/S," \
			"BOOKMARK/S,FONTNAME/K,FONTSIZE/K/N,BACKDROP/S,WINDOW/S," \
			"PORTNAME/K,IMMEDIATE/S,REPEAT/S,PRTUNIT/K/N"

#define ARG_FILE        0
#define ARG_CLIPBOARD   1
#define ARG_CLIPUNIT    2
#define ARG_SCREEN      3
#define ARG_PUBSCREEN   4
#define ARG_REQUESTER   5
#define ARG_BOOKMARK    6
#define ARG_FONTNAME    7
#define ARG_FONTSIZE    8
#define ARG_BACKDROP    9
#define ARG_WINDOW      10
#define ARG_PORTNAME    11
#define ARG_IMMEDIATE   12
#define ARG_REPEAT      13
#define ARG_PRTUNIT     14

#define NUM_ARGS        15

/*********************************************************************************************/

static struct libinfo
{
    APTR        var;
    STRPTR      name;
    WORD        version;
} libtable[] =
{
    {&IntuitionBase     , "intuition.library"           , 39    },
    {&GfxBase           , "graphics.library"            , 39    },
    {&GadToolsBase      , "gadtools.library"            , 39    },
    {&LayersBase        , "layers.library"              , 39    },
    {&UtilityBase       , "utility.library"             , 39    },
    {&KeymapBase        , "keymap.library"              , 39    },
    {&DataTypesBase     , "datatypes.library"           , 39    },
    {&DiskfontBase      , "diskfont.library"            , 39    },
    {NULL                                                       }
};

static struct TextAttr  textattr;
static struct TextFont  *font;
static struct RDArgs    *myargs;
static IPTR             args[NUM_ARGS];
static UBYTE            fontname[256];
static WORD             winwidth, winheight;
static WORD             sizeimagewidth, sizeimageheight;
static BOOL             model_has_members;

/*********************************************************************************************/

static void CloseLibs(void);
static void KillFont(void);
static void FreeArguments(void);
static void KillICObjects(void);
static void FreeVisual(void);
static void KillGadgets(void);
static void CloseDTO(void);
static void KillWindow(void);

/*********************************************************************************************/

WORD ShowMessage(STRPTR title, STRPTR text, STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(win, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("MultiView", msg, MSG(MSG_OK));     
	}
	else
	{
	    printf("MultiView: %s\n", msg);
	}
    }
    
    KillWindow();
    KillMenus();
    KillGadgets();
    FreeVisual();
    CloseDTO();
    KillICObjects();
    KillFont();
    FreeArguments();
    CloseLibs();
    CleanupLocale();
    
    exit(prog_exitcode);
}


/*********************************************************************************************/

static void OpenLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (!((*(struct Library **)li->var) = OpenLibrary(li->name, li->version)))
	{
	    sprintf(s, MSG(MSG_CANT_OPEN_LIB), li->name, li->version);
	    Cleanup(s);
	}       
    }
       
}

/*********************************************************************************************/

static void CloseLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (*(struct Library **)li->var) CloseLibrary((*(struct Library **)li->var));
    }
}

/*********************************************************************************************/

static void LoadFont(void)
{
    font = OpenDiskFont(&textattr);
    if (!font)
    {
	textattr.ta_Name  = "topaz.font";
	textattr.ta_YSize = 8;
	textattr.ta_Style = 0;
	textattr.ta_Flags = 0;
	
	font = OpenFont(&textattr);
    }
}

/*********************************************************************************************/

static void KillFont(void)
{
    if (font) CloseFont(font);
}

/*********************************************************************************************/

static void GetArguments(void)
{
    struct TextFont *defaultfont = GfxBase->DefaultFont;
    
    /* This might be a bit problematic depending on how default system font
       switching through Font prefs program works and if then the previous
       default system font is closed or not. So this is very likely only safe
       when in such a case the previous font is not closed (means -> the font
       will remain in memory in any case)
       
       ClipView example program on Amiga Dev CD also does it like this. So ... */
       
    textattr.ta_Name  = defaultfont->tf_Message.mn_Node.ln_Name;
    textattr.ta_YSize = defaultfont->tf_YSize;
    textattr.ta_Style = defaultfont->tf_Style;
    textattr.ta_Flags = defaultfont->tf_Flags;
    
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
	Fault(IoErr(), 0, s, 256);
	Cleanup(s);
    }
    
    filename = (STRPTR)args[ARG_FILE];
    if (!filename && !args[ARG_CLIPBOARD])
    {
	filename = GetFile();
	if (!filename) Cleanup(NULL);
    }
    
    if (args[ARG_FONTNAME])
    {
	strncpy(fontname, (char *)args[ARG_FONTNAME], 255 - 5);
	if (!strstr(fontname, ".font")) strcat(fontname, ".font");
	
	textattr.ta_Name = fontname;
    }
    
    if (args[ARG_FONTSIZE])
    {
	textattr.ta_YSize = *(LONG *)args[ARG_FONTSIZE];
    }
    
}

/*********************************************************************************************/

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

/*********************************************************************************************/

static void MakeICObjects(void)
{
    static const struct TagItem dto_to_vert_map[] =
    {
	{DTA_TopVert            , PGA_Top       },
	{DTA_VisibleVert        , PGA_Visible   },
	{DTA_TotalVert          , PGA_Total     },
	{TAG_DONE                               }
    };
    static const struct TagItem dto_to_horiz_map[] =
    {
	{DTA_TopHoriz           , PGA_Top       },
	{DTA_VisibleHoriz       , PGA_Visible   },
	{DTA_TotalHoriz         , PGA_Total     },
	{TAG_DONE                               }
    };
    static const struct TagItem vert_to_dto_map[] =
    {
	{PGA_Top                , DTA_TopVert   },
	{TAG_DONE                               }
    };
    static const struct TagItem horiz_to_dto_map[] =
    {
	{PGA_Top                , DTA_TopHoriz  },
	{TAG_DONE                               }
    };
	
    model_obj           = NewObject(NULL, MODELCLASS, ICA_TARGET, ICTARGET_IDCMP,
						      TAG_DONE);
    dto_to_vert_ic_obj  = NewObject(NULL, ICCLASS, ICA_MAP, dto_to_vert_map,
						   TAG_DONE);
    dto_to_horiz_ic_obj = NewObject(NULL, ICCLASS, ICA_MAP, dto_to_horiz_map,
						   TAG_DONE);
    vert_to_dto_ic_obj  = NewObject(NULL, ICCLASS, ICA_MAP, vert_to_dto_map,
						   TAG_DONE);
    horiz_to_dto_ic_obj = NewObject(NULL, ICCLASS, ICA_MAP, horiz_to_dto_map,
						   TAG_DONE);   

    if (!model_obj || !dto_to_vert_ic_obj || !dto_to_horiz_ic_obj ||
	!vert_to_dto_ic_obj || !horiz_to_dto_ic_obj)
    {
	Cleanup(MSG(MSG_CANT_CREATE_IC));
    }                                                                      

    DoMethod(model_obj, OM_ADDMEMBER, dto_to_vert_ic_obj);
    DoMethod(model_obj, OM_ADDMEMBER, dto_to_horiz_ic_obj);
    
    model_has_members = TRUE;
						 
}

/*********************************************************************************************/

static void KillICObjects(void)
{
    if (!model_has_members)
    {
	if (dto_to_vert_ic_obj) DisposeObject(dto_to_vert_ic_obj);
	if (dto_to_horiz_ic_obj) DisposeObject(dto_to_horiz_ic_obj);
    }
    
    if (model_obj) DisposeObject(model_obj);
    if (vert_to_dto_ic_obj) DisposeObject(vert_to_dto_ic_obj);
    if (horiz_to_dto_ic_obj) DisposeObject(horiz_to_dto_ic_obj);
}

/*********************************************************************************************/

static void GetVisual(void)
{
    scr = LockPubScreen(NULL);
    if (!scr) Cleanup(MSG(MSG_CANT_LOCK_SCR));
    
    dri = GetScreenDrawInfo(scr);
    if (!dri) Cleanup(MSG(MSG_CANT_GET_DRI));
    
    vi = GetVisualInfoA(scr, NULL);
    if (!vi) Cleanup(MSG(MSG_CANT_GET_VI));
}

/*********************************************************************************************/

static void FreeVisual(void)
{
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
}

/*********************************************************************************************/

static void MakeGadgets(void)
{
    static WORD img2which[] =
    {
	UPIMAGE,
	DOWNIMAGE,
	LEFTIMAGE,
	RIGHTIMAGE,
	SIZEIMAGE
    };
   
    IPTR imagew[NUM_IMAGES], imageh[NUM_IMAGES];
    WORD v_offset, h_offset, btop, i;

    for(i = 0; i < NUM_IMAGES; i++)
    {
	img[i] = NewObject(NULL, SYSICLASS, SYSIA_DrawInfo      , dri           ,
					    SYSIA_Which         , img2which[i]  ,
					    TAG_DONE);

	if (!img[i]) Cleanup(MSG(MSG_CANT_CREATE_SYSIMAGE));

	GetAttr(IA_Width,(Object *)img[i],&imagew[i]);
	GetAttr(IA_Height,(Object *)img[i],&imageh[i]);
    }

    sizeimagewidth  = imagew[IMG_SIZE];
    sizeimageheight = imageh[IMG_SIZE];
    
    btop = scr->WBorTop + dri->dri_Font->tf_YSize + 1;

    v_offset = imagew[IMG_DOWNARROW] / 4;
    h_offset = imageh[IMG_LEFTARROW] / 4;

    gad[GAD_UPARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , img[IMG_UPARROW]                                                      ,
	    GA_RelRight         , -imagew[IMG_UPARROW] + 1                                              ,
	    GA_RelBottom        , -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1   ,
	    GA_ID               , GAD_UPARROW                                                           ,
	    GA_RightBorder      , TRUE                                                                  ,
	    GA_Immediate        , TRUE                                                                  ,
	    GA_RelVerify        , TRUE                                          ,
	    TAG_DONE);

    gad[GAD_DOWNARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , img[IMG_DOWNARROW]                            ,
	    GA_RelRight         , -imagew[IMG_UPARROW] + 1                      ,
	    GA_RelBottom        , -imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1   ,
	    GA_ID               , GAD_DOWNARROW                                 ,
	    GA_RightBorder      , TRUE                                          ,
	    GA_Previous         , gad[GAD_UPARROW]                              ,
	    GA_Immediate        , TRUE                                          ,
	    GA_RelVerify        , TRUE                                          ,
	    TAG_DONE);

    gad[GAD_VERTSCROLL] = NewObject(NULL, PROPGCLASS,
	    GA_Top              , btop + 1                                                                      ,
	    GA_RelRight         , -imagew[IMG_DOWNARROW] + v_offset + 1                                         ,
	    GA_Width            , imagew[IMG_DOWNARROW] - v_offset * 2                                          ,
	    GA_RelHeight        , -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2     ,
	    GA_ID               , GAD_VERTSCROLL                                                                ,
	    GA_Previous         , gad[GAD_DOWNARROW]                                                            ,
	    GA_RightBorder      , TRUE                                                                          ,
	    GA_RelVerify        , TRUE                                                                          ,
	    GA_Immediate        , TRUE                                                                          ,
	    PGA_NewLook         , TRUE                                                                          ,
	    PGA_Borderless      , TRUE                                                                          ,
	    PGA_Total           , 100                                                                           ,
	    PGA_Visible         , 100                                                                           ,
	    PGA_Freedom         , FREEVERT                                                                      ,
	    TAG_DONE);

    gad[GAD_RIGHTARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , img[IMG_RIGHTARROW]                           ,
	    GA_RelRight         , -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1,
	    GA_RelBottom        , -imageh[IMG_RIGHTARROW] + 1                   ,
	    GA_ID               , GAD_RIGHTARROW                                ,
	    GA_BottomBorder     , TRUE                                          ,
	    GA_Previous         , gad[GAD_VERTSCROLL]                           ,
	    GA_Immediate        , TRUE                                          ,
	    GA_RelVerify        , TRUE                                          ,
	    TAG_DONE);

    gad[GAD_LEFTARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , img[IMG_LEFTARROW]                                                    ,
	    GA_RelRight         , -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1,
	    GA_RelBottom        , -imageh[IMG_RIGHTARROW] + 1                                           ,
	    GA_ID               , GAD_LEFTARROW                                                         ,
	    GA_BottomBorder     , TRUE                                                                  ,
	    GA_Previous         , gad[GAD_RIGHTARROW]                                                   ,
	    GA_Immediate        , TRUE                                                                  ,
	    GA_RelVerify        , TRUE                                                                  ,
	    TAG_DONE);

    gad[GAD_HORIZSCROLL] = NewObject(NULL,PROPGCLASS,
	    GA_Left             ,scr->WBorLeft                                                                          ,
	    GA_RelBottom        ,-imageh[IMG_LEFTARROW] + h_offset + 1                                                  ,
	    GA_RelWidth         ,-imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - scr->WBorRight - 2,
	    GA_Height           ,imageh[IMG_LEFTARROW] - (h_offset * 2)                                                 ,
	    GA_ID               ,GAD_HORIZSCROLL                                                                        ,
	    GA_Previous         ,gad[GAD_LEFTARROW]                                                                     ,
	    GA_BottomBorder     ,TRUE                                                                                   ,
	    GA_RelVerify        ,TRUE                                                                                   ,
	    GA_Immediate        ,TRUE                                                                                   ,
	    PGA_NewLook         ,TRUE                                                                                   ,
	    PGA_Borderless      ,TRUE                                                                                   ,
	    PGA_Total           ,100                                                                                    ,
	    PGA_Visible         ,100                                                                                    ,
	    PGA_Freedom         ,FREEHORIZ                                                                              ,
	    TAG_DONE);

    for(i = 0;i < NUM_GADGETS;i++)
    {
	if (!gad[i]) Cleanup(MSG(MSG_CANT_CREATE_GADGET));
    }
    
    SetAttrs(gad[GAD_VERTSCROLL] , ICA_TARGET, (IPTR)vert_to_dto_ic_obj, TAG_DONE);
    SetAttrs(gad[GAD_HORIZSCROLL], ICA_TARGET, (IPTR)horiz_to_dto_ic_obj, TAG_DONE);
    SetAttrs(dto_to_vert_ic_obj  , ICA_TARGET, (IPTR)gad[GAD_VERTSCROLL], TAG_DONE);
    SetAttrs(dto_to_horiz_ic_obj , ICA_TARGET, (IPTR)gad[GAD_HORIZSCROLL], TAG_DONE);
}

/*********************************************************************************************/

static void KillGadgets(void)
{
    WORD i;
    
    for(i = 0; i < NUM_GADGETS;i++)
    {
	if (win) RemoveGadget(win, (struct Gadget *)gad[i]);
	if (gad[i]) DisposeObject(gad[i]);
	gad[i] = 0;
    }
    
    for(i = 0; i < NUM_IMAGES;i++)
    {
	if (img[i]) DisposeObject(img[i]);
	img[i] = NULL;
    }
}

/*********************************************************************************************/

static void AddDTOToWin(void)
{
    EraseRect(win->RPort, win->BorderLeft,
			  win->BorderTop,
			  win->Width - 1 - win->BorderRight,
			  win->Height - 1 - win->BorderBottom);
			  
    SetDTAttrs (dto, NULL, NULL, GA_Left        , win->BorderLeft + 2                           ,
				 GA_Top         , win->BorderTop + 2                            ,
				 GA_RelWidth    , - win->BorderLeft - win->BorderRight - 4      ,
				 GA_RelHeight   , - win->BorderTop - win->BorderBottom - 4      ,
				 TAG_DONE);

    AddDTObject(win, NULL, dto, -1);
    RefreshDTObjects(dto, win, NULL, NULL);

}

/*********************************************************************************************/

static void OpenDTO(void)
{
    struct DTMethod *triggermethods;
    ULONG           *methods;
    STRPTR          objname = NULL;
    IPTR            val;
    
    old_dto = dto;

    if (!old_dto && args[ARG_CLIPBOARD])
    {
	APTR clipunit = 0;
	
	if (args[ARG_CLIPUNIT]) clipunit = *(APTR *)args[ARG_CLIPUNIT];

	D(bug("MultiView: calling NewDTObject\n"));
	
	dto = NewDTObject(clipunit, ICA_TARGET    , (IPTR)model_obj,
				    GA_ID         , 1000           ,
				    DTA_SourceType, DTST_CLIPBOARD ,
				    DTA_TextAttr  , (IPTR)&textattr,
				    TAG_DONE);
	
	D(bug("MultiView: NewDTObject returned %x\n", dto));                        
    }
    else
    {
	dto = NewDTObject(filename, ICA_TARGET      , (IPTR)model_obj,
				    GA_ID           , 1000           ,
				    DTA_TextAttr    , (IPTR)&textattr,
				    TAG_DONE);
    }

    if (!dto)
    {
	ULONG errnum = IoErr();
	
	if (errnum >= DTERROR_UNKNOWN_DATATYPE)
	    sprintf(s, GetDTString(errnum), filename);
	else
	    Fault(errnum, 0, s, 256);
	
	if (!old_dto) Cleanup(s);
	dto = old_dto;
	return;
    }
    
    strncpy(filenamebuffer, (filename ? filename : (STRPTR)""), 299);
    
    SetAttrs(vert_to_dto_ic_obj, ICA_TARGET, (IPTR)dto, TAG_DONE);
    SetAttrs(horiz_to_dto_ic_obj, ICA_TARGET, (IPTR)dto, TAG_DONE);

    val = 0;
    GetDTAttrs(dto, DTA_NominalHoriz, &val, TAG_DONE); winwidth  = (WORD)val;
    GetDTAttrs(dto, DTA_NominalVert , &val, TAG_DONE); winheight = (WORD)val;
    
    GetDTAttrs(dto, DTA_ObjName, &objname, TAG_DONE);    
    strncpy(objnamebuffer, objname ? objname : filenamebuffer, 299);
    
    dto_supports_copy = FALSE;
    dto_supports_clearselected = FALSE;
    
    if ((methods = GetDTMethods(dto)))
    {
	if (FindMethod(methods, DTM_COPY)) dto_supports_copy = TRUE;
	if (FindMethod(methods, DTM_CLEARSELECTED)) dto_supports_clearselected = TRUE;
    }

    if ((triggermethods = GetDTTriggerMethods(dto)))
    {
	if (FindTriggerMethod(triggermethods, NULL, STM_ACTIVATE_FIELD)) dto_supports_activate_field = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_NEXT_FIELD))     dto_supports_next_field     = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_PREV_FIELD))     dto_supports_prev_field     = TRUE;
   }
	
    if (old_dto)
    {
	if (win) RemoveDTObject(win, old_dto);
	DisposeDTObject(old_dto);

	if (win)
	{
	    AddDTOToWin();
	    SetWindowTitles(win, objnamebuffer, (UBYTE *)~0);
	    SetMenuFlags();
	}
    }
    
}

/*********************************************************************************************/

static void CloseDTO(void)
{
    if (dto)
    {
	if (win) RemoveDTObject(win, dto);
	DisposeDTObject(dto);
	dto = NULL;
    }
}

/*********************************************************************************************/

static void MakeWindow(void)
{
    WORD minwidth, minheight;
    
    if (!winwidth) winwidth = scr->Width;
    if (!winheight) winheight = scr->Height - scr->BarHeight - 1 - 
				scr->WBorTop - scr->Font->ta_YSize - 1 - sizeimageheight;
    
    minwidth  = (winwidth  < 50) ? winwidth : 50;
    minheight = (winheight < 50) ? winheight : 50;

    win = OpenWindowTags(0, WA_PubScreen        , (IPTR)scr             ,
			    WA_Title            , (IPTR)objnamebuffer   ,
			    WA_CloseGadget      , TRUE                  ,
			    WA_DepthGadget      , TRUE                  ,
			    WA_DragBar          , TRUE                  ,
			    WA_SizeGadget       , TRUE                  ,
			    WA_Activate         , TRUE                  ,
			    WA_SimpleRefresh    , TRUE                  ,
			    WA_NoCareRefresh    , TRUE                  ,
			    WA_NewLookMenus     , TRUE                  ,
			    WA_Left             , 0                     ,
			    WA_Top              , scr->BarHeight + 1    ,
			    WA_InnerWidth       , winwidth              ,
			    WA_InnerHeight      , winheight             ,
			    WA_AutoAdjust       , TRUE                  ,
			    WA_MinWidth         , minwidth              ,
			    WA_MinHeight        , minheight             ,
			    WA_MaxWidth         , 16383                 ,
			    WA_MaxHeight        , 16383                 ,
			    WA_Gadgets          , (IPTR)gad[GAD_UPARROW],
			    WA_IDCMP            , IDCMP_CLOSEWINDOW |
						  IDCMP_GADGETUP    |
						  IDCMP_GADGETDOWN  |
						  IDCMP_MOUSEMOVE   |
						  IDCMP_VANILLAKEY  |                                             
						  IDCMP_RAWKEY      |
						  IDCMP_IDCMPUPDATE |
						  IDCMP_MENUPICK    |
						  IDCMP_INTUITICKS      ,
			    TAG_DONE);

    if (!win) Cleanup(MSG(MSG_CANT_CREATE_WIN));                            

    AddDTOToWin();
	
    SetMenuStrip(win, menus);
}

/*********************************************************************************************/

static void KillWindow(void)
{
    if (win)
    {
	if (dto) RemoveDTObject(win, dto);
	if (menus) ClearMenuStrip(win);
	CloseWindow(win);
	win = NULL;
	
	winwidth = winheight = 0;
    }
}

/*********************************************************************************************/

static void ScrollTo(UWORD dir, UWORD quali)
{
    IPTR val;
    LONG oldtop, top, total, visible;
    BOOL horiz;
    BOOL inc;
    
    if ((dir == CURSORUP) || (dir == CURSORDOWN))
    {
	horiz = FALSE;
	if (dir == CURSORUP) inc = FALSE; else inc = TRUE;
	
	GetDTAttrs(dto, DTA_TopVert, &val, TAG_DONE);top = (LONG)val;
	GetDTAttrs(dto, DTA_TotalVert, &val, TAG_DONE);total = (LONG)val;
	GetDTAttrs(dto, DTA_VisibleVert, &val, TAG_DONE);visible = (LONG)val;
    }
    else
    {
	horiz = TRUE;
	if (dir == CURSORLEFT) inc = FALSE; else inc = TRUE;
	
	GetDTAttrs(dto, DTA_TopHoriz, &val, TAG_DONE);top = (LONG)val;
	GetDTAttrs(dto, DTA_TotalHoriz, &val, TAG_DONE);total = (LONG)val;
	GetDTAttrs(dto, DTA_VisibleHoriz, &val, TAG_DONE);visible = (LONG)val;

    }
    
    oldtop = top;
    if (quali & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL))
    {
	if (inc) top = total; else top = 0;
    }
    else
    if (quali & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
    {
	if (inc) top += visible - 1; else top -= visible - 1;
    }
    else
    {
	if (inc) top++; else top--;
    }
    
    if (top + visible > total) top = total - visible;
    if (top < 0) top = 0;
    
    if (top != oldtop)
    {
    	struct Gadget *g;
	
	if (horiz)
	{
	    g = (struct Gadget *)gad[GAD_HORIZSCROLL];
	}
	else
	{
	    g = (struct Gadget *)gad[GAD_VERTSCROLL];
	}
#if 0
	SetGadgetAttrs(g, win, NULL, PGA_Top, top,
				     TAG_DONE);
#else
    	{
    	    struct TagItem tags[] =
	    {
		{PGA_Top    , top   },
		{TAG_DONE	    }
	    };

	    DoGadgetMethod(g, win, NULL, OM_UPDATE, (IPTR)tags, NULL, 0);
	}   
#endif

    } /* if (top != oldtop) */
    
}

/*********************************************************************************************/

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    struct TagItem      *tstate, *tags, *tag;
    struct MenuItem     *item;
    struct Gadget       *activearrowgad = NULL;
    WORD                arrowticker = 0, activearrowkind = 0;
    IPTR                tidata;
    UWORD               men;
    BOOL                quitme = FALSE;
    
    while (!quitme)
    {
	WaitPort(win->UserPort);
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch (msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_VANILLAKEY:
		    switch(msg->Code)
		    {
			case 27: /* ESC */
			    quitme = TRUE;
			    break;
			    
			case 13: /* RETURN */
			    DoTrigger(STM_ACTIVATE_FIELD);
			    break;
			    
			case 9: /* TAB */
			    DoTrigger(STM_NEXT_FIELD);
			    break;
			    
		    } /* switch(msg->Code) */
		    break;

		case IDCMP_RAWKEY:
		    switch(msg->Code)
		    {
			case CURSORUP:
			case CURSORDOWN:
			case CURSORRIGHT:
			case CURSORLEFT:
			    ScrollTo(msg->Code, msg->Qualifier);
			    break;
			
			case 0x70: /* HOME */
			    ScrollTo(CURSORUP, IEQUALIFIER_LALT);
			    break;
			    
			case 0x71: /* END */
			    ScrollTo(CURSORDOWN, IEQUALIFIER_LALT);
			    break;
			    
			case 0x48: /* PAGE UP */
			    ScrollTo(CURSORUP, IEQUALIFIER_LSHIFT);
			    break;
			    
			case 0x49: /* PAGE DOWN */
			    ScrollTo(CURSORDOWN, IEQUALIFIER_LSHIFT);
			    break;
			    
			case 0x42: /* SHIFT TAB? */
			    if (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			    {
				DoTrigger(STM_PREV_FIELD);
			    }
			    break;
			    
		    } /* switch(msg->Code) */
		    break;
		
		case IDCMP_GADGETDOWN:
		    arrowticker = 3;
		    activearrowgad = (struct Gadget *)msg->IAddress;
		    switch(activearrowgad->GadgetID)
		    {
			case GAD_UPARROW:
			    activearrowkind = CURSORUP;
			    ScrollTo(CURSORUP, 0);
			    break;
			    
			case GAD_DOWNARROW:
			    activearrowkind = CURSORDOWN;
			    ScrollTo(CURSORDOWN, 0);
			    break;
			    
			case GAD_LEFTARROW:
			    activearrowkind = CURSORLEFT;
			    ScrollTo(CURSORLEFT, 0);
			    break;

			case GAD_RIGHTARROW:
			    activearrowkind = CURSORRIGHT;
			    ScrollTo(CURSORRIGHT, 0);
			    break;
		    }
		    break;
		
		case IDCMP_INTUITICKS:
		    if (activearrowkind)
		    {
			if (arrowticker)
			{
			    arrowticker--;
			}
			else if (activearrowgad->Flags & GFLG_SELECTED)
			{
			    ScrollTo(activearrowkind, 0);
			}
		    }
		    break;
		    
		case IDCMP_GADGETUP:
		    switch(((struct Gadget *)msg->IAddress)->GadgetID)
		    {
			case GAD_UPARROW:
			case GAD_DOWNARROW:
			case GAD_LEFTARROW:
			case GAD_RIGHTARROW:
			    activearrowkind = 0;
			    break;
		    }
		    break;
			
		case IDCMP_MENUPICK:
		    men = msg->Code;            
		    while(men != MENUNULL)
		    {
			if ((item = ItemAddress(menus, men)))
			{
			    switch((ULONG)GTMENUITEM_USERDATA(item))
			    {
				case MSG_MEN_PROJECT_ABOUT:
				    About();
				    break;
				    
				case MSG_MEN_PROJECT_QUIT:
				    quitme = TRUE;
				    break;
				
				case MSG_MEN_PROJECT_OPEN:
				    filename = GetFile();
				    if (filename) OpenDTO();
				    break;
				
				case MSG_MEN_EDIT_COPY:
				    {
					struct dtGeneral dtg;
					
					dtg.MethodID = DTM_COPY;
					dtg.dtg_GInfo = NULL;
					
					DoDTMethodA(dto, win, NULL, (Msg)&dtg);
				    }
				    break;
				
				case MSG_MEN_EDIT_CLEARSELECTED:
				    {
					struct dtGeneral dtg;
					
					dtg.MethodID = DTM_CLEARSELECTED;
					dtg.dtg_GInfo = NULL;
					
					DoDTMethodA(dto, win, NULL, (Msg)&dtg);
				    }
				    break;
				
			    } /* switch(GTMENUITEM_USERDATA(item)) */
			    
			    men = item->NextSelect;
			}
			else
			{
			    men = MENUNULL;
			}
			
		    } /* while(men != MENUNULL) */
		    break;
		    
		case IDCMP_IDCMPUPDATE:
		    tstate = tags = (struct TagItem *)msg->IAddress;
		    while ((tag = NextTagItem ((const struct TagItem **)&tstate)))
		    {
			tidata = tag->ti_Data;
			switch (tag->ti_Tag)
			{
			    /* Change in busy state */
			    case DTA_Busy:
				if (tidata)
				    SetWindowPointer (win, WA_BusyPointer, TRUE, TAG_DONE);
				else
				    SetWindowPointer (win, WA_Pointer, NULL, TAG_DONE);
				break;

			    /* Error message */
			    case DTA_ErrorLevel:
/*                              if (tidata)
				{
				    errnum = GetTagData (DTA_ErrorNumber, NULL, tags);
				    PrintErrorMsg (errnum, (STRPTR) options[OPT_NAME]);
				}*/
				break;

			    /* Time to refresh */
			    case DTA_Sync:
				/* Refresh the DataType object */
				RefreshDTObjects (dto, win, NULL, NULL);
				break;
					    
			} /* switch (tag->ti_Tag) */
			
		    } /* while ((tag = NextTagItem ((const struct TagItem **)&tstate))) */
		    break;
		
	    } /* switch (msg->Class) */
	    
	    ReplyMsg((struct Message *)msg);
	    
	} /* while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /* while (!quitme) */
}

/*********************************************************************************************/

int main(void)
{
    InitLocale("Sys/multiview.catalog", 1);
    InitMenus();
    OpenLibs();
    GetArguments();
    LoadFont();
    MakeICObjects();
    OpenDTO();
    GetVisual();
    MakeGadgets();
    MakeMenus();
    MakeWindow();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


