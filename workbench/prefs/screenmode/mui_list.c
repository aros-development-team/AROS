#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 1

#include <libraries/mui.h>
#include <dos/dos.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/iffparse.h>

#include <prefs/screenmode.h>
#include <prefs/prefhdr.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>
#include <zune/systemprefswindow.h>

#include <intuition/iprefs.h>

#include <clib/alib_protos.h>

#include <string.h>

#define HOOK(name) \
struct Hook name ## Hook = { .h_Entry =  HookEntry, .h_SubEntry = name ## Func } 

#define HOOKFUNC(name) IPTR name ## Func(struct Hook *hook, APTR obj, APTR msg)

static HOOKFUNC(Select)
{
    struct IScreenModePrefs i;
    struct DimensionInfo dim;
    set(_app(obj), MUIA_Application_Iconified, TRUE);
    
    i.smp_DisplayID = *(ULONG *)msg;
    
    if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, i.smp_DisplayID))
    {
        i.smp_Width   = dim.MinRasterWidth;
        i.smp_Height  = dim.MinRasterHeight;
	i.smp_Depth   = dim.MaxDepth;
	i.smp_Control = 0;
    }
    
    SetIPrefs(&i, sizeof(struct IScreenModePrefs), IPREFS_TYPE_SCREENMODE);
    
    if (CloseWorkBench())
	OpenWorkBench();
    else
    {
        kprintf("Couldn't close the WB!\n");
    }
    
    set(_app(obj), MUIA_Application_Iconified, FALSE);
    
    return 0;
}

static const HOOK(Select);

struct ScreenModeSelector_DATA
{
    STRPTR *modes_array;
    ULONG  *ids_array;
};

#define MUIA_ScreenModeSelector_Active (TAG_USER | 1)

Object *ScreenModeSelector__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    STRPTR *modes_array;
    ULONG  *ids_array;
    ULONG   id, num_modes, cur_mode;
         
    struct DisplayInfo    DisplayInfo;
    struct DimensionInfo  DimensionInfo;
    struct NameInfo       NameInfo;
    APTR handle;
	 
    struct ScreenModeSelector_DATA *data;
	 
    num_modes = 0; id = INVALID_ID;
    while ((id = NextDisplayInfo(id)) != INVALID_ID) num_modes++;
	 
    modes_array = AllocVec(sizeof(STRPTR) * (num_modes + 1), MEMF_CLEAR);
    if (!modes_array)
        goto err;
	
    ids_array   = AllocVec(sizeof(ULONG) * (num_modes), MEMF_ANY);
    if (!ids_array)
        goto err;
	 
    cur_mode = 0;
    while ((id = NextDisplayInfo(id)) != INVALID_ID)
    {
        if ((id & MONITOR_ID_MASK) == DEFAULT_MONITOR_ID)
	    continue;
	    
        if (!(handle = FindDisplayInfo(id)))
	    continue;
	    
        if (!GetDisplayInfoData(handle, (UBYTE *)&NameInfo, sizeof(struct NameInfo), DTAG_NAME, 0))
	    continue;
        
        if (!GetDisplayInfoData(handle, (UBYTE *)&DisplayInfo, sizeof(struct DisplayInfo), DTAG_DISP, 0))
	    continue;
        
        if (!(DisplayInfo.PropertyFlags & DIPF_IS_WB) || DisplayInfo.NotAvailable)
	    continue;
        
	if (!GetDisplayInfoData(handle, (UBYTE *)&DimensionInfo, sizeof(struct DimensionInfo), DTAG_DIMS, 0))
	    continue;
        
        modes_array[cur_mode] = AllocVec(sizeof(NameInfo.Name), MEMF_ANY);
        if (!modes_array[cur_mode])
	   continue;
		 
        CopyMem(NameInfo.Name, modes_array[cur_mode], sizeof(NameInfo.Name));
	ids_array[cur_mode] = id;
	     
	cur_mode++;
    }

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Cycle_Entries, (IPTR)modes_array,
        TAG_MORE,           (IPTR)message->ops_AttrList
    );

    if (!self)
        goto err;
	
    DoMethod(self, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             (IPTR)self, 3, MUIM_Set, MUIA_ScreenModeSelector_Active, MUIV_TriggerValue);
 
    data = INST_DATA(CLASS, self);    
    data->modes_array = modes_array;
    data->ids_array   = ids_array;
	 
    return self;
    
