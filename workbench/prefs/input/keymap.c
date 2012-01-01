#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "keymap.h"
#include "prefs.h"

struct Keymap_DATA
{
    struct ListviewEntry *keymap;
    char buf[KEYMAP_NAME_LEN + KEYMAP_FLAG_LEN];
};

static IPTR Keymap__OM_NEW(Class  *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        TextFrame,
        MUIA_Background, MUII_TextBack,
    	MUIA_Group_Horiz, TRUE,
    	TAG_MORE, msg->ops_AttrList
    );

    return (IPTR)obj;
}

static IPTR Keymap__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Keymap_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags  = msg->ops_AttrList;
    struct TagItem       *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch(tag->ti_Tag)
        {
        case MUIA_Keymap_Keymap:
            data->keymap = (struct ListviewEntry *)tag->ti_Data;
            if (data->keymap)
	    {
	        char *dst;

	    	if (data->keymap->displayflag)
	    	{
	    	    unsigned int len = strlen(data->keymap->displayflag);
	    	    
	    	    CopyMem(data->keymap->displayflag, data->buf, len);
	    	    dst = data->buf + len;
	    	    *dst++ = ' ';
	    	 }
	    	 else
	    	     dst = data->buf;

		strcpy(dst, data->keymap->node.ln_Name);
	    }
	    else
	    	data->buf[0] = 0;

	    SET(obj, MUIA_Text_Contents, data->buf);
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Keymap__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Keymap_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Keymap_Keymap:
        *msg->opg_Storage = (IPTR)data->keymap;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

ZUNE_CUSTOMCLASS_3
(
    Keymap, NULL, MUIC_Text, NULL,
    OM_NEW, struct opSet *,
    OM_SET, struct opSet *,
    OM_GET, struct opGet *
);
