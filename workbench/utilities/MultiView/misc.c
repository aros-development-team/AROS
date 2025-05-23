/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <string.h>

#include "compilerspecific.h"
#include "debug.h"

/*********************************************************************************************/

static struct MenuItem * FindMenuItem( struct Menu *menu, ULONG msgid );
static void ChangeItemState( ULONG msgid, BOOL state );
static void SetItemChecked( ULONG msgid, BOOL state );

/*********************************************************************************************/

#ifdef __AROS__
/* NOTE: AROS uses '%id' for IPTR arrays */
static const char internal_about_tmpl[] = "%s %id.%id (%s)\n%s %s\n\n%s %s\n%s %s\n%s %s";
#else
static const char internal_about_tmpl[] = "%s %ld.%ld (%s)\n%s %s\n\n%s %s\n%s %s\n%s %s";
#endif

/*********************************************************************************************/

struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT                                                  },      /* 0 */
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_OPEN                                             },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_EXPORT                                           },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS_IFF                                       },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_PRINT                                            },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ABOUT                                            },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT                                             },
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT                                                     },      /* 1 */
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_MARK                                                },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_COPY                                                },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_SELECTALL                                           },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_CLEARSELECTED                                       },
    {NM_TITLE, (STRPTR)MSG_MEN_WINDOW                                                   },      /* 2 */
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_SEPSCREEN         , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MINIMIZE                                          },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_NORMAL                                            },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MAXIMIZE                                          },
    {NM_TITLE, (STRPTR)MSG_MEN_SETTINGS                                                 },      /* 3 */
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SAVEDEF                                         },
    {NM_END}
};

struct NewMenu nmpict[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PICT                                                      },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_ZOOM_IN                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_ZOOM_OUT                                             },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_RESET                                                },
     {NM_ITEM, NM_BARLABEL                                                               },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_FIT_WIN              , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_KEEP_ASPECT          , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, NM_BARLABEL                                                               },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_FORCE_MAP            , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_DITHER               , 0, CHECKIT | MENUTOGGLE | CHECKED },
    {NM_END}
};

struct NewMenu nmtext[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_TEXT                                                      },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_WORDWRAP             , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_SEARCH                                               },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_SEARCH_PREV                                          },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_SEARCH_NEXT                                          },
    {NM_END}
};

/*********************************************************************************************/

static struct MenuItem * FindMenuItem( struct Menu *menu, ULONG msgid )
{
    struct MenuItem *item;

    D(bug("[MultiView] %s()\n", __func__));

    while( menu )
    {
        if( (IPTR)GTMENU_USERDATA(menu) == msgid )
            return (struct MenuItem *)menu;
        item = menu->FirstItem;
        while( item )
        {
            if( (IPTR)GTMENUITEM_USERDATA(item) == msgid )
                return item;
            item = item->NextItem;
        }
        menu = menu->NextMenu;
    }
    return NULL;
}

/*********************************************************************************************/
                            
static void ChangeItemState( ULONG msgid, BOOL state )
{
    struct MenuItem *item;

    D(bug("[MultiView] %s()\n", __func__));

    item = FindMenuItem(menus, msgid);
    if (item)
    {
        if (state) item->Flags |= ITEMENABLED; else item->Flags &= ~ITEMENABLED;
    }
}

/*********************************************************************************************/
                            
static void SetItemChecked( ULONG msgid, BOOL state )
{
    struct MenuItem *item;

    D(bug("[MultiView] %s()\n", __func__));

    item = FindMenuItem(menus, msgid);
    if (item)
    {
        if (state) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
    }
}

/*********************************************************************************************/

