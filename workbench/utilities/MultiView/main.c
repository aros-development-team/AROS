/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include "global.h"

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

#include "compilerspecific.h"
#include "debug.h"
#include "arossupport.h"

extern struct NewMenu nm[];
extern struct NewMenu nmpict[];
extern struct NewMenu nmtext[];

/*********************************************************************************************/

/* Many datatype classes seem to rely on OM_NOTIFY calls coming back to the datatype object
   as OM_UPDATE :-\ */
   
#define BACK_CONNECTION 1

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
static void ScrollTo(UWORD dir, UWORD quali);
static void FitToWindow(void);

/*********************************************************************************************/

void OutputMessage(CONST_STRPTR msg)
{
    struct EasyStruct es;
    
    if (msg)
    {
	if ( IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI )
	{
	    es.es_StructSize   = sizeof(es);
	    es.es_Flags        = 0;
	    es.es_Title        = "MultiView";
	    es.es_TextFormat   = msg;
	    es.es_GadgetFormat = MSG(MSG_OK);
	   
	    EasyRequestArgs(win, &es, NULL, NULL);  
	}
	else
	{
	    printf("MultiView: %s\n", msg);
	}
    }
}

/*********************************************************************************************/

void Cleanup(CONST_STRPTR msg)
{
    OutputMessage(msg);
    
    KillWindow();
    KillMenus();
    KillGadgets();
    FreeVisual();
    CloseDTO();
    KillICObjects();
    KillFont();
    FreeArguments();
    
    if (cd != NULL) CurrentDir(cd); /* restore current directory */
    
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

static void InitDefaults(void)
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
}

/*********************************************************************************************/

static void GetArguments(void)
{

    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
	Fault(IoErr(), 0, s, 256);
	Cleanup(s);
    }

    filename = (STRPTR)args[ARG_FILE];
    if (!filename && !args[ARG_CLIPBOARD])
    {
	filename = GetFileName(MSG_ASL_OPEN_TITLE);
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
    dto_to_vert_ic_obj  = NewObject(NULL, ICCLASS, ICA_MAP, (IPTR)dto_to_vert_map,
						   TAG_DONE);
    dto_to_horiz_ic_obj = NewObject(NULL, ICCLASS, ICA_MAP, (IPTR)dto_to_horiz_map,
						   TAG_DONE);
    vert_to_dto_ic_obj  = NewObject(NULL, ICCLASS, ICA_MAP, (IPTR)vert_to_dto_map,
						   TAG_DONE);
    horiz_to_dto_ic_obj = NewObject(NULL, ICCLASS, ICA_MAP, (IPTR)horiz_to_dto_map,
						   TAG_DONE);
#if BACK_CONNECTION
    model_to_dto_ic_obj = NewObject(NULL, ICCLASS, TAG_DONE);
#endif
    
    if (!model_obj  	    	||
    	!dto_to_vert_ic_obj 	||
	!dto_to_horiz_ic_obj 	||
	!vert_to_dto_ic_obj 	||
	!horiz_to_dto_ic_obj 
    #if BACK_CONNECTION
	|| !model_to_dto_ic_obj
    #endif
       )
    {
	Cleanup(MSG(MSG_CANT_CREATE_IC));
    }

    DoMethod(model_obj, OM_ADDMEMBER, (IPTR) dto_to_vert_ic_obj);
    DoMethod(model_obj, OM_ADDMEMBER, (IPTR) dto_to_horiz_ic_obj);
#if BACK_CONNECTION
    DoMethod(model_obj, OM_ADDMEMBER, (IPTR) model_to_dto_ic_obj);
#endif
    
    model_has_members = TRUE;

}

/*********************************************************************************************/

