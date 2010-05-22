/*
    Copyright  2003-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

// #define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/locale.h>
#include <prefs/prefhdr.h>
//#define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/codesets.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

#include "global.h"
#include "locale.h"
#include "registertab.h"
#include "page_language.h"

/*** Instance Data **********************************************************/

struct MUI_LanguageData
{
  int nr_languages;
  int nr_preferred;

  Object *child;
  Object *available;
  Object *preferred;
  Object *charset;
  Object *cslist;
  Object *cslistview;
  Object *popup;
  Object *clear;
  Object *prefs;

  char  **strings_available;
  char  **strings_preferred;
  STRPTR *strings_charsets;

  char  *result[10]; /* result array for Gadget2Prefs */
};

struct MUI_CustomClass     *Language_CLASS;

static struct Hook hook_available;
static struct Hook hook_preferred;
static struct Hook hook_clear;

VOID ShowMsg(char *msg);

/*** Helpers *****************************************************************/

STATIC VOID update_language_lists(struct MUI_LanguageData *data) {
    struct LanguageEntry *entry;
    int a;
    int p;

    a=0;
    p=0;
    ForeachNode(&language_list, entry)
    {
	if(entry->preferred) 
	{
	    data->strings_preferred[p]=entry->lve.name;
	    p++;
	}
	else
	{
	    data->strings_available[a]=entry->lve.name;
	    a++;
	}
    }
    data->strings_available[a]=NULL;
    data->strings_preferred[p]=NULL;
}

STATIC VOID init_language_lists(struct MUI_LanguageData *data) {

    struct LanguageEntry *entry;
    int i;

    data->nr_languages=0;
    ForeachNode(&language_list, entry)
    {
	/* D(bug("entry->lve.name: %s\n",entry->lve.name)); */
	entry->preferred=FALSE;
	i=0;
	D(bug("[language class]   language %s\n",entry->lve.name));
	while(i<10 &&           /* max 10 preferred langs, see prefs/locale.h */
	      entry->preferred==FALSE && 
	      localeprefs.lp_PreferredLanguages[i][0])
	{

	    if (Stricmp(localeprefs.lp_PreferredLanguages[i], entry->lve.name) == 0)
	    {
		D(bug("[language class]            %s is preferred\n",
		                                   entry->lve.name));
		entry->preferred=TRUE;
	    }
	    i++;
	}
	data->nr_languages++;
    }

    D(bug("[language class]: nr of languages: %d\n",data->nr_languages));

    data->strings_available=AllocVec(sizeof(char *) * (data->nr_languages+1), MEMF_CLEAR);
    data->strings_preferred=AllocVec(sizeof(char *) * (data->nr_languages+1), MEMF_CLEAR);

    update_language_lists(data);
}

/*** Hooks ******************************************************************
 *
 * hook_func_available
 *   handles double clicks on the available languages
 *
 * hook_func_preferred
 *   handles double clicks on the preferred languages
 *
 * hook_func_clear
 *   handles clearing of preferred languages
 *
 ****************************************************************************/

STATIC VOID func_move_to_selected(char* selstr, struct MUI_LanguageData *data)
{
    struct LanguageEntry *entry;
    unsigned int i = 0;
    char *test;

    D(bug("func_move_to_selected(%s,..)\n",selstr));

    if(selstr) {
	ForeachNode(&language_list, entry)
	{
	    if (stricmp(selstr, entry->lve.name) == 0) 
	    {
		DoMethod(data->preferred,
			 MUIM_List_InsertSingle, entry->lve.name,
			 MUIV_List_Insert_Bottom); 

		entry->preferred=TRUE;
	    }
	}
    }

    /* we could use
     * DoMethod(data->available,MUIM_List_Remove, MUIV_List_Remove_Active);
     * of course, but I also want to be able to use that for
     * the Set method.
     */

    get(data->available,MUIA_List_Entries,&i);
    while(i)
    {
	i--;
	DoMethod(data->available, MUIM_List_GetEntry, i, &test);
	if (stricmp(selstr, test) == 0) 
	{
	    DoMethod(data->available, MUIM_List_Remove, i);
	    i=0;
	}
    }
}

AROS_UFH2(
	void, hook_func_available,
	AROS_UFHA(struct Hook *,    hook,   A0),
	AROS_UFHA(APTR,             obj,    A2)
)
{
	AROS_USERFUNC_INIT

	struct MUI_LanguageData *data= hook->h_Data;
	char  *selstr;

	D(bug("[register class] hook_func_available\n"));

	DoMethod(obj,MUIM_List_GetEntry,
	             MUIV_List_GetEntry_Active,(ULONG) &selstr);

	func_move_to_selected(selstr,data);


	AROS_USERFUNC_EXIT
}