void InitMenus(struct NewMenu *newm)
{
    struct NewMenu *actnm;

    D(bug("[MultiView] %s()\n", __func__));

    for(actnm = newm; actnm->nm_Type != NM_END; actnm++)
    {
        if (actnm->nm_Label != NM_BARLABEL)
        {
            ULONG  id = (IPTR)actnm->nm_Label;
            CONST_STRPTR str = MSG(id);
            
            if (actnm->nm_Type == NM_TITLE)
            {
                actnm->nm_Label = str;
            } else {
                actnm->nm_Label = str + 2;
                if (str[0] != ' ') actnm->nm_CommKey = str;
            }
            actnm->nm_UserData = (APTR)(IPTR)id;
            
        } /* if (actnm->nm_Label != NM_BARLABEL) */
        
    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */

}

/*********************************************************************************************/

struct Menu * MakeMenus(struct NewMenu *newm)
{
    struct Menu *menu;
    struct TagItem menu_tags[] =
    {
        {GTMN_NewLookMenus, TRUE},
        {TAG_DONE               }
    };

    D(bug("[MultiView] %s()\n", __func__));

    menu = CreateMenusA(newm, NULL);
    if (!menu) Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    
    if (!LayoutMenusA(menu, vi, menu_tags))
    {
        FreeMenus(menu);
        Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    }
    return menu;
}

/*********************************************************************************************/

void KillMenus(void)
{
    D(bug("[MultiView] %s()\n", __func__));

    if (win) ClearMenuStrip(win);
    if (menus) FreeMenus(menus);
    if (pictmenus) FreeMenus(pictmenus);
    if (textmenus) FreeMenus(textmenus);
    
    menus = NULL;
    pictmenus = NULL;
    textmenus = NULL;
}

/*********************************************************************************************/

void SetMenuFlags(void)
{
    struct Menu *menu;
    struct MenuItem *item;
    IPTR val;
    BOOL ret;

    D(bug("[MultiView] %s()\n", __func__));

    if (win) ClearMenuStrip(win);

    ChangeItemState( MSG_MEN_PROJECT_EXPORT, (cmdexport != NULL) ? TRUE : FALSE );
    ChangeItemState( MSG_MEN_PROJECT_SAVEAS_IFF, dto_supports_write_iff );
    ChangeItemState( MSG_MEN_PROJECT_PRINT, dto_supports_print );
    ChangeItemState( MSG_MEN_EDIT_COPY, dto_supports_copy );
    ChangeItemState( MSG_MEN_EDIT_SELECTALL, dto_supports_selectall );
    ChangeItemState( MSG_MEN_EDIT_CLEARSELECTED, dto_supports_clearselected );

    item = FindMenuItem(menus, MSG_MEN_SETTINGS);       /* Search last menu, then append dt group dependent menu */
    menu = (struct Menu *)item;
    if (menu)
    {
        if (dto_subclass_gid == GID_PICTURE)
        {
            D(bug("[MultiView] is picture.datatype\n"));
            menu->NextMenu = pictmenus;
            SetItemChecked( MSG_MEN_PICT_FIT_WIN, pdt_fit_win );
            SetItemChecked( MSG_MEN_PICT_KEEP_ASPECT, pdt_keep_aspect );
            SetItemChecked( MSG_MEN_PICT_FORCE_MAP, pdt_force_map );
            SetItemChecked( MSG_MEN_PICT_DITHER, pdt_pict_dither );
        }
        else if (dto_subclass_gid == GID_TEXT)
        {
            D(bug("[MultiView] is text.datatype\n"));
            menu->NextMenu = textmenus;
            ret = GetDTAttrs(dto, TDTA_WordWrap, (IPTR)&val, TAG_DONE);
            item = FindMenuItem(menus, MSG_MEN_TEXT_WORDWRAP);
            if (ret && item)
            {
                if (val) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
            }
            SetItemChecked( MSG_MEN_TEXT_WORDWRAP, tdt_text_wordwrap );
            ChangeItemState( MSG_MEN_TEXT_WORDWRAP, ret );
            ChangeItemState( MSG_MEN_TEXT_SEARCH, dto_supports_search );
            ChangeItemState( MSG_MEN_TEXT_SEARCH_PREV, dto_supports_search_prev );
            ChangeItemState( MSG_MEN_TEXT_SEARCH_NEXT, dto_supports_search_next );
        }
        else
        {
            D(bug("[MultiView] is unknown datatype\n"));
            menu->NextMenu = NULL;
        }
    }

    {
        struct TagItem menu_tags[] =
        {
            {GTMN_NewLookMenus, TRUE},
            {TAG_DONE               }
        };
        
        LayoutMenusA(menus, vi, menu_tags);
    }
    
    if (win) SetMenuStrip(win, menus);
}

/*********************************************************************************************/

STRPTR GetFileName(ULONG msgtextid)
{
    static UBYTE         pathbuffer[300];
    static UBYTE         filebuffer[300];
    struct FileRequester *req;
    STRPTR               filepart, retval = NULL;

    D(bug("[MultiView] %s()\n", __func__));

    AslBase = OpenLibrary("asl.library", 39);
    if (AslBase)
    {
        filebuffer[299] = 0;
        pathbuffer[299] = 0;
        
        strncpy(filebuffer, FilePart(filenamebuffer), 299);
        CopyMem(filenamebuffer, pathbuffer, 299);
        filepart = FilePart(pathbuffer);
        *filepart = 0;

        req = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText     , (IPTR)MSG(msgtextid),
                                                   ASLFR_DoPatterns    , TRUE                ,
                                                   ASLFR_InitialPattern, "~(#?.info)"        ,
                                                   ASLFR_InitialDrawer , (IPTR)pathbuffer    ,
                                                   ASLFR_InitialFile   , (IPTR)filebuffer    ,
                                                   ASLFR_Window        , (IPTR)win           ,
                                                   TAG_DONE);
        if (req)
        {
            if (AslRequest(req, NULL))
            {
                strncpy(filebuffer, req->fr_Drawer, 299);
                AddPart(filebuffer, req->fr_File, 299);
                
                retval = filebuffer;
                
            } /* if (AslRequest(req, NULL) */
            
            FreeAslRequest(req);

        } /* if (req) */
        
        CloseLibrary(AslBase);
        
    } /* if (AslBase) */
    
    return retval;
}

