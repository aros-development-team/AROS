#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define HANDLER_NAME "con-handler"
#define VERSION      41
#define REVISION     5

#define _STR(A) #A
#define STR(A) _STR(A)

extern LONG CONMain(void);

static const TEXT dev_name_con[] = "CON";
static const TEXT dev_name_raw[] = "RAW";

extern const char con_End;
static const TEXT version_string[];

static AROS_UFP3 (APTR, con_Init,
		  AROS_UFPA(APTR, unused, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

const struct Resident con_romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&con_romtag,
   (APTR)&con_End + 1,
   RTF_AFTERDOS,
   VERSION,
   NT_PROCESS,
   -124,
   HANDLER_NAME,
   (STRPTR)version_string,
   con_Init
};

static const TEXT version_string[] = HANDLER_NAME " " STR(VERSION) "." STR(REVISION) " (" ADATE ")\n";

static AROS_UFH3 (APTR, con_Init,
		  AROS_UFHA(APTR, unused, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
   AROS_USERFUNC_INIT

   struct DosLibrary *DOSBase;
   struct DeviceNode *dev_node;
   BPTR conseg;

   DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
   if (DOSBase == NULL)
       Alert(AG_OpenLib | AO_DOSLib);

   /* Create device node and add it to the system. The handler will then be
      started when it is first accessed */
   conseg = CreateSegList(CONMain);
   if (conseg == BNULL)
       Alert(AT_DeadEnd | AG_NoMemory);

   dev_node = (APTR)MakeDosEntry(dev_name_con, DLT_DEVICE);
   if(dev_node == NULL)
      Alert(AT_DeadEnd | AG_NoMemory);

   dev_node->dn_StackSize = AROS_STACKSIZE;
   dev_node->dn_SegList = conseg;
   dev_node->dn_Startup = 0;
   dev_node->dn_Priority = 5;
   dev_node->dn_GlobalVec = (BPTR)(SIPTR)-1;
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   dev_node = (APTR)MakeDosEntry(dev_name_raw, DLT_DEVICE);
   if(dev_node == NULL)
      Alert(AT_DeadEnd | AG_NoMemory);
   dev_node->dn_StackSize = AROS_STACKSIZE;
   dev_node->dn_SegList = conseg;
   dev_node->dn_Startup = (BPTR)1;
   dev_node->dn_Priority = 5;
   dev_node->dn_GlobalVec = (BPTR)(SIPTR)-1;
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   D(bug("CON: and RAW: added\n"));

   CloseLibrary(&DOSBase->dl_lib);
   return NULL;

   AROS_USERFUNC_EXIT
}
