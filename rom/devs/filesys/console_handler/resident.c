
#define DEBUG 0
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "con_handler_intern.h"
#include "support.h"

#include LC_LIBDEFS_FILE

extern LONG CONMain(void);

static const TEXT dev_name_con[] = "CON";
static const TEXT dev_name_raw[] = "RAW";

/* This is both a segment, and the data for the segment.
 * We will feed in to DOS/AddSegment() the BPTR to 
 * &aws_Next as the 'seglist' to add.
 */
#undef DOSBase

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR conbase)
{
   APTR DOSBase;
   struct DeviceNode *dev_node;
   BPTR conseg;

   DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
   if (DOSBase == NULL) {
       Alert(AG_OpenLib | AO_DOSLib);
       return FALSE;
   }

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

   CloseLibrary(DOSBase);
   return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