/*********************************************************************************************/

static struct Library * FindClassBase(CONST_STRPTR className)
{
    struct Library *lib;

    D(bug("[MultiView] %s()\n", __func__));

    Forbid();
    lib = (struct Library *)FindName(&SysBase->LibList, className);
    Permit();

    return lib;
}

static struct IClass *FindClassSuper(struct IClass *baseClass)
{
    struct IClass *tmp = baseClass->cl_Super;

    D(bug("[MultiView] %s()\n", __func__));

    while ((tmp->cl_Super != NULL) &&
           (strncmp(tmp->cl_Super->cl_ID, "datatypesclass", 14)))
    {
        tmp = tmp->cl_Super;
    }

    return tmp;
}

static char *versToStr(char *version)
{
    UBYTE  stringBuff[100];
    char *tmp;
    STRPTR              sp;
    WORD                i = 0;

    D(bug("[MultiView] %s()\n", __func__));

    for(sp = version;
        (*sp != 0) && ((*sp < '0') || (*sp > '9'));
        sp++)
    {
    }
    while ((*sp != 0) && (*sp != '\r') && (*sp != '\n') && (*(sp-1) !=')') && (i < 99))
    {
        stringBuff[i++] = *sp++;
    }
    stringBuff[i] = '\0';

    tmp = AllocVec(i + 1, MEMF_ANY);
    CopyMem(stringBuff, tmp, i + 1);

    return tmp;
}

