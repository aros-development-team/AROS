/* 
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

/* This is based on muimaster/class/text.c (first string version)
 * and on rom/intuition/str*.c
 */

#define MUIMASTER_YES_INLINE_STDARG

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <exec/types.h>
#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#ifdef __AROS__
#include <devices/rawkeycodes.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"
#include "penspec.h"
#include "imspec.h"
#include "clipboard.h"
#include "string.h"

//#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

struct MUI_StringData {
    ULONG        msd_Flags;
    CONST_STRPTR msd_Accept; /* MUIA_String_Accept */
    CONST_STRPTR msd_Reject; /* MUIA_String_Reject */
    LONG         msd_Align;
    struct Hook *msd_EditHook;
    Object      *msd_AttachedList;
    LONG         msd_RedrawReason;

    ULONG        msd_useSecret;
    /* Fields mostly ripped from rom/intuition/strgadgets.c */
    STRPTR Buffer;      /* char container                   */
    ULONG  BufferSize;  /* memory allocated                 */
    STRPTR SecBuffer;   /* Buffer for secret string         */
    ULONG  NumChars;    /* string length                    */
    ULONG  BufferPos;   /* cursor (insert/delete) position  */
    ULONG  MarkPos; 	/* cursor text marking start pos    */
    LONG   DispPos;     /* leftmost visible char            */
    ULONG  DispCount;   /* number of visible chars          */
    
    ULONG  OldClick_Sec;
    ULONG  OldClick_Micro;
    ULONG  NewClick_Sec;
    ULONG  NewClick_Micro;
    WORD   MultiClick;
    
    WORD   Columns;
    
    struct MUI_EventHandlerNode ehn;
    struct MUI_PenSpec_intern inactive_text;
    struct MUI_PenSpec_intern active_text;
    struct MUI_PenSpec_intern marked_text;
    struct MUI_PenSpec_intern marked_bg;
    struct MUI_PenSpec_intern cursor;

    BOOL is_active;
};

#define MSDF_ADVANCEONCR    (1<<0)
#define MSDF_LONELYEDITHOOK (1<<1)
#define MSDF_MARKING	    (1<<2)
#define MSDF_KEYMARKING	    (1<<3)
#define MSDF_NOINPUT	    (1<<4)
#define MSDF_STAYACTIVE     (1<<5)

enum {
    NO_REASON = 0,
    WENT_ACTIVE = 1,
    WENT_INACTIVE,
    DO_CURSOR_LEFT = 3,
    DO_CURSOR_RIGHT,
    DO_DELETE = 5,
    DO_BACKSPACE,
    DO_ADDCHAR = 7,
    NEW_CONTENTS,
    MOVE_CURSOR,
    DO_UNKNOWN,
};


/**************************************************************************
 Buffer_SetNewContents
 Allocate memory for buffer
**************************************************************************/
static BOOL Buffer_Alloc (struct MUI_StringData *data)
{
    data->Buffer = (STRPTR)AllocVec(data->BufferSize * sizeof(char), MEMF_ANY);
    if (NULL == data->Buffer)
    {
	bug("MUIC_String: Can't allocate %ld bytes for buffer1\n",
	    data->BufferSize * sizeof(char));
	return FALSE;
    }
    if (data->msd_useSecret)
    {
        data->SecBuffer = (STRPTR)AllocVec(data->BufferSize * sizeof(char), MEMF_ANY);
        if (NULL == data->SecBuffer)
        {
	    bug("MUIC_String: Can't allocate %ld bytes for buffer2\n",
	        data->BufferSize * sizeof(char));
	    FreeVec(data->Buffer);
            data->Buffer = NULL;
            return FALSE;
        }
    }
    return TRUE;
}

/**************************************************************************
 Buffer_SetNewContents
 Initialize buffer with a string, replace former content if any
**************************************************************************/
static BOOL Buffer_SetNewContents (struct MUI_StringData *data, CONST_STRPTR str)
{
    if (NULL == data->Buffer)
	return FALSE;

    if (NULL == str)
    {
	data->Buffer[0] = 0;
        if (data->msd_useSecret) data->SecBuffer[0] = 0;
	data->NumChars = 0;
    }
    else
    {
	data->NumChars = strlen(str);
	if (data->NumChars >= data->BufferSize)
	    data->NumChars = data->BufferSize - 1;

        if (data->msd_useSecret) 
        {
            strncpy(data->SecBuffer, str, data->BufferSize);
            int i;
            for (i=0;i<data->BufferSize;i++) data->Buffer[i]=0x78;
            data->SecBuffer[data->BufferSize - 1] = 0;
        }
        else 
        {
            strncpy(data->Buffer, str, data->BufferSize);
        }
        data->Buffer[data->BufferSize - 1] = 0;

    }
    data->BufferPos = data->NumChars;
    data->DispPos = 0;
    return TRUE;
}

/**************************************************************************
 Buffer_AddChar
 Add a char on cursor position
**************************************************************************/
static BOOL Buffer_AddChar (struct MUI_StringData *data, unsigned char code)
{
    STRPTR dst;

    if (data->Buffer == NULL)
	return FALSE;

    if (data->NumChars + 1 >= data->BufferSize)
	return FALSE;

    if (data->msd_useSecret)
    {
        dst = &data->SecBuffer[data->BufferPos + 1];

        memmove(dst, &data->SecBuffer[data->BufferPos],
	        data->NumChars - data->BufferPos);

        data->Buffer[data->NumChars]=0x78;
        data->Buffer[data->NumChars + 1]=0;
    }
    else
    {
        dst = &data->Buffer[data->BufferPos + 1];

        memmove(dst, &data->Buffer[data->BufferPos],
	        data->NumChars - data->BufferPos);

    }
    
    dst[data->NumChars - data->BufferPos] = 0;
    dst[-1] = code;

    data->BufferPos++;
    data->NumChars++;
    return TRUE;
}

static WORD Buffer_GetWordStartIndex(struct MUI_StringData *data, WORD startindex)
{
    WORD index = startindex;
    
    while(index > 0)
    {	
    	if (data->Buffer[index - 1] == ' ')
	{
	    break;
	}
	index--;
    }
    
    return index;
}

static WORD Buffer_GetWordEndIndex(struct MUI_StringData *data, WORD startindex)
{
    WORD index = startindex;
    
    while(index < data->NumChars)
    {	
    	if (data->Buffer[index] == ' ')
	{
	    break;
	}
	index++;
    }
    
    return index;
}

static WORD Buffer_GetPrevWordIndex(struct MUI_StringData *data, WORD startindex)
{
    WORD index = startindex;
    
    while(index > 0)
    {
    	index--;
	
    	if ((index == 0) ||
	    ((data->Buffer[index - 1] == ' ') &&
	     (data->Buffer[index] != ' ')))
	{
	    break;
	}
	    
    }
    
    return index;
}

static WORD Buffer_GetSuccWordIndex(struct MUI_StringData *data, WORD startindex)
{
    WORD index = startindex;
    
    while(index < data->NumChars)
    {
    	index++;
	
	if ((index == data->NumChars) ||
	    ((data->Buffer[index - 1] == ' ') &&
	     (data->Buffer[index] != ' ')))
	{
	    break;
	}
    }
    
    return index;
}

