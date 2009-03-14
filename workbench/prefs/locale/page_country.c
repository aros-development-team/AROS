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
#include "page_country.h"
#include "registertab.h"

/*** String Data ************************************************************/

/*** Instance Data **********************************************************/

typedef struct MUI_CountryPic
{
  char          *strings_country;
  Object        *pic;
  Object        *list_pic;
} MUI_CountryPic;

struct MUI_CountryData
{
  Object         *me;
  Object         *child;
  Object         *prefs;
  ULONG           active;
  unsigned int    nr_countries;
  unsigned int    filled;
  MUI_CountryPic **pic;
};

struct MUI_CustomClass     *Country_CLASS;

/*** Helpers *****************************************************************/

#define MAX_COUNTRY_LEN 256

/*******************************
 * prepare for fill_country_list
 *******************************/

STATIC VOID init_country_list(struct MUI_CountryData *data) {

    struct CountryEntry *entry;
    BPTR                 lock;
    int                  i;
    char                 filename[MAX_COUNTRY_LEN];

    if(data->filled)
    {
	return;
    }

    data->filled=1;

    /* count countries */
    data->nr_countries=0;
    ForeachNode(&country_list, entry)
    {
	data->nr_countries++;
    }

    D(bug("[country class] nr of countries: %d\n",data->nr_countries));

    data->pic=AllocVec(sizeof(struct MUI_CountryPic *) * (data->nr_countries+1),MEMF_CLEAR);

    i=0;
    ForeachNode(&country_list, entry)
    {
	data->pic[i]=AllocVec(sizeof(struct MUI_CountryPic),MEMF_CLEAR);

	snprintf(filename,MAX_COUNTRY_LEN-1,
	            "LOCALE:Flags/Countries/%s",entry->lve.realname);

	if ((lock = Lock(filename, ACCESS_READ)) != NULL) {
	    data->pic[i]->pic=(APTR) MUI_NewObject("Dtpic.mui",
	                               MUIA_Dtpic_Name,(ULONG) filename,
				       TAG_DONE);

	    UnLock(lock);
	    if(!data->pic[i]->pic) 
	    {
		D(bug("[country class] Picture %s failed to load\n",filename));
	    }
	    else
	    {
		D(bug("[country class] Picture %s loaded: %lx\n",filename,data->pic[i]->pic));
	    }
	}

	if(data->pic[i]->pic)
	{
	    data->pic[i]->list_pic=(Object *) DoMethod(data->child,
	       		  MUIM_List_CreateImage,data->pic[i]->pic, 
	    		  0);
	}
	else
	{
	    data->pic[i]->list_pic=NULL; /* should be ok */
	    D(bug("ERROR: [country class] data->pic[%d]->pic is NULL!\n",i));
	}
	data->pic[i]->strings_country=AllocVec(sizeof(char) * MAX_COUNTRY_LEN,MEMF_CLEAR);
	snprintf(data->pic[i]->strings_country,
	         MAX_COUNTRY_LEN,"\33O[%08lx] %s",
		 (long unsigned int) data->pic[i]->list_pic,
		 entry->lve.name); /* 64-bit !? */

	DoMethod(data->child,
		      MUIM_List_InsertSingle, data->pic[i]->strings_country,
		      MUIV_List_Insert_Bottom);

	i++;
    }

    /* we did remember that */
    nnset(data->child, MUIA_List_Active, data->active);
}

/*** Methods ****************************************************************
 *
 */

