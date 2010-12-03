
#define DEBUG 1
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "con_handler_intern.h"
#include "support.h"

#define _STR(A) #A
#define STR(A) _STR(A)

#define HANDLER_NAME "console.handler"
#define VERSION 43
#define REVISION 1

#ifdef __AROS__
#define rom_tag Con_ROMTag
#endif

extern LONG CONMain(void);

static AROS_UFP3 (APTR, Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT handler_name[] = HANDLER_NAME;
static const TEXT version_string[] =
   HANDLER_NAME " " STR(VERSION) "." STR(REVISION) " (" __DATE__ ")\n";
static const TEXT dev_name_con[] = "CON";
static const TEXT dev_name_raw[] = "RAW";

extern const TEXT dos_name[];

const struct Resident rom_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&rom_tag,
   (APTR)(&rom_tag + 1),
   RTF_AFTERDOS,
   VERSION,
   NT_PROCESS,
   -125,
   (STRPTR)handler_name,
   (STRPTR)version_string,
   (APTR)Init
};

#ifdef DOSBase
#undef DOSBase
#endif

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
   struct DosLibrary *DOSBase;
   struct DeviceNode *dev_node;

    AROS_USERFUNC_INIT

   /* Create device node and add it to the system. The handler will then be
      started when it is first accessed */

   DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);

   dev_node = (APTR)MakeDosEntry(dev_name_con, DLT_DEVICE);
   if(dev_node == NULL)
      Alert(AT_DeadEnd | AG_NoMemory);
   dev_node->dn_StackSize = 10000;
   dev_node->dn_SegList = MKBADDR((BPTR *)CONMain - 1);
   dev_node->dn_Startup = 0;
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   dev_node = (APTR)MakeDosEntry(dev_name_raw, DLT_DEVICE);
   if(dev_node == NULL)
      Alert(AT_DeadEnd | AG_NoMemory);
   dev_node->dn_StackSize = 10000;
   dev_node->dn_SegList = MKBADDR((BPTR *)CONMain - 1);
   dev_node->dn_Startup = 1;
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   D(bug("CON: and RAW: added\n"));

   AROS_USERFUNC_EXIT

   return NULL;
}