static BOOL Buffer_GetMarkedRange(struct MUI_StringData *data, WORD *start, WORD *stop)
{
    WORD markstart = data->MarkPos;
    WORD markstop  = data->BufferPos;
    
    markstart = MIN(markstart, data->NumChars);
    markstart = MAX(markstart, 0);
    
    markstop  = MIN(markstop, data->NumChars);
    markstop  = MAX(markstop, 0);
 
    if (markstart > markstop)
    {
    	markstart ^= markstop;
    	markstop  ^= markstart;
    	markstart ^= markstop;	
    }
 
    switch(data->MultiClick)
    {
    	case 0:
	    /* char select */
	    break;
	    
	case 1:
	    /* word select */
	    markstart = Buffer_GetWordStartIndex(data, markstart);
	    markstop = Buffer_GetWordEndIndex(data, markstop);
	    break;
	    
	default:
	    /* select all */
	    markstart = 0;
	    markstop = data->NumChars;
	    break;
	    
    }
    
    if (markstart == markstop) return FALSE;
        
    if (start) *start = markstart;
    if (stop) *stop = markstop;

    //kprintf("Buffer_GetMarkedRange: returning %d .. %d\n", markstart, markstop);
    
    return TRUE;
}

static BOOL Buffer_AnythingMarked(struct MUI_StringData *data)
{
    if (!(data->msd_Flags & MSDF_MARKING)) return FALSE;

    return Buffer_GetMarkedRange(data, NULL, NULL);
}

static BOOL Buffer_KillMarked(struct MUI_StringData *data)
{
    WORD markstart = data->MarkPos;
    WORD markstop  = data->BufferPos;
    WORD marklen;
    
    //kprintf("\nBuffer_KillMarked 1  markpos %d  bufferpos %d  numchars %d\n", markstart, markstop, data->NumChars);    

    if (!(data->msd_Flags & MSDF_MARKING)) return FALSE;
    
    data->msd_Flags &= ~MSDF_MARKING;
    
    if (!Buffer_GetMarkedRange(data, &markstart, &markstop)) return FALSE;
    
    //kprintf("Buffer_KillMarked 2  markstart %d  markstop %d\n", markstart, markstop);    
    
    if (markstart > markstop)
    {
    	markstart ^= markstop;
    	markstop  ^= markstart;
    	markstart ^= markstop;	
    }
    
    marklen = markstop - markstart;

    //kprintf("Buffer_KillMarked: markstart %d  markstop %d\n", markstart, markstop);
       
    memmove(&data->Buffer[markstart],
    	    &data->Buffer[markstart + marklen], data->NumChars - markstart - marklen + 1);
    
    data->NumChars -= marklen;
    data->BufferPos = markstart;
    
    return TRUE;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR String_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_StringData *data;
    struct TagItem *tags,*tag;
    CONST_STRPTR str = NULL;
    char    	 integerbuf[20];
    
    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
				   /*  MUIA_FillArea, TRUE, */
				   TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);
    data->msd_useSecret = FALSE;
    data->msd_Align = MUIV_String_Format_Left;
    data->BufferSize = 80;
    data->Columns = -1;
    
    Buffer_SetNewContents(data, ""); /* <-- isnt this pointless? */

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case  MUIA_String_Accept:
		data->msd_Accept = (CONST_STRPTR)tag->ti_Data;
		break;

	    case  MUIA_String_Reject:
		data->msd_Reject = (CONST_STRPTR)tag->ti_Data;
		break;

            case MUIA_String_AdvanceOnCR:
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_ADVANCEONCR);
		break;

            case MUIA_String_AttachedList:
		data->msd_AttachedList = (Object *)tag->ti_Data;
		break;

            case MUIA_String_Secret:
		data->msd_useSecret = (BOOL)tag->ti_Data;
		break;

            case MUIA_String_Contents:
		str = (CONST_STRPTR)tag->ti_Data;
		break;

            case MUIA_String_EditHook:
		data->msd_EditHook = (struct Hook *)tag->ti_Data;
		break;

	    case MUIA_String_Format:
		data->msd_Align = (LONG)tag->ti_Data;
		break;

	    case MUIA_String_Integer:
		snprintf(integerbuf, 19, "%ld", tag->ti_Data);
		str = integerbuf;
		break;

	    case MUIA_String_LonelyEditHook:
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_LONELYEDITHOOK);
		break;

	    case MUIA_String_MaxLen:
		data->BufferSize = tag->ti_Data;
		if (data->BufferSize < 1)
		    data->BufferSize = 1;
		break;
		
	    case MUIA_String_Columns: /* BetterString */
	    	data->Columns = (WORD)tag->ti_Data;
		break;
		
	    case MUIA_String_NoInput: /* BetterString */
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_NOINPUT);
		break;

	    case MUIA_String_StayActive: /* BetterString */
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_STAYACTIVE);
		break;
	    	
	}
    }

    if (Buffer_Alloc(data))
    {
        Buffer_SetNewContents(data, str);
    }

    if (NULL == data->Buffer)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return 0;
    }

    D(bug("String_New(%p)\n",obj));

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    CurrentTime(&data->OldClick_Sec, &data->OldClick_Micro);

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR String_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    if (data->Buffer)
	FreeVec(data->Buffer);

    if (data->SecBuffer)
	FreeVec(data->SecBuffer);

    D(bug("String_Dispose %p\n", obj));

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR String_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    struct TagItem      *tags = msg->ops_AttrList;
    struct TagItem      *tag;

    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_String_Contents:
		Buffer_SetNewContents(data, (STRPTR)tag->ti_Data);
		data->msd_RedrawReason = NEW_CONTENTS;
		data->msd_Flags &= ~(MSDF_MARKING | MSDF_KEYMARKING);
		MUI_Redraw(obj, MADF_DRAWOBJECT);
		break;

	    case MUIA_String_Accept:
		data->msd_Accept = (CONST_STRPTR)tag->ti_Data;
		break;

	    case MUIA_String_Reject:
		data->msd_Reject = (CONST_STRPTR)tag->ti_Data;
		break;

            case MUIA_String_AttachedList:
		data->msd_AttachedList = (Object *)tag->ti_Data;
		break;
	    
            case MUIA_String_Integer:
	    {
		char buf[20];

		snprintf(buf, 19, "%ld", tag->ti_Data);
		set(obj, MUIA_String_Contents, buf);
	    }
	    break;
	    
	    case MUIA_String_AdvanceOnCR:
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_ADVANCEONCR);
		break;

	    case MUIA_String_BufferPos:
		data->BufferPos = (ULONG)tag->ti_Data;
		data->msd_Flags &= ~MSDF_MARKING;
		data->msd_RedrawReason = MOVE_CURSOR;
	    	MUI_Redraw(obj, MADF_DRAWUPDATE);
		break;

	    case MUIA_String_DisplayPos:
		data->BufferPos = (ULONG)tag->ti_Data;
		data->DispPos = data->BufferPos; // move both pos
		data->msd_Flags &= ~MSDF_MARKING;
		data->msd_RedrawReason = MOVE_CURSOR;
	    	MUI_Redraw(obj, MADF_DRAWUPDATE);
		break;

	    case MUIA_String_NoInput: /* BetterString */
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_NOINPUT);
		break;

	    case MUIA_String_StayActive: /* BetterString */
		_handle_bool_tag(data->msd_Flags, tag->ti_Data, MSDF_STAYACTIVE);
		break;
		
	    case MUIA_String_SelectSize: /* BetterString */
	    	#warning "TODO: Implement OM_SET(MUIA_String_SelectSize)!"
	    	break;

	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR String_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch(msg->opg_AttrID)
    {
	case MUIA_String_Contents:
            if (data->msd_useSecret) STORE = (IPTR) data->SecBuffer;
            else STORE = (IPTR) data->Buffer;
	    return 1;
        
	case MUIA_String_Secret:
	    STORE = (IPTR) data->msd_useSecret;
	    return 1;
        
        case MUIA_String_Accept:
	    STORE = (IPTR) data->msd_Accept;
	    return 1;
        
        case MUIA_String_AttachedList:
            STORE = (IPTR) data->msd_AttachedList;
            return 1;

	case MUIA_String_Integer:
	{
	    STRPTR buf = NULL;
	    STORE = 0;
	    get(obj, MUIA_String_Contents, &buf);
	    if (NULL != buf)
	    {
		LONG val = 0;
		StrToLong(buf, &val);
		STORE = val;
	    }
	    return 1;
	}
	
	case MUIA_String_MaxLen:
	    STORE = (IPTR) data->BufferSize;
	    return 1;

	case MUIA_String_AdvanceOnCR:
	    STORE = (data->msd_Flags & MSDF_ADVANCEONCR) ? TRUE : FALSE;
	    return 1;

	case MUIA_String_BufferPos:
	    STORE = data->BufferPos;
	    return 1;

	case MUIA_String_DisplayPos:
	    STORE = data->DispPos;
	    return 1;
	    
    	case MUIA_String_NoInput: /* BetterString */
	    STORE = (data->msd_Flags & MSDF_NOINPUT) ? TRUE : FALSE;
	    return 1;
	    
	case MUIA_String_StayActive: /* BetterString */
	    STORE = (data->msd_Flags & MSDF_STAYACTIVE) ? TRUE : FALSE;
	    return 1;
	
	case MUIA_String_SelectSize: /* BetterString */
	    if (data->msd_Flags & MSDF_MARKING)
	    {
	    	WORD markstart, markstop;
		
    	    	if (Buffer_GetMarkedRange(data, &markstart, &markstop))
    	    	{
		    LONG size = markstop - markstart;
		    
		    if (data->MarkPos < data->BufferPos) size = -size;
		    
		    STORE = (IPTR)size;
		    
		    return 1;
		}
		
	    }

	    STORE = 0;
	    return 1;

    }
    
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR String_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    if (0 == DoSuperMethodA(cl, obj, (Msg) msg))
	return FALSE;

    data->is_active = FALSE;
    set(obj, MUIA_Background,
	(IPTR)muiGlobalInfo(obj)->mgi_Prefs->string_bg_inactive);

    zune_pen_spec_to_intern(
	(const struct MUI_PenSpec *)muiGlobalInfo(obj)->mgi_Prefs->string_text_inactive,
	&data->inactive_text);
    zune_penspec_setup(&data->inactive_text, muiRenderInfo(obj));

    zune_pen_spec_to_intern(
	(const struct MUI_PenSpec *)muiGlobalInfo(obj)->mgi_Prefs->string_text_active,
	&data->active_text);
    zune_penspec_setup(&data->active_text, muiRenderInfo(obj));

    zune_pen_spec_to_intern(
	(const struct MUI_PenSpec *)muiGlobalInfo(obj)->mgi_Prefs->string_text_marked,
	&data->marked_text);
    zune_penspec_setup(&data->marked_text, muiRenderInfo(obj));

    zune_pen_spec_to_intern(
	(const struct MUI_PenSpec *)muiGlobalInfo(obj)->mgi_Prefs->string_bg_marked,
	&data->marked_bg);
    zune_penspec_setup(&data->marked_bg, muiRenderInfo(obj));

    zune_pen_spec_to_intern(
	(const struct MUI_PenSpec *)muiGlobalInfo(obj)->mgi_Prefs->string_cursor,
	&data->cursor);
    zune_penspec_setup(&data->cursor, muiRenderInfo(obj));

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR String_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    D(bug("String_Cleanup %p\n", obj));
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

    zune_penspec_cleanup(&data->inactive_text);
    zune_penspec_cleanup(&data->active_text);
    zune_penspec_cleanup(&data->marked_text);
    zune_penspec_cleanup(&data->marked_bg);
    zune_penspec_cleanup(&data->cursor);

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR String_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->Columns >= 0)
    {
    	msg->MinMaxInfo->MinWidth += _font(obj)->tf_XSize * data->Columns;
    	msg->MinMaxInfo->DefWidth += _font(obj)->tf_XSize * data->Columns;
    	msg->MinMaxInfo->MaxWidth += _font(obj)->tf_XSize * data->Columns;
    }
    else
    {
    	msg->MinMaxInfo->MinWidth += _font(obj)->tf_XSize * 4;
    	msg->MinMaxInfo->DefWidth += _font(obj)->tf_XSize * 12;
    	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    }
    
    msg->MinMaxInfo->MinHeight += _font(obj)->tf_YSize;
    msg->MinMaxInfo->DefHeight += _font(obj)->tf_YSize;
    msg->MinMaxInfo->MaxHeight += _font(obj)->tf_YSize;
    