static void KillICObjects(void)
{
    if (!model_has_members)
    {
	if (dto_to_vert_ic_obj) DisposeObject(dto_to_vert_ic_obj);
	if (dto_to_horiz_ic_obj) DisposeObject(dto_to_horiz_ic_obj);
    #if BACK_CONNECTION
	if (model_to_dto_ic_obj) DisposeObject(model_to_dto_ic_obj);
    #endif
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
    if (vi)  FreeVisualInfo(vi);
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
	img[i] = NewObject(NULL, SYSICLASS, SYSIA_DrawInfo      , (IPTR)( dri ),
					    SYSIA_Which         , (IPTR)( img2which[i] ),
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
	    GA_Image            , (IPTR)( img[IMG_UPARROW] ),
	    GA_RelRight         , (IPTR)( -imagew[IMG_UPARROW] + 1 ),
	    GA_RelBottom        , (IPTR)( -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1 ),
	    GA_ID               , (IPTR)( GAD_UPARROW ),
	    GA_RightBorder      , (IPTR)TRUE,
	    GA_Immediate        , (IPTR)TRUE,
	    GA_RelVerify        , (IPTR)TRUE,
	    TAG_DONE);

    gad[GAD_DOWNARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , (IPTR)( img[IMG_DOWNARROW] ),
	    GA_RelRight         , (IPTR)( -imagew[IMG_UPARROW] + 1 ),
	    GA_RelBottom        , (IPTR)( -imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1 ),
	    GA_ID               , (IPTR)( GAD_DOWNARROW ),
	    GA_RightBorder      , (IPTR)TRUE,
	    GA_Previous         , (IPTR)( gad[GAD_UPARROW] ),
	    GA_Immediate        , (IPTR)TRUE,
	    GA_RelVerify        , (IPTR)TRUE,
	    TAG_DONE);

    gad[GAD_VERTSCROLL] = NewObject(NULL, PROPGCLASS,
	    GA_Top              , (IPTR)( btop + 1 ),
	    GA_RelRight         , (IPTR)( -imagew[IMG_DOWNARROW] + v_offset + 1 ),
	    GA_Width            , (IPTR)( imagew[IMG_DOWNARROW] - v_offset * 2 ),
	    GA_RelHeight        , (IPTR)( -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2 ),
	    GA_ID               , (IPTR)( GAD_VERTSCROLL ),
	    GA_Previous         , (IPTR)( gad[GAD_DOWNARROW] ),
	    GA_RightBorder      , (IPTR)TRUE,
	    GA_RelVerify        , (IPTR)TRUE,
	    GA_Immediate        , (IPTR)TRUE,
	    PGA_NewLook         , (IPTR)TRUE,
	    PGA_Borderless      , (IPTR)TRUE,
	    PGA_Total           , (IPTR)100,
	    PGA_Visible         , (IPTR)100,
	    PGA_Freedom         , (IPTR)FREEVERT,
	    PGA_NotifyBehaviour , (IPTR)PG_BEHAVIOUR_NICE,
	    TAG_DONE);

    gad[GAD_RIGHTARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , (IPTR)( img[IMG_RIGHTARROW] ),
	    GA_RelRight         , (IPTR)( -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1 ),
	    GA_RelBottom        , (IPTR)( -imageh[IMG_RIGHTARROW] + 1 ),
	    GA_ID               , (IPTR)( GAD_RIGHTARROW ),
	    GA_BottomBorder     , (IPTR)TRUE,
	    GA_Previous         , (IPTR)( gad[GAD_VERTSCROLL] ),
	    GA_Immediate        , (IPTR)TRUE,
	    GA_RelVerify        , (IPTR)TRUE,
	    TAG_DONE);

    gad[GAD_LEFTARROW] = NewObject(NULL, BUTTONGCLASS,
	    GA_Image            , (IPTR)( img[IMG_LEFTARROW] ),
	    GA_RelRight         , (IPTR)( -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1 ),
	    GA_RelBottom        , (IPTR)( -imageh[IMG_RIGHTARROW] + 1 ),
	    GA_ID               , (IPTR)( GAD_LEFTARROW ),
	    GA_BottomBorder     , (IPTR)TRUE,
	    GA_Previous         , (IPTR)( gad[GAD_RIGHTARROW] ),
	    GA_Immediate        , (IPTR)TRUE,
	    GA_RelVerify        , (IPTR)TRUE,
	    TAG_DONE);

    gad[GAD_HORIZSCROLL] = NewObject(NULL, PROPGCLASS,
	    GA_Left             , (IPTR)( scr->WBorLeft ),
	    GA_RelBottom        , (IPTR)( -imageh[IMG_LEFTARROW] + h_offset + 1 ),
	    GA_RelWidth         , (IPTR)( -imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - scr->WBorRight - 2 ),
	    GA_Height           , (IPTR)( imageh[IMG_LEFTARROW] - (h_offset * 2) ),
	    GA_ID               , (IPTR)( GAD_HORIZSCROLL ),
	    GA_Previous         , (IPTR)( gad[GAD_LEFTARROW] ),
	    GA_BottomBorder     , (IPTR)TRUE,
	    GA_RelVerify        , (IPTR)TRUE,
	    GA_Immediate        , (IPTR)TRUE,
	    PGA_NewLook         , (IPTR)TRUE,
	    PGA_Borderless      , (IPTR)TRUE,
	    PGA_Total           , (IPTR)100,
	    PGA_Visible         , (IPTR)100,
	    PGA_Freedom         , (IPTR)FREEHORIZ,
	    PGA_NotifyBehaviour , (IPTR)PG_BEHAVIOUR_NICE,
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

void AddDTOToWin(void)
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
    // RefreshDTObjects(dto, win, NULL, NULL); needed ?

}

/*********************************************************************************************/

static void OpenDTO(void)
{
    struct DTMethod *triggermethods;
    ULONG           *methods;
    STRPTR          objname = NULL;
    IPTR            val;
    struct DataType *dt;

    old_dto = dto;

    do
    {
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

	    if (errnum == DTERROR_UNKNOWN_DATATYPE)
	    {
	        BPTR lock = Lock(filename,ACCESS_READ);
	        if (lock)
	        {
		    struct DataType *dtn;
		    if ((dtn = ObtainDataTypeA(DTST_FILE, lock, NULL)))
		    {
			if (!Stricmp(dtn->dtn_Header->dth_Name, "directory"))
			{
			    /* file is a directory and no directory.datatype is installed */
			    strncpy(filenamebuffer, (filename ? filename : (STRPTR)""), 298);
			    filenamebuffer[298]=0;

			    if (strlen(filenamebuffer) &&
			        filenamebuffer[strlen(filenamebuffer)-1] != ':' &&
				filenamebuffer[strlen(filenamebuffer)-1] != '/')
			    {
				strcat(filenamebuffer,"/");
			    }

			    filename = GetFileName(MSG_ASL_OPEN_TITLE);
			    if (filename) continue;
			}
		        ReleaseDataType(dtn);
		    }
		    UnLock(lock);
	        }
	    }

	    if (errnum >= DTERROR_UNKNOWN_DATATYPE)
	        sprintf(s, GetDTString(errnum), filename);
	    else
	        Fault(errnum, 0, s, 256);

	    if (!old_dto) Cleanup(s);
	    dto = old_dto;
	    return;
        }
    } while (!dto);

    strncpy(filenamebuffer, (filename ? filename : (STRPTR)""), 299);

    SetAttrs(vert_to_dto_ic_obj, ICA_TARGET, (IPTR)dto, TAG_DONE);
    SetAttrs(horiz_to_dto_ic_obj, ICA_TARGET, (IPTR)dto, TAG_DONE);
#if BACK_CONNECTION
    SetAttrs(model_to_dto_ic_obj, ICA_TARGET, (IPTR)dto, TAG_DONE);
#endif

    val = 0;
    GetDTAttrs(dto, DTA_NominalHoriz, (IPTR)&val, TAG_DONE);
    pdt_origwidth = winwidth = (WORD)val;
    GetDTAttrs(dto, DTA_NominalVert , (IPTR)&val, TAG_DONE);
    pdt_origheight = winheight = (WORD)val;
    pdt_zoom = 1;
    pdt_fit_win = FALSE;
    pdt_keep_aspect = FALSE;

    /*
     *  Add 4 Pixels for border around DataType-Object
     *  See AddDTOToWin() for details
     */
    if(winwidth)
    {
     winwidth += 4;
    }

    if(winheight)
    {
     winheight += 4;
    }

    GetDTAttrs(dto, DTA_ObjName, (IPTR)&objname, TAG_DONE);
    strncpy(objnamebuffer, objname ? objname : filenamebuffer, 299);
    
    dt = NULL;
    dto_subclass_gid = 0;
    if (GetDTAttrs(dto, DTA_DataType, (IPTR)&dt, TAG_DONE))
    {
	if (dt)
	{
	    dto_subclass_gid = dt->dtn_Header->dth_GroupID;
	}
    }

    dto_supports_write = FALSE;
    dto_supports_write_iff = FALSE;
    dto_supports_print = FALSE;
    dto_supports_copy = FALSE;
    dto_supports_selectall = FALSE;
    dto_supports_clearselected = FALSE;

    if (DoWriteMethod(NULL, DTWM_RAW)) dto_supports_write = TRUE;	/* probe raw saving */
    if ((methods = GetDTMethods(dto)))
    {
	if (FindMethod(methods, DTM_WRITE)) dto_supports_write_iff = TRUE;
	if (FindMethod(methods, DTM_PRINT)) dto_supports_print = TRUE;
	if (FindMethod(methods, DTM_COPY)) dto_supports_copy = TRUE;
	if (FindMethod(methods, DTM_SELECT)) dto_supports_selectall = TRUE;
	if (FindMethod(methods, DTM_CLEARSELECTED)) dto_supports_clearselected = TRUE;
    }

    dto_supports_activate_field =  FALSE;
    dto_supports_next_field =  FALSE;
    dto_supports_prev_field =  FALSE;
    dto_supports_retrace =  FALSE;
    dto_supports_browse_next =  FALSE;
    dto_supports_browse_prev =  FALSE;
    dto_supports_search =  FALSE;
    dto_supports_search_next =  FALSE;
    dto_supports_search_prev =  FALSE;
    
    if ((triggermethods = (struct DTMethod *)GetDTTriggerMethods(dto)))
    {
	if (FindTriggerMethod(triggermethods, NULL, STM_ACTIVATE_FIELD)) dto_supports_activate_field = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_NEXT_FIELD))     dto_supports_next_field     = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_PREV_FIELD))     dto_supports_prev_field     = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_RETRACE))        dto_supports_retrace        = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_BROWSE_NEXT))    dto_supports_browse_next    = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_BROWSE_PREV))    dto_supports_browse_prev    = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_SEARCH))         dto_supports_search         = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_SEARCH_NEXT))    dto_supports_search_next    = TRUE;
	if (FindTriggerMethod(triggermethods, NULL, STM_SEARCH_PREV))    dto_supports_search_prev    = TRUE;
    }

    D(bug("\nMultiview: Found Methods:%s%s%s%s%s%s\n",
	dto_supports_write ? " DTM_WRITE->RAW" : "",
	dto_supports_write_iff ? " DTM_WRITE->IFF" : "",
	dto_supports_print ? " DTM_PRINT" : "",
	dto_supports_copy ? " DTM_COPY" : "",
	dto_supports_selectall ? " DTM_SELECT" : "",
	dto_supports_clearselected ? " DTM_CLEARSELECTED" : ""));
    
    D(bug("Multiview: Found Triggers:%s%s%s%s%s%s%s\n\n",
	dto_supports_activate_field ? " STM_ACTIVATE_FIELD" : "",
	dto_supports_next_field ? " STM_NEXT_FIELD" : "",
	dto_supports_prev_field ? " STM_PREV_FIELD" : "",
	dto_supports_retrace ? " STM_RETRACE" : "",
	dto_supports_browse_next ? " STM_BROWSE_NEXT" : "",
	dto_supports_browse_prev ? " STM_BROWSE_PREV" : "",
	dto_supports_search ? " STM_SEARCH" : "",
	dto_supports_search_next ? " STM_SEARCH_NEXT" : "",
	dto_supports_search_prev ? " STM_SEARCH_PREV" : ""));
    
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
						  IDCMP_NEWSIZE     |
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
    LONG oldtop, top, total, visible, delta = 1;
    BOOL horiz;
    BOOL inc;

