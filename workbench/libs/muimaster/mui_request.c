/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <string.h>

#ifdef __AROS__
#include <proto/alib.h>
#else
#include <clib/alib_protos.h>
#endif
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#ifdef __AROS__
#include <stdlib.h>
#endif

#include "mui.h"
#include "support.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

#ifdef __AROS__
/* RawDoFmt callback: append chr, including its terminating NUL, and advance
 * the caller-owned output cursor. The caller supplies sufficient storage.
 */
AROS_UFH2S(void, cpy_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(STRPTR *, strPtrPtr, A3))
{
    AROS_USERFUNC_INIT
    
    *(*strPtrPtr)++ = chr;
    
    AROS_USERFUNC_EXIT
}

/* RawDoFmt callback: count each emitted byte, including the terminating NUL.
 * lenPtr must point to writable counter storage.
 */
AROS_UFH2S(void, len_func,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(LONG *, lenPtr, A3))
{
    AROS_USERFUNC_INIT
    
    (*lenPtr)++;
    
    AROS_USERFUNC_EXIT
}

/* Query the format argument layout through GetDataStreamFromFormat().
 * This wrapper supplies an empty variadic list; callers request sizes and
 * offsets with dataStream == NULL, never argument values. Output pointers
 * may be NULL as allowed by GetDataStreamFromFormat().
 */
static void muiDataStreamFromFormat(CONST_STRPTR format, APTR dataStream,  ULONG *dataSize,
                             ULONG *indexStream, ULONG *indexSize, ...)
{
    va_list empty;
    va_start(empty, indexSize);
    GetDataStreamFromFormat(format, empty, dataStream, dataSize,
                        indexStream, indexSize);
    va_end(empty);
}
#endif

/*****************************************************************************

    NAME */
        AROS_LH7(LONG, MUI_RequestA,

/*  SYNOPSIS */
        AROS_LHA(APTR,         app,     D0),
        AROS_LHA(APTR,         win,     D1),
        AROS_LHA(LONGBITS,     flags,   D2),
        AROS_LHA(CONST_STRPTR, title,   A0),
        AROS_LHA(CONST_STRPTR, gadgets, A1),
        AROS_LHA(CONST_STRPTR, format,  A2),
        AROS_LHA(APTR,         params,  A3),

/*  LOCATION */
        struct Library *, MUIMasterBase, 7, MUIMaster)

/*  FUNCTION
        Display a requester and wait for a response. With a MUI application,
        create a temporary window and process the application's input until
        a requester return ID is received. With app == NULL, delegate to
        intuition.library/EasyRequestArgs() instead.

    INPUTS
        app - MUI Application object, or NULL for the Intuition fallback.
        win - MUI Window object used as the reference window, or NULL.
              Ignored by the Intuition fallback.
        flags - Ignored by this implementation; pass zero.
        title - Requester title. With a MUI application, NULL selects
                MUIA_Application_Title.
        gadgets - Non-NULL NUL-terminated button labels separated by |.
                  Supply at least one label and no more than 32 labels.
                  On the MUI path, a leading * selects the active button;
                  if several labels have it, the last one takes precedence.
        format - Non-NULL RawDoFmt-compatible message format.
        params - Array of IPTR-sized format arguments in AROS, in format
                 order. Supply a valid array whenever arguments are used.
                 This is neither a tag list nor an already packed RAWARG.

    RESULT
        On the MUI path, buttons except the last return their one-based
        position; the last returns 0. A single button returns 1. Allocation
        or window-opening failure also returns 0. On the fallback path,
        return the result of EasyRequestArgs() unchanged.

    NOTES
        This is a synchronous GUI call. Application methods and input
        handlers may run before it returns; the application is not put to
        sleep. The temporary requester is removed and disposed of before
        returning. Keep input strings, formatting arguments, and referenced
        objects valid for the duration of the call.
        MUI_Request() supplies positional formatting arguments, not tags.

    BUGS
        The 32-button array has no bounds check. More labels can overwrite
        memory. A button-construction failure can leave a partial requester.
        On 32-bit AROS, one IPTR slot cannot preserve the full value of
        an eight-byte formatting argument during the packing step.
        Application return IDs are neither saved nor replayed. IDs from
        1 through the number of constructed buttons plus 1 can terminate
        the requester even if they came from another application action;
        other return IDs are consumed without being returned to the caller.

    SEE ALSO
        exec.library/RawDoFmt(), intuition.library/EasyRequestArgs()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG result;
    char *reqtxt;
    LONG reqtxt_len;
#ifndef __AROS__
    static const ULONG len_func = 0x52934e75; /* addq.l  #1,(A3) ; rts */
    static const ULONG cpy_func = 0x16c04e75; /* move.b d0,(a3)+ ; rts */
#endif

    Object *req_wnd;
    Object *req_group;
    Object *req_but[32]; /* shouldn't be more than 32 buttons per requester */