/*      D(bug("String_AskMinMax(%p): Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n", obj, */
/*  	  msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight, */
/*  	  msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight, */
/*  	  msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight)); */

    return TRUE;
}


static WORD MaxDispPos(struct IClass *cl, Object *obj)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    WORD 		numfit, max_disppos, numchars;
    struct TextExtent 	te;
    BOOL 		cursor_at_end;
    
    cursor_at_end = (data->BufferPos == data->NumChars);

/*      D(bug("MaxDispPos(current length: %d, bufferpos=%d)\n", */
/*  	  data->NumChars, data->BufferPos)); */
    
/*      D(bug("cursor_at_end: %d\n", cursor_at_end)); */
    
    if (cursor_at_end) /* Cursor at end of string ? */
    {
/*      	D(bug("Making cursor last char\n")); */
    	numchars = data->NumChars + 1; /* Take cursor into account */

/*    	This has already been done by UpdateDisp() which called us
	strinfo->Buffer[strinfo->NumChars] = 0x20; 

*/
    }
    else
    {
    	numchars = data->NumChars;
    }
    
    /* Find the amount of characters that fit into the bbox, counting
    ** from the last character in the buffer and forward,
    */
    numfit = TextFit(_rp(obj),
    	&(data->Buffer[numchars - 1]),
    	numchars, &te, NULL,
    	-1, _mwidth(obj), _mheight(obj));
    			
    max_disppos = numchars - numfit;
    
/*    if ((max_disppos > 0) && (!cursor_at_end))
    	max_disppos --;
  */  

/*      D(bug("Numchars w/cursor: %d, Numfit: %d, maxdisppos=%d  " */
/*  	  "bbox->Width = %d  te->te_Width = %d\n", */
/*  	  numchars, numfit, max_disppos, _mwidth(obj), te.te_Width)); */

    return max_disppos;
}