Object *Country_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CountryData *data;
    struct TagItem *tstate, *tag;

    D(bug("[country class] Country Class New\n"));

    /* 
     * country flags are at the moment 17 pixels high
     * MUIA_List_MinLineHeight, 18 leaves at least one
     * pixel space between the images
     * If images ever get bigger, this should be
     * no problem.
     */

    obj = (Object *) DoSuperNewTags(
			  cl, obj, NULL,
			  InputListFrame,
			  MUIA_List_MinLineHeight, 18,
			  TAG_DONE
		      );
    
    if (obj == NULL) 
    {
	D(bug("ERROR: [country class] DoSuperNewTags failed!\n"));
	return NULL;
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

    data->filled=0;
    data->child=obj;

    /* changed hook */
    DoMethod(obj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

    data->me=obj;
    return obj;
}

/**************************************************************
 * seems like you can only call CreateImage after the list
 * is setup, which is quite late. So we do it, after
 * the list is shown. Nice? Not really. Maybe I did
 * not understand, why this needs to be.
 **************************************************************/
static IPTR Country_Fill(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_CountryData *data = INST_DATA(cl, obj);

    IPTR         ret;

    ret=DoSuperMethodA(cl, obj, (Msg)msg);

    init_country_list(data);

    return ret;
}

/******************************************
 * According to the MUI docs, you should
 * call DeleteImage and Dispose during
 * Cleanup. 
 ******************************************/
static IPTR Country_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_CountryData *data = INST_DATA(cl, obj);
    unsigned int i;

    D(bug("[country class] Country_Cleanup\n"));

    if(data->filled)
    {
	for(i=0; i<data->nr_countries; i++)
	{
	    if(data->pic[i])
	    {
		if(data->pic[i]->list_pic)
		{
		    DoMethod(data->child,
			      MUIM_List_DeleteImage,data->pic[i]->list_pic);
		    data->pic[i]->list_pic=NULL;
		}
		if(data->pic[i]->pic)
		{
		    DoMethod(data->pic[i]->pic,OM_DISPOSE);
		    data->pic[i]->pic=NULL;
		}
	    }
	}
    }
    else
    {
	D(bug("[country class] Country_Cleanup and !data->filled!?\n"));
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

static IPTR Country_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_CountryData *data = INST_DATA(cl, obj);
    unsigned int i;

    D(bug("[country class] Country_Dispose\n"));

    if(!data->pic)
	return DoSuperMethodA(cl, obj, msg);

    for(i=0;i<data->nr_countries;i++)
    {
	if(data->pic[i])
	{
	    if(data->pic[i]->strings_country)
	    {
		FreeVec(data->pic[i]->strings_country);
	    }
	    FreeVec(data->pic[i]);
	}
    }
    FreeVec(data->pic);
    return DoSuperMethodA(cl, obj, msg);
}

/*** Get ******************************************************************/
static IPTR Country_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_CountryData *data = INST_DATA(cl, obj);
    struct CountryEntry    *entry;
    ULONG rc;
    ULONG nr;
    ULONG i;


    switch (msg->opg_AttrID)
    {
	case MA_CountryName:
	    get(data->child, MUIA_List_Active, &nr);
	    rc = -1;
	    i  = 0;
	    ForeachNode(&country_list, entry) {
		if(i==nr)
		{
		    rc = (ULONG)entry->lve.realname;
		}
		i++;
	    }

	    if(rc == -1)
	    {
		*msg->opg_Storage = 0;
		return FALSE; 
	    }
	    break;

	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    *msg->opg_Storage = rc;
    return TRUE;
}

/*** Set ******************************************************************/
static IPTR Country_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_CountryData *data = INST_DATA(cl, obj);
    struct TagItem *tstate, *tag;
    struct CountryEntry    *entry;
    ULONG update;
    ULONG nr;
    ULONG i;

    tstate = msg->ops_AttrList;
    update = FALSE;

    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
	switch (tag->ti_Tag)
	{
	    case MA_CountryName:

		nr = -1;
		i  = 0;
		ForeachNode(&country_list, entry) {
		    if(!stricmp(entry->lve.realname, (STRPTR)tag->ti_Data))
		    {
			nr=i;
		    }
		    i++;
		}

		if(nr < 0)
		{
		    D(bug("ERROR: [country class] could not find >%s< !?\n",tag->ti_Data));
		}
		else
		{
		    if(data->filled)
		    {
			nnset(data->child, MUIA_List_Active, nr);
			update=TRUE;
		    }
    		    else
    		    {
    			/* remember */
    			data->active=nr;
		    }
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

/*** Setup ******************************************************************/

BOOPSI_DISPATCHER(IPTR, Country_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:       return (IPTR) Country_New(cl, obj, (struct opSet *)msg);
    	case MUIM_Show:    return Country_Fill(cl, obj, (struct MUIP_Show *)msg);
	case OM_GET:       return Country_Get(cl, obj, (APTR)msg);
	case OM_SET:       return Country_Set(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Country_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	case OM_DISPOSE:   return Country_Dispose(cl, obj, msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUIP_Country_desc = 
{ 
    "Country",
    MUIC_List,
    sizeof(struct MUI_CountryData),
    (void*)Country_Dispatcher 
};

void InitCountry() 
{
    Country_CLASS=MUI_CreateCustomClass(NULL,MUIC_List,NULL,sizeof(struct MUI_CountryData), &Country_Dispatcher);
}

void CleanCountry() 
{
    if(Country_CLASS)
    {
	MUI_DeleteCustomClass(Country_CLASS);
	Country_CLASS=NULL;
    }
}