#ifdef __AROS__    
    switch(dir)
    {
    	case RAWKEY_NM_WHEEL_UP:
	    dir = CURSORUP;
	    delta = 3;
	    break;
	    
	case RAWKEY_NM_WHEEL_DOWN:
	    dir = CURSORDOWN;
	    delta = 3;
	    break;
	    
	case RAWKEY_NM_WHEEL_LEFT:
	    dir = CURSORLEFT;
	    delta = 3;
	    break;
	    
	case RAWKEY_NM_WHEEL_RIGHT:
	    dir = CURSORRIGHT;
	    delta = 3;
	    break;
    }
#endif
    
    if ((dir == CURSORUP) || (dir == CURSORDOWN))
    {
	horiz = FALSE;
	if (dir == CURSORUP) inc = FALSE; else inc = TRUE;

	GetDTAttrs(dto, DTA_TopVert, (IPTR)&val, TAG_DONE);
	top = (LONG)val;
	GetDTAttrs(dto, DTA_TotalVert, (IPTR)&val, TAG_DONE);
	total = (LONG)val;
	GetDTAttrs(dto, DTA_VisibleVert, (IPTR)&val, TAG_DONE);
	visible = (LONG)val;
    }
    else
    {
	horiz = TRUE;
	if (dir == CURSORLEFT) inc = FALSE; else inc = TRUE;
	
	GetDTAttrs(dto, DTA_TopHoriz, (IPTR)&val, TAG_DONE);
	top = (LONG)val;
	GetDTAttrs(dto, DTA_TotalHoriz, (IPTR)&val, TAG_DONE);
	total = (LONG)val;
	GetDTAttrs(dto, DTA_VisibleHoriz, (IPTR)&val, TAG_DONE);
	visible = (LONG)val;

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
	if (inc) top += delta; else top -= delta;
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

	SetGadgetAttrs(g, win, NULL, PGA_Top, top,
				     TAG_DONE);

#ifdef __MORPHOS__
	/* Looks like setting PGA_Top on Amiga does not cause OM_NOTIFIEs
	   to be sent (to dto). Or something like that. */

	SetDTAttrs(dto, win, NULL, (horiz ? DTA_TopHoriz : DTA_TopVert), top, TAG_DONE);
#endif

    } /* if (top != oldtop) */

}