static void UpdateDisp(struct IClass *cl, Object *obj)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    struct TextExtent 	te;
    STRPTR 		dispstr;

    /* If the cursor is at the trailing \0, insert a SPACE instead */
    if (data->BufferPos == data->NumChars)
    	data->Buffer[data->NumChars] = 0x20;

    /* In this function we check if the cursor has gone outside
    ** of the visible area (because of application setting
    ** strinfo->BufferPos or strinfo->DispPos to a different value, or
    ** because of user input).
    ** This is made a bit difficult by the rule (R), that there
    ** should NOT be available space on the right, and characters
    ** scrolled out at the left, at the same time.
    ** We have 3 possible scenarios:
    ** 1) Cursor to the left of DispPos:
    **    Set DispPos to the lowest of BufferPos and the
    **	  maximum allowed disppos (according to (R) ).
    ** 2) Cursor to the right of visible area:
    **    Set dispose sou that the cursor is the last visible character.
    **    This afheres to (R).
    ** 3) Cursor inside visible area. Do a check on rule (R),
    **    and if DispPos > max allowed, then adjust it down,
    **    so that the last character in the buffer becomes last character
    **	  displayed. (The cursor will still be visible after adjustion)
    */

    /* 1) Cursor to the left of visible area */
    if (data->BufferPos < data->DispPos)
    {
    	WORD max_disppos;
    	
    	max_disppos = MaxDispPos(cl, obj);
    	data->DispPos = MIN(data->BufferPos, max_disppos);
    }
    else /* Cursor equal to the right of disppos [ 2) or 3) ] */
    {
    	UWORD strsize;

	/* How many pixels are there from current 1st displayed to the cursor ? */    	
    	strsize = TextLength(_rp(obj),
    		data->Buffer + data->DispPos,
    		data->BufferPos - data->DispPos + 1);
    	    
    	/* 2) More than fits into the gadget ? */
	if (strsize > _mwidth(obj))
	{
	    /* Compute new DispPos such that the cursor is at the right */
    	    data->DispPos = data->BufferPos
    	    		- TextFit(_rp(obj), 
    				&(data->Buffer[data->BufferPos]),
    			   	data->NumChars, &te, NULL, -1,
    			   	_mwidth(obj), _mheight(obj))
    			+ 1;
	    
/*  	    D(bug("cursor right of visible area, new disppos: %d\n", data->DispPos)); */
	}
	else /* 3). Cursor inside gadget */
	{
    	    WORD max_disppos;
    	
    	    max_disppos = MaxDispPos(cl, obj);
    	    if (data->DispPos > max_disppos)
	    	data->DispPos = max_disppos;

	} /* if (cursor inside or to the right of visible area )*/
	
    }   

    /* Update the DispCount */
    /* It might be necessary with special handling for centre aligned gads */
    dispstr = &(data->Buffer[data->DispPos]);
/*      D(bug("DispCount before = %d\n", data->DispCount)); */
    data->DispCount = TextFit(_rp(obj), dispstr,
    		data->NumChars - data->DispPos,
    		&te, NULL, 1,
    		_mwidth(obj),
    		_mheight(obj));
/*      D(bug("DispCount after = %d\n", data->DispCount)); */

    /* 0-terminate string */
    data->Buffer[data->NumChars] = 0x00;
}


/* Gets left position of text in the string gadget */
static UWORD GetTextLeft(struct IClass *cl, Object *obj)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    UWORD   text_left = 0;
    STRPTR  dispstr = &(data->Buffer[data->DispPos]);
    UWORD   dispstrlen;
    BOOL    cursor_at_end;

    cursor_at_end = (data->BufferPos == data->NumChars);
    dispstrlen = MIN(data->DispCount, data->NumChars - data->DispPos);

    switch (data->msd_Align)
    {
	case MUIV_String_Format_Left:
	    text_left = _mleft(obj);
	    break;

	case MUIV_String_Format_Center: {
	    WORD textwidth = TextLength(_rp(obj), dispstr, dispstrlen);
	    if (cursor_at_end) textwidth += TextLength(_rp(obj), " ", 1);    	
	    text_left = _mleft(obj) + ((_mwidth(obj) - textwidth) / 2);
/*  	    D(bug("GetTextLeft: dispstr=%s, dispstrlen=%d, textw=%d, textl=%d\n", */
/*  		  dispstr, dispstrlen, textwidth, text_left)); */
	    } break;

	case MUIV_String_Format_Right: {
	    WORD textwidth = TextLength(_rp(obj), dispstr, dispstrlen);
	
	    if (cursor_at_end) textwidth += TextLength(_rp(obj), " ", 1);
	    text_left = _mleft(obj) + (_mwidth(obj) - 1 - textwidth);
	    } break;
    }
    return (text_left);
}

/* Gets right offset of text in the string gadget */
static UWORD GetTextRight(struct IClass *cl, Object *obj)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    UWORD   text_right = 0;
    STRPTR  dispstr = &(data->Buffer[data->DispPos]);
    UWORD   dispstrlen;
    BOOL    cursor_at_end;

    cursor_at_end = (data->BufferPos == data->NumChars);
    dispstrlen = MIN(data->DispCount, data->NumChars - data->DispPos);

    switch (data->msd_Align)
    {
	case MUIV_String_Format_Left:
	    text_right = _mleft(obj) + TextLength(_rp(obj), dispstr, dispstrlen);
	    break;

	case MUIV_String_Format_Center: {
	    WORD textwidth = TextLength(_rp(obj), dispstr, dispstrlen);

	    if (cursor_at_end) textwidth += TextLength(_rp(obj), " ", 1);	    	
	    text_right = _mright(obj) - ((_mwidth(obj) - textwidth) / 2);
	    } break;

	case MUIV_String_Format_Right:
	    text_right = _mright(obj);
	    break;
    }
    return (text_right);    
}


/* Updates the stringdata in case user has set some fields */
static VOID UpdateStringData(struct IClass *cl, Object *obj)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);   

    data->NumChars = strlen(data->Buffer);

    if (data->BufferPos > data->NumChars)
    {
    	data->BufferPos = data->NumChars;
    }
}

static VOID TextM(Object *obj, struct MUI_StringData *data,
    	    	  STRPTR text, WORD textlen, WORD markstart, WORD markend)
{
    struct RastPort *rp = _rp(obj);
    ULONG textpen;
    WORD len;
    
    if (data->is_active)
	textpen = data->active_text.p_pen;
    else
	textpen = data->inactive_text.p_pen;
   
    //kprintf("TextM: textlen %d  markstart %d  markend %d ... \n", textlen, markstart, markend);
    
    /* <unmarked><marked><unmarked> */
    
    /* <unmarked> */
    
    len = MIN(markstart, textlen);
    len = MAX(len, 0);
    
    if (len)
    {
    	//kprintf("A: %d   ", len);

    	SetABPenDrMd(rp, textpen, _pens(obj)[MPEN_BACKGROUND], JAM1);
    	Text(rp, text, len);
	
	text += len; textlen -= len;

    }
    
    len = MIN(markend - len, textlen);
    len = MAX(len, 0);
    
    if (len)
    {
    	//kprintf("B: %d   ", len);
	SetABPenDrMd(_rp(obj), data->marked_text.p_pen, data->marked_bg.p_pen, JAM2);
    	Text(rp, text, len);
	
	text += len; textlen -= len;
    }
    
    if (textlen)
    {
    	//kprintf("C: %d   ", textlen);

    	SetABPenDrMd(rp, textpen, _pens(obj)[MPEN_BACKGROUND], JAM1);
    	Text(rp, text, textlen);
    }
    //kprintf("\n");        
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR String_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    UWORD   text_left;
    UWORD   text_top;   
    STRPTR  dispstr;
    UWORD   dispstrlen;
    ULONG   textpen;
    UWORD   textleft_save;
    WORD    markstart = 0, markstop = 0;
    
    /*  D(bug("\nString_Draw(%p) %ldx%ldx%ldx%ld reason=%ld msgflgs=%ld curs=%d " */
/*  	  "displ=%ld len=%ld buf='%s'\n",obj,_mleft(obj),_mtop(obj), */
/*  	  _mwidth(obj),_mheight(obj), data->msd_RedrawReason, msg->flags, */
/*  	  data->BufferPos, data->DispPos, data->NumChars, data->Buffer)); */

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWUPDATE) && !(msg->flags & MADF_DRAWOBJECT))
	return 0;

    SetFont(_rp(obj), _font(obj));
    if (data->is_active)
	textpen = data->active_text.p_pen;
    else
	textpen = data->inactive_text.p_pen;

    /* Update the stringdata in case of user change */
    UpdateStringData(cl, obj);
    /* Update the DispPos and DispCount fields so that the gadget renders properly */
    UpdateDisp(cl, obj);

    text_top = _mtop(obj)
	+ ((_mheight(obj) - _rp(obj)->Font->tf_YSize) >> 1)
	+ _rp(obj)->Font->tf_Baseline;

    dispstr = data->Buffer + data->DispPos;
    dispstrlen = MIN(data->DispCount, data->NumChars - data->DispPos);
    textleft_save = text_left = GetTextLeft(cl, obj);

    // little flicker improvement, dont redraw first part of string
    // when adding a char
    if (msg->flags & MADF_DRAWUPDATE &&
	data->msd_RedrawReason == DO_ADDCHAR &&
	data->msd_Align == MUIV_String_Format_Left &&
	data->DispPos == 0)
    {
	text_left += TextLength(_rp(obj), dispstr, data->BufferPos - 1);
	dispstr += data->BufferPos - 1;
	dispstrlen -= data->BufferPos - 1;
	DoMethod(obj, MUIM_DrawBackground, text_left, _mtop(obj),
		 _mwidth(obj) - text_left + _mleft(obj), _mheight(obj),
		 text_left, _mtop(obj), 0);
    }
    else if (msg->flags & MADF_DRAWUPDATE)
    {
	DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
		 _mwidth(obj), _mheight(obj), _mleft(obj), _mtop(obj), 0);
    }

    SetABPenDrMd(_rp(obj), textpen, _pens(obj)[MPEN_BACKGROUND], JAM1);
    Move(_rp(obj), text_left, text_top);

    if ((data->msd_Flags & MSDF_MARKING) && Buffer_GetMarkedRange(data, &markstart, &markstop))
    {
   	TextM(obj, data, dispstr, dispstrlen, markstart - data->DispPos,
	    	    	    	    	      markstop - data->DispPos);
		    
    }
    else
    {
    	Text(_rp(obj), dispstr, dispstrlen);

	if (data->is_active) // active, draw cursor
	{
	    UWORD cursoroffset = data->BufferPos - data->DispPos;

	    dispstr = data->Buffer + data->DispPos;
	    text_left = textleft_save;

	    SetABPenDrMd(_rp(obj), data->active_text.p_pen, data->cursor.p_pen, JAM2);
	    text_left += TextLength(_rp(obj), dispstr, cursoroffset);


	    Move(_rp(obj), text_left, text_top);
	    Text(_rp(obj), 
		 ((data->BufferPos < data->NumChars)
		  ? dispstr + cursoroffset
		  : (STRPTR)" "),
		 1 );
	}
    }
    
    data->msd_RedrawReason = NO_REASON;
    return TRUE;
}

/**************************************************************************
 Returns wether object needs redrawing
**************************************************************************/
int String_HandleVanillakey(struct IClass *cl, Object * obj,
			    unsigned char code, UWORD qual)
{
    struct MUI_StringData *data = (struct MUI_StringData*)INST_DATA(cl, obj);
    BOOL doinput;
    
    D(bug("String_HandleVanillakey: code=%d qual=%d\n", code, qual));

    if (0 == code)
	return 0;

    doinput = (data->msd_Flags & MSDF_NOINPUT) ? FALSE : TRUE;
    
    if (doinput && (code == '\b')) /* backspace */
    {
	if (Buffer_KillMarked(data))
	{
	    return 1;
	}
	
	if (data->BufferPos > 0)
	{
	    LONG shift;
    
	    if ((qual & IEQUALIFIER_LSHIFT) || (qual & IEQUALIFIER_RSHIFT))
	    {
		shift = data->BufferPos;
		data->msd_RedrawReason = NEW_CONTENTS;
	    }
	    else
	    {
		shift = 1;
		data->msd_RedrawReason = DO_BACKSPACE;
	    }

	    strcpy(&data->Buffer[data->BufferPos - shift],
		   &data->Buffer[data->BufferPos]);
	    data->BufferPos -= shift;
	    data->NumChars -= shift;
	    return 1;
	}
	return 0;
    }

    if (doinput && (code == 21)) // ctrl-u == NAK (like shift-bs)
    {
	if (Buffer_KillMarked(data))
	{
	    return 1;
	}
	
	if (data->BufferPos > 0)
	{
	    strcpy(&data->Buffer[0],
		   &data->Buffer[data->BufferPos]);
	    data->NumChars -= data->BufferPos;
	    data->BufferPos = 0;
	    data->msd_RedrawReason = NEW_CONTENTS;
	    return 1;
	}
	return 0;
    }

    if (doinput && (code == 127)) /* del */
    {
    	if (!Buffer_KillMarked(data))
	{
	    if ((qual & IEQUALIFIER_LSHIFT) || (qual & IEQUALIFIER_RSHIFT))
	    {
		data->Buffer[data->BufferPos] = 0;
		data->NumChars = data->BufferPos;
		data->msd_RedrawReason = NEW_CONTENTS;
	    }
	    else
	    {
		if (data->BufferPos < data->NumChars)
		{
		    strcpy(&data->Buffer[data->BufferPos],
			   &data->Buffer[data->BufferPos+1]);
		    data->NumChars--;
		}

		data->msd_RedrawReason = DO_DELETE;
	    }
	}
	return 1;
    }

    if (doinput && (code == 11)) // ctrl-k == VT == \v (like shift-del)
    {
    	if (!Buffer_KillMarked(data))
	{
	    data->Buffer[data->BufferPos] = 0;
	    data->NumChars = data->BufferPos;
	    data->msd_RedrawReason = NEW_CONTENTS;
	}
	return 1;
    }

    if (doinput && (code == 24)) /* ctrl x == ascii cancel */
    {
    	if (!Buffer_KillMarked(data))
	{
	    data->Buffer[0] = 0;
	    data->BufferPos = 0;
	    data->NumChars = 0;
	    data->msd_RedrawReason = NEW_CONTENTS;
	}
	return 1;
    }

    if (code == 1) // ctrl-a, linestart
    {
	data->BufferPos = 0;
	data->msd_Flags &= ~(MSDF_MARKING | MSDF_KEYMARKING);
	return 1;
    }

    if (code == 26) // ctrl-z, lineend
    {
	data->BufferPos = data->NumChars;
	data->msd_Flags &= ~(MSDF_MARKING | MSDF_KEYMARKING);
	return 1;
    }

    if (((ToLower(code) == 'c') || (ToLower(code) == 'x')) &&
    	(qual & IEQUALIFIER_RCOMMAND))
    {
    	WORD markstart, markstop;
	
    	if ((data->msd_Flags & MSDF_MARKING) && Buffer_GetMarkedRange(data, &markstart, &markstop))
	{	    
	    clipboard_write_text(&data->Buffer[markstart], markstop - markstart);
	    
	    if (doinput && (ToLower(code) == 'x'))
	    {
		Buffer_KillMarked(data);
	    }
	    else
	    {
	    	data->BufferPos = markstop;
	    	data->msd_Flags &= ~MSDF_MARKING;
	    }
    	    return 1;
	}
	return 0;
    }

    if (doinput && (ToLower(code) == 'v') && (qual & IEQUALIFIER_RCOMMAND))
    {
    	STRPTR text;
	int    retval;

	retval = Buffer_KillMarked(data);
	if ((text = clipboard_read_text()))
	{
	    STRPTR text2 = text;
	    UBYTE  c;
	    
	    while((c = *text2++))
	    {
	    	if (!isprint(c)) break;
		if (!(Buffer_AddChar(data, c))) break;
		if (!retval) retval = 1;
	    }
	    
	    clipboard_free_text(text);
	}
 
	return retval;
    }
    
    if (data->msd_Accept != NULL)
    {
    	/* Check if character is accepted */
	if (NULL == strchr(data->msd_Accept, code))
	    return 0;
    }

    if (data->msd_Reject != NULL)
    {
    	/* Check if character is rejected */
	if (NULL != strchr(data->msd_Reject, code))
	{
	    DisplayBeep(NULL);
	    return 0;
	}
    }

    if (doinput && isprint(code))
    {
    	Buffer_KillMarked(data);
	if (Buffer_AddChar(data, code))
	{
	    data->msd_RedrawReason = DO_ADDCHAR;
	    return 2;
	}
    }

    data->msd_RedrawReason = DO_UNKNOWN;
    return 0;
}