void About(void)
{
    struct Library      *tmpBase;
    struct IClass       *clSuper;
    struct DataType     *dt = NULL;
    struct EasyStruct   es;
    struct DTClassInfo  *classInfo;
    char                *fmtTemplate;
    int                 count = 13, tmplLen;
    IPTR                *abouttxt;
    WORD                i = 0;

    D(bug("[MultiView] %s()\n", __func__));

    tmplLen = strlen(internal_about_tmpl);
    
    if ((classInfo = FindClassInfo(dto_subclass_gid)) != NULL)
    {
        i += classInfo->templen + 1;
        count += classInfo->entries;
    }

    fmtTemplate = AllocVec(tmplLen + i + 1, MEMF_ANY);
    CopyMem(internal_about_tmpl, fmtTemplate, tmplLen + 1);
    if (classInfo)
    {
        fmtTemplate[tmplLen++] = '\n';
        classInfo->aboutTemplate(&fmtTemplate[tmplLen]);
    }
    es.es_TextFormat = fmtTemplate;

    abouttxt = AllocVec(count * sizeof(IPTR), MEMF_ANY | MEMF_CLEAR);

    GetAttr(DTA_Name, dto, (IPTR *)&abouttxt[6]);
    if (abouttxt[6])
        abouttxt[6] = (IPTR)FilePart((CONST_STRPTR)abouttxt[6]);

    if (GetDTAttrs(dto, DTA_DataType, (IPTR)&dt, TAG_DONE))
    {
        if (dt)
        {
            abouttxt[7] = (IPTR) GetDTString(dt->dtn_Header->dth_GroupID);
            abouttxt[6] = (IPTR) dt->dtn_Header->dth_Name;
        }
    }
    
    if (!abouttxt[7]) abouttxt[7] = (IPTR)"";
    if (!abouttxt[6]) abouttxt[6] = (IPTR)"";

    /* display version info about datatypes.library */
    abouttxt[5] = (IPTR) versToStr(DataTypesBase->lib_IdString);

    /* display version info about datatype object's class */
    if ((tmpBase = FindClassBase(OCLASS(dto)->cl_ID)) != NULL)
    {
        abouttxt[8] = (IPTR) tmpBase->lib_Node.ln_Name;
        abouttxt[9] = (IPTR) versToStr(tmpBase->lib_IdString);
    }

    /* display version info about datatype object's superclass */
    if ((clSuper = FindClassSuper(OCLASS(dto))) != NULL)
    {
        if ((tmpBase = FindClassBase(clSuper->cl_ID)) != NULL)
        {
            abouttxt[10] = (IPTR) tmpBase->lib_Node.ln_Name;
            abouttxt[11] = (IPTR) versToStr(tmpBase->lib_IdString);
        }
    }

    if (!abouttxt[10]) abouttxt[10] = (IPTR)"";
    if (!abouttxt[11]) abouttxt[11] = (IPTR)StrDup("");

    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = MSG(MSG_ABOUT_TITLE);
    es.es_GadgetFormat = MSG(MSG_CONTINUE);
 
    abouttxt[0]   = (IPTR) es.es_Title;
    abouttxt[1]   = VERSION;
    abouttxt[2]   = REVISION;
    abouttxt[3]   = (IPTR) DATESTR;
    abouttxt[4]   = (IPTR)DataTypesBase->lib_Node.ln_Name;

    if (classInfo)
        classInfo->aboutFunc(dto, (char **)&abouttxt[12]);
    else
        abouttxt[12] = (IPTR)"";

    EasyRequestArgs(win, &es, NULL, (RAWARG)abouttxt);
  
    if (classInfo)
        classInfo->aboutDispose((char **)&abouttxt[12]);

    FreeVec(fmtTemplate);
    FreeVec((APTR)abouttxt[5]);
    FreeVec((APTR)abouttxt[9]);
    FreeVec((APTR)abouttxt[11]);
    FreeVec(abouttxt);
}

/*********************************************************************************************/

ULONG DoTrigger(ULONG what)
{
    struct dtTrigger msg;

    D(bug("[MultiView] %s()\n", __func__));

    msg.MethodID          = DTM_TRIGGER;
    msg.dtt_GInfo         = NULL;
    msg.dtt_Function      = what;
    msg.dtt_Data          = NULL;

    return DoDTMethodA(dto, win, NULL, (Msg)&msg);
}

/*********************************************************************************************/

ULONG DoWriteMethod(STRPTR name, ULONG mode)
{
    struct dtWrite msg;
    BPTR fh;
    ULONG retval;

    D(bug("[MultiView] %s()\n", __func__));

    fh = BNULL;
    if (name)
    {
        fh = Open( name, MODE_NEWFILE );
        if (!fh)
        {
            D(bug("[MultiView] Cannot open %s\n", name));
            OutputMessage(MSG(MSG_SAVE_FAILED));
            return FALSE;
        }
    }

    
    
    msg.MethodID          = DTM_WRITE;
    msg.dtw_GInfo         = NULL;
    msg.dtw_FileHandle    = fh;
    msg.dtw_Mode          = mode;
    msg.dtw_AttrList      = NULL;

    D(bug("[MultiView] Saving %s mode %ld\n", name ? name : (STRPTR)"[nothing]", mode));
    retval = DoDTMethodA(dto, win, NULL, (Msg)&msg);
    if (fh)
    {
        Close( fh );
        if( !retval )
        {
            D(bug("[MultiView] Error during write !\n"));
            OutputMessage(MSG(MSG_SAVE_FAILED));
        }
    }
    return retval;
}

