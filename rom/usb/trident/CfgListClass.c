
/*****************************************************************************
** This is the CfgList custom class, a sub class of List.mui.
******************************************************************************/

#include "debug.h"

#define USE_INLINE_STDARG
#define __NOLIBBASE__
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/usbclass.h>
#include <proto/icon.h>
#include <proto/utility.h>

#include "Trident.h"
#include "ActionClass.h"
#include "IconListClass.h"
#include "DevWinClass.h"
#include "CfgListClass.h"

extern struct ExecBase *SysBase;
extern struct Library *ps;
extern struct IntuitionBase *IntuitionBase;

/* /// "CheckDragAccept()" */
BOOL CheckDragAccept(Object *obj, LONG targetentry)
{
    struct PrefsListEntry *targetplnode = NULL;
    struct PrefsListEntry *sourceplnode = NULL;
    ULONG sourceid, targetid;
    IPTR sourceentry = -1;

    get(obj, MUIA_List_Active, &sourceentry);
    if((((LONG) sourceentry) < 0) || (targetentry < 0))
    {
        return(FALSE);
    }
    DoMethod(obj, MUIM_List_GetEntry, sourceentry, &sourceplnode);
    DoMethod(obj, MUIM_List_GetEntry, targetentry, &targetplnode);
    if((!(sourceplnode && targetplnode)) || (sourceplnode == targetplnode))
    {
        return(FALSE);
    }
    sourceid = sourceplnode->chunkid;
    targetid = targetplnode->chunkid;

    switch(sourceid)
    {
        case IFFFORM_DEVICECFG:
            if(targetid == IFFFORM_DEVICECFG)
            {
                return(TRUE);
            }
            break;

        case IFFFORM_DEVCFGDATA:
            if((targetid == IFFFORM_DEVICECFG) ||
               (targetid == IFFFORM_DEVCFGDATA))
            {
                return(TRUE);
            }
            break;

        case IFFFORM_IFCFGDATA:
            if((targetid == IFFFORM_DEVICECFG) ||
               (targetid == IFFFORM_IFCFGDATA))
            {
                return(TRUE);
            }
            break;

        case IFFCHNK_FORCEDBIND:
            if((targetid == IFFFORM_DEVICECFG) ||
               (targetid == IFFFORM_DEVCFGDATA) ||
               (targetid == IFFFORM_IFCFGDATA) ||
               (targetid == IFFCHNK_FORCEDBIND))
            {
                return(TRUE);
            }
            break;
    }
    return(FALSE);
}
/* \\\ */