/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static IPTR String_HandleEvent(struct IClass *cl, Object * obj,
			       struct MUIP_HandleEvent *msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);
    ULONG retval = 0;
    int update = 0;
    LONG muikey = msg->muikey;
    BOOL cursor_kills_marking = FALSE;
    
    if ((data->msd_Flags & MSDF_MARKING) && !(data->msd_Flags & MSDF_KEYMARKING))
    {
    	cursor_kills_marking = TRUE;
    }
     
    if (muikey == MUIKEY_NONE)
    {
    	if (msg->imsg->Class == IDCMP_RAWKEY)
	{
	    static LONG muikeytable[3][2] =
	    {
	    	{MUIKEY_LEFT	    , MUIKEY_RIGHT  	},
		{MUIKEY_WORDLEFT    , MUIKEY_WORDRIGHT	},
		{MUIKEY_LINESTART   , MUIKEY_LINEEND	}
	    };
	    WORD dirindex = -1, amountindex = 0;
	    
	    switch(msg->imsg->Code)
	    {
	    	case 0x4F:
	    	    dirindex = 0;
		    break;
		    
		case 0x4E:
		    dirindex = 1;
		    break;

	    #ifdef __AROS__
		case RAWKEY_HOME:
		    muikey = MUIKEY_LINESTART;
		    break;

		case RAWKEY_END:
		    muikey = MUIKEY_LINEEND;
		    break;
	    #endif
		    		    
	    }
	    
	    if ((dirindex != -1) && (muikey == MUIKEY_NONE))
	    {
	    	if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		{
		    amountindex = 2;
		}
		else if (msg->imsg->Qualifier & IEQUALIFIER_CONTROL)
		{
		    amountindex = 1;
		}
		
		muikey = muikeytable[amountindex][dirindex];
	    }
	    
	}
    }
    
    D(bug("String_HandleEvent: muikey %d, imsg %p is_active=%d\n", muikey,
	  msg->imsg, data->is_active));
    if (muikey != MUIKEY_NONE && data->is_active)
    {
	retval = MUI_EventHandlerRC_Eat;

	switch (muikey)
	{
	    case MUIKEY_LEFT:
	    	if (cursor_kills_marking)
		{
		    update = 1;
		    data->BufferPos = MIN(data->BufferPos, data->MarkPos);
		    if (data->BufferPos > 0) data->BufferPos--;
		    
		    data->msd_Flags &= ~MSDF_MARKING;
		}
		else if (data->BufferPos > 0)
		{
		    update = 1;
		    
		    data->BufferPos--;
		    data->msd_RedrawReason = DO_CURSOR_LEFT;
		}
		break;
	    case MUIKEY_RIGHT:
	    	if (cursor_kills_marking)
		{
		    update = 1;
		    data->BufferPos = MAX(data->BufferPos, data->MarkPos);
		    data->msd_Flags &= ~MSDF_MARKING;
		}
		else if (data->BufferPos < data->NumChars)
		{
		    update = 1;
		    data->BufferPos++;
		    data->msd_RedrawReason = DO_CURSOR_RIGHT;
		}
		break;
	    case MUIKEY_WORDLEFT:
		if (data->BufferPos > 0)
		{
		    data->BufferPos = Buffer_GetPrevWordIndex(data, data->BufferPos);
		    update = 1;
		    data->msd_RedrawReason = DO_CURSOR_LEFT;
		}
		if (cursor_kills_marking)
		{
		    data->msd_Flags &= ~MSDF_MARKING;
		    update = 1;
		}
		break;
	    case MUIKEY_WORDRIGHT:
		if (data->BufferPos < data->NumChars)
		{
		    data->BufferPos = Buffer_GetSuccWordIndex(data, data->BufferPos);
		    update = 1;
		    data->msd_RedrawReason = DO_CURSOR_RIGHT;		   
		}
		if (cursor_kills_marking)
		{
		    data->msd_Flags &= ~MSDF_MARKING;
		    update = 1;
		}
		break;
	    case MUIKEY_LINESTART:
		data->BufferPos = 0;
		update = 1;
		if (cursor_kills_marking)
		{
		    data->msd_Flags &= ~MSDF_MARKING;
		}
		break;

	    case MUIKEY_LINEEND:
		data->BufferPos = data->NumChars;
		update = 1;
		if (cursor_kills_marking)
		{
		    data->msd_Flags &= ~MSDF_MARKING;
		}
		break;

	    case MUIKEY_UP:
		if (data->msd_AttachedList)
		    set(data->msd_AttachedList,
			MUIA_List_Active, MUIV_List_Active_Up);
		break;
	    case MUIKEY_DOWN:
		if (data->msd_AttachedList)
		    set(data->msd_AttachedList,
			MUIA_List_Active, MUIV_List_Active_Down);
		break;
	    case MUIKEY_PAGEUP:
		if (data->msd_AttachedList)
		    set(data->msd_AttachedList,
			MUIA_List_Active, MUIV_List_Active_PageUp);
		break;
	    case MUIKEY_PAGEDOWN:
		if (data->msd_AttachedList)
		    set(data->msd_AttachedList,
			MUIA_List_Active, MUIV_List_Active_PageDown);
		break;
	    case MUIKEY_TOP:
		if (data->msd_AttachedList)
		    set(data->msd_AttachedList,
			MUIA_List_Active, MUIV_List_Active_Top);
		break;
	    case MUIKEY_BOTTOM:
		if (data->msd_AttachedList)
		    set(data->msd_AttachedList,
			MUIA_List_Active, MUIV_List_Active_Bottom);
		break;
	    case MUIKEY_PRESS: {
		UBYTE *buf = NULL;

		get(obj, MUIA_String_Contents, (IPTR *)&buf);

    	    	if (data->msd_Flags & MSDF_STAYACTIVE)
		{
		    /* Do not change active object */
		}
		else if (data->msd_Flags & MSDF_ADVANCEONCR)
		{
		    set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
		}
		else if (!(data->msd_Flags & MSDF_STAYACTIVE))
		{
		    set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
		}
		
		set(obj, MUIA_String_Acknowledge, buf);
	    } break;

	    case MUIKEY_WINDOW_CLOSE:
		data->is_active = FALSE;
		set(obj, MUIA_Background,
		    (IPTR)muiGlobalInfo(obj)->mgi_Prefs->string_bg_inactive);
		DoMethod(obj, MUIM_GoInactive);
		retval = 0;
		break;

	    default:
		retval = 0;
	} // switch(muikey)
    } // if (muikey != MUIKEY_NONE)

    if (msg->imsg)
    {
	UWORD code = msg->imsg->Code;
	//UWORD qual = msg->imsg->Qualifier;
	WORD x = msg->imsg->MouseX;
	WORD y = msg->imsg->MouseY;

	//bug("String_HandleEvent: parsing imsg %p, class=%ld\n", msg->imsg, msg->imsg->Class);

	switch (msg->imsg->Class)
	{
	    case IDCMP_MOUSEBUTTONS: /* set cursor and activate it */
		if (code == SELECTDOWN)
		{
		    //bug("String_HandleEvent: code == SELECTDOWN, x=%d y=%d\n", x, y);

		    if (_isinobject(x, y))
		    {
			UWORD text_left, text_right;
			
			retval = MUI_EventHandlerRC_Eat;

			CurrentTime(&data->NewClick_Sec, &data->NewClick_Micro);
			if (DoubleClick(data->OldClick_Sec, data->OldClick_Micro,
		    	    		data->NewClick_Sec, data->NewClick_Micro))
			{
		    	    data->MultiClick++;
			}
			else
			{
		    	    data->MultiClick = 0;
			}
    	    	    	data->OldClick_Sec = data->NewClick_Sec;
			data->OldClick_Micro = data->NewClick_Micro;
			
			//kprintf("multiclick %d\n", data->MultiClick);
			
			if (!data->is_active)
			{
			    //bug("String got button, lets activate\n");
			    data->is_active = TRUE;
			    data->msd_RedrawReason = WENT_ACTIVE;
			    // redraw
			    set(obj, MUIA_Background,
				(IPTR)muiGlobalInfo(obj)->mgi_Prefs->string_bg_active);

			    //DoMethod(obj, MUIM_GoActive);
			    set(_win(obj), MUIA_Window_ActiveObject, obj);
			    // let other objects a chance to get desactivated
			    //retval = 0;
			}
			
			if (!(data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
			{
			    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    	    	    	    data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
    	    	    	    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    	    	    	}
			
			text_left  = GetTextLeft (cl, obj);
			text_right = GetTextRight(cl, obj);

			/* Check if mouseclick is inside displayed text */
			if ((x >= text_left) && (x <= text_right))
			{
			    /* Find new cursor pos. */
			    struct TextExtent te;
			    ULONG newpos;
			    STRPTR dispstr = data->Buffer + data->DispPos;
    	
			    newpos = data->DispPos 
				+ TextFit(_rp(obj), dispstr, data->NumChars - data->DispPos,
					  &te, NULL, 1,
					  x - text_left, _rp(obj)->Font->tf_YSize);

			    if (data->BufferPos != newpos)
			    {
				data->BufferPos = newpos;
				update = 1;
			    }
			}
			else if (x < text_left)
			{
			    /* Click on empty space at left. Set cursor to first visible */
			    if (data->BufferPos != data->DispPos)
			    {
				data->BufferPos = data->DispPos;
				update = 1;
			    }
			}   
			else
			{
			    /* Click on empty space at right. Set cursor to last visible */
			    if (data->BufferPos != data->DispPos + data->DispCount)
			    {
				data->BufferPos = data->DispPos + data->DispCount;
				update = 1;
			    }
			} /* if (click is on text or not) */

			data->MarkPos = data->BufferPos;
			
			if (data->MultiClick == 0)
			{
			    if (data->msd_Flags & MSDF_MARKING)
			    {
				data->msd_Flags &= ~MSDF_MARKING;
				update = 1;
			    }
			}
			else if (data->MultiClick && Buffer_GetMarkedRange(data, NULL, NULL))
			{
			    data->msd_Flags |= MSDF_MARKING;
			    update = 1;
			}
			
			
		    } /* is in object */
		    else if (data->is_active && !(data->msd_Flags & MSDF_STAYACTIVE)) /* and click not on object */
		    {
			data->is_active = FALSE;
			set(obj, MUIA_Background,
			    (IPTR)muiGlobalInfo(obj)->mgi_Prefs->string_bg_inactive);
			//DoMethod(obj, MUIM_GoInactive);
			// let other objects a chance to get activated
			//retval = 0;
		    }
		} /* if (code == SELECTDOWN) */
		else if (code == SELECTUP)
		{
		    if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
		    {
    	    	    	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
			data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
    	    	    	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);	    	
		    }
		}
		break;

    	    case IDCMP_MOUSEMOVE:
	    	if (data->is_active)
		{
		    UWORD text_left, text_right;
		    
		    if (!(data->msd_Flags & MSDF_MARKING))
		    {
		    	data->msd_Flags |= MSDF_MARKING;
		    }
		    
		    text_left  = GetTextLeft (cl, obj);
		    text_right = GetTextRight(cl, obj);

		    /* Check if mouseclick is inside displayed text */
		    if ((x >= text_left) && (x <= text_right))
		    {
			/* Find new cursor pos. */
			struct TextExtent te;
			ULONG newpos;
			STRPTR dispstr = data->Buffer + data->DispPos;

			newpos = data->DispPos 
			    + TextFit(_rp(obj), dispstr, data->NumChars - data->DispPos,
				      &te, NULL, 1,
				      x - text_left, _rp(obj)->Font->tf_YSize);

			if (data->BufferPos != newpos)
			{
			    WORD old_markstart = 0, old_markstop = 0;
			    WORD markstart = 0, markstop = 0;	    
			    BOOL was_marked, is_marked;
			    
			    was_marked = Buffer_AnythingMarked(data) &&
			    	    	 Buffer_GetMarkedRange(data, &old_markstart, &old_markstop);
			    
			    data->BufferPos = newpos;
			    
			    is_marked = Buffer_AnythingMarked(data) &&
			    	    	Buffer_GetMarkedRange(data, &markstart, &markstop);
			    
			    if ((was_marked != is_marked) ||
			    	(old_markstart != markstart) ||
				(old_markstop != markstop))
			    {
			    	update = 1;
			    }
			}
		    }
		    else if ((x < text_left) && (data->BufferPos > 0))
		    {
		    	data->BufferPos--;
			update = 1;
		    	data->msd_RedrawReason = DO_CURSOR_LEFT;			
		    }
		    else if ((x > text_right) && (data->BufferPos < data->NumChars))
		    {
		    	data->BufferPos++;
			update = 1;
		    	data->msd_RedrawReason = DO_CURSOR_RIGHT;			
		    }
		    //kprintf("  ---- bp: %d\n", data->BufferPos);
		}
		break;
		
	    case IDCMP_RAWKEY:
	    {
		unsigned char code;

		//bug("String_HandleEvent: idcmp_rawkey\n");

		if (!data->is_active)
		    break;

		code = ConvertKey(msg->imsg);
		if (!code)
		{
		    switch(msg->imsg->Code)
		    {
    	    	    	case 0x64: /* LALT */
			case 0x65: /* RALT */
   	    	    	case 0x64 | IECODE_UP_PREFIX:
			case 0x65 | IECODE_UP_PREFIX:
			    if (msg->imsg->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
			    {
			    	data->MarkPos = data->BufferPos;
				data->msd_Flags |= (MSDF_MARKING | MSDF_KEYMARKING);
			    }
			    else
			    {
			    	data->msd_Flags &= ~MSDF_KEYMARKING;
			    }
			    break;
			
		    }
		}
	    
		if (code)
		{
		    update = String_HandleVanillakey(cl, obj, code, msg->imsg->Qualifier);
		    if (update)
			retval = MUI_EventHandlerRC_Eat;
		}
	    }
	    break;
	}
    }

    if (update)
    {
    	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    //D(bug("String eh return %ld\n", retval));
    return retval;
}


/**************************************************************************
 MUIM_Export : to export an objects "contents" to a dataspace object.
**************************************************************************/
static IPTR String_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if (data->Buffer != NULL)
	    DoMethod(msg->dataspace, MUIM_Dataspace_Add,
		     (IPTR)data->Buffer,
		     data->NumChars + 1, 
		     (IPTR)id);
	else
	    DoMethod(msg->dataspace, MUIM_Dataspace_Remove,
		     (IPTR)id);
    }
    return 0;
}