/*********************************************************************************************/

ULONG DoPrintMethod(VOID)
{
    struct dtPrint msg;
    struct MsgPort *mp;
    struct IORequest *pio;
    ULONG retval = PDERR_CANCEL;

    D(bug("[MultiView] %s()\n", __func__));

    if ((mp = CreateMsgPort())) {
        if ((pio = CreateIORequest(mp, sizeof(union printerIO)))) {
            if (0 == OpenDevice("printer.device", 0, pio, 0)) {
                ULONG IDCMPFlags;

                msg.MethodID          = DTM_PRINT;
                msg.dtp_GInfo         = NULL;
                msg.dtp_PIO           = (union printerIO *)pio;
                msg.dtp_AttrList      = NULL;
                D(bug("[MultiView] Printing...\n"));

                /* We're not using PrintDTObjectA() here at this
                 * time, because we don't plan on waiting for the
                 * IDCMP_IDCMPUPDATE (DTA_PrinterStatus) message.
                 *
                 * So we just use the busy pointer while printing.
                 */
                IDCMPFlags = win->IDCMPFlags;
                ModifyIDCMP(win, 0);
                SetWindowPointer(win, WA_BusyPointer, TRUE);
                retval = DoDTMethodA(dto, win, NULL, (Msg)&msg);
                ModifyIDCMP(win, IDCMPFlags);
                ClearPointer(win);

                CloseDevice(pio);
            }
            DeleteIORequest(pio);
        }
        DeleteMsgPort(mp);
    }

    return retval;
}

/*********************************************************************************************/

ULONG DoLayout(ULONG initial)
{
    ULONG res = 0;
    struct gpLayout msg;

    D(bug("[MultiView] %s()\n", __func__));

    D(bug("=> erase\n"));
    EraseRect(win->RPort, win->BorderLeft,
                          win->BorderTop,
                          win->Width - 1 - win->BorderRight,
                          win->Height - 1 - win->BorderBottom);

#if 1
    msg.MethodID        = GM_LAYOUT;
    msg.gpl_GInfo       = NULL;
    msg.gpl_Initial     = initial;

#if 0
    D(bug("=> doasynclayout libcall\n"));
    res = DoAsyncLayout(dto, &msg);
#else
    D(bug("=> GM_Layout method\n"));
    res = DoDTMethodA(dto, win, 0, (Msg)&msg);
#endif
    D(bug("layout result %ld\n", res));
#else
    RemoveDTObject(win, dto);
    AddDTObject(win, NULL, dto, -1);
#endif

    D(bug("[MultiView] %s: done\n", __func__));
    return res;
}

/*********************************************************************************************/

ULONG DoScaleMethod(ULONG xsize, ULONG ysize, BOOL aspect)
{
    D(bug("[MultiView] %s()\n", __func__));

#ifdef __GNUC__
    struct pdtScale msg;
    
    D(bug(" scale width %d height %d\n", xsize, ysize));
    msg.MethodID        = PDTM_SCALE;
    msg.ps_NewWidth     = xsize;
    msg.ps_NewHeight    = ysize;
    msg.ps_Flags        = aspect ? PScale_KeepAspect : 0;
    // D(bug("- method %08lx newwidth %ld newheight %ld flags %08lx\n", msg.MethodID, msg.ps_NewWidth, msg.ps_NewHeight, msg.ps_Flags));

    return DoMethodA(dto, (Msg)&msg);
#else
    return 0;
#endif
}

/*********************************************************************************************/

void DoZoom(WORD zoomer)
{
    UWORD curwidth, curheight;

    D(bug("[MultiView] %s()\n", __func__));

    if (zoomer > 0)
    {
        curwidth = pdt_origwidth * zoomer;
        curheight = pdt_origheight * zoomer;
    }
    else
    {
        curwidth = pdt_origwidth / -zoomer;
        curheight = pdt_origheight / -zoomer;
    }
    D(bug(" zoom %d width %d height %d\n", zoomer, curwidth, curheight));
    DoScaleMethod(curwidth, curheight, 0);
    DoLayout(TRUE);
}

/*********************************************************************************************/
