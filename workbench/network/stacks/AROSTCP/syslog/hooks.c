/* SysLog Listview hooks */

#if !defined(__AROS__)
#define __NOLIBBASE__
#endif

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <clib/alib_protos.h>
#include <libraries/asl.h>
#include <mui/NList_mcc.h>
//#include <exec/rawfmt.h>
#include <string.h>
#include "main.h"
#include "colorlist.h"

extern STRPTR levels[];

/* PutCharFunc hook for FormatDate */

#if !defined(__AROS__)
void PutCharFunc(void)
{
  struct Hook *h = (struct Hook*)REG_A0;
  BYTE ch = (BYTE)REG_A1;
#else
AROS_UFH3(
    void, PutCharFunc,
    AROS_UFHA(struct Hook *,    h,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(BYTE,             ch,  A1)
)
{
    AROS_USERFUNC_INIT
#endif

#if !defined(__AROS__)
  *((STRPTR)h->h_Data)++ = ch;
#else
    AROS_USERFUNC_EXIT
#endif
}

#if !defined(__AROS__)
struct EmulLibEntry g_PutCharFunc = {TRAP_LIB, 0, (void (*)(void))PutCharFunc};
struct Hook h_PutCharFunc = {{NULL, NULL}, (HOOKFUNC)&g_PutCharFunc, NULL, NULL};
#else
struct Hook               h_PutCharFunc;
#endif

/* SaveAs hook called on menu notification, saves all log to a text file. */
/*
LONG SaveAsHook(void)
{
  Object *list = (Object*)REG_A2;
  struct FileRequester *freq;

  if (freq = MUI_AllocAslRequest(ASL_FileRequest, NULL))
  {
    if (MUI_AslRequestTags(freq,
     ASLFR_Window, (ULONG)_window(list),
     ASLFR_TitleText, "Save media log file",
     ASLFR_DoSaveMode, TRUE,
    TAG_END))
    {
      BPTR lock;

      if (lock = Lock(freq->fr_Drawer, SHARED_LOCK))
      {
        BPTR olddir, logfile;

        olddir = CurrentDir(lock);
        if (logfile = Open(freq->fr_File, MODE_NEWFILE))
        {
          LONG entry;
					struct SysLogEntry *sle;

          for (entry = 0; ; entry++)
          {
						DoMethod(list, MUIM_List_GetEntry, entry, (ULONG)&sle);
						if (!sle) break;
						FPrintf(logfile, "\"%s\", \"%s\", \"%s\", \"%s\"\n", sle->Time, levels[LOG_PRI(sle->Level)],
						 sle->ProcessName, sle->EventDescription);
          }
          Close(logfile);
        }
        CurrentDir(olddir);
        UnLock(lock);
      }
    }
    MUI_FreeAslRequest(freq);
  }
  return 0;
}

struct EmulLibEntry g_SaveAsHook = {TRAP_LIB, 0, (void (*)(void))SaveAsHook};
struct Hook h_SaveAsHook = {{NULL, NULL}, (HOOKFUNC)&g_SaveAsHook, NULL, NULL};
*/