/* /// "ApplyDragAction()" */
BOOL ApplyDragAction(Object *obj, LONG targetentry)
{
    struct PrefsListEntry *targetplnode = NULL;
    struct PrefsListEntry *sourceplnode = NULL;
    ULONG sourceid, targetid;
    IPTR sourceentry = -1;
    LONG result;
    APTR pic, spic, tpic;
    APTR form;

    get(obj, MUIA_List_Active, &sourceentry);
    if((((LONG) sourceentry) < 0) || (targetentry < 0))
    {
        return(FALSE);
    }
    DoMethod(obj, MUIM_List_GetEntry, sourceentry, &sourceplnode);
    DoMethod(obj, MUIM_List_GetEntry, targetentry, &targetplnode);
    if((!(sourceplnode && targetplnode)) || (sourceplnode == targetplnode))
    {
        return(FALSE);
    }
    sourceid = sourceplnode->chunkid;
    targetid = targetplnode->chunkid;

    switch(sourceid)
    {
        case IFFFORM_DEVICECFG:
            if(targetid == IFFFORM_DEVICECFG)
            {
                result = MUI_Request(_app(obj), _win(obj), 0, NULL, "Replace|Merge|Cancel",
                                     "Do you want to \33breplace\33n or \33bmerge\33n the prefs of\n"
                                     "\33b%s\33n\n"
                                     "with the contents from\n"
                                     "\33b%s\33n?", targetplnode->id, sourceplnode->id);
                if(result < 1)
                {
                    return(FALSE);
                }
                tpic = NULL;
                spic = NULL;
                pic = psdFindCfgForm(NULL, IFFFORM_DEVICECFG);
                while(pic)
                {
                    if(psdMatchStringChunk(pic, IFFCHNK_DEVID, targetplnode->devid))
                    {
                        tpic = pic;
                    }
                    if(psdMatchStringChunk(pic, IFFCHNK_DEVID, sourceplnode->devid))
                    {
                        spic = pic;
                    }
                    pic = psdNextCfgForm(pic);
                }
                if(!(tpic && spic))
                {
                    return(FALSE);
                }
                // delete old contents
                if(result == 1)
                {
                    while((pic = psdFindCfgForm(tpic, IFFFORM_DEVCFGDATA)))
                    {
                        psdRemCfgForm(pic);
                    }
                    while((pic = psdFindCfgForm(tpic, IFFFORM_IFCFGDATA)))
                    {
                        psdRemCfgForm(pic);
                    }
                }

                // copy device cfg data
                pic = psdFindCfgForm(spic, IFFFORM_DEVCFGDATA);
                while(pic)
                {
                    form = psdWriteCfg(pic);
                    if(form)
                    {
                        psdAddCfgEntry(tpic, form);
                        psdFreeVec(form);
                    }
                    pic = psdNextCfgForm(pic);
                }
                // copy interface cfg data
                pic = psdFindCfgForm(spic, IFFFORM_IFCFGDATA);
                while(pic)
                {
                    form = psdWriteCfg(pic);
                    if(form)
                    {
                        psdAddCfgEntry(tpic, form);
                        psdFreeVec(form);
                    }
                    pic = psdNextCfgForm(pic);
                }
                return(TRUE);
            }
            break;

        case IFFFORM_IFCFGDATA:
        case IFFFORM_DEVCFGDATA:
            if((targetid == IFFFORM_DEVCFGDATA) || (targetid == IFFFORM_IFCFGDATA))
            {
                if(strcmp(sourceplnode->owner, targetplnode->owner))
                {
                    result = MUI_RequestA(_app(obj), _win(obj), 0, NULL, "Add to device|Cancel",
                                          "Sorry, but only prefs of the same owner can\n"
                                          "be replaced.\n\n"
                                          "Do you wish to add this prefs\n"
                                          "to the device instead?", NULL);
                    if(result < 1)
                    {
                        return(FALSE);
                    }
                    targetid = IFFFORM_DEVICECFG;
                } else {
                    result = MUI_Request(_app(obj), _win(obj), 0, NULL, "Replace|Cancel",
                                         "Do you want to \33breplace\33n the prefs of\n"
                                         "\33b%s\33n\n"
                                         "by those of\n"
                                         "\33b%s\33n?", targetplnode->id, sourceplnode->id);

                    if(result < 1)
                    {
                        return(FALSE);
                    }
                    pic = psdGetUsbDevCfg(sourceplnode->owner, sourceplnode->devid, sourceplnode->ifid);
                    if(pic)
                    {
                        form = psdWriteCfg(pic);
                        if(form)
                        {
                            psdSetUsbDevCfg(sourceplnode->owner, targetplnode->devid, targetplnode->ifid, form);
                            psdFreeVec(form);
                        }
                    }
                    return(TRUE);
                }
            }
            if(targetid == IFFFORM_DEVICECFG)
            {
                pic = psdGetUsbDevCfg(sourceplnode->owner, targetplnode->devid, sourceplnode->ifid);
                if(pic)
                {
                    result = MUI_Request(_app(obj), _win(obj), 0, NULL, "Replace|Cancel",
                                         "Do you want to \33breplace\33n the prefs of\n"
                                         "\33b%s\33n\n"
                                         "by the one in\n"
                                         "\33b%s\33n?", targetplnode->id, sourceplnode->id);
                } else {
                    result = MUI_Request(_app(obj), _win(obj), 0, NULL, "Add|Cancel",
                                         "Do you want to \33badd\33n the prefs of\n"
                                         "\33b%s\33n\n"
                                         "to the device\n"
                                         "\33b%s\33n?", sourceplnode->id, targetplnode->id);
                }
                if(result < 1)
                {
                    return(FALSE);
                }
                pic = psdGetUsbDevCfg(sourceplnode->owner, sourceplnode->devid, sourceplnode->ifid);
                if(pic)
                {
                    form = psdWriteCfg(pic);
                    if(form)
                    {
                        psdSetUsbDevCfg(sourceplnode->owner, targetplnode->devid, sourceplnode->ifid, form);
                        psdFreeVec(form);
                    }
                }
                return(TRUE);
            }
            break;

        case IFFCHNK_FORCEDBIND:
            if((targetid == IFFFORM_DEVICECFG) ||
               (targetid == IFFFORM_DEVCFGDATA) ||
               (targetid == IFFFORM_IFCFGDATA) ||
               (targetid == IFFCHNK_FORCEDBIND))
            {
                psdSetForcedBinding(sourceplnode->owner, targetplnode->devid, targetplnode->ifid);
                return(TRUE);
            }
            break;
    }
    return(FALSE);
}
/* \\\ */

/* /// "CfgListDispatcher()" */
AROS_UFH3(IPTR, CfgListDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    // There should never be an uninitialized pointer, but just in case, try to get an mungwall hit if so.
    struct CfgListData *data = (struct CfgListData *) 0xABADCAFE;

    // on OM_NEW the obj pointer will be void, so don't try to get the data base in this case.
    if(msg->MethodID != OM_NEW) data = INST_DATA(cl,obj);

    switch(msg->MethodID)
    {
        case OM_NEW:
            if(!(obj = (Object *) DoSuperMethodA(cl,obj,msg)))
                return(0);
            return((IPTR) obj);

        case MUIM_DragBegin:
            data->cl_Dragging = TRUE;
            break;

        case MUIM_DragFinish:
            data->cl_Dragging = FALSE;
            break;

        case MUIM_DragQuery:
        {
            struct MUI_List_TestPos_Result tpr;
            struct Window *win;

            win = _window(obj);
            if(!win)
            {
                return(MUIV_DragQuery_Refuse);
            }
            DoMethod(obj, MUIM_List_TestPos, win->MouseX, win->MouseY, &tpr);
            return((IPTR) (CheckDragAccept(obj, tpr.entry) ? MUIV_DragQuery_Accept : MUIV_DragQuery_Refuse));
        }

#ifndef MUI_LPR_FULLDROP
#define MUI_LPR_FULLDROP (1<<15)
#endif
        case MUIM_List_TestPos:
        {
            struct MUIP_List_TestPos *tpmsg = (struct MUIP_List_TestPos *) msg;
            struct MUI_List_TestPos_Result *res = tpmsg->res;
            IPTR rc;
            rc = DoSuperMethodA(cl, obj, msg);
            if(data->cl_Dragging && (res->entry != -1))
            {
                if(!CheckDragAccept(obj, res->entry))
                {
                    res->entry = -1; // illegal combination
                } else {
                    res->flags |= MUI_LPR_FULLDROP;
                }
            }
            return(rc);
        }

        case MUIM_DragDrop:
        {
            //struct MUIP_DragDrop *ddmsg = (struct MUIP_DragDrop *) msg;
            struct MUI_List_TestPos_Result tpr;
            struct Window *win;

            win = _window(obj);
            if(!win)
            {
                return(FALSE);
            }
            DoMethod(obj, MUIM_List_TestPos, win->MouseX, win->MouseY, &tpr);
            //DoMethod(obj, MUIM_List_TestPos, ddmsg->x, ddmsg->y, &tpr);
            if(CheckDragAccept(obj, tpr.entry))
            {
                ApplyDragAction(obj, tpr.entry);
            } else {
                MUI_RequestA(_app(obj), _win(obj), 0, NULL, "Oops!",
                             "Sorry, drag'n drop operation to\n"
                             "that target is not supported.", NULL);
                return(FALSE);
            }
            return(TRUE);
        }

    }
    return(DoSuperMethodA(cl,obj,msg));
    AROS_USERFUNC_EXIT
}
/* \\\ */