/**************************************************************************
 MUIM_Import : to import an objects "contents" from a dataspace object.
**************************************************************************/
static IPTR String_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    ULONG id;
    STRPTR s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if ((s = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_Find, (IPTR)id)))
	{
	    set(obj, MUIA_String_Contents, s);
	}
    }
    return 0;
}

/**************************************************************************
 MUIM_GoActive
**************************************************************************/
static IPTR String_GoActive(struct IClass * cl, Object * obj, Msg msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);

    //D(bug("String_GoActive %p\n", obj));
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    data->is_active = TRUE;
    data->msd_Flags &= ~MSDF_KEYMARKING;
    data->msd_RedrawReason = WENT_ACTIVE;
    // redraw
    set(obj, MUIA_Background,
	(IPTR)muiGlobalInfo(obj)->mgi_Prefs->string_bg_active);
    return 0;
}

/**************************************************************************
 MUIM_GoInactive
**************************************************************************/
static IPTR String_GoInactive(struct IClass * cl, Object * obj, Msg msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);

    //D(bug("String_GoInactive %p\n", obj));

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    data->is_active = FALSE;
    data->msd_RedrawReason = WENT_INACTIVE;
    data->MultiClick = 0;
    
    // redraw
    set(obj, MUIA_Background,
	(IPTR)muiGlobalInfo(obj)->mgi_Prefs->string_bg_inactive);
    return 0;
}