err:
    CoerceMethod(CLASS, self, OM_DISPOSE);
    return NULL;
}

IPTR ScreenModeSelector__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct ScreenModeSelector_DATA *data;
    ULONG cur_mode;
	 
    data = INST_DATA(CLASS, self);    

    if (data->modes_array)
    {
        for (cur_mode = 0; data->modes_array[cur_mode]; cur_mode++)
	    FreeVec(data->modes_array[cur_mode]);
		 
	FreeVec(data->modes_array);
    }
    
    FreeVec(data->ids_array);
	 
    return DoSuperMethodA(CLASS, self, message);
}


IPTR ScreenModeSelector__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeSelector_DATA *data = INST_DATA(CLASS, self);    
    struct TagItem *tags, *tag;
    struct TagItem noforward_tags[] =
    {
        {MUIA_Group_Forward , FALSE                       },
        {TAG_MORE           , (IPTR)message->ops_AttrList }
    };
    struct opSet noforward_message = *message;
    noforward_message.ops_AttrList = noforward_tags;
    
    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
	    case MUIA_ScreenModeSelector_Active:
	    {
	        ULONG cycle_active = XGET(self, MUIA_Cycle_Active);
		if (cycle_active != tag->ti_Data)
		{
		    NFSET(self, MUIA_Cycle_Active, tag->ti_Data);
		    cycle_active = XGET(self, MUIA_Cycle_Active);
		}
		
		tag->ti_Data = data->ids_array[cycle_active];
		break;		
	    }
	}
    }		    

    return DoSuperMethodA(CLASS, self, (Msg)&noforward_message);	
}

IPTR ScreenModeSelector__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    struct ScreenModeSelector_DATA *data = INST_DATA(CLASS, self);    
    
    switch (message->opg_AttrID)
    {
        case MUIA_ScreenModeSelector_Active:
	    *message->opg_Storage = data->ids_array[XGET(self, MUIA_Cycle_Active)];
            break;
	default:
            return DoSuperMethodA(CLASS, self, (Msg)message);
    }
    
    return TRUE;
}


ZUNE_CUSTOMCLASS_4
(
    ScreenModeSelector, NULL, MUIC_Cycle, NULL,   
    OM_NEW,     struct opSet *,
    OM_DISPOSE, Msg,
    OM_GET,     struct opGet *,
    OM_SET,     struct opSet *
);
#define ScreenModeSelectorObject BOOPSIOBJMACRO_START(ScreenModeSelector_CLASS->mcc_Class)

#define MUIA_ScreenModeProperties_DisplayID (TAG_USER | 1)
#define MUIA_ScreenModeProperties_Width     (TAG_USER | 2)
#define MUIA_ScreenModeProperties_Height    (TAG_USER | 3)
#define MUIA_ScreenModeProperties_Depth     (TAG_USER | 4)

struct ScreenModeProperties_DATA
{
    Object *width, *height, *depth;
    ULONG DisplayID;
};

#define HLeft(obj...) \
    (IPTR)(HGroup, (IPTR)GroupSpacing(0), Child, (IPTR)(obj), Child, (IPTR)HSpace(0), End)

