/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include "psi.h"

#define _WBenchMsg WBenchMsg

#define BETWEEN(a,x,b) ((x)>=(a) && (x)<=(b))


/* $setver$ */
static const char VersionString[] = "$VER: PSI 20.0 (14.04.2009)";
#define VERSION  19
#define REVISION 8

/****************************************************************************/
/* Global Vars                                                              */
/****************************************************************************/

LONG __stack = 10000;

struct Library *MUIScreenBase;
struct Library *MUIMasterBase;

struct Catalog *Catalog;

struct MUI_CustomClass *CL_EditWindow  ;
struct MUI_CustomClass *CL_EditPanel   ;
struct MUI_CustomClass *CL_SysPenField ;
struct MUI_CustomClass *CL_DispIDlist  ;
struct MUI_CustomClass *CL_DispIDinfo  ;
struct MUI_CustomClass *CL_ScreenList  ;
struct MUI_CustomClass *CL_ScreenPanel ;
struct MUI_CustomClass *CL_MainWindow  ;

#define TG CHECKIT|MENUTOGGLE

enum { MEN_OPEN=1,MEN_APPEND,MEN_SAVEAS,MEN_ABOUT,MEN_QUIT,MEN_LASTSAVED,MEN_RESTORE,MEN_MUI };
struct NewMenu MainMenu[] =
{
	{ NM_TITLE, (STRPTR)MSG_MENU_PROJECT          , 0 ,0,0,(APTR)0             },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_OPEN     ,"O",0,0,(APTR)MEN_OPEN      },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_APPEND   ,"P",0,0,(APTR)MEN_APPEND    },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_SAVEAS   ,"A",0,0,(APTR)MEN_SAVEAS    },
	{ NM_ITEM , (STRPTR)NM_BARLABEL               , 0 ,0,0,(APTR)0             },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_ABOUT    ,"?",0,0,(APTR)MEN_ABOUT     },
	{ NM_ITEM , (STRPTR)NM_BARLABEL               , 0 ,0,0,(APTR)0             },
	{ NM_ITEM , (STRPTR)MSG_MENU_PROJECT_QUIT     ,"Q",0,0,(APTR)MEN_QUIT      },

	{ NM_TITLE, (STRPTR)MSG_MENU_EDIT             , 0 ,0,0,(APTR)0             },
	{ NM_ITEM , (STRPTR)MSG_MENU_EDIT_LASTSAVED   ,"L",0,0,(APTR)MEN_LASTSAVED },
	{ NM_ITEM , (STRPTR)MSG_MENU_EDIT_RESTORE     ,"R",0,0,(APTR)MEN_RESTORE   },

	{ NM_TITLE, (STRPTR)MSG_MENU_SETTINGS         , 0 ,0,0,(APTR)0             },
	{ NM_ITEM , (STRPTR)MSG_MENU_SETTINGS_MUI     , 0 ,0,0,(APTR)MEN_MUI       },

	{ NM_END,NULL,0,0,0,(APTR)0 },
};

struct NewMenu PaletteMenu[] =
{
	{ NM_TITLE, (STRPTR)MSG_PALPRES_TITLE, 0 , 0,0,(APTR)0   },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_CLONEWB  , 0,0,0,(APTR)10},
	{ NM_ITEM , (STRPTR)NM_BARLABEL          , 0,0,0,(APTR)0 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_MAGICWB  , 0,0,0,(APTR)9 },
	{ NM_ITEM , (STRPTR)NM_BARLABEL          , 0,0,0,(APTR)0 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_STANDARD , 0,0,0,(APTR)0 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_TINT     , 0,0,0,(APTR)1 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_PHARAO   , 0,0,0,(APTR)2 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_SUNSET   , 0,0,0,(APTR)3 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_OCEAN    , 0,0,0,(APTR)4 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_STEEL    , 0,0,0,(APTR)5 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_CHOCOLATE, 0,0,0,(APTR)6 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_PEWTER   , 0,0,0,(APTR)7 },
	{ NM_ITEM , (STRPTR)MSG_PALPRES_WINE     , 0,0,0,(APTR)8 },

	{ NM_END,NULL,0,0,0,(APTR)0 },
};

char *CYA_EditPages[] =
{
	(char *)MSG_EDITPAGE_ATTRIBUTES,
	(char *)MSG_EDITPAGE_DISPLAY,
	(char *)MSG_EDITPAGE_COLORS,
	NULL
};

char *CYA_Overscan[] =
{
	(char *)MSG_OVERSCAN_TEXT,
	(char *)MSG_OVERSCAN_GRAPHICS,
	(char *)MSG_OVERSCAN_EXTREME,
	(char *)MSG_OVERSCAN_MAXIMUM,
	NULL
};


/****************************************************************************/
/* Locale Stuff                                                             */
/****************************************************************************/

char *GetStr(int num)
{
	struct CatCompArrayType *cca = (struct CatCompArrayType *)CatCompArray;
	while (cca->cca_ID != num) cca++;
	if (LocaleBase) return(GetCatalogStr(Catalog,num,cca->cca_Str));
	return((char *)cca->cca_Str);
}

VOID LocalizeStringArray(char **array)
{
	char **x;
	for (x=array;*x;x++)
		*x = GetStr((int)*x);
}

VOID LocalizeNewMenu(struct NewMenu *nm)
{
	for (;nm->nm_Type!=NM_END;nm++)
		if (nm->nm_Label != NM_BARLABEL)
			nm->nm_Label = GetStr((int)nm->nm_Label);
}

VOID InitLocale(VOID)
{
	if (LocaleBase = OpenLibrary("locale.library",38))
		Catalog = OpenCatalog(NULL,"psi.catalog",OC_Version,VERSION,TAG_DONE);

	LocalizeNewMenu(MainMenu);
	LocalizeNewMenu(PaletteMenu);
	/*LocalizeNewMenu(EditMenu);*/
	LocalizeStringArray(CYA_Overscan);
	LocalizeStringArray(CYA_EditPages);
}

VOID ExitLocale(VOID)
{
	if (Catalog) { CloseCatalog(Catalog); Catalog=NULL; }
	if (LocaleBase) CloseLibrary(LocaleBase); { LocaleBase=NULL; }
}



/****************************************************************************/
/* Misc Help Functions                                                      */
/****************************************************************************/

IPTR xget(Object *obj,IPTR attribute)
{
	IPTR x;
	get(obj,attribute,&x);
	return(x);
}

char *getstr(Object *obj)
{
	return((char *)xget(obj,MUIA_String_Contents));
}

BOOL getbool(Object *obj)
{
	return((BOOL)xget(obj,MUIA_Selected));
}

VOID setstr(APTR str,LONG num)
{
	DoMethod(str,MUIM_SetAsString,MUIA_String_Contents,"%ld",num);
}

VOID settxt(APTR str,LONG num)
{
	DoMethod(str,MUIM_SetAsString,MUIA_Text_Contents,"%ld",num);
}

Object *MakeCheck(int num)
{
	Object *obj = MUI_MakeObject(MUIO_Checkmark,GetStr(num));
	if (obj) set(obj,MUIA_CycleChain,1);
	return(obj);
}

Object *MakeButton(int num)
{
	Object *obj = MUI_MakeObject(MUIO_Button,GetStr(num));
	if (obj) set(obj,MUIA_CycleChain,1);
	return(obj);
}

/*
Object *MakeBackground(int num)
{
	Object *obj;

	obj = MUI_NewObject(MUIC_Popimage,
		MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
		MUIA_Window_Title, "Adjust Screen Background",
		MUIA_CycleChain, 1,
		TAG_DONE);

	return(obj);
}
*/

Object *MakeString(int maxlen,int num)
{
	Object *obj = MUI_MakeObject(MUIO_String,GetStr(num),maxlen);

	if (obj)
	{
		SetAttrs(obj,
			MUIA_CycleChain,1,
			MUIA_String_AdvanceOnCR,TRUE,
			TAG_DONE);
	}

	return(obj);
}

Object *MakeCycle(char **array,int num)
{
	Object *obj = MUI_MakeObject(MUIO_Cycle ,GetStr(num),array);
	if (obj) set(obj,MUIA_CycleChain,1);
	return(obj);
}

Object *MakeSlider(int min,int max,int num)
{
	Object *obj = MUI_MakeObject(MUIO_Slider,GetStr(num),min,max);
	if (obj) set(obj,MUIA_CycleChain,1);
	return(obj);
}


static Object *MakeCLabel(int num) { return(MUI_MakeObject(MUIO_Label,GetStr(num),MUIO_Label_Centered)); }

Object *MakeLabel    (int num)  { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),0)); }
Object *MakeLabel1   (int num)  { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),MUIO_Label_SingleFrame)); }
Object *MakeLabel2   (int num)  { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),MUIO_Label_DoubleFrame)); }
Object *MakeLLabel   (int num)  { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),MUIO_Label_LeftAligned)); }
Object *MakeLLabel1  (int num)  { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),MUIO_Label_LeftAligned|MUIO_Label_SingleFrame)); }
Object *MakeFreeLabel(int num)  { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),MUIO_Label_FreeVert)); }
Object *MakeFreeLLabel(int num) { return(MUI_MakeObject(MUIO_Label    ,GetStr(num),MUIO_Label_FreeVert|MUIO_Label_LeftAligned)); }

BOOL TestPubScreen(char *name)
{
	struct List *pslist;
	struct PubScreenNode *psn,*succ;
	BOOL res = FALSE;
	pslist = LockPubScreenList();
	ForEntries(pslist,psn,succ)
	{
		if (!stricmp(psn->psn_Node.ln_Name,name))
		{
			res = TRUE;
			break;
		}
	}
	UnlockPubScreenList();
	return(res);
}


/****************************************************************************/
/* DisplayIDinfo class                                                      */
/****************************************************************************/

struct DispIDinfo_Data
{
	Object *TX_Visible[2];
	Object *TX_Minimum[2];
	Object *TX_Maximum[2];
	Object *TX_BitsPerGun;
	Object *TX_NumColors;
	Object *TX_ScanRate;
	Object *TX_ScanLine;
};

Object *MakeMoni(LONG w,LONG h,LONG d,const UBYTE *body,const ULONG *colors)
{
	Object *obj = BodychunkObject,
		MUIA_FixWidth             , w,
		MUIA_FixHeight            , h,
		MUIA_Bitmap_Width         , w,
		MUIA_Bitmap_Height        , h,
		MUIA_Bodychunk_Depth      , d,
		MUIA_Bodychunk_Body       , (UBYTE *)body,
		MUIA_Bodychunk_Compression, PSI_COLORS_COMPRESSION,
		MUIA_Bodychunk_Masking    , PSI_COLORS_MASKING,
		MUIA_Bitmap_SourceColors  , (ULONG *)colors,
		MUIA_Bitmap_Transparent   , 0,
		End;
	return(obj);
}

Object *MakeSize(void)
{
	Object *obj = TextObject,
		MUIA_Text_Contents, "0",
		MUIA_Text_PreParse, "\33r",
		MUIA_FixWidthTxt, "00000",
		End;
	return(obj);
}

ULONG DispIDinfo_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	Object *TX_Visible[2];
	Object *TX_Minimum[2];
	Object *TX_Maximum[2];
	Object *TX_BitsPerGun;
	Object *TX_NumColors;
	Object *TX_ScanRate;
	Object *TX_ScanLine;
	Object *g1,*g2,*g3;

	obj = (Object *)DoSuperNewTags(cl,obj,NULL,
		MUIA_Group_Columns, 2,
		Child, g1 = MakeMoni(PSI_SIZES_WIDTH,PSI_SIZES_HEIGHT,PSI_SIZES_DEPTH,psi_sizes_body,psi_sizes_colors),
		Child, ColGroup(4), MUIA_Group_VertSpacing, 0, MUIA_Group_HorizSpacing, 4,
			Child, MakeLabel(MSG_LABEL_VISIBLE), Child, TX_Visible[0]=MakeSize(), Child, Label("x"), Child, TX_Visible[1]=MakeSize(),
			Child, MakeLabel(MSG_LABEL_MINIMUM), Child, TX_Minimum[0]=MakeSize(), Child, Label("x"), Child, TX_Minimum[1]=MakeSize(),
			Child, MakeLabel(MSG_LABEL_MAXIMUM), Child, TX_Maximum[0]=MakeSize(), Child, Label("x"), Child, TX_Maximum[1]=MakeSize(),
			End,
		Child, g2 = MakeMoni(PSI_COLORS_WIDTH,PSI_COLORS_HEIGHT,PSI_COLORS_DEPTH,psi_colors_body,psi_sizes_colors),
		Child, ColGroup(2), MUIA_Group_VertSpacing, 0, MUIA_Group_HorizSpacing, 4,
			Child, MakeLabel(MSG_LABEL_BITSPERGUN), Child, TX_BitsPerGun = TextObject, End,
			Child, MakeLabel(MSG_LABEL_MAXIMUM   ), Child, TX_NumColors  = TextObject, End,
			End,
		Child, g3 = MakeMoni(PSI_FREQS_WIDTH,PSI_FREQS_HEIGHT,PSI_FREQS_DEPTH,psi_freqs_body,psi_sizes_colors),
		Child, ColGroup(2), MUIA_Group_VertSpacing, 0, MUIA_Group_HorizSpacing, 4,
			Child, MakeLabel(MSG_LABEL_SCANRATE), Child, TX_ScanRate = TextObject, End,
			Child, MakeLabel(MSG_LABEL_SCANLINE), Child, TX_ScanLine = TextObject, End,
			End,
		TAG_MORE,msg->ops_AttrList);

	if (obj)
	{
		struct DispIDinfo_Data *data = INST_DATA(cl,obj);

/*
		set(g1,MUIA_VertDisappear,3);
		set(g2,MUIA_VertDisappear,2);
		set(g3,MUIA_VertDisappear,1);
*/

		data->TX_Visible[0] = TX_Visible[0];
		data->TX_Visible[1] = TX_Visible[1];
		data->TX_Minimum[0] = TX_Minimum[0];
		data->TX_Minimum[1] = TX_Minimum[1];
		data->TX_Maximum[0] = TX_Maximum[0];
		data->TX_Maximum[1] = TX_Maximum[1];
		data->TX_BitsPerGun = TX_BitsPerGun;
		data->TX_NumColors  = TX_NumColors;
		data->TX_ScanRate   = TX_ScanRate;
		data->TX_ScanLine   = TX_ScanLine;

		return((ULONG)obj);
	}

	return(0);
}