/**************************************************************************
 MUIM_String_ClearSelected (BetterString)
**************************************************************************/
static IPTR String_ClearSelected(struct IClass * cl, Object * obj, struct MUIP_String_ClearSelected *msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);

    //D(bug("String_ClearSelected %p\n", obj));

    if (Buffer_KillMarked(data))
    {
    	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return 0;
}

/**************************************************************************
 MUIM_String_Insert (BetterString)
**************************************************************************/
static IPTR String_Insert(struct IClass * cl, Object * obj, struct MUIP_String_Insert *msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);
    LONG pos;
    ULONG old_bufferpos;
    ULONG num_inserted = 0;
    
    //D(bug("String_Insert %p\n", obj));

    switch((ULONG)msg->pos)
    {
    	case MUIV_String_Insert_StartOfString:
	    pos = 0;
	    break;
	    
	case MUIV_String_Insert_EndOfString:
	    pos = data->NumChars;
	    break;
	    
	case MUIV_String_Insert_BufferPos:
	    pos = data->BufferPos;
	    break;
	
	default:
	    pos = msg->pos;
	    break;
    }
	    
    if ((pos < 0) || (pos > data->NumChars)) return 0;
    
    old_bufferpos = data->BufferPos;
    data->BufferPos = pos;
    
    while(msg->text[num_inserted] && Buffer_AddChar(data, msg->text[num_inserted]))
    {
    	num_inserted++;
    }
    
    if (num_inserted)
    {
    	if (old_bufferpos >= pos)
	{
	    data->BufferPos = old_bufferpos + num_inserted;
	}
	else
	{
	    data->BufferPos = old_bufferpos;
	}
	
    	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return 0;
}

/**************************************************************************
 MUIM_String_FileNameStart (BetterString)
**************************************************************************/
static IPTR String_FileNameStart(struct IClass * cl, Object * obj, struct MUIP_String_FileNameStart *msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);
    STRPTR buf;
    
    //D(bug("String_FileNameStart %p\n", obj));

    if (data->msd_useSecret)
    {
    	buf = data->SecBuffer;
    }
    else
    {
    	buf = data->Buffer;
    }
#warning "TODO: Implement String_FileNameStart correctly!"

    return (IPTR)buf;
}

BOOPSI_DISPATCHER(IPTR, String_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return String_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return String_Dispose(cl, obj, msg);
	case OM_SET: return String_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return String_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return String_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return String_Cleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return String_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return String_Draw(cl, obj, (APTR)msg);
	case MUIM_Export: return String_Export(cl, obj, (APTR)msg);
	case MUIM_Import: return String_Import(cl, obj, (APTR)msg);
	case MUIM_GoActive: return String_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return String_GoInactive(cl,obj,(APTR)msg);
	case MUIM_HandleEvent: return String_HandleEvent(cl,obj,(APTR)msg);
	case MUIM_String_ClearSelected: return String_ClearSelected(cl,obj,(APTR)msg); /* BetterString */
	case MUIM_String_Insert: return String_Insert(cl,obj,(APTR)msg); /* BetterString */
	case MUIM_String_FileNameStart: return String_FileNameStart(cl,obj,(APTR)msg); /* BetterString */
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_String_desc = { 
    MUIC_String,
    MUIC_Area,
    sizeof(struct MUI_StringData), 
    (void*)String_Dispatcher
};