Object *ScreenModeProperties__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data;	 
    Object *width, *height, *depth;     
    ULONG id;
    
    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        Child, (IPTR)HGroup,
	    Child, (IPTR)GroupObject,
	        MUIA_Group_Columns, 2,
	        Child, (IPTR)Label("\33lWidth:"),
	        Child, HLeft(width = NumericbuttonObject, End),
	        Child, (IPTR)Label("\33lHeight:"),
	        Child, HLeft(height = NumericbuttonObject, End),
	        Child, (IPTR)Label("\33lDepth:"),
	        Child, HLeft(depth = NumericbuttonObject, End),
		MUIA_Weight, 50,
	    End,  
	End,
	
        TAG_MORE, (IPTR)message->ops_AttrList
    );
    
    if (!self)
        goto err;
    
    data = INST_DATA(CLASS, self);    
    data->width  = width;
    data->height = height;
    data->depth  = depth;
    
    DoMethod
    (
        width, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	(IPTR)self, 3,
	MUIM_Set, MUIA_ScreenModeProperties_Width, MUIV_TriggerValue
    );
    
    DoMethod
    (
        height, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	(IPTR)self, 3,
	MUIM_Set, MUIA_ScreenModeProperties_Height, MUIV_TriggerValue
    );
    
    DoMethod
    (
        depth, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	(IPTR)self, 3,
	MUIM_Set, MUIA_ScreenModeProperties_Depth, MUIV_TriggerValue
    );
    
    id = GetTagData(MUIA_ScreenModeProperties_DisplayID, INVALID_ID, message->ops_AttrList); 
    set(self, MUIA_ScreenModeProperties_DisplayID, id);
    
    return self;

err:
    CoerceMethod(CLASS, self, OM_DISPOSE);
    return NULL;
}

IPTR ScreenModeProperties__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeProperties_DATA *data = INST_DATA(CLASS, self);    
    struct TagItem *tags, *tag;
    struct TagItem noforward_tags[] =
    {
        {MUIA_Group_Forward , FALSE                       },
        {TAG_MORE           , (IPTR)message->ops_AttrList }
    };
    struct opSet noforward_message = *message;
    noforward_message.ops_AttrList = noforward_tags;
    
    ULONG id        = INVALID_ID;
    IPTR  no_notify = TAG_IGNORE;
    
    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
	    case MUIA_NoNotify:
	        no_notify = MUIA_NoNotify;
		break;
		
	    case MUIA_ScreenModeProperties_DisplayID:
	    {
	        struct TagItem width_tags[] =
		{
		    { no_notify,            TRUE },
		    { MUIA_Numeric_Min,        0 },
		    { MUIA_Numeric_Max,        0 },
		    { MUIA_Numeric_Default,    0 },
		    { TAG_DONE,                0 }
		};
	        struct TagItem height_tags[] =
		{
		    { no_notify,            TRUE },
		    { MUIA_Numeric_Min,        0 },
		    { MUIA_Numeric_Max,        0 },
		    { MUIA_Numeric_Default,    0 },
		    { TAG_DONE,                0 }
		};
	        struct TagItem depth_tags[] =
		{
		    { no_notify,            TRUE },
		    { MUIA_Numeric_Min,        0 },
		    { MUIA_Numeric_Max,        0 },
		    { MUIA_Numeric_Default,    0 },
		    { TAG_DONE,                0 }
		};
	        
                struct DimensionInfo dim;
		
                if (GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, tag->ti_Data))
                {
                    width_tags[1].ti_Data  = dim.MinRasterWidth;
                    height_tags[1].ti_Data = dim.MinRasterHeight;
                    depth_tags[1].ti_Data  = 1;
	    
                    width_tags[2].ti_Data  = dim.MaxRasterWidth;
                    height_tags[2].ti_Data = dim.MaxRasterHeight;
	            depth_tags[2].ti_Data  = dim.MaxDepth;
	    
                    width_tags[3].ti_Data  = dim.Nominal.MaxX - dim.Nominal.MinX + 1;
                    height_tags[3].ti_Data = dim.Nominal.MaxY - dim.Nominal.MinY + 1;
	            depth_tags[3].ti_Data  = 1;
                    
		    id = tag->ti_Data;
		}
		    
  	        data->DisplayID = id;
 	        nfset(self, MUIA_Disabled, id == INVALID_ID);
		
		SetAttrsA(data->width,  width_tags);
		SetAttrsA(data->height, height_tags);
		SetAttrsA(data->depth,  depth_tags);		
                
		break;
	    }
	    
	    case MUIA_ScreenModeProperties_Width:
	        if (id != INVALID_ID)
		{
		    WORD width = tag->ti_Data;
		    if (width != -1)
		        SetAttrs(data->width, no_notify, TRUE, MUIA_Numeric_Value, width, TAG_DONE);
		    else
		        DoMethod(data->width, MUIM_Numeric_SetDefault);
		}
		break;
		
	    case MUIA_ScreenModeProperties_Height:
	        if (id != INVALID_ID)
		{
		    WORD height = tag->ti_Data;
		    if (height != -1)
		        SetAttrs(data->height, no_notify, TRUE, MUIA_Numeric_Value, height, TAG_DONE);
		    else
		        DoMethod(data->height, MUIM_Numeric_SetDefault);
		}
		break;
	    
	    case MUIA_ScreenModeProperties_Depth:
	        if (id != INVALID_ID)
		{
		    WORD depth = tag->ti_Data;
		    if (depth != -1)
		        SetAttrs(data->depth, no_notify, TRUE, MUIA_Numeric_Value, depth, TAG_DONE);
		    else
		        DoMethod(data->depth, MUIM_Numeric_SetDefault);
		}
		break;
	}
    }		    

    return DoSuperMethodA(CLASS, self, (Msg)&noforward_message);	
}

IPTR ScreenModeProperties__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    struct ScreenModeProperties_DATA *data = INST_DATA(CLASS, self);    
    
    switch (message->opg_AttrID)
    {
        case MUIA_ScreenModeProperties_DisplayID:
	    *message->opg_Storage = data->DisplayID;
            break;
        
	case MUIA_ScreenModeProperties_Width:
	    *message->opg_Storage = XGET(data->width, MUIA_Numeric_Value);
            break;
        
	case MUIA_ScreenModeProperties_Height:
	    *message->opg_Storage = XGET(data->height, MUIA_Numeric_Value);
            break;
        
	case MUIA_ScreenModeProperties_Depth:
	    *message->opg_Storage = XGET(data->depth, MUIA_Numeric_Value);
            break;
	
	default:
            return DoSuperMethodA(CLASS, self, (Msg)message);
    }
    
    return TRUE;
}

ZUNE_CUSTOMCLASS_3
(
    ScreenModeProperties, NULL, MUIC_Group, NULL,   
    OM_NEW,     struct opSet *,
    /*
    OM_DISPOSE, Msg, */
    OM_GET,     struct opGet *,
    OM_SET,     struct opSet *
);

#define ScreenModePropertiesObject BOOPSIOBJMACRO_START(ScreenModeProperties_CLASS->mcc_Class)

struct SMEditor_DATA
{
    Object *selector, *properties;
};

#define SMEditorObject BOOPSIOBJMACRO_START(SMEditor_CLASS->mcc_Class)
#define SETUP_INST_DATA struct SMEditor_DATA *data = INST_DATA(CLASS, self)

Object *SMEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *selector, *properties;
    kprintf("--(1)--\n");
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name, (IPTR)"ScreenMode",
        MUIA_PrefsEditor_Path, (IPTR)"SYS/ScreenMode.prefs",
        
	Child, (IPTR)VGroup,
	    Child, (IPTR)(selector   = ScreenModeSelectorObject, End),
	    Child, (IPTR)(properties = ScreenModePropertiesObject, GroupFrame, End),
	End,
        
        TAG_DONE
    );
    
    kprintf("--(2)--\n");
    if (self)
    {
        SETUP_INST_DATA;
        
	data->selector   = selector;
        data->properties = properties;
	     
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
	(
	    selector, MUIM_Notify, MUIA_ScreenModeSelector_Active, MUIV_EveryTime,
	    (IPTR)properties, 3,
	    MUIM_Set, MUIA_ScreenModeProperties_DisplayID, MUIV_TriggerValue
	);
	
        DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_DisplayID, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
    
        DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_Width, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
        
	DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_Height, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
        
	DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_Depth, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
    }
    
    kprintf("--(3)--\n");
    return self;
}

#define ShowError(x) (void)0

void SMPByteSwap(struct ScreenModePrefs *smp)
{
#if 0
    #undef  SWAP
    #define SWAP(type, field) smp->smp_ ## field = AROS_BE2 ## type(smp->smp_ ## field)
    
    int i;
    for (i = 0; i < sizeof(smp->smp_Reserved)/sizeof(ULONG); i++)
        SWAP(LONG, Reserved[i]);
	
        SWAP(LONG, DisplayID);
	SWAP(WORD, Width);
	SWAP(WORD, Height);
	SWAP(WORD, Depth);
	SWAP(WORD, Control);
#endif
}