AROS_UFH2(
	void, hook_func_preferred,
	AROS_UFHA(struct Hook *,    hook,   A0),
	AROS_UFHA(APTR,             obj,    A2)
)
{
	AROS_USERFUNC_INIT
	struct MUI_LanguageData *data= hook->h_Data;
	char  *selstr;
	struct LanguageEntry *entry;

	D(bug("[register class] hook_func_preferred\n"));

	DoMethod(obj,MUIM_List_GetEntry,
	             MUIV_List_GetEntry_Active,(ULONG) &selstr);

	if(selstr) {
	    D(bug("move: %s\n",selstr));

	    ForeachNode(&language_list, entry)
	    {
		if (strcmp(selstr, entry->lve.name) == 0) {
		    DoMethod(data->available,
			     MUIM_List_InsertSingle, entry->lve.name, 
			     MUIV_List_Insert_Sorted);
		    entry->preferred=FALSE;
		}
	    }
	    DoMethod(obj,MUIM_List_Remove, MUIV_List_Remove_Active);
	}

	AROS_USERFUNC_EXIT
}

STATIC VOID func_clear(struct MUI_LanguageData *data)
{
    struct LanguageEntry *entry;

    D(bug("[register class] func_clear\n"));

    /* clear it */
    DoMethod(data->preferred,MUIM_List_Clear);

    /* add all old preferred languages again */
    DoMethod(data->available, MUIM_Group_InitChange);
    ForeachNode(&language_list, entry)
    {
	if(entry->preferred) 
	{
	    entry->preferred=FALSE;
	    DoMethod(data->available,
		      MUIM_List_InsertSingle, entry->lve.name, 
		      MUIV_List_Insert_Bottom);
	}
    }
    DoMethod(data->available, MUIM_List_Sort);
    DoMethod(data->available, MUIM_Group_ExitChange);

}

AROS_UFH2(
	void, hook_func_clear,
	AROS_UFHA(struct Hook *,    hook,   A0),
	AROS_UFHA(APTR *,           obj,    A2)
)
{
	AROS_USERFUNC_INIT
	struct MUI_LanguageData *data= hook->h_Data;

	func_clear(data);

	AROS_USERFUNC_EXIT
}