ULONG DispIDinfo_Set(struct IClass *cl,Object *obj,struct opSet *msg)
{
	#define offset(a,b) (((ULONG)b)-(((ULONG)a)))
	struct DispIDinfo_Data *data = INST_DATA(cl,obj);
	struct TagItem *tag;

	if (tag = FindTagItem(MUIA_DispIDinfo_ID,msg->ops_AttrList))
	{
		struct DisplayInfo   dis;
		struct DimensionInfo dim;
		struct MonitorInfo   mon;
		int dislen;
		int dimlen;
		int monlen;

		set(data->TX_Visible[0],MUIA_String_Contents,"");
		set(data->TX_Visible[1],MUIA_String_Contents,"");
		set(data->TX_Minimum[0],MUIA_String_Contents,"");
		set(data->TX_Minimum[1],MUIA_String_Contents,"");
		set(data->TX_Maximum[0],MUIA_String_Contents,"");
		set(data->TX_Maximum[1],MUIA_String_Contents,"");
		set(data->TX_BitsPerGun,MUIA_String_Contents,"");
		set(data->TX_NumColors ,MUIA_String_Contents,"");
		set(data->TX_ScanRate  ,MUIA_String_Contents,"");
		set(data->TX_ScanLine  ,MUIA_String_Contents,"");

		dislen = GetDisplayInfoData(0,(char *)&dis,sizeof(struct DisplayInfo  ),DTAG_DISP,tag->ti_Data);
		dimlen = GetDisplayInfoData(0,(char *)&dim,sizeof(struct DimensionInfo),DTAG_DIMS,tag->ti_Data);
		monlen = GetDisplayInfoData(0,(char *)&mon,sizeof(struct MonitorInfo  ),DTAG_MNTR,tag->ti_Data);

		if (dimlen>offset(&dim,&dim.MaxOScan))
		{
			settxt(data->TX_Visible[0],RectangleWidth (dim.MaxOScan));
			settxt(data->TX_Visible[1],RectangleHeight(dim.MaxOScan));
			settxt(data->TX_Minimum[0],dim.MinRasterWidth );
			settxt(data->TX_Minimum[1],dim.MinRasterHeight);
			settxt(data->TX_Maximum[0],dim.MaxRasterWidth );
			settxt(data->TX_Maximum[1],dim.MaxRasterHeight);
			settxt(data->TX_NumColors,1<<dim.MaxDepth);
		}

		if (dislen>offset(&dis,&dis.BlueBits))
		{
			DoMethod(data->TX_BitsPerGun,MUIM_SetAsString,MUIA_Text_Contents,"%ld x %ld x %ld",dis.RedBits,dis.GreenBits,dis.BlueBits);
		}

		if (monlen>offset(&mon,&mon.TotalColorClocks))
		{
			/* These calculations were taken from ScreenManager by Bernhard "ZZA" Moellemann. Thanks! */

			if (mon.TotalRows)
			{
				ULONG vfreqint=1000000000L/((ULONG)mon.TotalColorClocks*280*mon.TotalRows/1000)+5;
				DoMethod(data->TX_ScanRate,MUIM_SetAsString,MUIA_Text_Contents,"%ld.%02ld Hz",vfreqint/1000,(vfreqint-(vfreqint/1000)*1000)/10);
			}
			if (mon.TotalColorClocks)
			{
				ULONG hfreq=1000000000L/((ULONG)mon.TotalColorClocks*280)+5;
				ULONG hfreqint=hfreq/1000;
				DoMethod(data->TX_ScanLine,MUIM_SetAsString,MUIA_Text_Contents,"%ld.%02ld kHz",hfreqint,(hfreq-hfreqint*1000)/10);
			}
		}
	}

	return(DoSuperMethodA(cl,obj,msg));
}