IPTR SMEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    struct ScreenModePrefs  smp;
    BOOL                    success = TRUE;
    LONG                    error;
    
    if (!(handle = AllocIFF()))
    {
        ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        return(FALSE);
    }
    
    handle->iff_Stream = (IPTR) message->fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
	
        BYTE i;
        
        // FIXME: We want some sanity checking here!
        for (i = 0; i < 1; i++)
        {
            if ((error = StopChunk(handle, ID_PREF, ID_SCRM)) == 0)
            {
                if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
                {
                    context = CurrentChunk(handle);
                    
                    error = ReadChunkBytes
                    (
                        handle, &smp, sizeof(struct ScreenModePrefs)
                    );
                    
                    if (error < 0)
                    {
                        printf("Error: ReadChunkBytes() returned %ld!\n", error);
                    }                    
                }
                else
                {
                    printf("ParseIFF() failed, returncode %ld!\n", error);
                    success = FALSE;
                    break;
                }
            }
            else
            {
                printf("StopChunk() failed, returncode %ld!\n", error);
                success = FALSE;
            }
        }

        CloseIFF(handle);
    }
    else
    {
        ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);
    
    
    if (success)
    {
        SMPByteSwap(&smp);
	SetAttrs
	(
	    data->properties,
	    MUIA_NoNotify,                       TRUE,
	    MUIA_ScreenModeProperties_DisplayID, smp.smp_DisplayID,
	    MUIA_ScreenModeProperties_Width,     smp.smp_Width,
	    MUIA_ScreenModeProperties_Height,    smp.smp_Height,
	    MUIA_ScreenModeProperties_Depth,     smp.smp_Depth,
	    TAG_DONE
        );
    }
    
    return success;
}

IPTR SMEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    
    struct PrefHeader header = { 0 }; 
    struct IFFHandle *handle;
    BOOL              success = TRUE;
    LONG              error   = 0;
        
    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR) message->fh;
        
        InitIFFasDOS(handle);
        
        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            struct ScreenModePrefs smp;
	    
	    memset(&smp, 0, sizeof(smp));
            
	    BYTE i;
            
            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            header.ph_Version = PHV_CURRENT;
            header.ph_Type    = 0;
            
            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
            
            PopChunk(handle);
            
            for (i = 0; i < 1; i++)
            {
                error = PushChunk(handle, ID_PREF, ID_SCRM, sizeof(struct ScreenModePrefs));
                
                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PushChunk() = %d ", error);
                }
                
	        smp.smp_DisplayID = XGET(data->properties, MUIA_ScreenModeProperties_DisplayID);
                smp.smp_Width     = XGET(data->properties, MUIA_ScreenModeProperties_Width);
                smp.smp_Height    = XGET(data->properties, MUIA_ScreenModeProperties_Height);
                smp.smp_Depth     = XGET(data->properties, MUIA_ScreenModeProperties_Depth);
                smp.smp_Control   = 0;
                
		SMPByteSwap(&smp);
			
                error = WriteChunkBytes(handle, &smp, sizeof(struct ScreenModePrefs));
                error = PopChunk(handle);
                                
                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PopChunk() = %d ", error);
                }
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            ShowError(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }
        
        CloseIFF(handle);
	FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}

ZUNE_CUSTOMCLASS_3
(
    SMEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *
);


int main()
{
    Object *app, *win;
    
    kprintf("--- 1 ---\n");
    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"Test List Class",

        SubWindow, (IPTR)(win = SystemPrefsWindowObject,
            WindowContents, (IPTR) SMEditorObject,
            End,
	End),
    End;

    kprintf("--- 2 ---\n");
    if (!app)
        return RETURN_ERROR;        
    
    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	     
    set(win, MUIA_Window_Open,TRUE);
    
    {
        ULONG sigs = 0;

        while (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if (sigs & SIGBREAKF_CTRL_C) break;
            }
        }
    }

    kprintf("--- 3 ---\n");
	
    MUI_DisposeObject(app);
    
    return RETURN_OK;
}


