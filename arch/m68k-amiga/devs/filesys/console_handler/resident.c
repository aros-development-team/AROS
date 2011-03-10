
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
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   dev_node = (APTR)MakeDosEntry(dev_name_raw, DLT_DEVICE);
   if(dev_node == NULL)
      Alert(AT_DeadEnd | AG_NoMemory);
   dev_node->dn_StackSize = AROS_STACKSIZE;
   dev_node->dn_SegList = conseg;
   dev_node->dn_Startup = 1;
   dev_node->dn_Priority = 5;
   if(!AddDosEntry((APTR)dev_node))
      Alert(AT_DeadEnd);

   D(bug("CON: and RAW: added\n"));

   CloseLibrary(DOSBase);
   return TRUE;
}

/* NOTE: This is only here because architectures cannot
 *       override a libraries's *.conf file
 */
AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct conbase *, conbase, 5, Con)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct conbase *, conbase, 6, Con)
{
	AROS_LIBFUNC_INIT
	return 0;
	AROS_LIBFUNC_EXIT
}
ADD2INITLIB(GM_UNIQUENAME(Init),0)
