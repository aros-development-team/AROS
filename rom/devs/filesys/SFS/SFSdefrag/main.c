/*
    Copyright © 2003, The AROS Development Team.
    $Id$
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <graphics/rastport.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include <aros/debug.h>

#include "support.h"
#include "../FS/packets.h"
#include "../FS/query.h"

#include <dos/filesystem.h>

#define _DEBUG_H
#include "../FS/asmsupport.c"

#include <stdio.h>
#include <stdlib.h>

#include "locale.h"

#define APPNAME "SFSdefrag"
#define VERSION "sfsdefrag 0.1 (29.11.05) ©AROS Dev Team"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

static const char version[] = "$VER: " VERSION "\n";

struct DefragmentStep {
  ULONG id;       // id of the step ("MOVE", "DONE" or 0)
  ULONG length;   // length in longwords (can be 0)
  ULONG data[0];  // size of this array is determined by length.
};

ULONG *bitmap;
UBYTE *oldname;

ULONG blocks_total;
ULONG blocks_inone;

ULONG steps[200];

struct Library *MUIMasterBase = NULL;
struct UtilityBase *UtilityBase = NULL;
struct MsgPort *mp;
    
int openLibs()
{
    if ((MUIMasterBase=OpenLibrary("muimaster.library", 0)) != NULL)
    {
        if ((UtilityBase=(struct UtilityBase*)OpenLibrary("utility.library", 0)) != NULL)
        {
            return 1;
        }
        CloseLibrary(MUIMasterBase);
    }
    return 0;
}

void closeLibs()
{
    if (UtilityBase != NULL)
        CloseLibrary((struct Library *)UtilityBase);

    if (MUIMasterBase != NULL)
        CloseLibrary(MUIMasterBase);

    MUIMasterBase = NULL;
    UtilityBase = NULL;
}

void cleanup(CONST_STRPTR message)
{
    if (message != NULL)
    {
        ShowError(NULL, NULL, message, TRUE);
        exit(RETURN_FAIL);
    }
    else
    {
        exit(RETURN_OK);
    }
}

Object *MakeLabel(STRPTR str)
{
  return (MUI_MakeObject(MUIO_Label, str, 0));
}

IPTR xget(Object * obj, ULONG attr)
{
    IPTR x = 0;

    get(obj, attr, &x);
    return x;
}

struct Hook select_hook;
struct Hook start_hook;
struct Hook stop_hook;
struct Hook refresh_hook;

BOOL stop_me;

struct BitMap *bm;
struct RastPort *rp;
struct DrawInfo *dri;

ULONG pen1, pen2, pen3;

Object *app;
Object *MainWindow;

Object *DevList;
Object *StartButton, *StopButton, *RefreshButton;
Object *Bmp;

void updateBitmap(ULONG first, ULONG last)
{
    ULONG cnt;
    ULONG blck = blocks_inone;
    
    for (cnt=first/blocks_inone; cnt <= (last+blocks_inone-1)/blocks_inone; cnt++)
    {
        ULONG x,y,pen=pen3;
        y = (cnt / 166) * 3;
        x = (cnt % 166) * 3;
        
        if ((cnt+1) * blocks_inone > blocks_total) blck = blocks_total - cnt*blocks_inone;
        
        if (bmtstz(bitmap, cnt*blocks_inone, blck))
            pen=pen2;
        
        SetAPen(rp, pen);
        RectFill(rp, x, y, x+1, y+1);
    }
    DoMethod(Bmp, MUIM_Draw);
}

void getDeviceData(STRPTR device)
{
    struct DosList *dl;

    dl=LockDosList(LDF_DEVICES|LDF_READ);

    if((dl=FindDosEntry(dl,device,LDF_DEVICES))!=0)
    {
        struct TagItem tags[] = {
            { ASQ_TOTAL_BLOCKS, 0},
            { TAG_DONE,         0}
        };
        
        mp = dl->dol_Task;
        
        UnLockDosList(LDF_DEVICES|LDF_READ);
        
        if(DoPkt(mp, ACTION_SFS_QUERY, (SIPTR)tags, 0, 0, 0, 0)!=DOSFALSE)
        {
            blocks_total=tags[0].ti_Data;
            blocks_inone=(blocks_total + 19255)/19256;
            bug("blocks_total=%d\nblocks_inone=%d\n",blocks_total,blocks_inone);
            
            if (bitmap)
                FreeVec(bitmap);
            
            if ((bitmap = AllocVec(blocks_total / 8 + 32, MEMF_CLEAR))!=0)
            {
                if(DoPkt(mp, ACTION_SFS_READ_BITMAP, (SIPTR)bitmap, 0, blocks_total, 0, 0)!=DOSFALSE)
                {
                    updateBitmap(0, blocks_total);
                }
            }
        }
    }
    else
        UnLockDosList(LDF_DEVICES|LDF_READ);
}           

AROS_UFH3(void, start_function,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *,      object, A2),
    AROS_UFHA(APTR,          msg,    A1))
{
    AROS_USERFUNC_INIT
    ULONG sigs;
    BOOL defragmented = FALSE;
    
    set(StartButton, MUIA_Disabled, TRUE);
    set(RefreshButton, MUIA_Disabled, TRUE);
    set(StopButton, MUIA_Disabled, FALSE);
    set(DevList, MUIA_Disabled, TRUE);

    stop_me = FALSE;

    if(DoPkt(mp, ACTION_SFS_DEFRAGMENT_INIT, 0, 0, 0, 0, 0)!=DOSFALSE)
    {
        do {
            ULONG clrlo=blocks_total,clrhi=0;
            ULONG setlo=blocks_total,sethi=0;
            
            if(DoPkt(mp, ACTION_SFS_DEFRAGMENT_STEP, (SIPTR)steps, 190, 0, 0, 0)!=DOSFALSE)
            {
                struct DefragmentStep *ds=(struct DefragmentStep *)steps;
                
                while(ds->id!=0)
                {
                    if(ds->id==AROS_BE2LONG(MAKE_ID('M','O','V','E')) && ds->length==3)
                    {
                        bmclr(bitmap, (blocks_total+31)/32, ds->data[2], ds->data[0]);
                        bmset(bitmap, (blocks_total+31)/32, ds->data[1], ds->data[0]);
                        
                        clrlo = MIN(clrlo, ds->data[2]);
                        clrhi = MAX(clrhi, (ds->data[2]+ds->data[0]));

                        setlo = MIN(setlo, ds->data[1]);
                        sethi = MAX(sethi, (ds->data[1]+ds->data[0]));
                        
                    }
                    else if(ds->id==AROS_BE2LONG(MAKE_ID('D','O','N','E')))
                    {
                        defragmented=TRUE;
                        break;
                    }
                    ds=(struct DefragmentStep *)((ULONG *)ds + 2 + ds->length);
                }
            }
            updateBitmap(clrlo, clrhi);
            updateBitmap(setlo, sethi);
            DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);
        }while(!stop_me && !defragmented);
    }

    set(StartButton, MUIA_Disabled, FALSE);
    set(StopButton, MUIA_Disabled, TRUE);
    set(RefreshButton, MUIA_Disabled, FALSE);
    set(DevList, MUIA_Disabled, FALSE);
        
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, stop_function,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *,      object, A2),
    AROS_UFHA(APTR,          msg,    A1))
{
    AROS_USERFUNC_INIT
    
    stop_me=TRUE;
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, refresh_function,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *,      object, A2),
    AROS_UFHA(APTR,          msg,    A1))
{
    AROS_USERFUNC_INIT
    
    ULONG active = xget(DevList, MUIA_List_Active);
    
    if (active != MUIV_List_Active_Off)
    {
        if(DoPkt(mp, ACTION_SFS_READ_BITMAP, (SIPTR)bitmap, 0, blocks_total, 0, 0)!=DOSFALSE)
        {
            updateBitmap(0, blocks_total);
        }
    }
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, select_function,
    AROS_UFHA(struct Hook *, h,      A0),
    AROS_UFHA(Object *,      object, A2),
    AROS_UFHA(APTR,          msg,    A1))
{
    AROS_USERFUNC_INIT
    
    ULONG active = xget(object, MUIA_List_Active);
    
    if (active != MUIV_List_Active_Off)
    {
        UBYTE *name;
        DoMethod(object, MUIM_List_GetEntry, active, (IPTR)&name);
        if (name != oldname)
        {
            oldname = name;
            SetAPen(rp, pen1);
            RectFill(rp, 0, 0, 497, 347);
            getDeviceData(name);
            set(StartButton, MUIA_Disabled, FALSE);
            set(RefreshButton, MUIA_Disabled, FALSE);
        }
    }
    else
    {
        set(StartButton, MUIA_Disabled, TRUE);
        set(RefreshButton, MUIA_Disabled, TRUE);
        oldname = NULL;
    }
    
    AROS_USERFUNC_EXIT
}

BOOL GUIinit()
{
    BOOL retval = FALSE;

    app = ApplicationObject,
            MUIA_Application_Title,         (IPTR)APPNAME,
            MUIA_Application_Version,       (IPTR)VERSION,
            MUIA_Application_Copyright,     (IPTR)"© 2006, The AROS Development Team",
            MUIA_Application_Author,        (IPTR)"Michal Schulz",
            MUIA_Application_Base,          (IPTR)APPNAME,
            MUIA_Application_Description,   _(MSG_DESCRIPTION),

            SubWindow, MainWindow = WindowObject,
                MUIA_Window_Title,      _(MSG_DESCRIPTION),
//              MUIA_Window_Height,     MUIV_Window_Height_Visible(50),
//              MUIA_Window_Width,      MUIV_Window_Width_Visible(60),
                WindowContents, HGroup,
                    MUIA_Group_SameWidth, FALSE,
                    Child, VGroup, GroupFrameT(_(MSG_BITMAP)),
                        Child, Bmp = BitmapObject,
                            MUIA_FixWidth, 498,
                            MUIA_FixHeight, 348,
                            MUIA_Bitmap_Width, 498,
                            MUIA_Bitmap_Height, 348,
                        End, // ImageObject
                    End, // VGroup
                    Child, VGroup, 
                        Child, VGroup, GroupFrameT(_(MSG_SELECTION)),
                            Child, DevList = ListObject,
                                InputListFrame,
//                                MUIA_List_AdjustWidth, TRUE,
                            End, // ListObject
                        End, // VGroup
//                        Child, HVSpace,
                        Child, RefreshButton = MUI_MakeObject(MUIO_Button, _(MSG_REFRESH)),
                        Child, StartButton = MUI_MakeObject(MUIO_Button, _(MSG_START)),
                        Child, StopButton = MUI_MakeObject(MUIO_Button, _(MSG_CANCEL)),
                    End, // HGroup
                End, // WindowContents
            End, // MainWindow
        End; // ApplicationObject

    if (app)
    {   
        /* Quit application if the windowclosegadget or the esc key is pressed. */
        struct DosList *dl,*dll;
        
        dl = LockDosList(LDF_READ | LDF_DEVICES);
        dll = dl;
        while((dll = NextDosEntry(dll, LDF_DEVICES)))
        {
            struct TagItem tags[] = {
                { ASQ_VERSION,      0},
                { TAG_DONE,         0}
            };
        
            mp = dll->dol_Task;
            if(DoPkt(mp, ACTION_SFS_QUERY, (SIPTR)tags, 0, 0, 0, 0)!=DOSFALSE)
            {
                const char *name;
#ifdef AROS_DOS_PACKETS
                name = AROS_BSTR_ADDR(dll->dol_Name);
#else
                name = dll->dol_Ext.dol_AROS.dol_DevName;
#endif
                if(tags[0].ti_Data >= (1<<16) + 83)
                    DoMethod(DevList, MUIM_List_InsertSingle, name, MUIV_List_Insert_Bottom);
            }
        }
        UnLockDosList(LDF_READ | LDF_DEVICES);

        DoMethod(StartButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)app, 3, MUIM_CallHook, (IPTR)&start_hook);

        DoMethod(StopButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)app, 3, MUIM_CallHook, (IPTR)&stop_hook);

        DoMethod(RefreshButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)app, 3, MUIM_CallHook, (IPTR)&refresh_hook);

        DoMethod(DevList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, 
            (IPTR)DevList, 2, MUIM_CallHook, (IPTR)&select_hook);

        DoMethod(MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                 (IPTR)app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        set(StartButton, MUIA_Disabled, TRUE);
        set(StopButton, MUIA_Disabled, TRUE);
        set(RefreshButton, MUIA_Disabled, TRUE);
        retval=TRUE;
    }

    return retval;
}

