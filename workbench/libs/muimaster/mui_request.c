/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#ifdef _AROS
#include <aros/asmcall.h>
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

static char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}

#ifdef _AROS
AROS_UFH2S(void, cpy_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *, strPtrPtr, A3))
{
    AROS_USERFUNC_INIT
    
    *(*strPtrPtr)++ = chr;
    
    AROS_USERFUNC_EXIT
}

AROS_UFH2S(void, len_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(LONG *, lenPtr, A3))
{
    AROS_USERFUNC_INIT
    
    (*lenPtr)++;
    
    AROS_USERFUNC_EXIT
}
#endif

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm LONG MUI_RequestA(register __d0 APTR app, register __d1 APTR win, register __d2 LONGBITS flags, register __a0 char *title, register __a1 char *gadgets, register __a2 char *format, register __a3 APTR params)
#else
	AROS_LH7(LONG, MUI_RequestA,

/*  SYNOPSIS */
	AROS_LHA(APTR, app, D0),
	AROS_LHA(APTR, win, D1),
	AROS_LHA(LONG, flags, D2),
	AROS_LHA(char *, title, A0),
	AROS_LHA(char *, gadgets, A1),
	AROS_LHA(char *, format, A2),
	AROS_LHA(APTR, params, A3),

/*  LOCATION */
	struct Library *, MUIMasterBase, 7, MUIMaster)
#endif
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,MUIMasterBase)

    LONG result;
    char *reqtxt;
    LONG reqtxt_len;
#ifndef _AROS
    static const ULONG len_func = 0x52934e75; /* addq.l  #1,(A3) ; rts */
    static const ULONG cpy_func = 0x16c04e75; /* move.b d0,(a3)+ ; rts */
#endif

    Object *req_wnd;
    Object *req_group;
    Object *req_but[32]; /* more than 32 buttongadgets within a requester shouldn`t happen */

    if (!app)
    {
	struct EasyStruct es;
	es.es_StructSize = sizeof(struct EasyStruct);
	es.es_Flags = 0;
	es.es_Title = title;
	es.es_TextFormat = format;
	es.es_GadgetFormat = gadgets;
	return EasyRequestArgs(NULL,&es,NULL,params);
    }

#ifdef _AROS
    reqtxt_len = 0;
    RawDoFmt(format,params,(VOID_FUNC)AROS_ASMSYMNAME(len_func),&reqtxt_len);
#else
    reqtxt_len = 0;
    RawDoFmt(format,params,(void(*)())&len_func,&reqtxt_len);
#endif

    if (!(reqtxt = AllocVec(reqtxt_len+1,0))) return 0; /* Return cancel if something failed */

#ifdef _AROS
    {
    	char *reqtxtptr = reqtxt;
	
	RawDoFmt(format,params,(VOID_FUNC)AROS_ASMSYMNAME(cpy_func),&reqtxtptr);
    }  
#else
    RawDoFmt(format,params,(void(*)())&cpy_func,reqtxt);
#endif

    req_wnd = WindowObject,
	MUIA_Window_Title,        title,
	MUIA_Window_RefWindow,    win,
	MUIA_Window_LeftEdge,     MUIV_Window_LeftEdge_Centered,
	MUIA_Window_TopEdge,      MUIV_Window_TopEdge_Centered,
	MUIA_Window_CloseGadget,  FALSE,
	MUIA_Window_SizeGadget,   FALSE,
	WindowContents, VGroup,
	    MUIA_Background,       MUII_RequesterBack,
	    Child, HGroup,
		Child, TextObject,
		    TextFrame,
		    MUIA_InnerBottom,   8,
		    MUIA_InnerLeft,     8,
		    MUIA_InnerRight,    8,
		    MUIA_InnerTop,      8,
		    MUIA_Background,    MUII_TextBack,
		    MUIA_Text_SetMax,   TRUE,
		    MUIA_Text_Contents, reqtxt,
		    End,
		End,
	    Child, VSpace(2),
	    Child, req_group = HGroup, End,
            End,
	End;

  FreeVec(reqtxt);

  result = 0;

    if (req_wnd)
    {
	char *gadgs = StrDup(gadgets);
	if (gadgs)
	{
    	    char *current = gadgs;
	    int active = -1;
	    int num_gads = 0;
	    LONG isopen;

//	    set(app, MUIA_Application_Sleep, TRUE);
	    DoMethod(app, OM_ADDMEMBER, req_wnd);

	    while(current)
	    {
		char *next = strchr(current,'|');
		if (next) *next++ = 0;

		if (current[0] == '*')
		{
		    current++;
		    active = num_gads;
		}

		if (!(req_but[num_gads] = SimpleButton(current)))
		    break;
		num_gads++;
		current = next;
	    }

	    FreeVec(gadgs);
	    DoMethod(req_group, MUIM_Group_InitChange);

	    /* if this is only one button lets add it separatly */
	    if (num_gads == 1)
	    {
	        DoMethod(req_group, OM_ADDMEMBER, HSpace(0));
	        DoMethod(req_group, OM_ADDMEMBER, req_but[0]);
	        DoMethod(req_group, OM_ADDMEMBER, HSpace(0));
	        DoMethod(req_but[0], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, 2);
	    }
	    else
	    {
	        int j;

	        for(j=0;j < num_gads;j++)
	        {
	            if(j > 0) DoMethod(req_group, OM_ADDMEMBER, HSpace(0));
	            DoMethod(req_group, OM_ADDMEMBER, req_but[j]);
	            DoMethod(req_but[j], MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, j+2 <= num_gads ? j+2 : 1);
	            set(req_but[j], MUIA_CycleChain, 1);
	        }
	    }
	    DoMethod(req_group, MUIM_Group_ExitChange);

	    /* now activate that button with a starting "*" */
	    if(active != -1) set(req_wnd, MUIA_Window_ActiveObject, req_but[active]);

	    /* lets collect the waiting returnIDs now */
	    // COLLECT_RETURNIDS;

	    set(req_wnd,MUIA_Window_Open,TRUE);
	    get(req_wnd,MUIA_Window_Open,&isopen);

	    if (isopen)
	    {
	        ULONG sigs = 0;
	        result = -1;

	        while (result == -1)
	        {
		    ULONG ret = DoMethod(app, MUIM_Application_NewInput, &sigs);

		    /* if a button was hit, lets get outda here. */
		    if (ret > 0 && ret <= num_gads+1)
		    {
		        result = ret-1;
		        break;
		    }

		    if (sigs) sigs = Wait(sigs);
	        }
	    }

	    /* now lets reissue the collected returnIDs again */
	    // REISSUE_RETURNIDS;

//	    set(app, MUIA_Application_Sleep, FALSE);
        }

	DoMethod(app, OM_REMMEMBER, req_wnd);
	MUI_DisposeObject(req_wnd);
    }
    return result;

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestA */
