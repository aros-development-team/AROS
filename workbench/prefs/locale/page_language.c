/*
    Copyright  2003-2008, The AROS Development Team. All rights reserved.
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
  Object *clear;
  Object *prefs;

  char  **strings_available;
  char  **strings_preferred;

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
    unsigned int i;
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
/*** Methods ****************************************************************
 *
 */

static void free_strings(struct MUI_LanguageData *data)
{
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

    if(!data->clear || !data->available || !data->preferred)
	return handle_New_error(obj,cl,"ERROR: MakeObject failed\n");

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
    End; 

    if(!data->child)
	return handle_New_error(obj,cl,"ERROR: create child failed\n");

    /* add clear button */
    DoMethod(data->child,OM_ADDMEMBER,(ULONG) data->clear);

    /* add to self */
    DoMethod(obj,OM_ADDMEMBER,(ULONG) data->child);

    /* setup hooks */

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
    ULONG i;

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