void loop(void)
{
    ULONG sigs = 0;

    while((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
    {
        if (sigs)
        {
            sigs = Wait(sigs);
        }
    }
} /* loop(void)*/

int main(int argc, char *argv[])
{
    select_hook.h_Entry = (APTR)select_function;
    start_hook.h_Entry = (APTR)start_function;
    stop_hook.h_Entry = (APTR)stop_function;
    refresh_hook.h_Entry = (APTR)refresh_function;

    Locale_Initialize();

    if(openLibs())
    {
        if(GUIinit())
        {
            set(MainWindow, MUIA_Window_Open, TRUE);

            struct Window *window = (struct Window *)xget(MainWindow, MUIA_Window_Window);

            bm = AllocBitMap(500, 350, 
                 GetBitMapAttr(window->RPort->BitMap, BMA_DEPTH), BMF_CLEAR,
                 window->RPort->BitMap);
            rp = CreateRastPort();
            rp->BitMap = bm;
            dri = GetScreenDrawInfo(window->WScreen);
            pen1 = dri->dri_Pens[BACKGROUNDPEN];
            pen2 = dri->dri_Pens[TEXTPEN];
            pen3 = dri->dri_Pens[SHINEPEN];
            FreeScreenDrawInfo(window->WScreen, dri);
            
            SetAPen(rp, pen1);
            RectFill(rp, 0, 0, 499, 349);
            
            set(Bmp, MUIA_Bitmap_Bitmap, bm);
            DoMethod(Bmp, MUIM_Draw);

            if(xget(MainWindow, MUIA_Window_Open))
            {
                loop();
            }

            FreeRastPort(rp);
            FreeBitMap(bm);

            set(MainWindow, MUIA_Window_Open, FALSE);
            DisposeObject(app);

//          deinit_gui();
        }
        closeLibs();
    }
    cleanup(NULL);

    Locale_Deinitialize();

    return 0;
} /* main(int argc, char *argv[]) */