#ifdef __AROS__
    /* Repack IPTR[] to RAWARG */
    RAWARG _params;
    ULONG *_index;
    ULONG _indexsize = 0, _datasize = 0;
    UBYTE *ptr;
    muiDataStreamFromFormat(format, NULL, NULL, NULL, &_indexsize);
    _index = (ULONG *)alloca(_indexsize + sizeof(ULONG));
    muiDataStreamFromFormat(format, NULL, &_datasize, _index, &_indexsize);
    _index[_indexsize / sizeof(ULONG)] = _datasize;
    _params = (RAWARG)alloca(_datasize);
    ptr = (UBYTE *)_params;

    for (int i = 0; i < _indexsize / sizeof (ULONG); i++)
    {
        UBYTE dsize = _index[i + 1] - _index[i];

        switch(dsize)
        {
            case (2): *(UWORD *)ptr = (UWORD)((IPTR*)params)[i]; break;
            case (4): *(ULONG *)ptr = (ULONG)((IPTR*)params)[i]; break;
            case (8): *(UQUAD *)ptr = (UQUAD)((IPTR*)params)[i]; break;
        }
        ptr += dsize;
    }
#endif

    if (!app)
    {
        struct EasyStruct es;
        es.es_StructSize   = sizeof(struct EasyStruct);
        es.es_Flags        = 0;
        es.es_Title        = title;
        es.es_TextFormat   = format;
        es.es_GadgetFormat = gadgets;
        return EasyRequestArgs(NULL,&es,NULL,_params);
    }

    reqtxt_len = 0;
#ifdef __AROS__
    RawDoFmt(format,_params,(VOID_FUNC)AROS_ASMSYMNAME(len_func),&reqtxt_len);
#else
    RawDoFmt(format,params,(void(*)())&len_func,&reqtxt_len);
#endif

    /* Return cancel if something failed */
    if (!(reqtxt = AllocVec(reqtxt_len+1,0)))
        return 0;

#ifdef __AROS__
    {
        char *reqtxtptr = reqtxt;

        RawDoFmt(format, _params, (VOID_FUNC)AROS_ASMSYMNAME(cpy_func),
            &reqtxtptr);
    }
#else
    RawDoFmt(format, params, (void(*)())&cpy_func, reqtxt);
#endif
    
    if (title == NULL && app != NULL)
    {
        title = (CONST_STRPTR) XGET(app, MUIA_Application_Title);
    }

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
                TextFrame,
                MUIA_Background,    MUII_TextBack,
                Child, HSpace(0),
                Child, TextObject,
                    MUIA_InnerBottom,   8,
                    MUIA_InnerLeft,     8,
                    MUIA_InnerRight,    8,
                    MUIA_InnerTop,      8,
                    MUIA_Text_SetMax,   TRUE,
                    MUIA_Text_Contents, reqtxt,
                    End,
                Child, HSpace(0),
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
            IPTR isopen = FALSE;

//            set(app, MUIA_Application_Sleep, TRUE);
            DoMethod(app, OM_ADDMEMBER, (IPTR)req_wnd);

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
                DoMethod(req_group, OM_ADDMEMBER, (IPTR)HSpace(0));
                DoMethod(req_group, OM_ADDMEMBER, (IPTR)req_but[0]);
                DoMethod(req_group, OM_ADDMEMBER, (IPTR)HSpace(0));
                DoMethod(req_but[0], MUIM_Notify, MUIA_Pressed, FALSE,
                         (IPTR)app, 2, MUIM_Application_ReturnID, 2);
            }
            else
            {
                int j;

                for(j = 0; j < num_gads; j++)
                {
                    if (j > 0)
                    {
                        DoMethod(req_group, OM_ADDMEMBER, (IPTR)HSpace(0));
                    }
                    DoMethod(req_group, OM_ADDMEMBER, (IPTR)req_but[j]);
                    DoMethod(req_but[j], MUIM_Notify, MUIA_Pressed, FALSE,
                             (IPTR)app, 2, MUIM_Application_ReturnID,
                             (j+2 <= num_gads) ? j+2 : 1);
                    set(req_but[j], MUIA_CycleChain, 1);
                }
            }
            DoMethod(req_group, MUIM_Group_ExitChange);

            /* now activate that button with a starting "*" */
            if(active != -1) set(req_wnd, MUIA_Window_ActiveObject, (IPTR) req_but[active]);

            /* lets collect the waiting returnIDs now */
            // COLLECT_RETURNIDS;

            set(req_wnd, MUIA_Window_Open, TRUE);
            get(req_wnd, MUIA_Window_Open, &isopen);

            if (isopen)
            {
                ULONG sigs = 0;
                result = -1;

                while (result == -1)
                {
                    ULONG ret = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);

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

//            set(app, MUIA_Application_Sleep, FALSE);
        }

        DoMethod(app, OM_REMMEMBER, (IPTR)req_wnd);
        MUI_DisposeObject(req_wnd);
    }
    return result;

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestA */
