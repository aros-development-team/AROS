/* List subclass */

#if !defined(__AROS__)
#define __NOLIBBASE__
#endif

#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#if defined(__AROS__)
#include <stdio.h>
#else
#include <exec/rawfmt.h>
#endif
#include <dos/dos.h>
#include <string.h>
#include <syslog.h>

#include <libraries/mui.h>
#include <mui/NList_mcc.h>

#include "main.h"
#include "colorlist.h"
#include "hooks.h"
#include "str.h"

struct MUI_CustomClass *ColorList;

static char *months =
  "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";

static char *wdays = 
  "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";

char *levels[] = {
  "emergency",
  "alert",
  "critical",
  "error",
  "warning",
  "note",
  "info",
  "debug"
};

#if !defined(__AROS__)
LONG d_ColorList(void)
{
  Class *cl = (Class*)REG_A0;
  Object *obj = (Object*)REG_A2;
  Msg msg = (Msg)REG_A1;
#else
BOOPSI_DISPATCHER(IPTR, d_ColorList, cl, obj, msg)
{
#endif

  switch (msg->MethodID)
  {
    case MUIM_Show:
    {
      struct ColorListData *d = INST_DATA(cl, obj);
      struct MUI_PenSpec *spec;
      struct MUI_RenderInfo *mri;

      DoSuperMethodA(cl, obj, msg);
      mri = muiRenderInfo(obj);
      GetAttr(MUIA_Pendisplay_Spec, findobj(SWin, PREFS_POPPEN_ERRORS), (ULONG*)&spec);
      d->ErrorPen = MUI_ObtainPen(mri, spec, 0);
      GetAttr(MUIA_Pendisplay_Spec, findobj(SWin, PREFS_POPPEN_IMPORTANT), (ULONG*)&spec);
      d->InfoPen = MUI_ObtainPen(mri, spec, 0);
      GetAttr(MUIA_Pendisplay_Spec, findobj(SWin, PREFS_POPPEN_OTHERS), (ULONG*)&spec);
      d->VerbosePen = MUI_ObtainPen(mri, spec, 0);
    }
    return TRUE;

    case MUIM_Hide:
    {
      struct ColorListData *d = INST_DATA(cl, obj);
      struct MUI_RenderInfo *mri;

      mri = muiRenderInfo(obj);
      MUI_ReleasePen(mri, d->ErrorPen);
      MUI_ReleasePen(mri, d->InfoPen);
      MUI_ReleasePen(mri, d->VerbosePen);
    }
    return DoSuperMethodA(cl, obj, msg);

    case CLL_ChangePens:
    {
      struct ColorListData *d = INST_DATA(cl, obj);
      struct MUI_PenSpec *spec;
      struct MUI_RenderInfo *mri;

      mri = muiRenderInfo(obj);
      MUI_ReleasePen(mri, d->ErrorPen);
      MUI_ReleasePen(mri, d->InfoPen);
      MUI_ReleasePen(mri, d->VerbosePen);
      GetAttr(MUIA_Pendisplay_Spec, findobj(SWin, PREFS_POPPEN_ERRORS), (ULONG*)&spec);
      d->ErrorPen = MUI_ObtainPen(mri, spec, 0);
      GetAttr(MUIA_Pendisplay_Spec, findobj(SWin, PREFS_POPPEN_IMPORTANT), (ULONG*)&spec);
      d->InfoPen = MUI_ObtainPen(mri, spec, 0);
      GetAttr(MUIA_Pendisplay_Spec, findobj(SWin, PREFS_POPPEN_OTHERS), (ULONG*)&spec);
      d->VerbosePen = MUI_ObtainPen(mri, spec, 0);
      DoMethod(obj, MUIM_NList_Redraw, MUIV_List_Redraw_All);
    }
    return 0;

    case MUIM_NList_Construct:
    {
      struct MUIP_NList_Construct *msg2 = (struct MUIP_NList_Construct*)msg;
      struct SysLogPacket *slp = (struct SysLogPacket*)msg2->entry;
      APTR mpool = (APTR)msg2->pool;
      struct SysLogEntry *sle;

      if (sle = (struct SysLogEntry*)AllocPooled(mpool, sizeof(struct SysLogEntry)))
      {
	sle->Level = slp->Level;

	if (sle->ProcessName = strnew(mpool, slp->Tag))
        {
	    if (sle->EventDescription = strnew(mpool, slp->String))
            {
		struct ClockData clockdata;
		Amiga2Date(slp->Time, &clockdata);
		if (sle->Time = fmtnew(mpool, "%s %s %02d %02d:%02d:%02d %4d",
					wdays + 4 * clockdata.wday,
					months + 4 * (clockdata.month - 1),
					clockdata.mday,
					clockdata.hour,
					clockdata.min,
					clockdata.sec,
					clockdata.year))
		{
			return (LONG)sle;
                }
            }
        }
      }
    }
    return 0;

    case MUIM_NList_Display:
    {
      struct MUIP_NList_Display *msg2 = (struct MUIP_NList_Display*)msg;
			struct SysLogEntry *sle = (struct SysLogEntry*)msg2->entry;
      struct ColorListData *d = INST_DATA(cl, obj);

      if (!sle)      // render titles
      {
	msg2->strings[0] = "\33c\33bTime";
	msg2->strings[1] = "\33c\33bLevel";
	msg2->strings[2] = "\33c\33bProcess";
	msg2->strings[3] = "\33c\33bEvent";
      }
      else          // render entry
      {
	static UBYTE time[64], appname[128], level[32];
	static UBYTE event[512];
        LONG id = 0;

	switch (LOG_PRI(sle->Level))
        {
		case LOG_EMERG:
		case LOG_ALERT:
		case LOG_CRIT:
		case LOG_ERR:      id = d->ErrorPen;      break;
		case LOG_WARNING:
		case LOG_NOTICE:
        	case LOG_INFO:     id = d->InfoPen;       break;
		case LOG_DEBUG:    id = d->VerbosePen;    break;
        }

#if defined(__AROS__)
		sprintf(time, "\33P[%ld]%s", id, sle->Time);
		sprintf(level, "\33P[%ld]%s", id, levels[LOG_PRI(sle->Level)]);
		sprintf(appname, "\33P[%ld]%s", id, sle->ProcessName);
		sprintf(event, "\33P[%ld]%s", id, sle->EventDescription);
#else
	NewRawDoFmt("\33P[%ld]%s", RAWFMTFUNC_STRING, time, id, sle->Time);
	NewRawDoFmt("\33P[%ld]%s", RAWFMTFUNC_STRING, level, id, levels[LOG_PRI(sle->Level)]);
	NewRawDoFmt("\33P[%ld]%s", RAWFMTFUNC_STRING, appname, id, sle->ProcessName);
	NewRawDoFmt("\33P[%ld]%s", RAWFMTFUNC_STRING, event, id, sle->EventDescription);
#endif

        msg2->strings[0] = time;
	msg2->strings[1] = level;
	msg2->strings[2] = appname;
	msg2->strings[3] = event;
      }
    }
    return 0;

    default: return DoSuperMethodA(cl, obj, msg);
  }
}
#if defined(__AROS__)
BOOPSI_DISPATCHER_END
#endif

#if !defined(__AROS__)
struct EmulLibEntry g_ColorList =
 {TRAP_LIB, 0, (void(*)(void))d_ColorList};
#endif

struct MUI_CustomClass *CreateColorListClass(void)
{
  return (MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct ColorListData), 
#if !defined(__AROS__)
               (APTR)&g_ColorList
#else
		d_ColorList
#endif
  ));
}