BOOPSI_DISPATCHER(IPTR, DispIDinfo_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW: return(DispIDinfo_New(cl,obj,(APTR)msg));
		case OM_SET: return(DispIDinfo_Set(cl,obj,(APTR)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



/****************************************************************************/
/* DisplayIDlist class                                                      */
/****************************************************************************/

struct DispIDlist_Data
{
	ULONG CurrentID;
	struct Hook ConstructHook;
	struct Hook DisplayHook;
};

IPTR DispIDlist_DisplayFunc(struct Hook *hook, char **array, struct NameInfo *ni)
{
	*array = ni->Name;
	return(0);
}

IPTR DispIDlist_CompareFunc(struct Hook *hook, struct NameInfo *n1, struct NameInfo *n2)
{
	return(stricmp(n1->Name,n2->Name));
}

VOID DispIDlist_DestructFunc(struct Hook *hook, APTR pool, struct NameInfo *ni)
{
	FreeVec(ni);
}

IPTR DispIDlist_ConstructFunc(struct Hook *hook, APTR pool, ULONG modeid)
{
	APTR handle;
	struct NameInfo NameInfo;
	struct DisplayInfo DisplayInfo;
	struct DimensionInfo DimensionInfo;
	struct NameInfo *ni;

	if ((modeid & MONITOR_ID_MASK)==DEFAULT_MONITOR_ID) return(NULL);
	if (!(handle = FindDisplayInfo(modeid))) return(NULL);
	if (!GetDisplayInfoData(handle,(char *)&NameInfo     ,sizeof(struct NameInfo     ),DTAG_NAME,0)) return(NULL);
	if (!GetDisplayInfoData(handle,(char *)&DisplayInfo  ,sizeof(struct DisplayInfo  ),DTAG_DISP,0)) return(NULL);
	if (!GetDisplayInfoData(handle,(char *)&DimensionInfo,sizeof(struct DimensionInfo),DTAG_DIMS,0)) return(NULL);
	if (!(DisplayInfo.PropertyFlags & DIPF_IS_WB)) return(NULL);
	if (DisplayInfo.NotAvailable) return(NULL);
	if (!(ni = AllocVec(sizeof(struct NameInfo),MEMF_ANY))) return(NULL);

	*ni = NameInfo;
	return(ni);
}

ULONG DispIDlist_New(struct IClass *cl,Object *obj,Msg msg)
{
	static struct Hook ConstructHook;
	ConstructHook.h_Entry = HookEntry;
	ConstructHook.h_SubEntry = DispIDlist_ConstructFunc;
	static struct Hook DestructHook;
	DestructHook.h_Entry = HookEntry;
	DestructHook.h_SubEntry = DispIDlist_DestructFunc;
	static struct Hook CompareHook;
	CompareHook.h_Entry = HookEntry;
	CompareHook.h_SubEntry = DispIDlist_CompareFunc;
	static struct Hook DisplayHook;
	DisplayHook.h_Entry = HookEntry;
	DisplayHook.h_SubEntry = DispIDlist_DisplayFunc;
	LONG id = INVALID_ID;

	if (!(obj=(Object *)DoSuperMethodA(cl,obj,msg)))
		return(0);

	SetSuperAttrs(cl,obj,
		MUIA_List_ConstructHook, &ConstructHook,
		MUIA_List_DestructHook , &DestructHook,
		MUIA_List_CompareHook  , &CompareHook,
		MUIA_List_DisplayHook  , &DisplayHook,
		MUIA_List_AutoVisible  , TRUE,
		TAG_DONE);

	while ((id=NextDisplayInfo(id))!=INVALID_ID)
		DoMethod(obj,MUIM_List_InsertSingle,id,MUIV_List_Insert_Bottom);

	DoMethod(obj,MUIM_List_Sort);

	DoMethod(obj,MUIM_Notify,MUIA_List_Active,MUIV_EveryTime,obj,1,MUIM_DispIDlist_Change);

	return((ULONG)obj);
}

ULONG DispIDlist_Set(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct DispIDlist_Data *data = INST_DATA(cl,obj);
	struct TagItem *tag,*quiet;

	quiet = FindTagItem(MUIA_DispIDlist_Quiet,msg->ops_AttrList);

	if (tag = FindTagItem(MUIA_DispIDlist_CurrentID,msg->ops_AttrList))
	{
		data->CurrentID = tag->ti_Data;

		if (!quiet)
		{
			int i;
			struct NameInfo *ni;
			ULONG mask = 0;

			for (;;)
			{
				for (i=0;;i++)
				{
					DoMethod(obj,MUIM_List_GetEntry,i,&ni);
					if (!ni) break;
					if ((ni->Header.DisplayID & ~mask)==(data->CurrentID & ~mask)) { mask = MONITOR_ID_MASK; break; }
				}
				if (!ni) break;
				if (mask==MONITOR_ID_MASK) break;
				mask = MONITOR_ID_MASK;
			}

			if (ni)
				set(obj,MUIA_List_Active,i);
			else
				set(obj,MUIA_List_Active,MUIV_List_Active_Off);
		}
	}
	return(DoSuperMethodA(cl,obj,msg));
}

ULONG DispIDlist_Get(struct IClass *cl,Object *obj,struct opGet *msg)
{
	struct DispIDlist_Data *data = INST_DATA(cl,obj);

	switch (msg->opg_AttrID)
	{
		case MUIA_DispIDlist_CurrentID:
			*(msg->opg_Storage) = data->CurrentID;
			return(TRUE);
	}

	return(DoSuperMethodA(cl,obj,msg));
}

ULONG DispIDlist_Change(struct IClass *cl,Object *obj,Msg msg)
{
	struct NameInfo *ni;
	DoMethod(obj,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&ni);
	SetAttrs(obj,
		MUIA_DispIDlist_Quiet,TRUE,
		MUIA_DispIDlist_CurrentID,ni ? ni->Header.DisplayID : INVALID_ID,
		TAG_DONE);
	return(0);
}

BOOPSI_DISPATCHER(IPTR, DispIDlist_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW: return(DispIDlist_New(cl,obj,(APTR)msg));
		case OM_SET: return(DispIDlist_Set(cl,obj,(APTR)msg));
		case OM_GET: return(DispIDlist_Get(cl,obj,(APTR)msg));
		case MUIM_DispIDlist_Change: return(DispIDlist_Change(cl,obj,(APTR)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



/****************************************************************************/
/* SysPenField class                                                        */
/****************************************************************************/

struct SysPenField_Data
{
	LONG dummy;
};


ULONG SysPenField_DragQuery(struct IClass *cl,Object *obj,struct MUIP_DragQuery *msg)
{
	if (msg->obj==obj)
		return(MUIV_DragQuery_Refuse);

	if (muiUserData(msg->obj)<1 || muiUserData(msg->obj)>8)
		return(MUIV_DragQuery_Refuse);

	return(MUIV_DragQuery_Accept);
}



ULONG SysPenField_DragDrop(struct IClass *cl,Object *obj,struct MUIP_DragDrop *msg)
{
	set(obj,MUIA_Pendisplay_Reference,msg->obj);
	return(0);
}


BOOPSI_DISPATCHER(IPTR, SysPenField_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case MUIM_DragQuery  : return(SysPenField_DragQuery  (cl,obj,(APTR)msg));
		case MUIM_DragDrop   : return(SysPenField_DragDrop   (cl,obj,(APTR)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



static Object *MakePalette(void)
{
	Object *obj;

	obj = MUI_NewObject(MUIC_Poppen,
		MUIA_CycleChain, 1,
		MUIA_Window_Title, "Adjust Color",
		MUIA_Penadjust_PSIMode, 2,
		MUIA_MaxHeight, 20,
		TAG_DONE);

	return(obj);
}


Object *MakeMUIPen(int nr,Object **adr)
{
	Object *obj;

	obj = VGroup, MUIA_Group_Spacing, 1,
		Child, *adr = MUI_NewObject(MUIC_Poppen,
			MUIA_CycleChain, 1,
			MUIA_Window_Title, "Adjust MUI Pen",
			MUIA_Penadjust_PSIMode, 1,
			MUIA_MaxHeight, 20,
			TAG_DONE),
		Child, TextObject,
			MUIA_Text_Contents, GetStr(nr),
			MUIA_Text_PreParse, "\33c",
			MUIA_Font, MUIV_Font_Tiny,
			End,
		End;

	return(obj);
}


Object *MakeSysPen(int nr,Object **adr)
{
	Object *obj;

	obj = VGroup, MUIA_Group_Spacing, 1,
		Child, *adr = NewObject(CL_SysPenField->mcc_Class,NULL,
			TextFrame,
			MUIA_Background, MUII_BACKGROUND,
			MUIA_InnerLeft  , 4,
			MUIA_InnerRight , 4,
			MUIA_InnerTop   , 4,
			MUIA_InnerBottom, 4,
			TAG_DONE),
		Child, TextObject,
			MUIA_Font, MUIV_Font_Tiny,
			MUIA_Text_Contents, GetStr(nr),
			MUIA_Text_PreParse, "\33c",
			End,
		End;

	return(obj);
}




/****************************************************************************/
/* EditPanel class                                                          */
/****************************************************************************/

#define ForChilds(group) \
{\
	APTR child,cstate;\
	struct MinList *list;\
	get(group,MUIA_Group_ChildList,&list);\
	cstate=list->mlh_Head;\
	while (child=NextObject(&cstate))

#define NextChilds }

struct EditPanel_Data
{
	Object *TX_Info;
	/*
	Object *CM_Adjustable;
	*/

	Object *GR_EditPages;

	Object *ST_Name;
	Object *ST_Title;
	Object *ST_Font;
	Object *ST_Background;
	Object *CM_AutoScroll;
	Object *CM_NoDrag;
	Object *CM_Exclusive;
	Object *CM_Interleaved;
	Object *CM_Behind;
	Object *CM_SysDefault;
	Object *CM_AutoClose;
	Object *CM_CloseGadget;

	Object *GR_Size;
	Object *LV_Modes;
	Object *LI_Modes;
	Object *CY_Overscan;
	Object *ST_Width;
	Object *ST_Height;
	Object *SL_Depth;
	Object *TX_ModeInfo;

	Object *palette[PSD_NUMCOLS];
	Object *syspens[PSD_NUMSYSPENS];
	/*Object *muipens[PSD_NUMMUIPENS];*/
	Object *ColorMenu;

	LONG update;
};


ULONG EditPanel_SetScreen(struct IClass *cl,Object *obj,struct MUIP_EditPanel_SetScreen *msg)
{
	struct EditPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc = msg->desc;
	int i;

	set(data->ST_Name       ,MUIA_String_Contents ,desc->Name      );
	set(data->ST_Title      ,MUIA_String_Contents ,desc->Title     );
	set(data->ST_Font       ,MUIA_String_Contents ,desc->Font      );
	set(data->ST_Background ,MUIA_String_Contents ,desc->Background);

	/*
	set(data->CM_Adjustable ,MUIA_Selected       ,!desc->Foreign   );
	*/
	set(data->CM_AutoScroll ,MUIA_Selected       ,desc->AutoScroll );
	set(data->CM_NoDrag     ,MUIA_Selected       ,desc->NoDrag     );
	set(data->CM_Exclusive  ,MUIA_Selected       ,desc->Exclusive  );
	set(data->CM_Interleaved,MUIA_Selected       ,desc->Interleaved);
	set(data->CM_SysDefault ,MUIA_Selected       ,desc->SysDefault );
	set(data->CM_Behind     ,MUIA_Selected       ,desc->Behind     );
	set(data->CM_AutoClose  ,MUIA_Selected       ,desc->AutoClose  );
	set(data->CM_CloseGadget,MUIA_Selected       ,desc->CloseGadget);

	set(data->LI_Modes,MUIA_DispIDlist_CurrentID,desc->DisplayID);

	setstr(data->ST_Width ,desc->DisplayWidth );
	setstr(data->ST_Height,desc->DisplayHeight);
	set(data->SL_Depth,MUIA_Slider_Level,desc->DisplayDepth);
	set(data->CY_Overscan,MUIA_Cycle_Active,desc->OverscanType);

	for (i=0;i<PSD_NUMCOLS;i++)
	{
		set(data->palette[i],MUIA_Pendisplay_RGBcolor,&desc->Palette[i]);
	}

	/*
	for (i=0;i<PSD_NUMMUIPENS;i++)
	{
		set(data->muipens[i],MUIA_Pendisplay_Spec,&desc->MUIPens[i]);
	}
	*/

	for (i=0;i<PSD_NUMSYSPENS;i++)
	{
		if (data->syspens[i])
		{
			BYTE p = desc->SystemPens[i];
			p = BETWEEN(0,p,3) ? p : BETWEEN(-4,p,-1) ? 8+p : 0;
			set(data->syspens[i],MUIA_Pendisplay_Reference,data->palette[p]);
		}
	}

	return(0);
}


ULONG EditPanel_GetScreen(struct IClass *cl,Object *obj,struct MUIP_EditPanel_GetScreen *msg)
{
	struct EditPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc = msg->desc;
	int i;

	strcpy(desc->Name      ,getstr(data->ST_Name      ));
	strcpy(desc->Title     ,getstr(data->ST_Title     ));
	strcpy(desc->Font      ,getstr(data->ST_Font      ));
	strcpy(desc->Background,getstr(data->ST_Background));

	/*
	desc->Foreign     = !getbool(data->CM_Adjustable );
	*/
	desc->AutoScroll  = getbool(data->CM_AutoScroll );
	desc->NoDrag      = getbool(data->CM_NoDrag     );
	desc->Exclusive   = getbool(data->CM_Exclusive  );
	desc->Interleaved = getbool(data->CM_Interleaved);
	desc->SysDefault  = getbool(data->CM_SysDefault );
	desc->Behind      = getbool(data->CM_Behind     );
	desc->AutoClose   = getbool(data->CM_AutoClose  );
	desc->CloseGadget = getbool(data->CM_CloseGadget);

	desc->DisplayID     = xget(data->LI_Modes,MUIA_DispIDlist_CurrentID);
	desc->DisplayWidth  = atol(getstr(data->ST_Width ));
	desc->DisplayHeight = atol(getstr(data->ST_Height));

	desc->DisplayDepth  = xget(data->SL_Depth,MUIA_Slider_Level);
	desc->OverscanType  = xget(data->CY_Overscan,MUIA_Cycle_Active);

	for (i=0;i<PSD_NUMCOLS;i++)
	{
		desc->Palette[i] = *((struct MUI_RGBcolor *)xget(data->palette[i],MUIA_Pendisplay_RGBcolor));
	}

	/*
	for (i=0;i<PSD_NUMMUIPENS;i++)
	{
		desc->MUIPens[i] = *((struct MUI_PenSpec *)xget(data->muipens[i],MUIA_Pendisplay_Spec));
	}
	*/

	for (i=0;i<PSD_NUMSYSPENS;i++)
	{
		if (data->syspens[i])
		{
			BYTE p = muiUserData(xget(data->syspens[i],MUIA_Pendisplay_Reference));
			desc->SystemPens[i] = BETWEEN(1,p,4) ? p-1 : p-9;
		}
	}

	return(0);
}


ULONG EditPanel_ContextMenuChoice(struct IClass *cl,Object *obj,struct MUIP_ContextMenuChoice *msg)
{
	struct PalettePreset
	{
		struct MUI_RGBcolor col[8];
	};

	static const struct PalettePreset PalettePreset[10] =
	{
		/* def */
		{
			{
			{ 0xAAAAAAAA,0xAAAAAAAA,0xAAAAAAAA },
			{ 0x00000000,0x00000000,0x00000000 },
			{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
			{ 0x66666666,0x88888888,0xBBBBBBBB },
			{ 0xEEEEEEEE,0x44444444,0x44444444 },
			{ 0x55555555,0xDDDDDDDD,0x55555555 },
			{ 0x00000000,0x44444444,0xDDDDDDDD },
			{ 0xEEEEEEEE,0x99999999,0x00000000 },
			}
		},
		/* tint */
		{
			{
			{ 0xCCCCCCCC,0xCCCCCCCC,0xBBBBBBBB },
			{ 0x00000000,0x00000000,0x33333333 },
			{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
			{ 0x99999999,0xAAAAAAAA,0xBBBBBBBB },
			{ 0xEEEEEEEE,0x44444444,0x44444444 },
			{ 0x55555555,0xDDDDDDDD,0x55555555 },
			{ 0xE0E0E0E0,0xF0F0F0F0,0x88888888 },
			{ 0x00000000,0x44444444,0xDDDDDDDD },
			}
		},
		/* pharao */
		{
			{
			{ 0x55555555,0xBBBBBBBB,0xAAAAAAAA },
			{ 0x00000000,0x00000000,0x22222222 },
			{ 0xEEEEEEEE,0xEEEEEEEE,0xFFFFFFFF },
			{ 0x55555555,0x77777777,0xAAAAAAAA },
			{ 0xF6F6F6F6,0xF6F6F6F6,0x00000000 },
			{ 0x62626262,0x51515151,0xF0F0F0F0 },
			{ 0x00000000,0xF0F0F0F0,0x00000000 },
			{ 0xF0F0F0F0,0x30303030,0x10101010 },
			}
		},
		/* sunset */
		{
			{
			{ 0xAAAAAAAA,0x99999999,0x88888888 },
			{ 0x33333333,0x22222222,0x11111111 },
			{ 0xFFFFFFFF,0xEEEEEEEE,0xEEEEEEEE },
			{ 0xFFFFFFFF,0xDDDDDDDD,0xBBBBBBBB },
			{ 0xEEEEEEEE,0x44444444,0x44444444 },
			{ 0x55555555,0xDDDDDDDD,0x55555555 },
			{ 0xCFCFCFCF,0xDBDBDBDB,0xFFFFFFFF },
			{ 0x00000000,0x44444444,0xDDDDDDDD },
			}
		},
		/* ocean */
		{
			{
			{ 0x88888888,0xAAAAAAAA,0xCCCCCCCC },
			{ 0x00000000,0x00000000,0x22222222 },
			{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
			{ 0xFFFFFFFF,0xCCCCCCCC,0x99999999 },
			{ 0x00000000,0x00000000,0xF0F0F0F0 },
			{ 0xF9F9F9F9,0x21212121,0x21212121 },
			{ 0x52525252,0xF2F2F2F2,0x76767676 },
			{ 0xDFDFDFDF,0xA5A5A5A5,0x26262626 },
			}
		},
		/* steel */
		{
			{
			{ 0x99999999,0xBBBBBBBB,0xDDDDDDDD },
			{ 0x00000000,0x00000000,0x22222222 },
			{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
			{ 0x66666666,0x88888888,0xBBBBBBBB },
			{ 0xB2B2B2B2,0xDEDEDEDE,0xFFFFFFFF },
			{ 0xFFFFFFFF,0xA1A1A1A1,0x1C1C1C1C },
			{ 0xF0F0F0F0,0x44444444,0x87878787 },
			{ 0xBFBFBFBF,0xFFFFFFFF,0x90909090 },
			}
		},
		/* chocolate */
		{
			{
			{ 0xBBBBBBBB,0xAAAAAAAA,0x99999999 },
			{ 0x00000000,0x00000000,0x22222222 },
			{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
			{ 0x66666666,0x88888888,0xBBBBBBBB },
			{ 0xEEEEEEEE,0x44444444,0x44444444 },
			{ 0x55555555,0xDDDDDDDD,0x55555555 },
			{ 0x00000000,0x44444444,0xDDDDDDDD },
			{ 0xEEEEEEEE,0x99999999,0x00000000 },
			}
		},
		/* pewter */
		{
			{
			{ 0x88888888,0xAAAAAAAA,0xCCCCCCCC },
			{ 0x00000000,0x00000000,0x22222222 },
			{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF },
			{ 0xEEEEEEEE,0x99999999,0x77777777 },
			{ 0xD9D9D9D9,0xFFFFFFFF,0x09090909 },
			{ 0xF0F0F0F0,0x2D2D2D2D,0x31313131 },
			{ 0x38383838,0xF2F2F2F2,0x38383838 },
			{ 0x44444444,0x49494949,0xF0F0F0F0 },
			}
		},
		/* wine */
		{
			{
			{ 0xCCCCCCCC,0x99999999,0x99999999 },
			{ 0x00000000,0x00000000,0x22222222 },
			{ 0xFFFFFFFF,0xEEEEEEEE,0xEEEEEEEE },
			{ 0xBBBBBBBB,0x66666666,0x77777777 },
			{ 0xE0E0E0E0,0xF0F0F0F0,0x88888888 },
			{ 0x79797979,0x46464646,0xE8E8E8E8 },
			{ 0x60606060,0xC7C7C7C7,0x52525252 },
			{ 0x89898989,0xEAEAEAEA,0xC8C8C8C8 },
			}
		},
		/* magicwb */
		{
			{
			{ 0x95959595,0x95959595,0x95959595 },
			{ 0x00000000,0x00000000,0x00000000 },
			{ 0xffffffff,0xffffffff,0xffffffff },
			{ 0x3b3b3b3b,0x67676767,0xa2a2a2a2 },
			{ 0x7b7b7b7b,0x7b7b7b7b,0x7b7b7b7b },
			{ 0xafafafaf,0xafafafaf,0xafafafaf },
			{ 0xaaaaaaaa,0x90909090,0x7c7c7c7c },
			{ 0xffffffff,0xa9a9a9a9,0x97979797 },
			}
		},
	};
	/*{   0, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1 }*/

	LONG i = muiUserData(msg->item);
	struct MUI_PubScreenDesc *desc = NULL;
	struct MUI_RGBcolor *new = NULL;
	struct EditPanel_Data *data = INST_DATA(cl,obj);

	if (i>=0 && i<10)
	{
		new = (struct MUI_RGBcolor *)PalettePreset[i].col;
	}
	else if (i==10)
	{
		if (desc = MUIS_AllocPubScreenDesc(NULL))
		{
			new = desc->Palette;
		}
	}

	if (new)
	{
		for (i=0;i<PSD_NUMCOLS;i++)
		{
			set(data->palette[i],MUIA_Pendisplay_RGBcolor,&new[i]);
		}
	}

	if (desc)
		MUIS_FreePubScreenDesc(desc);

	return(0);
}


ULONG EditPanel_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct EditPanel_Data tmp = {0};
	Object *l1,*l2;

	obj = (Object *)DoSuperNewTags(cl,obj,NULL,
		MUIA_Group_Horiz, FALSE,

		MUIA_ContextMenu, tmp.ColorMenu = MUI_MakeObject(MUIO_MenustripNM,PaletteMenu,MUIO_MenustripNM_CommandKeyCheck),

		/*
		Child, HGroup, GroupSpacing(1),
			MUIA_Weight, 0,
			*/
			Child, tmp.TX_Info = TextObject,TextFrame,MUIA_Background,MUII_TextBack,MUIA_Text_PreParse,"\33c",End,
			/*
			Child, tmp.CM_Adjustable = MUI_MakeObject(MUIO_Checkmark,NULL),
			End,
			*/

		Child, tmp.GR_EditPages = RegisterGroup(CYA_EditPages),
			MUIA_CycleChain, 1,

			Child, ColGroup(2),
				Child, MakeLabel2(MSG_LABEL_PUBLICNAME),
				Child, tmp.ST_Name = MUI_NewObject(MUIC_Popscreen,
					MUIA_Popstring_String, MakeString(PSD_MAXLEN_NAME,MSG_LABEL_PUBLICNAME),
					MUIA_Popstring_Button, PopButton(MUII_PopUp),
					TAG_DONE),
				Child, MakeLabel2(MSG_LABEL_SCREENTITLE), Child, tmp.ST_Title = MakeString(PSD_MAXLEN_TITLE,MSG_LABEL_SCREENTITLE),
				Child, MakeLabel2(MSG_LABEL_DEFAULTFONT), Child, tmp.ST_Font  = PopaslObject, MUIA_Popstring_String, MakeString(PSD_MAXLEN_FONT,MSG_LABEL_DEFAULTFONT), MUIA_Popstring_Button, PopButton(MUII_PopUp), MUIA_Popasl_Type, ASL_FontRequest, End,
				Child, MakeLabel2(MSG_LABEL_BACKGROUND ), Child, tmp.ST_Background = PopaslObject, MUIA_Popstring_String, MakeString(PSD_MAXLEN_BACKGROUND,MSG_LABEL_BACKGROUND), MUIA_Popstring_Button, PopButton(MUII_PopUp), MUIA_Popasl_Type, ASL_FileRequest, End,
				Child, VSpace(2), Child, VSpace(2),
				Child, MakeFreeLabel(MSG_LABEL_PUBLICFLAGS),
				Child, HGroup,
					Child, ColGroup(2),
						Child, tmp.CM_AutoScroll  = MakeCheck(MSG_LABEL_AUTOSCROLL   ), Child, MakeLLabel1(MSG_LABEL_AUTOSCROLL   ),
						Child, tmp.CM_NoDrag      = MakeCheck(MSG_LABEL_NODRAG       ), Child, MakeLLabel1(MSG_LABEL_NODRAG       ),
						Child, tmp.CM_Exclusive   = MakeCheck(MSG_LABEL_EXCLUSIVE    ), Child, MakeLLabel1(MSG_LABEL_EXCLUSIVE    ),
						End,
					Child, HSpace(0),
					Child, ColGroup(2),
						Child, tmp.CM_AutoClose   = MakeCheck(MSG_LABEL_AUTOCLOSE    ), Child, MakeLLabel1(MSG_LABEL_AUTOCLOSE    ),
						Child, tmp.CM_Interleaved = MakeCheck(MSG_LABEL_INTERLEAVED  ), Child, MakeLLabel1(MSG_LABEL_INTERLEAVED  ),
						Child, tmp.CM_Behind      = MakeCheck(MSG_LABEL_OPENBEHIND   ), Child, MakeLLabel1(MSG_LABEL_OPENBEHIND   ),
						End,
					Child, HSpace(0),
					Child, ColGroup(2),
						Child, tmp.CM_SysDefault  = MakeCheck(MSG_LABEL_SYSTEMDEFAULT), Child, MakeLLabel1(MSG_LABEL_SYSTEMDEFAULT),
						Child, tmp.CM_CloseGadget = MakeCheck(MSG_LABEL_CLOSEGADGET  ), Child, MakeLLabel1(MSG_LABEL_CLOSEGADGET  ),
						Child, VSpace(0), Child, VSpace(0),
						End,
					End,
				End,

			Child, HGroup,
				Child, tmp.LV_Modes = ListviewObject,
					MUIA_CycleChain, 1,
					MUIA_Listview_List, tmp.LI_Modes = NewObject(CL_DispIDlist->mcc_Class,NULL,
						InputListFrame,
						MUIA_List_AdjustWidth, TRUE,
						TAG_DONE),
					End,
				Child, VGroup,
					Child, ScrollgroupObject,
						MUIA_Scrollgroup_FreeHoriz, FALSE,
                                                MUIA_Scrollgroup_Contents, VirtgroupObject,
							TextFrame,
							GroupSpacing(0),
							MUIA_Group_Horiz, TRUE,
							Child, tmp.TX_ModeInfo = NewObject(CL_DispIDinfo->mcc_Class,NULL,TAG_DONE),
							Child, HSpace(0),
							End,
						End,
					Child, RectangleObject, MUIA_VertWeight, 1, End,
					Child, tmp.GR_Size = ColGroup(2),
						Child, MakeLabel1(MSG_LABEL_OVERSCAN), Child, tmp.CY_Overscan = MakeCycle(CYA_Overscan,MSG_LABEL_OVERSCAN),
						Child, MakeLabel2(MSG_LABEL_SIZE),
						Child, HGroup,
							Child, tmp.ST_Width = MakeString(8,MSG_LABEL_SIZE),
							Child, MakeLabel2(MSG_LABEL_CROSS),
							Child, tmp.ST_Height = MakeString(8,MSG_LABEL_CROSS),
							End,
						Child, MakeLabel2(MSG_LABEL_DEPTH), Child, tmp.SL_Depth = MakeSlider(1,24,MSG_LABEL_DEPTH),
						End,
					End,
				End,

			Child, VGroup,
				Child, ColGroup(3),
					GroupFrameT(GetStr(MSG_PALETTE_TITLE)),
					MUIA_Group_VertSpacing, 1,
					Child, l1 = MakeCLabel(MSG_PALETTE_FIRST),
					Child, HSpace(4),
					Child, l2 = MakeCLabel(MSG_PALETTE_LAST),
					Child, HGroup,
						Child, tmp.palette[0] = MakePalette(),
						Child, tmp.palette[1] = MakePalette(),
						Child, tmp.palette[2] = MakePalette(),
						Child, tmp.palette[3] = MakePalette(),
						End,
					Child, HSpace(4),
					Child, HGroup,
						Child, tmp.palette[4] = MakePalette(),
						Child, tmp.palette[5] = MakePalette(),
						Child, tmp.palette[6] = MakePalette(),
						Child, tmp.palette[7] = MakePalette(),
						End,
					End,
				Child, HGroup,
					Child, ColGroup(3),
						GroupFrameT(GetStr(MSG_SYSPENS_TITLE)),
						Child, MakeSysPen(MSG_SYSPEN_TEXT     ,&tmp.syspens[TEXTPEN         ]),
						Child, MakeSysPen(MSG_SYSPEN_SHINE    ,&tmp.syspens[SHINEPEN        ]),
						Child, MakeSysPen(MSG_SYSPEN_SHADOW   ,&tmp.syspens[SHADOWPEN       ]),
						Child, MakeSysPen(MSG_SYSPEN_FILL     ,&tmp.syspens[FILLPEN         ]),
						Child, MakeSysPen(MSG_SYSPEN_FILLTEXT ,&tmp.syspens[FILLTEXTPEN     ]),
						Child, MakeSysPen(MSG_SYSPEN_HIGHLIGHT,&tmp.syspens[HIGHLIGHTTEXTPEN]),
						Child, MakeSysPen(MSG_SYSPEN_BARDETAIL,&tmp.syspens[BARDETAILPEN    ]),
						Child, MakeSysPen(MSG_SYSPEN_BARBLOCK ,&tmp.syspens[BARBLOCKPEN     ]),
						Child, MakeSysPen(MSG_SYSPEN_BARTRIM  ,&tmp.syspens[BARTRIMPEN      ]),
						End,
					/*
					Child, VGroup,
						GroupFrameT(GetStr(MSG_MUIPENS_TITLE)),
						Child, ColGroup(3),
							Child, MakeMUIPen(MSG_MUIPEN_SHINE     ,&tmp.muipens[MPEN_SHINE     ]),
							Child, MakeMUIPen(MSG_MUIPEN_HALFSHINE ,&tmp.muipens[MPEN_HALFSHINE ]),
							Child, MakeMUIPen(MSG_MUIPEN_BACKGROUND,&tmp.muipens[MPEN_BACKGROUND]),
							Child, MakeMUIPen(MSG_MUIPEN_HALFSHADOW,&tmp.muipens[MPEN_HALFSHADOW]),
							Child, MakeMUIPen(MSG_MUIPEN_SHADOW    ,&tmp.muipens[MPEN_SHADOW    ]),
							Child, MakeMUIPen(MSG_MUIPEN_TEXT      ,&tmp.muipens[MPEN_TEXT      ]),
							Child, MakeMUIPen(MSG_MUIPEN_FILL      ,&tmp.muipens[MPEN_FILL      ]),
							Child, MakeMUIPen(MSG_MUIPEN_MARK      ,&tmp.muipens[MPEN_MARK      ]),
							End,
						End,
					*/
					End,
				End,

			End,
		TAG_MORE,msg->ops_AttrList);

	if (obj)
	{
		struct EditPanel_Data *data = INST_DATA(cl,obj);
		int i;

		*data = tmp;

		DoMethod(tmp.LI_Modes     ,MUIM_Notify,MUIA_DispIDlist_CurrentID,MUIV_EveryTime,obj,2,MUIM_EditPanel_Update,3);
		DoMethod(tmp.CY_Overscan  ,MUIM_Notify,MUIA_Cycle_Active        ,MUIV_EveryTime,obj,2,MUIM_EditPanel_Update,2);
		DoMethod(tmp.ST_Width     ,MUIM_Notify,MUIA_String_Acknowledge  ,MUIV_EveryTime,obj,2,MUIM_EditPanel_Update,1);
		DoMethod(tmp.ST_Height    ,MUIM_Notify,MUIA_String_Acknowledge  ,MUIV_EveryTime,obj,2,MUIM_EditPanel_Update,1);
		DoMethod(tmp.SL_Depth     ,MUIM_Notify,MUIA_Slider_Level        ,MUIV_EveryTime,obj,2,MUIM_EditPanel_Update,1);

		/*
		set(tmp.CM_Adjustable,MUIA_Selected,TRUE);
		DoMethod(tmp.CM_Adjustable,MUIM_Notify,MUIA_Selected            ,MUIV_EveryTime,obj,1,MUIM_EditPanel_ToggleForeign);
		*/

		set(tmp.TX_Info       ,MUIA_ShortHelp,GetStr(MSG_HELP_INFO         ));
		/*
		set(tmp.CM_Adjustable ,MUIA_ShortHelp,GetStr(MSG_HELP_ADJUSTABLE   ));
		*/
		set(tmp.ST_Name       ,MUIA_ShortHelp,GetStr(MSG_HELP_NAME         ));
		set(tmp.ST_Title      ,MUIA_ShortHelp,GetStr(MSG_HELP_TITLE        ));
		set(tmp.ST_Font       ,MUIA_ShortHelp,GetStr(MSG_HELP_FONT         ));
		set(tmp.ST_Background ,MUIA_ShortHelp,GetStr(MSG_HELP_BACKGROUND   ));
		set(tmp.CM_AutoScroll ,MUIA_ShortHelp,GetStr(MSG_HELP_AUTOSCROLL   ));
		set(tmp.CM_NoDrag     ,MUIA_ShortHelp,GetStr(MSG_HELP_NODRAG       ));
		set(tmp.CM_Exclusive  ,MUIA_ShortHelp,GetStr(MSG_HELP_EXCLUSIVE    ));
		set(tmp.CM_Interleaved,MUIA_ShortHelp,GetStr(MSG_HELP_INTERLEAVED  ));
		set(tmp.CM_Behind     ,MUIA_ShortHelp,GetStr(MSG_HELP_BEHIND       ));
		set(tmp.CM_AutoClose  ,MUIA_ShortHelp,GetStr(MSG_HELP_AUTOCLOSE    ));
		set(tmp.CM_CloseGadget,MUIA_ShortHelp,GetStr(MSG_HELP_CLOSEGADGET  ));
		set(tmp.CM_SysDefault ,MUIA_ShortHelp,GetStr(MSG_HELP_SYSTEMDEFAULT));
		set(tmp.LV_Modes      ,MUIA_ShortHelp,GetStr(MSG_HELP_MODELIST     ));
		set(tmp.CY_Overscan   ,MUIA_ShortHelp,GetStr(MSG_HELP_OVERSCAN     ));
		set(tmp.ST_Width      ,MUIA_ShortHelp,GetStr(MSG_HELP_WIDTH        ));
		set(tmp.ST_Height     ,MUIA_ShortHelp,GetStr(MSG_HELP_HEIGHT       ));
		set(tmp.SL_Depth      ,MUIA_ShortHelp,GetStr(MSG_HELP_DEPTH        ));
		set(tmp.TX_ModeInfo   ,MUIA_ShortHelp,GetStr(MSG_HELP_MODEINFO     ));

		for (i=0;i<8;i++)
			set(data->palette[i],MUIA_UserData,i+1);

		set(l1,MUIA_Font,MUIV_Font_Tiny);
		set(l2,MUIA_Font,MUIV_Font_Tiny);

		/*
		DoMethod(obj,MUIM_EditPanel_ToggleForeign);
		*/

		/*
		if (IntuitionBase->lib_Version<39)
			set(tmp.ST_Background,MUIA_Disabled,TRUE);
		*/

		return((ULONG)obj);
	}

	return(0);
}


ULONG EditPanel_Dispose(struct IClass *cl,Object *obj,Msg msg)
{
	struct EditPanel_Data *data = INST_DATA(cl,obj);

	if (data->ColorMenu)
		MUI_DisposeObject(data->ColorMenu);

	return(DoSuperMethodA(cl,obj,msg));
}


/*
ULONG EditPanel_ToggleForeign(struct IClass *cl,Object *obj,Msg msg)
{
	struct EditPanel_Data *data = INST_DATA(cl,obj);
	BOOL disable = !getbool(data->CM_Adjustable);

	if (disable)
		set(data->GR_EditPages,MUIA_Group_ActivePage,2);

	DoMethod(obj,MUIM_EditPanel_Update,1);

	DoMethod(obj,MUIM_MultiSet,MUIA_Disabled,disable,
		data->ST_Title,
		data->ST_Font,
		data->ST_Background,
		data->CM_AutoScroll,
		data->CM_NoDrag,
		data->CM_Exclusive,
		data->CM_Interleaved,
		data->CM_Behind,
		data->CM_SysDefault,
		data->CM_AutoClose,
		data->CM_CloseGadget,
		data->LV_Modes,
		data->CY_Overscan,
		data->ST_Width,
		data->ST_Height,
		data->SL_Depth,
		data->TX_ModeInfo,
		NULL);

	return(0);
}
*/


ULONG EditPanel_Update(struct IClass *cl,Object *obj,struct MUIP_EditPanel_Update *msg)
{
	struct EditPanel_Data *data = INST_DATA(cl,obj);
	struct NameInfo *ni;
	struct DimensionInfo DimensionInfo;

	if (data->update) return(0);
	data->update = TRUE;

	/*
	if (getbool(data->CM_Adjustable))
	*/
	{
		DoMethod(data->LI_Modes,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&ni);

		if (ni && GetDisplayInfoData(NULL,(char *)&DimensionInfo,sizeof(DimensionInfo),DTAG_DIMS,ni->Header.DisplayID))
		{
			/*set(data->GR_SysPots,MUIA_Coloradjust_ModeID,ni->Header.DisplayID); !!!*/

			if (msg->level>=1)
			{
				set(data->GR_Size,MUIA_Disabled,FALSE);
				set(data->SL_Depth,MUIA_Slider_Max,DimensionInfo.MaxDepth);
			}

			if (msg->level>=3)
				set(data->TX_ModeInfo,MUIA_DispIDinfo_ID,ni->Header.DisplayID);

			if (msg->level>=2)
			{
				int w = RectangleWidth(DimensionInfo.TxtOScan);
				int h = RectangleHeight(DimensionInfo.TxtOScan);

				switch (xget(data->CY_Overscan,MUIA_Cycle_Active))
				{
					case 1:
						w = RectangleWidth(DimensionInfo.StdOScan);
						h = RectangleHeight(DimensionInfo.StdOScan);
						break;

					case 2:
						w = RectangleWidth(DimensionInfo.MaxOScan);
						h = RectangleHeight(DimensionInfo.MaxOScan);
						break;

					case 3:
						w = RectangleWidth(DimensionInfo.VideoOScan);
						h = RectangleHeight(DimensionInfo.VideoOScan);
						break;
				}

				setstr(data->ST_Width ,w);
				setstr(data->ST_Height,h);
			}

			if (msg->level>=1)
				DoMethod(data->TX_Info,MUIM_SetAsString,MUIA_Text_Contents,"%s (%ld x %ld x %ld)",ni->Name,atol(getstr(data->ST_Width)),atol(getstr(data->ST_Height)),xget(data->SL_Depth,MUIA_Slider_Level));
		}
		else
		{
			/* set(data->GR_SysPots,MUIA_Coloradjust_ModeID,INVALID_ID); !!!*/
			set(data->TX_ModeInfo,MUIA_DispIDinfo_ID,INVALID_ID);
			set(data->TX_Info,MUIA_Text_Contents,GetStr(MSG_TEXT_UNKNOWNMODE));
			set(data->GR_Size,MUIA_Disabled,TRUE);
		}
	}
	/*
	else
	{
		set(data->TX_Info,MUIA_Text_Contents,GetStr(MSG_TEXT_FOREIGNSCREEN));
	}
	*/

	data->update = FALSE;
	return(0);
}


BOOPSI_DISPATCHER(IPTR, EditPanel_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW                      : return(EditPanel_New              (cl,obj,(APTR)msg));
		case OM_DISPOSE                  : return(EditPanel_Dispose          (cl,obj,(APTR)msg));

		case MUIM_ContextMenuChoice      : return(EditPanel_ContextMenuChoice(cl,obj,(APTR)msg));

		case MUIM_EditPanel_SetScreen    : return(EditPanel_SetScreen        (cl,obj,(APTR)msg));
		case MUIM_EditPanel_GetScreen    : return(EditPanel_GetScreen        (cl,obj,(APTR)msg));
		case MUIM_EditPanel_Update       : return(EditPanel_Update           (cl,obj,(APTR)msg));
		/*
		case MUIM_EditPanel_ToggleForeign: return(EditPanel_ToggleForeign    (cl,obj,(APTR)msg));
		*/
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



/****************************************************************************/
/* EditWindow class                                                         */
/****************************************************************************/

struct EditWindow_Data
{
	Object *panel;
	char wtitle[PSD_MAXLEN_TITLE+20];
};

ULONG EditWindow_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct EditWindow_Data *data;
	Object *ok;
	Object *cancel;
	Object *panel;
	Object *originator;
	/*Object *strip;*/

	if (obj = (Object *)DoSuperNewTags(cl,obj,NULL,
		/*MUIA_Window_Menustrip, strip = MUI_MakeObject(MUIO_MenustripNM,EditMenu,0),*/
		WindowContents, VGroup,
			Child, panel = NewObject(CL_EditPanel->mcc_Class,NULL,TAG_DONE),
			Child, HGroup, MUIA_Group_SameSize, TRUE,
				Child, ok = MakeButton(MSG_BUTTON_OK),
				Child, HSpace(0),
				Child, HSpace(0),
				Child, HSpace(0),
				Child, cancel = MakeButton(MSG_BUTTON_CANCEL),
				End,
			End,
		TAG_MORE,msg->ops_AttrList))
	{
		data = INST_DATA(cl,obj);

		data->panel = panel;

		strcpy(data->wtitle,GetStr(MSG_TITLE_PUBSCREENWINDOW));
		strcat(data->wtitle," ");
		strcat(data->wtitle,(char *)GetTagData(MUIA_EditWindow_Title,(ULONG)"",msg->ops_AttrList));

		set(obj,MUIA_Window_Title,data->wtitle);
		set(obj,MUIA_Window_ID   ,MAKE_ID('E','D','I','T'));

		originator = (Object *)GetTagData(MUIA_EditWindow_Originator,0,msg->ops_AttrList);

		DoMethod(obj   ,MUIM_Notify,MUIA_Window_CloseRequest,TRUE ,MUIV_Notify_Application,6,MUIM_Application_PushMethod,originator,3,MUIM_ScreenPanel_Finish,obj,FALSE);
		DoMethod(cancel,MUIM_Notify,MUIA_Pressed            ,FALSE,MUIV_Notify_Application,6,MUIM_Application_PushMethod,originator,3,MUIM_ScreenPanel_Finish,obj,FALSE);
		DoMethod(ok    ,MUIM_Notify,MUIA_Pressed            ,FALSE,MUIV_Notify_Application,6,MUIM_Application_PushMethod,originator,3,MUIM_ScreenPanel_Finish,obj,TRUE );

		/*
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_2COL),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,panel,2,MUIM_EditPanel_DefColors,0);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_4COL),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,panel,2,MUIM_EditPanel_DefColors,1);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_8COL),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,panel,2,MUIM_EditPanel_DefColors,2);
		*/

		set(ok    ,MUIA_ShortHelp,GetStr(MSG_HELP_EDITOK    ));
		set(cancel,MUIA_ShortHelp,GetStr(MSG_HELP_EDITCANCEL));

		return((IPTR)obj);
	}
	return(0);
}

BOOPSI_DISPATCHER(IPTR, EditWindow_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW: return(EditWindow_New(cl,obj,(APTR)msg));

		/*
		**	The next methods actually don't belong to the
		** edit window class. We just forward them here to
		** allow treating an edit window much like an edit
		** panel from outside.
		*/

		case MUIM_EditPanel_SetScreen:
		case MUIM_EditPanel_GetScreen:
		{
			struct EditWindow_Data *data = INST_DATA(cl,obj);
			return(DoMethodA(data->panel,msg));
		}
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



/****************************************************************************/
/* ScreenList class                                                         */
/****************************************************************************/

#define USE_PSI_SCREENON_HEADER
#define USE_PSI_SCREENON_BODY
#define USE_PSI_SCREENON_COLORS
#include "psi_screenon.bh"

/*
#define USE_PSI_SCREENOF_BODY
#include "psi_screenof.bh"
*/

#define USE_PSI_SCREENCN_BODY
#include "psi_screencn.bh"

/*
#define USE_PSI_SCREENCF_BODY
#include "psi_screencf.bh"
*/



struct ScreenList_Data
{
	Object *list;
	Object *onormal[2];
	/*
	Object *oforeign[2];
	*/
	APTR inormal[2];
	/*
	APTR iforeign[2];
	*/
	struct Hook DisplayHook;
};

IPTR ScreenList_ConstructFunc(struct Hook *hook, APTR pool, struct MUI_PubScreenDesc *src)
{
	struct MUI_PubScreenDesc *desc;

	if (desc = MUIS_AllocPubScreenDesc(src))
	{
		desc->Changed  = FALSE;
		desc->UserData = NULL;
	}
	return(desc);
}

VOID ScreenList_DestructFunc(struct Hook *hook, APTR pool, struct MUI_PubScreenDesc *desc)
{
	MUIS_FreePubScreenDesc(desc);
}

LONG ScreenList_CompareFunc(struct Hook *hook, struct MUI_PubScreenDesc *d2, struct MUI_PubScreenDesc *d1)
{
	if (!strcmp(d1->Name,PSD_INITIAL_NAME))
		return(strcmp(d2->Name,PSD_INITIAL_NAME) ? 1 : 0);
	else if (!strcmp(d2->Name,PSD_INITIAL_NAME))
		return(-1);
	else
		return(stricmp(d1->Name,d2->Name));
}

LONG ScreenList_DisplayFunc(struct Hook *hook, char **array, struct MUI_PubScreenDesc *desc)
{
	struct ScreenList_Data *data = (APTR)hook->h_Data;

	*array++ = "";

	if (!desc)
	{
		static char buf1[30],buf2[30];
		strcpy(buf1,"\33b\33u");
		strcpy(buf2,"\33b\33u");
		strcat(buf1,GetStr(MSG_LIST_SCREENNAME));
		strcat(buf2,GetStr(MSG_LIST_SCREENMODE));
		*array++ = "";
		*array++ = buf1;
		*array   = buf2;
	}
	else
	{
		static struct NameInfo ni;
		static char buf1[PSD_MAXLEN_NAME+2];
		static char buf2[50];

		strcpy(buf1,desc->UserData ? "\33b" : desc->Changed ? "\33u" : "");
		strcat(buf1,desc->Name);

		/*
		if (desc->Foreign)
		{
			strcpy(ni.Name,GetStr(MSG_LIST_FOREIGNSCREEN));
			sprintf(buf2,"\33O[%08lx]",data->iforeign[TestPubScreen(desc->Name) ? 1 : 0]);
		}
		else
		*/
		{
			if (!GetDisplayInfoData(0,(UBYTE *)&ni,sizeof(ni),DTAG_NAME,desc->DisplayID))
				strcpy(ni.Name,GetStr(MSG_LIST_UNKNOWNMODE));

			sprintf(buf2,"\33O[%08lx]",data->inormal[TestPubScreen(desc->Name) ? 1 : 0]);
		}

		*array++ = buf2;
		*array++ = buf1;
		*array   = ni.Name;
	}

	return(0);
}


ULONG ScreenList_Load(struct IClass *cl,Object *obj,struct MUIP_ScreenList_Load *msg)
{
	ULONG result = FALSE;
	struct MUI_PubScreenDesc *desc;
	APTR pfh;

	if (pfh = MUIS_OpenPubFile(msg->name,MODE_OLDFILE))
	{
		result = TRUE;

		if (msg->clear)
			DoMethod(obj,MUIM_List_Clear);

		set(obj,MUIA_List_Quiet,TRUE);

		while (desc = MUIS_ReadPubFile(pfh))
		{
			DoMethod(obj,MUIM_List_InsertSingle,desc,MUIV_List_Insert_Sorted);
		}

		set(obj,MUIA_List_Quiet,FALSE);

		MUIS_ClosePubFile(pfh);
	}
	return(result);
}


ULONG ScreenList_Save(struct IClass *cl,Object *obj,struct MUIP_ScreenList_Save *msg)
{
	ULONG result = FALSE;
	struct MUI_PubScreenDesc *desc;
	APTR pfh;
	int i;

	DoMethod(obj,MUIM_List_Sort);

	if (pfh = MUIS_OpenPubFile(msg->name,MODE_NEWFILE))
	{
		result = TRUE;

		for (i=0;result;i++)
		{
			DoMethod(obj,MUIM_List_GetEntry,i,&desc);
			if (!desc) break;

			desc->Changed  = FALSE;
			desc->UserData = NULL;

			if (!MUIS_WritePubFile(pfh,desc))
				result = FALSE;
		}
		MUIS_ClosePubFile(pfh);
	}
	return(result);
}


ULONG ScreenList_Find(struct IClass *cl,Object *obj,struct MUIP_ScreenList_Find *msg)
{
	int i;
	struct MUI_PubScreenDesc *desc;

	*(msg->desc) = NULL;

	for (i=0;;i++)
	{
		DoMethod(obj,MUIM_List_GetEntry,i,&desc);
		if (!desc) break;
		if (!stricmp(desc->Name,msg->name))
		{
			*(msg->desc) = desc;
			set(obj,MUIA_List_Active,i);
			break;
		}
	}
	return(0);
}


static Object *makescreenimage(UBYTE *body)
{
	Object *obj = BodychunkObject,
		MUIA_FixWidth             , PSI_SCREENON_WIDTH ,
		MUIA_FixHeight            , PSI_SCREENON_HEIGHT,
		MUIA_Bitmap_Width         , PSI_SCREENON_WIDTH ,
		MUIA_Bitmap_Height        , PSI_SCREENON_HEIGHT,
		MUIA_Bodychunk_Depth      , PSI_SCREENON_DEPTH ,
		MUIA_Bodychunk_Body       , (UBYTE *)body,
		MUIA_Bodychunk_Compression, PSI_SCREENON_COMPRESSION,
		MUIA_Bodychunk_Masking    , PSI_SCREENON_MASKING,
		MUIA_Bitmap_SourceColors  , (ULONG *)psi_screenon_colors,
		MUIA_Bitmap_Transparent   , 0,
		End;

	return(obj);
}


ULONG ScreenList_Setup(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenList_Data *data = INST_DATA(cl,obj);

	if (!DoSuperMethodA(cl,obj,msg))
		return(FALSE);

	data->onormal[0]  = makescreenimage((UBYTE *)psi_screencn_body);
	data->onormal[1]  = makescreenimage((UBYTE *)psi_screenon_body);
	/*
	data->oforeign[0] = makescreenimage((UBYTE *)psi_screencf_body);
	data->oforeign[1] = makescreenimage((UBYTE *)psi_screenof_body);
	*/

	data->inormal[0]  = (APTR)DoMethod(obj,MUIM_List_CreateImage,data->onormal[0] ,0);
	data->inormal[1]  = (APTR)DoMethod(obj,MUIM_List_CreateImage,data->onormal[1] ,0);
	/*
	data->iforeign[0] = (APTR)DoMethod(obj,MUIM_List_CreateImage,data->oforeign[0],0);
	data->iforeign[1] = (APTR)DoMethod(obj,MUIM_List_CreateImage,data->oforeign[1],0);
	*/

	MUI_RequestIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY);

	return(TRUE);
}


ULONG ScreenList_Cleanup(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenList_Data *data = INST_DATA(cl,obj);

	DoMethod(obj,MUIM_List_DeleteImage,data->inormal[0] );
	DoMethod(obj,MUIM_List_DeleteImage,data->inormal[1] );
	/*
	DoMethod(obj,MUIM_List_DeleteImage,data->iforeign[0]);
	DoMethod(obj,MUIM_List_DeleteImage,data->iforeign[1]);
	*/

	if (data->onormal[0] ) MUI_DisposeObject(data->onormal[0] );
	if (data->onormal[1] ) MUI_DisposeObject(data->onormal[1] );
	/*
	if (data->oforeign[0]) MUI_DisposeObject(data->oforeign[0]);
	if (data->oforeign[1]) MUI_DisposeObject(data->oforeign[1]);
	*/

	MUI_RejectIDCMP(obj,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY);

	return(DoSuperMethodA(cl,obj,msg));
}


ULONG ScreenList_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	static struct Hook ScreenList_ConstructHook;
	ScreenList_ConstructHook.h_Entry = HookEntry;
	ScreenList_ConstructHook.h_SubEntry = ScreenList_ConstructFunc;
	static struct Hook ScreenList_DestructHook;
	ScreenList_DestructHook.h_Entry = HookEntry;
	ScreenList_DestructHook.h_SubEntry = ScreenList_DestructFunc;
	static struct Hook ScreenList_CompareHook;
	ScreenList_CompareHook.h_Entry = HookEntry;
	ScreenList_CompareHook.h_SubEntry = ScreenList_CompareFunc;

	obj=(Object *)DoSuperNewTags(cl,obj,NULL,
		MUIA_List_ConstructHook, &ScreenList_ConstructHook,
		MUIA_List_DestructHook , &ScreenList_DestructHook,
		MUIA_List_CompareHook  , &ScreenList_CompareHook,
		MUIA_List_Format       , "DELTA=2,,,",
		MUIA_List_Title        , TRUE,
		MUIA_List_MinLineHeight, 14,
		TAG_MORE,msg->ops_AttrList);

	if (obj)
	{
		struct ScreenList_Data *data = INST_DATA(cl,obj);

		data->DisplayHook.h_Entry = HookEntry;
		data->DisplayHook.h_SubEntry = (VOID *)ScreenList_DisplayFunc;
		data->DisplayHook.h_Data  = (APTR)data;

		set(obj,MUIA_List_DisplayHook,&data->DisplayHook);
	}

	return((ULONG)obj);
}

BOOPSI_DISPATCHER(IPTR, ScreenList_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW              : return(ScreenList_New    (cl,obj,(APTR)msg));
		case MUIM_Setup          : return(ScreenList_Setup  (cl,obj,(APTR)msg));
		case MUIM_Cleanup        : return(ScreenList_Cleanup(cl,obj,(APTR)msg));
		case MUIM_ScreenList_Save: return(ScreenList_Save   (cl,obj,(APTR)msg));
		case MUIM_ScreenList_Load: return(ScreenList_Load   (cl,obj,(APTR)msg));
		case MUIM_ScreenList_Find: return(ScreenList_Find   (cl,obj,(APTR)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



/****************************************************************************/
/* ScreenPanel class                                                        */
/****************************************************************************/

struct ScreenPanel_Data
{
	Object *LV_Screens;
	Object *BT_Create;
	Object *BT_Copy;
	Object *BT_Delete;
	Object *BT_Edit;
	Object *BT_Open;
	Object *BT_Close;
	Object *BT_Jump;
	#ifdef MYDEBUG
	Object *BT_Foo;
	#endif
};

ULONG ScreenPanel_Finish(struct IClass *cl,Object *obj,struct MUIP_ScreenPanel_Finish *msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	int i;
	Object *win = msg->win;
	BOOL ok = msg->ok;

	for (i=0;;i++)
	{
		DoMethod(data->LV_Screens,MUIM_List_GetEntry,i,&desc);
		if (!desc) return(0); /* should never happen */
		if (desc->UserData==win) break;
	}

	desc->UserData = NULL;

	if (ok)
	{
		DoMethod(win,MUIM_EditPanel_GetScreen,desc);
		desc->Changed = TRUE;
	}
	DoMethod(data->LV_Screens,MUIM_List_Redraw,i);
	DoMethod(obj,MUIM_ScreenPanel_SetStates);

	set(win,MUIA_Window_Open,FALSE);
	DoMethod((Object *)xget(obj,MUIA_ApplicationObject),OM_REMMEMBER,win);
	MUI_DisposeObject(win);

	return(0);
}

ULONG ScreenPanel_Edit(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;

	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&desc);

	if (desc)
	{
		set((Object *)xget(obj,MUIA_ApplicationObject),MUIA_Application_Sleep,TRUE);

		if (!desc->UserData)
		{
			if (desc->UserData = (APTR)NewObject(CL_EditWindow->mcc_Class,NULL,
				MUIA_Window_Width , MUIV_Window_Width_MinMax(0),
				MUIA_Window_Height, MUIV_Window_Height_MinMax(0),
				MUIA_EditWindow_Title, desc->Name,
				MUIA_EditWindow_Originator, obj,
				TAG_DONE))
			{
				DoMethod((Object *)xget(obj,MUIA_ApplicationObject),OM_ADDMEMBER,desc->UserData);
				DoMethod(desc->UserData,MUIM_EditPanel_SetScreen,desc);
				DoMethod(data->LV_Screens,MUIM_List_Redraw,MUIV_List_Redraw_Active);
			}
		}

		if (desc->UserData)
			set(desc->UserData,MUIA_Window_Open,TRUE);
		else
			DisplayBeep(0);

		set((Object *)xget(obj,MUIA_ApplicationObject),MUIA_Application_Sleep,FALSE);
	}
	return(0);
}

ULONG ScreenPanel_Delete(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;

	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&desc);
	if (desc)
	{
		if (!desc->UserData)
		{
			MUIS_ClosePubScreen(desc->Name);
			DoMethod(data->LV_Screens,MUIM_List_Remove,MUIV_List_Remove_Active);
		}
		else
			DisplayBeep(0);
	}
	return(0);
}

ULONG ScreenPanel_Create(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	DoMethod(data->LV_Screens,MUIM_List_InsertSingle,NULL,MUIV_List_Insert_Bottom);
	set(data->LV_Screens,MUIA_List_Active,MUIV_List_Active_Bottom);
	return(0);
}

ULONG ScreenPanel_Copy(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *new,*src;
	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&src);
	if (new = MUIS_AllocPubScreenDesc(src))
	{
		char namebuf[PSD_MAXLEN_NAME];
		strcpy(namebuf,new->Name);
		strcpy(new->Name,">");
		stccpy(new->Name+1,namebuf,PSD_MAXLEN_NAME-1);
		DoMethod(data->LV_Screens,MUIM_List_InsertSingle,new,MUIV_List_Insert_Bottom);
		set(data->LV_Screens,MUIA_List_Active,MUIV_List_Active_Bottom);
		MUIS_FreePubScreenDesc(new);
	}
	else
		DisplayBeep(0);
	return(0);
}

ULONG ScreenPanel_SetStates(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&desc);
	if (desc)
	{
		/*
		if (desc->Foreign)
		{
			set(data->BT_Copy  ,MUIA_Disabled,FALSE);
			set(data->BT_Delete,MUIA_Disabled,FALSE);
			set(data->BT_Edit  ,MUIA_Disabled,FALSE);
			set(data->BT_Open  ,MUIA_Disabled,TRUE );
			set(data->BT_Close ,MUIA_Disabled,TRUE );
			set(data->BT_Jump  ,MUIA_Disabled,FALSE);
		}
		else
		*/
		{
			BOOL opened = TestPubScreen(desc->Name);

			set(data->BT_Copy  ,MUIA_Disabled,FALSE);
			set(data->BT_Delete,MUIA_Disabled,FALSE);
			set(data->BT_Edit  ,MUIA_Disabled,FALSE);
			set(data->BT_Open  ,MUIA_Disabled, opened);
			set(data->BT_Close ,MUIA_Disabled,!opened);
			set(data->BT_Jump  ,MUIA_Disabled,FALSE);
		}
	}
	else
	{
		set(data->BT_Copy  ,MUIA_Disabled,TRUE);
		set(data->BT_Delete,MUIA_Disabled,TRUE);
		set(data->BT_Edit  ,MUIA_Disabled,TRUE);
		set(data->BT_Open  ,MUIA_Disabled,TRUE);
		set(data->BT_Close ,MUIA_Disabled,TRUE);
		set(data->BT_Jump  ,MUIA_Disabled,TRUE);
	}
	return(0);
}

ULONG ScreenPanel_Close(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&desc);
	if (!desc || !MUIS_ClosePubScreen(desc->Name))
		DisplayBeep(0);
	return(0);
}

ULONG ScreenPanel_Open(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&desc);
   if (desc)
	{
		if (desc->Changed)
		{
			/* !!! */
		}
		if (!MUIS_OpenPubScreen(desc))
			DisplayBeep(0);
	}
	else
		DisplayBeep(0);
	return(0);
}

ULONG ScreenPanel_Jump(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	DoMethod(data->LV_Screens,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&desc);
	if (desc)
	{
		if (desc->Changed)
		{
			/* !!! */
		}
		DoMethod((Object *)xget(obj,MUIA_ApplicationObject),MUIM_Application_SetConfigItem,MUICFG_PublicScreen,desc->Name);
	}
	else
		DisplayBeep(0);

	return(0);
}

ULONG ScreenPanel_Update(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	DoMethod(data->LV_Screens,MUIM_List_Redraw,MUIV_List_Redraw_All);
	DoMethod(obj,MUIM_ScreenPanel_SetStates);
	return(0);
}

ULONG ScreenPanel_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct ScreenPanel_Data tmp = {0};

	obj = (Object *)DoSuperNewTags(cl,obj,NULL,
		MUIA_Group_Horiz, FALSE,
		MUIA_Group_VertSpacing, 0,
		Child, tmp.LV_Screens = ListviewObject,
			MUIA_CycleChain, 1,
			MUIA_Listview_List, NewObject(CL_ScreenList->mcc_Class,NULL,InputListFrame,MUIA_List_AutoVisible,TRUE,TAG_DONE),
			End,
		Child, ColGroup(4), GroupSpacing(0), MUIA_Group_SameSize, TRUE,
			Child, tmp.BT_Create = MakeButton(MSG_BUTTON_NEW),
			Child, tmp.BT_Copy   = MakeButton(MSG_BUTTON_COPY),
			Child, tmp.BT_Delete = MakeButton(MSG_BUTTON_DELETE),
			Child, tmp.BT_Edit   = MakeButton(MSG_BUTTON_EDIT),
			Child, tmp.BT_Open   = MakeButton(MSG_BUTTON_OPEN),
			Child, tmp.BT_Close  = MakeButton(MSG_BUTTON_CLOSE),
			Child, tmp.BT_Jump   = MakeButton(MSG_BUTTON_JUMP),
			#ifdef MYDEBUG
			Child, tmp.BT_Foo    = SimpleButton("Foo"),
			#else
			Child, HVSpace,
			#endif
			End,
		TAG_DONE);

	if (obj)
	{
		struct ScreenPanel_Data *data = INST_DATA(cl,obj);

		*data = tmp;

		DoMethod(tmp.BT_Delete ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Delete);
		DoMethod(tmp.BT_Create ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Create);
		DoMethod(tmp.BT_Copy   ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Copy  );
		DoMethod(tmp.BT_Edit   ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Edit  );
		DoMethod(tmp.BT_Open   ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Open  );
		DoMethod(tmp.BT_Close  ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Close );
		DoMethod(tmp.BT_Jump   ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Jump  );
		#ifdef MYDEBUG
		DoMethod(tmp.BT_Foo    ,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MUIM_ScreenPanel_Foo   );
		#endif
		DoMethod(tmp.LV_Screens,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,obj,1,MUIM_ScreenPanel_Edit);
		DoMethod(tmp.LV_Screens,MUIM_Notify,MUIA_List_Active,MUIV_EveryTime,obj,1,MUIM_ScreenPanel_SetStates);

		set(tmp.BT_Delete ,MUIA_ShortHelp,GetStr(MSG_HELP_DELETESCREEN));
		set(tmp.BT_Create ,MUIA_ShortHelp,GetStr(MSG_HELP_NEWSCREEN   ));
		set(tmp.BT_Copy   ,MUIA_ShortHelp,GetStr(MSG_HELP_COPYSCREEN  ));
		set(tmp.BT_Edit   ,MUIA_ShortHelp,GetStr(MSG_HELP_EDITSCREEN  ));
		set(tmp.BT_Open   ,MUIA_ShortHelp,GetStr(MSG_HELP_OPENSCREEN  ));
		set(tmp.BT_Close  ,MUIA_ShortHelp,GetStr(MSG_HELP_CLOSESCREEN ));
		set(tmp.BT_Jump   ,MUIA_ShortHelp,GetStr(MSG_HELP_JUMPSCREEN  ));
		set(tmp.LV_Screens,MUIA_ShortHelp,GetStr(MSG_HELP_SCREENLIST  ));

		DoMethod(obj,MUIM_ScreenPanel_SetStates);
	}
	return((ULONG)obj);
}

ULONG ScreenPanel_Dispose(struct IClass *cl,Object *obj,Msg msg)
{
	DoMethod(obj,MUIM_ScreenPanel_CloseWindows);
	return(DoSuperMethodA(cl,obj,msg));
}

ULONG ScreenPanel_CloseWindows(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	int i;
	for (i=0;;i++)
	{
		DoMethod(data->LV_Screens,MUIM_List_GetEntry,i,&desc);
		if (!desc) break;
		if (desc->UserData) DoMethod(obj,MUIM_ScreenPanel_Finish,desc->UserData,FALSE);
	}
	return(0);
}

#ifdef MYDEBUG

BOOL SavePubScreen(struct MUI_PubScreenDesc *desc)
{
	BPTR file;
	char filename[PSD_MAXLEN_NAME+16];
	int p;
	BOOL rc = FALSE;

	strcpy(filename,"mui:Screens");
	AddPart(filename,desc->Name,sizeof(filename));
	strcat(filename,".mps");

	if (file = Open(filename,MODE_NEWFILE))
	{
		FPrintf(file,"T=\"%s\"\n",desc->Title       );
		FPrintf(file,"F=\"%s\"\n",desc->Font        );
		FPrintf(file,"B=\"%s\"\n",desc->Background  );
		FPrintf(file,"W=%ld\n"   ,desc->DisplayWidth);
		FPrintf(file,"H=%ld\n"   ,desc->DisplayHeight);
		FPrintf(file,"D=%ld\n"   ,desc->DisplayDepth);
		FPrintf(file,"I=%ld\n"   ,desc->DisplayID   );
		if (desc->OverscanType) FPrintf(file,"OS\n");
		if (desc->AutoScroll  ) FPrintf(file,"AS\n");
		if (desc->NoDrag      ) FPrintf(file,"ND\n");
		if (desc->Exclusive   ) FPrintf(file,"EX\n");
		if (desc->Interleaved ) FPrintf(file,"IN\n");
		if (desc->SysDefault  ) FPrintf(file,"SD\n");
		if (desc->Behind      ) FPrintf(file,"BH\n");
		if (desc->AutoClose   ) FPrintf(file,"AC\n");
		if (desc->CloseGadget ) FPrintf(file,"CG\n");

		FPrintf(file,"PEN=\"");
		for (p=0;p<PSD_NUMSYSPENS;p++)
			FPrintf(file,"%ld:%ld ",p,desc->SystemPens[p]);
		FPrintf(file,"\"\n");

		FPrintf(file,"PAL=\"");
		for (p=0;p<PSD_NUMCOLS;p++)
			FPrintf(file,"%ld:%02lx%02lx%02lx ",p<4 ? p : p-8,desc->Palette[p].red>>24,desc->Palette[p].green>>24,desc->Palette[p].blue>>24);
		FPrintf(file,"\"\n");

		rc = TRUE;

		Close(file);
	}
	return(rc);
}


ULONG ScreenPanel_Foo(struct IClass *cl,Object *obj,Msg msg)
{
	struct ScreenPanel_Data *data = INST_DATA(cl,obj);
	struct MUI_PubScreenDesc *desc;
	int i;

	for (i=0;;i++)
	{
		int p;

		DoMethod(data->LV_Screens,MUIM_List_GetEntry,i,&desc);
		if (!desc) break;

		printf("N=\"%s\"\n",desc->Name        );
		printf("T=\"%s\"\n",desc->Title       );
		printf("F=\"%s\"\n",desc->Font        );
		printf("B=\"%s\"\n",desc->Background  );
		printf("W=%ld\n"   ,desc->DisplayWidth);
		printf("H=%ld\n"   ,desc->DisplayHeight);
		printf("D=%ld\n"   ,desc->DisplayDepth);
		printf("I=%ld\n"   ,desc->DisplayID   );
		if (desc->OverscanType) printf("OS\n");
		if (desc->AutoScroll  ) printf("AS\n");
		if (desc->NoDrag      ) printf("ND\n");
		if (desc->Exclusive   ) printf("EX\n");
		if (desc->Interleaved ) printf("IN\n");
		if (desc->SysDefault  ) printf("SD\n");
		if (desc->Behind      ) printf("BH\n");
		if (desc->AutoClose   ) printf("AC\n");
		if (desc->CloseGadget ) printf("CG\n");

		printf("PENS=\"");
		for (p=0;p<PSD_NUMSYSPENS;p++)
			printf("%ld:%ld ",p,desc->SystemPens[p]);
		printf("\"\n");

		printf("PALETTE=\"");
		for (p=0;p<PSD_NUMCOLS;p++)
			printf("%ld:%02lx%02lx%02lx ",p<4 ? p : p-8,desc->Palette[p].red>>24,desc->Palette[p].green>>24,desc->Palette[p].blue>>24);
		printf("\"\n");

		printf("\n");

		/*
		if (!desc->Foreign)
		*/
			SavePubScreen(desc);
	}
	return(0);
}
#endif

BOOPSI_DISPATCHER(IPTR, ScreenPanel_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW                       : return(ScreenPanel_New         (cl,obj,(APTR)msg));
		case OM_DISPOSE                   : return(ScreenPanel_Dispose     (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Create      : return(ScreenPanel_Create      (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Copy        : return(ScreenPanel_Copy        (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Delete      : return(ScreenPanel_Delete      (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Edit        : return(ScreenPanel_Edit        (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Finish      : return(ScreenPanel_Finish      (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_CloseWindows: return(ScreenPanel_CloseWindows(cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_SetStates   : return(ScreenPanel_SetStates   (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Open        : return(ScreenPanel_Open        (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Close       : return(ScreenPanel_Close       (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Jump        : return(ScreenPanel_Jump        (cl,obj,(APTR)msg));
		case MUIM_ScreenPanel_Update      : return(ScreenPanel_Update      (cl,obj,(APTR)msg));
		#ifdef MYDEBUG
		case MUIM_ScreenPanel_Foo         : return(ScreenPanel_Foo         (cl,obj,(APTR)msg));
		#endif

		case MUIM_ScreenList_Find:
		{
			struct ScreenPanel_Data *data = INST_DATA(cl,obj);
			return(DoMethodA(data->LV_Screens,msg));
		}
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END



/****************************************************************************/
/* MainWindow class                                                         */
/****************************************************************************/

struct MainWindow_Data
{
	Object *PA_Screens;
};


ULONG MainWindow_Finish(struct IClass *cl,Object *obj,struct MUIP_MainWindow_Finish *msg)
{
	struct MainWindow_Data *data = INST_DATA(cl,obj);
	if (msg->level>=1) DoMethod(data->PA_Screens,MUIM_ScreenList_Save,PSD_FILENAME_USE );
	if (msg->level>=2) DoMethod(data->PA_Screens,MUIM_ScreenList_Save,PSD_FILENAME_SAVE);
	DoMethod((Object *)xget(obj,MUIA_ApplicationObject),MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

	return(0);
}

ULONG MainWindow_About(struct IClass *cl,Object *obj,Msg msg)
{
	struct MainWindow_Data *data = INST_DATA(cl,obj);
	Object *app = (Object *)xget(obj,MUIA_ApplicationObject);
	static const char AboutText[] = "\33b\33cPSI - Public Screen Inspector\33n\n\33cVersion: %s\n\33c%s\n\n\33c%s %ld\n\33cARexx-Port: %s";
	MUI_Request(app,obj,0,NULL,GetStr(MSG_BUTTON_OK),(char *)AboutText,
		((char *)xget(app,MUIA_Application_Version))+10,
		((char *)xget(app,MUIA_Application_Copyright)),
		GetStr(MSG_ABOUT_NUMBEROFSCREENS),
		xget(data->PA_Screens,MUIA_List_Entries),
		((char *)xget(app,MUIA_Application_Base)));
	return(0);
}

VOID IntuiMsgFunc(struct Hook *hook, struct FileRequester *req, struct IntuiMessage *imsg)
{
	if (imsg->Class==IDCMP_REFRESHWINDOW)
		DoMethod(req->fr_UserData,MUIM_Application_CheckRefresh);
}

char *getfilename(Object *win,char *title,BOOL save)
{
	static char buf[512];
	struct FileRequester *req;
	struct Window *w;
	static LONG left=-1,top=-1,width=-1,height=-1;
	Object *app = (Object *)xget(win,MUIA_ApplicationObject);
	char *res = NULL;
	static struct Hook IntuiMsgHook;

	IntuiMsgHook.h_Entry = HookEntry;
	IntuiMsgHook.h_SubEntry = IntuiMsgFunc;

	get(win,MUIA_Window_Window,&w);
	if (left==-1)
	{
		left   = w->LeftEdge+w->BorderLeft+2;
		top    = w->TopEdge+w->BorderTop+2;
		width  = w->Width-w->BorderLeft-w->BorderRight-4;
		height = w->Height-w->BorderTop-w->BorderBottom-4;
	}

	if (req=MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_Window, w,
		ASLFR_TitleText, title,
		ASLFR_InitialLeftEdge, left,
		ASLFR_InitialTopEdge , top,
		ASLFR_InitialWidth   , width,
		ASLFR_InitialHeight  , height,
		ASLFR_InitialDrawer  , "envarc:Zune",
		ASLFR_InitialPattern , "#?.iff",
		ASLFR_DoSaveMode     , save,
		ASLFR_DoPatterns     , TRUE,
		ASLFR_RejectIcons    , TRUE,
		ASLFR_UserData       , app,
		ASLFR_IntuiMsgFunc   , &IntuiMsgHook,
		TAG_DONE))
	{
		set(app,MUIA_Application_Sleep,TRUE);
		if (MUI_AslRequestTags(req,TAG_DONE))
		{
			if (*req->fr_File)
			{
				res = buf;
				stccpy(buf,req->fr_Drawer,sizeof(buf));
				AddPart(buf,req->fr_File,sizeof(buf));
			}
			left   = req->fr_LeftEdge;
			top    = req->fr_TopEdge;
			width  = req->fr_Width;
			height = req->fr_Height;
		}
		MUI_FreeAslRequest(req);
		set(app,MUIA_Application_Sleep,FALSE);
	}
	return(res);
}

ULONG MainWindow_Open(struct IClass *cl,Object *obj,struct MUIP_MainWindow_Open *msg)
{
	struct MainWindow_Data *data = INST_DATA(cl,obj);
	char *title = msg->append ? GetStr(MSG_TITLE_APPEND) : GetStr(MSG_TITLE_OPEN);
	char *name;
	if ((name=getfilename(obj,title,FALSE)) && *name)
	{
		if (!msg->append) DoMethod(data->PA_Screens,MUIM_ScreenPanel_CloseWindows);
		DoMethod(data->PA_Screens,MUIM_ScreenList_Load,name,msg->append ? FALSE : TRUE);
	}
	return(0);
}

ULONG MainWindow_SaveAs(struct IClass *cl,Object *obj,Msg msg)
{
	struct MainWindow_Data *data = INST_DATA(cl,obj);
	char *title = GetStr(MSG_TITLE_SAVE);
	char *name;
	if ((name=getfilename(obj,title,TRUE)) && *name)
	{
		DoMethod(data->PA_Screens,MUIM_ScreenList_Save,name);
	}
	return(0);
}

ULONG MainWindow_Restore(struct IClass *cl,Object *obj,struct MUIP_MainWindow_Restore *msg)
{
	struct MainWindow_Data *data = INST_DATA(cl,obj);
	DoMethod(data->PA_Screens,MUIM_ScreenPanel_CloseWindows);
	DoMethod(data->PA_Screens,MUIM_ScreenList_Load,msg->envarc ? PSD_FILENAME_SAVE : PSD_FILENAME_USE,TRUE);
	return(0);
}

ULONG MainWindow_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	Object *BT_Save;
	Object *BT_Use;
	Object *BT_Cancel;
	Object *PA_Screens;
	Object *strip;

	if (obj = (Object *)DoSuperNewTags(cl,obj,NULL,
			MUIA_Window_Title, "PSI - Public Screen Inspector",
			MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
			MUIA_Window_Menustrip, strip = MUI_MakeObject(MUIO_MenustripNM,MainMenu,0),
			WindowContents, VGroup,
				Child, PA_Screens = NewObject(CL_ScreenPanel->mcc_Class,NULL,TAG_DONE),
				Child, MUI_MakeObject(MUIO_HBar,2),
				Child, HGroup, MUIA_Group_SameSize, TRUE,
					Child, BT_Save = MakeButton(MSG_BUTTON_SAVE),
					Child, HSpace(0),
					Child, BT_Use = MakeButton(MSG_BUTTON_USE),
					Child, HSpace(0),
					Child, BT_Cancel = MakeButton(MSG_BUTTON_CANCEL),
					End,
				End,
			TAG_MORE,msg->ops_AttrList))
	{
		struct MainWindow_Data *data = INST_DATA(cl,obj);

		data->PA_Screens = PA_Screens;

		DoMethod(obj      ,MUIM_Notify,MUIA_Window_CloseRequest,TRUE ,obj,2,MUIM_MainWindow_Finish,0);
		DoMethod(BT_Cancel,MUIM_Notify,MUIA_Pressed            ,FALSE,obj,2,MUIM_MainWindow_Finish,0);
		DoMethod(BT_Use   ,MUIM_Notify,MUIA_Pressed            ,FALSE,obj,2,MUIM_MainWindow_Finish,1);
		DoMethod(BT_Save  ,MUIM_Notify,MUIA_Pressed            ,FALSE,obj,2,MUIM_MainWindow_Finish,2);

		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_ABOUT    ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,1,MUIM_MainWindow_About);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_OPEN     ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_MainWindow_Open,0);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_APPEND   ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_MainWindow_Open,1);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_SAVEAS   ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,1,MUIM_MainWindow_SaveAs);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_QUIT     ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_MainWindow_Finish,0);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_LASTSAVED),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_MainWindow_Restore,1);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_RESTORE  ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_MainWindow_Restore,0);
		DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_MUI      ),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,MUIV_Notify_Application,2,MUIM_Application_OpenConfigWindow,0);

		DoMethod(PA_Screens,MUIM_ScreenList_Load,PSD_FILENAME_USE,TRUE);

		set(BT_Save  ,MUIA_ShortHelp,GetStr(MSG_HELP_SAVE  ));
		set(BT_Use   ,MUIA_ShortHelp,GetStr(MSG_HELP_USE   ));
		set(BT_Cancel,MUIA_ShortHelp,GetStr(MSG_HELP_CANCEL));

		return((ULONG)obj);
	}
	return(0);
}

BOOPSI_DISPATCHER(IPTR, MainWindow_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case OM_NEW                 : return(MainWindow_New    (cl,obj,(APTR)msg));
		case MUIM_MainWindow_Finish : return(MainWindow_Finish (cl,obj,(APTR)msg));
		case MUIM_MainWindow_About  : return(MainWindow_About  (cl,obj,(APTR)msg));
		case MUIM_MainWindow_Open   : return(MainWindow_Open   (cl,obj,(APTR)msg));
		case MUIM_MainWindow_SaveAs : return(MainWindow_SaveAs (cl,obj,(APTR)msg));
		case MUIM_MainWindow_Restore: return(MainWindow_Restore(cl,obj,(APTR)msg));

		case MUIM_ScreenPanel_CloseWindows:
		case MUIM_ScreenPanel_Update:
		case MUIM_ScreenList_Find:
		{
			struct MainWindow_Data *data = INST_DATA(cl,obj);
			return(DoMethodA(data->PA_Screens,msg));
		}
	}
	return(DoSuperMethodA(cl,obj,msg));
}
BOOPSI_DISPATCHER_END


/****************************************************************************/
/* Init/Exit Functions                                                      */
/****************************************************************************/

VOID ExitLibs(VOID)
{
	if (IntuitionBase) CloseLibrary(IntuitionBase);
	if (GfxBase      ) CloseLibrary(GfxBase      );
	if (AslBase      ) CloseLibrary(AslBase      );
	if (UtilityBase  ) CloseLibrary(UtilityBase  );
	if (MUIScreenBase) CloseLibrary(MUIScreenBase);
	if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

BOOL InitLibs(VOID)
{
	MUIMasterBase = OpenLibrary("muimaster.library",11);
	MUIScreenBase = OpenLibrary("muiscreen.library",1);
	UtilityBase   = OpenLibrary("utility.library"  ,36);
	AslBase       = OpenLibrary("asl.library"      ,36);
	GfxBase       = OpenLibrary("graphics.library" ,36);
	IntuitionBase = OpenLibrary("intuition.library",36);

	if (MUIMasterBase && MUIScreenBase && UtilityBase && AslBase && GfxBase && IntuitionBase)
		return(TRUE);

	ExitLibs();
	return(FALSE);
}

VOID ExitClasses(VOID)
{
	if (CL_MainWindow  ) MUI_DeleteCustomClass(CL_MainWindow  );
	if (CL_ScreenPanel ) MUI_DeleteCustomClass(CL_ScreenPanel );
	if (CL_ScreenList  ) MUI_DeleteCustomClass(CL_ScreenList  );
	if (CL_DispIDinfo  ) MUI_DeleteCustomClass(CL_DispIDinfo  );
	if (CL_DispIDlist  ) MUI_DeleteCustomClass(CL_DispIDlist  );
	if (CL_EditPanel   ) MUI_DeleteCustomClass(CL_EditPanel   );
	if (CL_EditWindow  ) MUI_DeleteCustomClass(CL_EditWindow  );
	if (CL_SysPenField ) MUI_DeleteCustomClass(CL_SysPenField );
}

BOOL InitClasses(VOID)
{
	CL_SysPenField  = MUI_CreateCustomClass(NULL,MUIC_Pendisplay,NULL,sizeof(struct SysPenField_Data ),SysPenField_Dispatcher );
	CL_EditWindow   = MUI_CreateCustomClass(NULL,MUIC_Window    ,NULL,sizeof(struct EditWindow_Data  ),EditWindow_Dispatcher  );
	CL_EditPanel    = MUI_CreateCustomClass(NULL,MUIC_Group     ,NULL,sizeof(struct EditPanel_Data   ),EditPanel_Dispatcher   );
	CL_DispIDlist   = MUI_CreateCustomClass(NULL,MUIC_List      ,NULL,sizeof(struct DispIDlist_Data  ),DispIDlist_Dispatcher  );
	CL_DispIDinfo   = MUI_CreateCustomClass(NULL,MUIC_Group     ,NULL,sizeof(struct DispIDinfo_Data  ),DispIDinfo_Dispatcher  );
	CL_ScreenList   = MUI_CreateCustomClass(NULL,MUIC_List      ,NULL,sizeof(struct ScreenList_Data  ),ScreenList_Dispatcher  );
	CL_ScreenPanel  = MUI_CreateCustomClass(NULL,MUIC_Group     ,NULL,sizeof(struct ScreenPanel_Data ),ScreenPanel_Dispatcher );
	CL_MainWindow   = MUI_CreateCustomClass(NULL,MUIC_Window    ,NULL,sizeof(struct MainWindow_Data  ),MainWindow_Dispatcher  );

	if (CL_SysPenField && CL_EditWindow && CL_EditPanel && CL_DispIDlist && CL_DispIDinfo && CL_ScreenList && CL_ScreenPanel && CL_MainWindow)
		return(TRUE);

	ExitClasses();
	return(FALSE);
}


const char CLITemplate[] = "NAME,OPEN/S,CLOSE/S";

const char CLIHelp[] = "\
\n\
Usage: PSI <name> OPEN/CLOSE\n\
<name>: name of (preconfigured) public screen\n\
 OPEN : open this public screen\n\
 CLOSE: close this public screen\n\
";

LONG HandleArgs(Object *mainwin)
{
	struct MUI_PubScreenDesc *desc;
	struct RDArgs *rda,*rdas;
	LONG msg = 0;
	struct CLIArgs
	{
		char *Name;
		LONG Open;
		LONG Close;
	} argarray = { 0,0,0 };

	if (rdas = AllocDosObject(DOS_RDARGS,NULL))
	{
		rdas->RDA_ExtHelp = (char *)CLIHelp;

		if (rda = ReadArgs((char *)CLITemplate,(LONG *)&argarray,rdas))
		{
			if (argarray.Name)
			{
				DoMethod(mainwin,MUIM_ScreenList_Find,argarray.Name,&desc);

				if (argarray.Open)
				{
					if (!desc)
						msg = MSG_CLI_SCREENNOTFOUND;
					else if (MUIS_OpenPubScreen(desc))
						msg = MSG_CLI_SCREENOPENED;
					else
						msg = MSG_CLI_SCREENOPENFAILED;
				}
				else if (argarray.Close)
				{
					if (!desc)
						msg = MSG_CLI_SCREENNOTFOUND;
					else if (MUIS_ClosePubScreen(desc->Name))
						msg = MSG_CLI_SCREENCLOSED;
					else
						msg = MSG_CLI_SCREENCLOSEFAILED;
				}
			}
			else
			{
				if (argarray.Open || argarray.Close)
					msg = MSG_CLI_SYNTAXERROR;
			}
			FreeArgs(rda);
		}
		FreeDosObject(DOS_RDARGS,rdas);
	}
	else
		msg = MSG_CLI_OUTOFMEMORY;

	return(msg);
}



/****************************************************************************/
/* Main Program                                                             */
/****************************************************************************/

int main(int argc,char *argv[])
{
	extern struct WBStartup *_WBenchMsg;
	struct MUIS_InfoClient sic;
	ULONG sigs=0;
	Object *app;
	Object *win;
	int res;
	int msg;

	InitLocale();

	if (InitLibs())
	{
		if (InitClasses())
		{
			app = ApplicationObject,
				MUIA_Application_Title      , "PSI",
				MUIA_Application_Version    , VersionString,
				MUIA_Application_Copyright  , "©1995-97, Stefan Stuntz",
				MUIA_Application_Author     , "Stefan Stuntz",
				MUIA_Application_Description, "Public Screen Inspector",
				MUIA_Application_Base       , "PSI",
				MUIA_Application_Window     , win = NewObject(CL_MainWindow->mcc_Class,NULL,TAG_DONE),
				End;

			if (app)
			{
				if (/*argc==0*/ _WBenchMsg)
					msg = 0;
				else
					msg = HandleArgs(win);

				if (!msg)
				{
					set(win,MUIA_Window_Open,TRUE);

					/* special magic to keep track about public screen open/close status */
					sic.task   = FindTask(NULL);
					sic.sigbit = SIGBREAKF_CTRL_E;
					MUIS_AddInfoClient(&sic);

					while (DoMethod(app,MUIM_Application_NewInput,&sigs) != MUIV_Application_ReturnID_Quit)
					{
						if (sigs)
						{
							sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);

							/* quit when receiving break from console */
							if (sigs & SIGBREAKF_CTRL_C)
								break;

							/* update listview whenever a screen was opened/closed */
							if (sigs & SIGBREAKF_CTRL_E)
								DoMethod(win,MUIM_ScreenPanel_Update);

							/* deiconify & activate on ctrl-f just like the other prefs programs */
							if (sigs & SIGBREAKF_CTRL_F)
							{
								set(app,MUIA_Application_Iconified,FALSE);
								set(win,MUIA_Window_Open,TRUE);
							}
						}
					}

					MUIS_RemInfoClient(&sic);

					DoMethod(win,MUIM_ScreenPanel_CloseWindows);
					set(win,MUIA_Window_Open,FALSE);
				}
				MUI_DisposeObject(app);
			}
			else msg = MSG_CLI_NOAPPLICATION;
			ExitClasses();
		}
		else msg = MSG_CLI_OUTOFMEMORY;
		ExitLibs();
	}
	else msg = MSG_CLI_NOMUIMASTER;

	if (msg)
	{
		char *str = GetStr(msg);
		char *c = strchr(str,'(');
		Write(Output(),str,strlen(str));
		res = c ? atol(c+1) : RETURN_OK;
	}
	else
		res = RETURN_OK;

	ExitLocale();
	/*return(res);*/
	exit(res);
}