AROS_UFH3(void, charset_popup_to_string,
	  AROS_UFHA(struct Hook*, hook, A0),
	  AROS_UFHA(Object *, list, A2),
	  AROS_UFHA(Object *, string, A1))
{
    AROS_USERFUNC_INIT

    STRPTR listentry;
    STRPTR oldentry;

    GetAttr(MUIA_Text_Contents, string, (IPTR *)&oldentry);
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&listentry);
    if (strcmp(listentry, oldentry)) {
        SetAttrs(string, MUIA_Text_Contents, listentry, TAG_DONE);
	SetAttrs(hook->h_Data, MUIA_PrefsEditor_Changed, TRUE, TAG_DONE);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(ULONG, charset_string_to_popup,
	  AROS_UFHA(struct Hook*, hook, A0),
	  AROS_UFHA(Object *, list, A2),
	  AROS_UFHA(Object *, str, A1))
{
    AROS_USERFUNC_INIT

    STRPTR strtext, listentry;
    LONG index;
    
    GetAttr(MUIA_Text_Contents, str, (IPTR *)&strtext);
    
    for(index = 0; ; index++)
    {
    	DoMethod(list, MUIM_List_GetEntry, index, (IPTR)&listentry);
	
	if (!listentry)
	{
	    set(list, MUIA_List_Active, MUIV_List_Active_Off);
	    break;
	}
	
	if (stricmp(strtext, listentry) == 0)
	{
	    set(list, MUIA_List_Active, index);
	    break;
	}
    }
    
    return TRUE;

    AROS_USERFUNC_EXIT
}

static struct Hook charset_popup_to_string_hook = {
    {NULL, NULL},
    (HOOKFUNC)AROS_ASMSYMNAME(charset_popup_to_string),
    NULL,
    NULL
};

static struct Hook charset_string_to_popup_hook = {
    {NULL, NULL},
    (HOOKFUNC)AROS_ASMSYMNAME(charset_string_to_popup),
    NULL,
    NULL
};

/*** Methods ****************************************************************
 *
 */

static void free_strings(struct MUI_LanguageData *data)
{
    if (data->strings_charsets) {
        CodesetsFreeA(data->strings_charsets, NULL);
	data->strings_charsets = NULL;
    }
    if(data->strings_available)
    {
	FreeVec(data->strings_available);
	data->strings_available=NULL;
    }

    if(data->strings_preferred)
    {
	FreeVec(data->strings_preferred);
	data->strings_preferred=NULL;
    }

}

static Object *handle_New_error(Object *obj, struct IClass *cl, char *error)
{
    struct MUI_LanguageData *data;

    ShowMsg(error);
    D(bug("[Language class] %s\n"));

    if(!obj)
	return NULL;

    data = INST_DATA(cl, obj);

    if(data->clear)
    {
	DisposeObject(data->clear);
	data->clear=NULL;
    }

    if(data->available)
    {
	DisposeObject(data->available);
	data->available=NULL;
    }

    if(data->preferred)
    {
	DisposeObject(data->preferred);
	data->preferred=NULL;
    }

    if(data->child)
    {
	DisposeObject(data->child);
	data->child=NULL;
    }

    if (data->charset) {
        DisposeObject(data->charset);
	data->charset = NULL;
    }
    
    if (data->cslist) {
	DisposeObject(data->cslist);
	data->cslist = NULL;
    }

    if (data->cslistview) {
	DisposeObject(data->cslistview);
	data->cslist = NULL;
    }

    if (data->popup) {
	DisposeObject(data->popup);
	data->popup = NULL;
    }

    free_strings(data);

    CoerceMethod(cl, obj, OM_DISPOSE);
    return NULL;
}

Object *Language_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_LanguageData *data;
    struct TagItem *tstate, *tag;

    D(bug("[language class] Language Class New\n"));

    /* 
     * we create self first and then create the child,
     * so we have self->data available already
     */

    obj = (Object *) DoSuperNewTags(
			  cl, obj, NULL,
			  TAG_DONE
		      );
    
    if (obj == NULL) 
    {
	return handle_New_error(obj,cl,"ERROR: Unable to create object!\n");
    }

    data = INST_DATA(cl, obj);

    tstate=((struct opSet *)msg)->ops_AttrList;
    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
	switch (tag->ti_Tag)
	{
	    case MA_PrefsObject:
		data->prefs = (Object *) tag->ti_Data;
		break;
	}
    }

    if(!data->prefs)
	return handle_New_error(obj,cl,"ERROR: MA_PrefsObject not supplied!\n");

    init_language_lists(data);
    data->strings_charsets = CodesetsSupportedA(NULL);

    data->clear=MUI_MakeObject(MUIO_Button, (ULONG) MSG(MSG_GAD_CLEAR_LANGUAGES));

    data->available= ListviewObject, MUIA_Listview_List, 
			ListObject,
			    InputListFrame,
			    MUIA_List_SourceArray, data->strings_available,
			End,
		    End;

    data->preferred= ListviewObject, MUIA_Listview_List, 
			ListObject,
			    InputListFrame,
			    MUIA_List_SourceArray, data->strings_preferred,
			End,
		    End;

    data->charset = TextObject,
			TextFrame,
			MUIA_Background, MUII_TextBack,
		    End;

    data->cslist = ListObject,
			MUIA_Frame, MUIV_Frame_InputList,
			MUIA_Background, MUII_ListBack,
			MUIA_List_AutoVisible, TRUE,
			MUIA_List_SourceArray, data->strings_charsets,
		    End;

    data->cslistview = ListviewObject,
			MUIA_Listview_List, data->cslist,
		    End;

    charset_popup_to_string_hook.h_Data = data->prefs;

    data->popup = PopobjectObject,
		    MUIA_Popobject_Object, data->cslistview,
		    MUIA_Popobject_StrObjHook, &charset_string_to_popup_hook,
		    MUIA_Popobject_ObjStrHook, &charset_popup_to_string_hook,
		    MUIA_Popstring_Button, PopButton(MUII_PopUp),
		    MUIA_Popstring_String, data->charset,
		End;

    if(!data->clear || !data->available || !data->preferred || !data->charset || !data->cslist ||
       !data->cslistview || !data->popup)
	return handle_New_error(obj,cl,"ERROR: MakeObject failed\n");

    DoMethod(data->cslist, MUIM_List_Sort);
    DoMethod(data->cslist, MUIM_List_InsertSingle, MSG(MSG_NOT_SPECIFIED), MUIV_List_Insert_Top);

    data->child= 
    VGroup,
	Child, 
	HGroup,
    	    Child,
		/* Available Languages */
		VGroup,
		Child,
		    CLabel1(MSG(MSG_GAD_AVAIL_LANGUAGES)), 
		Child,
		    data->available,
		End,
	    Child,
		/* Preferred Languages */
		VGroup,
		Child,
		    CLabel1(MSG(MSG_GAD_PREF_LANGUAGES)), 
		Child,
		    data->preferred,
		End,
	End,
	Child,
	    data->clear,
	Child,
	HGroup,
	    Child,
	        CLabel1(MSG(MSG_CHARACTER_SET)),
	    Child,
		data->popup,
	    End,
    End; 

    if(!data->child)
	return handle_New_error(obj,cl,"ERROR: create child failed\n");

    /* add to self */
    DoMethod(obj,OM_ADDMEMBER,(ULONG) data->child);

    /* setup hooks */
    DoMethod(data->cslistview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->popup, 2, MUIM_Popstring_Close, TRUE);

    /* move hooks */
    hook_available.h_Entry    = (HOOKFUNC) hook_func_available;
    hook_available.h_Data     = data ;
    DoMethod(data->available,MUIM_Notify,MUIA_Listview_DoubleClick,MUIV_EveryTime, (ULONG) data->available,2,MUIM_CallHook,(IPTR) &hook_available);

    hook_preferred.h_Entry    = (HOOKFUNC) hook_func_preferred;
    hook_preferred.h_Data     = data ;
    DoMethod(data->preferred,MUIM_Notify,MUIA_Listview_DoubleClick,MUIV_EveryTime, (ULONG) data->preferred,2,MUIM_CallHook,(IPTR) &hook_preferred);

    /* clear hook */
    hook_clear.h_Entry    = (HOOKFUNC) hook_func_clear;
    hook_clear.h_Data     = data ;
    DoMethod(data->clear,MUIM_Notify,MUIA_Selected,MUIV_EveryTime, (ULONG) data->preferred,2,MUIM_CallHook,(IPTR) &hook_clear);

    /* changed hooks */
    DoMethod(data->preferred, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime, (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->available, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime, (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    
    return obj;
}

/*** Get ******************************************************************/

static IPTR Language_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_LanguageData *data = INST_DATA(cl, obj);
    ULONG rc;
    IPTR i;

    switch (msg->opg_AttrID)
    {
	case MA_Preferred: /* return array of preferred language strings */
	    for(i=0;i<10;i++)
	    {
		DoMethod(data->preferred,
		         MUIM_List_GetEntry,i,
			 &data->result[i]);
	    }
	    rc = (ULONG) data->result;
	    break;
	case MA_CharacterSet:
	    GetAttr(MUIA_List_Active, data->cslist, (IPTR *)&i);
	    D(bug("[Language::Get] Active character set entry is %d\n", i));
	    if ((i == 0) || (i == MUIV_List_Active_Off))
	    	*msg->opg_Storage = NULL;
	    else
	        GetAttr(MUIA_Text_Contents, data->charset, msg->opg_Storage);
	    return TRUE;
	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    *msg->opg_Storage = rc;
    return TRUE;
}

/*** Set ******************************************************************/
static IPTR Language_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_LanguageData *data = INST_DATA(cl, obj);
    struct TagItem *tstate, *tag;
    ULONG update;
    ULONG i;

    tstate = msg->ops_AttrList;
    update = FALSE;

    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
	switch (tag->ti_Tag)
	{
	    case MA_Preferred:
		/* clear preferred */
		func_clear(data);
		for(i=0; i<10; i++)
		{
		    if(localeprefs.lp_PreferredLanguages[i][0] != '\0')
		    {
			func_move_to_selected(localeprefs.lp_PreferredLanguages[i],data);
		    }
		}

		update=TRUE;
		break;
	    case MA_CharacterSet:
	    {
	        char *charset = (char *)tag->ti_Data;
		
		if (!charset || !*charset)
		    DoMethod(data->cslist, MUIM_List_GetEntry, 0, (IPTR *)&charset);
	        SetAttrs(data->charset, MUIA_Text_Contents, charset, TAG_DONE);
	    }
	    break;

	    default:
		return DoSuperMethodA(cl, obj, (Msg)msg);
	}
    }

    if(update)
    {
	MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return TRUE;
}

static IPTR Language_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_LanguageData *data = INST_DATA(cl, obj);

    D(bug("Language_Dispose\n"));


    free_strings(data);

    return DoSuperMethodA(cl, obj, msg);
}

/*** Setup ******************************************************************/

BOOPSI_DISPATCHER(IPTR, Language_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return (IPTR) Language_New(cl, obj, (struct opSet *)msg);
	case OM_GET: return        Language_Get(cl, obj, (struct opGet *)msg);
	case OM_SET: return        Language_Set(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return    Language_Dispose(cl, obj, msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Language_desc = 
{ 
    "Language",
    MUIC_Group,
    sizeof(struct MUI_LanguageData),
    (void*)Language_Dispatcher 
};

void InitLanguage() 
{
    Language_CLASS=MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct MUI_LanguageData),(APTR) &Language_Dispatcher);

}

void CleanLanguage() 
{
    if(Language_CLASS)
    {
	MUI_DeleteCustomClass(Language_CLASS);
	Language_CLASS=NULL;
    }
}