/*********************************************************************************************/

static void FitToWindow(void)
{
    if( pdt_fit_win )
    {
	int x, y;
	
	x = win->Width - (win->BorderLeft + win->BorderRight + 4);
	y = win->Height - (win->BorderTop + win->BorderBottom + 4);
	D(bug("=> width %ld height %ld\n", x, y));
	DoScaleMethod(x, y, pdt_keep_aspect);
//	DoLayout(TRUE); seems to be done by intuition ?
    }
}

/*********************************************************************************************/

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    const struct TagItem *tstate, *tags;
    struct TagItem      *tag;
    struct MenuItem     *item;
    struct Gadget       *activearrowgad = NULL;
    WORD                arrowticker = 0, activearrowkind = 0;
    IPTR                tidata;
    UWORD               men;
    BOOL                quitme = FALSE;
    const STRPTR	not_supported = "Sorry, not supported yet\n";
    
    while (!quitme)
    {
	WaitPort(win->UserPort);
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
//	    D(if (msg->Class!=IDCMP_INTUITICKS) bug("  Msg Class %08lx\n", (long)msg->Class));
	    switch (msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_VANILLAKEY:
//		    D(bug("Vanillakey %d\n", (int)msg->Code));
		    switch(msg->Code)
		    {
			case 27: /* ESC */
			    quitme = TRUE;
			    break;
			    
			case 13: /* RETURN */
			    if (dto_supports_activate_field) DoTrigger(STM_ACTIVATE_FIELD);
			    else if (dto_supports_search) DoTrigger(STM_SEARCH);
			    RefreshDTObjects (dto, win, NULL, (IPTR) NULL);
			    break;
			    
			case 9: /* TAB */
			    if (dto_supports_next_field) DoTrigger(STM_NEXT_FIELD);
			    else if (dto_supports_search_next) DoTrigger(STM_SEARCH_NEXT);
			    break;
			    
			case 8: /* Backspace */
			    if (dto_supports_retrace) DoTrigger(STM_RETRACE);
			    break;

			case '>':
			    if (dto_supports_browse_next) DoTrigger(STM_BROWSE_NEXT);
			    break;

			case '<':
			    if (dto_supports_browse_prev) DoTrigger(STM_BROWSE_PREV);
			    break;

		    } /* switch(msg->Code) */
		    break;

		case IDCMP_RAWKEY:
		    switch(msg->Code)
		    {
		    #ifdef __AROS__
		    	case RAWKEY_NM_WHEEL_UP:
		    	case RAWKEY_NM_WHEEL_DOWN:
		    	case RAWKEY_NM_WHEEL_LEFT:
		    	case RAWKEY_NM_WHEEL_RIGHT:
		    #endif
			case CURSORUP:
			case CURSORDOWN:
			case CURSORRIGHT:
			case CURSORLEFT:
			    ScrollTo(msg->Code, msg->Qualifier);
			    break;
			
		    #ifdef __AROS__
			case RAWKEY_HOME: /* HOME */
			    ScrollTo(CURSORUP, IEQUALIFIER_LALT);
			    break;
			    
			case RAWKEY_END: /* END */
			    ScrollTo(CURSORDOWN, IEQUALIFIER_LALT);
			    break;
			    
			case RAWKEY_PAGEUP: /* PAGE UP */
			    ScrollTo(CURSORUP, IEQUALIFIER_LSHIFT);
			    break;
			    
			case RAWKEY_PAGEDOWN: /* PAGE DOWN */
			    ScrollTo(CURSORDOWN, IEQUALIFIER_LSHIFT);
			    break;
		    #endif
		      
			case 0x42: /* SHIFT TAB? */
			    if (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			    {
				if (dto_supports_prev_field) DoTrigger(STM_PREV_FIELD);
				else if (dto_supports_search_prev) DoTrigger(STM_SEARCH_PREV);
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
			    
			default:
			    activearrowkind = 0;
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
//		    D(bug(" * MV: men %08lx\n", (long)men));
		    while(men != MENUNULL)
		    {
			if ((item = ItemAddress(menus, men)))
			{
//			    D(bug(" * MV: item %08lx  menus %08lx\n", (long)item, (long)menus));
			    switch((ULONG)GTMENUITEM_USERDATA(item))
			    {
				case MSG_MEN_PROJECT_OPEN:
				    filename = GetFileName(MSG_ASL_OPEN_TITLE);
				    if (filename) OpenDTO();
				    break;

				case MSG_MEN_PROJECT_SAVEAS:
				    filename = GetFileName(MSG_ASL_SAVE_TITLE);
				    if (filename) DoWriteMethod(filename, DTWM_RAW);
				    break;

				case MSG_MEN_PROJECT_SAVEAS_IFF:
				    filename = GetFileName(MSG_ASL_SAVE_TITLE);
				    if (filename) DoWriteMethod(filename, DTWM_IFF);
				    break;

				case MSG_MEN_PROJECT_PRINT:
				    OutputMessage(not_supported);
				    break;

				case MSG_MEN_PROJECT_ABOUT:
				    About();
				    break;

				case MSG_MEN_PROJECT_QUIT:
				    quitme = TRUE;
				    break;

				case MSG_MEN_EDIT_MARK:
				#if defined(__AROS__) && !defined(__MORPHOS__)
				    if (StartDragSelect(dto))
				#else
				    {
					struct DTSpecialInfo *si;

					/*
					** ClipView example on AmigaDev CD does just the following.
					** None of the checks AROS datatypes.library/StartDragSelect()
					** does.
					*/
					   
					si = (struct DTSpecialInfo *)(((struct Gadget *)dto)->SpecialInfo);
					si->si_Flags |= DTSIF_DRAGSELECT;
				    }
				#endif
				    {
					#warning TODO: change mouse pointer to crosshair
				    }
				    break;

				case MSG_MEN_EDIT_COPY:
				    {
					struct dtGeneral dtg;
					
					dtg.MethodID = DTM_COPY;
					dtg.dtg_GInfo = NULL;
					
					DoDTMethodA(dto, win, NULL, (Msg)&dtg);
				    }
				    break;
				
				case MSG_MEN_EDIT_SELECTALL:
				    OutputMessage(not_supported);
				    break;
				    
				case MSG_MEN_EDIT_CLEARSELECTED:
				    {
					struct dtGeneral dtg;

					dtg.MethodID = DTM_CLEARSELECTED;
					dtg.dtg_GInfo = NULL;
					
					DoDTMethodA(dto, win, NULL, (Msg)&dtg);
				    }
				    break;

				case MSG_MEN_WINDOW_SEPSCREEN:
				    OutputMessage(not_supported);
				    break;

				case MSG_MEN_WINDOW_MINIMIZE:
				    OutputMessage(not_supported);
				    break;

				case MSG_MEN_WINDOW_NORMAL:
				    OutputMessage(not_supported);
				    break;

				case MSG_MEN_WINDOW_MAXIMIZE:
				    OutputMessage(not_supported);
				    break;

				case MSG_MEN_SETTINGS_SAVEDEF:
				    OutputMessage(not_supported);
				    break;
				    
				case MSG_MEN_PICT_ZOOM_IN:
				    pdt_zoom++;
				    if (pdt_zoom == -1 ) pdt_zoom = 1;
				    DoZoom(pdt_zoom);
				    break;

				case MSG_MEN_PICT_ZOOM_OUT:
				    pdt_zoom--;
				    if (pdt_zoom == 0 ) pdt_zoom = -2;
				    DoZoom(pdt_zoom);
				    break;

				case MSG_MEN_PICT_RESET:
				    pdt_zoom = 1;
				    DoZoom(pdt_zoom);
				    break;

				case MSG_MEN_PICT_FIT_WIN:
				    pdt_fit_win = (item->Flags & CHECKED) ? TRUE : FALSE;
				    FitToWindow();
				    DoLayout(TRUE);
				    break;

				case MSG_MEN_PICT_KEEP_ASPECT:
				    pdt_keep_aspect = (item->Flags & CHECKED) ? TRUE : FALSE;
				    FitToWindow();
				    DoLayout(TRUE);
				    break;

				case MSG_MEN_PICT_FORCE_MAP:
				    SetDTAttrs (dto, NULL, NULL,
						PDTA_DestMode, (item->Flags & CHECKED) ? FALSE : TRUE,
						TAG_DONE);
				    DoLayout(TRUE);
				    break;

				case MSG_MEN_PICT_DITHER:
				    SetDTAttrs (dto, NULL, NULL,
						PDTA_DitherQuality, (item->Flags & CHECKED) ? 4 : 0,
						TAG_DONE);
				    DoLayout(TRUE);
				    break;

				case MSG_MEN_TEXT_WORDWRAP:
				    if (item->Flags & CHECKED)
					D(bug("wordwrap enabled\n"));
				    else
					D(bug("wordwrap disabled\n"));
				    SetDTAttrs (dto, NULL, NULL,
						TDTA_WordWrap, (item->Flags & CHECKED) ? TRUE : FALSE,
						TAG_DONE);
				    DoLayout(TRUE);
				    break;

				case MSG_MEN_TEXT_SEARCH:
				    if (dto_supports_search) DoTrigger(STM_SEARCH);
				    break;

				case MSG_MEN_TEXT_SEARCH_PREV:
				    if (dto_supports_search_prev) DoTrigger(STM_SEARCH_PREV);
				    break;

				case MSG_MEN_TEXT_SEARCH_NEXT:
				    if (dto_supports_search_next) DoTrigger(STM_SEARCH_NEXT);
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
		    
		case IDCMP_NEWSIZE:
		    D(bug("IDCMP NEWSIZE\n"));
		    FitToWindow();
		    break;

		case IDCMP_IDCMPUPDATE:
		    tstate = tags = (const struct TagItem *) msg->IAddress;
		    while ((tag = NextTagItem(&tstate)) != NULL)
		    {
			tidata = tag->ti_Data;
//			D(bug("IDCMP UPDATE %08lx %08lx\n", (long)tag->ti_Tag, (long)tag->ti_Data));
			switch (tag->ti_Tag)
			{
			    /* Change in busy state */
			    case DTA_Busy:
				if (tidata)
				    SetWindowPointer (win, WA_BusyPointer, TRUE, TAG_DONE);
				else
				    SetWindowPointer (win, WA_Pointer, (IPTR) NULL, TAG_DONE);
				break;

			    case DTA_Title:
				SetWindowTitles(win, (UBYTE *)tidata, (UBYTE *)~0);
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
				D(bug("Multiview: DTA_SYNC\n"));
				RefreshDTObjects (dto, win, NULL, (IPTR) NULL);
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

int main(int argc, char **argv)
{
    InitLocale("System/Utilities/MultiView.catalog", 1);
    InitMenus(nm);
    InitMenus(nmpict);
    InitMenus(nmtext);
    OpenLibs();
    InitDefaults();
    
    if (argc == 0)
    {
        struct WBStartup *startup = (struct WBStartup *) argv;
        
        if (startup->sm_NumArgs >= 2)
        {
            /* FIXME: all arguments but the first are ignored */
            cd       = CurrentDir(startup->sm_ArgList[1].wa_Lock);
            filename = startup->sm_ArgList[1].wa_Name;
        }
        else
        {
            filename = GetFileName(MSG_ASL_OPEN_TITLE);
            if (!filename) Cleanup(NULL);
        }
    }
    else
    {
        GetArguments();
    }
    
    LoadFont();
    MakeICObjects();
    OpenDTO();
    GetVisual();
    MakeGadgets();
    menus = MakeMenus(nm);
    pictmenus = MakeMenus(nmpict);
    textmenus = MakeMenus(nmtext);
    SetMenuFlags();
    MakeWindow();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/
