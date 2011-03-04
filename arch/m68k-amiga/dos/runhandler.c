
#define  DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/filesystem.h>

#include <string.h>
#include <stdio.h>

struct Process *RunPacketHandler(struct DeviceNode *dn, const char *path, struct DosLibrary *DOSBase)
{
	struct FileSysStartupMsg *fssm;
	struct DosPacket *dp;
	struct MsgPort *reply_port;
	struct Process *process = NULL;
	UBYTE *bpath;

	if (dn->dn_SegList == BNULL) {
	    struct Segment *seg;
	    CONST_STRPTR cp;

	    /* If no handler name was supplied,
	     * try to use the default.
	     */
	    if (dn->dn_Handler == BNULL) {
	    	dn->dn_SegList = DOSBase->dl_Root->rn_FileHandlerSegment;
	    } else {
	    	cp = AROS_BSTR_ADDR(dn->dn_Handler);

	    	/* Try to find in the Resident Segment list */
	    	Forbid();
	    	seg = FindSegment(cp, NULL, TRUE);
	    	Permit();
	    	if (seg != NULL)
		    dn->dn_SegList = seg->seg_Seg;
	    }
	}
	if (dn->dn_SegList == BNULL) {
		D(bug("[packet] name '%b' seglist=NULL?\n", dn->dn_Name));
		return NULL;
	}

	if (path) {
		bpath = AllocVec(strlen(path) + 2, MEMF_PUBLIC);
		strcpy (bpath + 1, path);
	} else {
		bpath = AllocVec(strlen(AROS_DOSDEVNAME(dn)) + 3, MEMF_PUBLIC);
		strcpy (bpath + 1, AROS_DOSDEVNAME(dn));
		strcat (bpath + 1, ":");
	}
	bpath[0] = strlen(bpath + 1);

	D(bug("[packet] in open by Task '%s'\n", FindTask(NULL)->tc_Node.ln_Name));

	fssm = (struct FileSysStartupMsg *)BADDR(dn->dn_Startup);

   	D(bug("[packet] devicenode=%08lx path='%s' devicename '%b' unit %d dosname '%b' handler=%x seg=%08lx startup=%08lx\n",
            dn,
            bpath + 1,
            fssm->fssm_Device,
            fssm->fssm_Unit,
            dn->dn_Name,
            dn->dn_Handler,
            dn->dn_SegList,
            dn->dn_Startup));

        /* start it up */
        process = CreateNewProcTags(
        	NP_Entry,     (IPTR) BADDR(dn->dn_SegList + 1),
		NP_Name,      (char*)BADDR(dn->dn_Name) + 1, /* GB: always NUL terminated */
		NP_StackSize, dn->dn_StackSize,
		NP_Priority,  dn->dn_Priority,
		TAG_DONE);
  
        D(bug("[packet] started, process structure is 0x%08x\n", process));
        reply_port = CreateMsgPort();

        /* build the startup packet */
        dp = (struct DosPacket *) AllocDosObject(DOS_STDPKT, NULL);
        dp->dp_Arg1 = (SIPTR)MKBADDR(bpath);
        dp->dp_Arg2 = (SIPTR)dn->dn_Startup;
        dp->dp_Arg3 = (SIPTR)MKBADDR(dn);
        dp->dp_Port = reply_port;

        /* A handler can add volumes during startup, so we have to be fully functional before it
           replies the startup packet */

        D(bug("[packet] sending startup packet port=%x\n", &(process->pr_MsgPort)));
        PutMsg(&(process->pr_MsgPort), dp->dp_Link);
        WaitPort(reply_port);
        GetMsg(reply_port);

        DeleteMsgPort(reply_port);

        if (dp->dp_Res1 == DOSFALSE) {
            D(bug("[packet] handler failed startup [%d]\n", dp->dp_Res2));
            process = NULL;
        } else {
            D(bug("[packet] '%b' now online\n", dn->dn_Name));
        }

        FreeDosObject(DOS_STDPKT, dp);

	return process;
}
