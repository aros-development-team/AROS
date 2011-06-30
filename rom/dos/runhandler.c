
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

AROS_LH2(struct MsgPort *, RunHandler,
	 AROS_LHA(struct DeviceNode *, deviceNode, A0),
	 AROS_LHA(const char *, path, A1),
	 struct DosLibrary *, DOSBase, 27, Dos)
{
	AROS_LIBFUNC_INIT

	struct FileSysStartupMsg *fssm;
	struct DosPacket *dp;
	struct MsgPort *reply_port;
	struct Process *process = NULL;
	BSTR bpath;

	/* First check if already started */
	if (deviceNode->dn_Task)
	    return deviceNode->dn_Task;

	if (deviceNode->dn_SegList == BNULL) {
	    struct Segment *seg = NULL;
	    CONST_STRPTR cp;

	    if (deviceNode->dn_Handler != BNULL) {
	    	cp = AROS_BSTR_ADDR(deviceNode->dn_Handler);

	    	/* Try to find in the Resident Segment list */
	    	Forbid();
	    	seg = FindSegment(cp, NULL, TRUE);
	    	Permit();
	    	if (seg == NULL) {
		    D(bug("[RunHandler] handler '%s' not found\n", cp));
		    return NULL;
		}
	    }
	    deviceNode->dn_SegList = seg ? seg->seg_Seg : DOSBase->dl_Root->rn_FileHandlerSegment;
	}
	if (deviceNode->dn_SegList == BNULL) {
		D(bug("[RunHandler] name '%b' seglist=NULL?\n", deviceNode->dn_Name));
		return NULL;
	}

	if (path) {
		bpath = MKBADDR(AllocVec(strlen(path) + 2, MEMF_PUBLIC));
		if (bpath == BNULL)
		    return NULL;

		strcpy(AROS_BSTR_ADDR(bpath), path);
	} else {
		bpath = MKBADDR(AllocVec(strlen(AROS_DOSDEVNAME(deviceNode)) + 3, MEMF_PUBLIC));
		strcpy (AROS_BSTR_ADDR(bpath), AROS_DOSDEVNAME(deviceNode));
		strcat (AROS_BSTR_ADDR(bpath), ":");
	}
	AROS_BSTR_setstrlen(bpath, strlen(AROS_BSTR_ADDR(bpath)));

	D(bug("[RunHandler] in open by Task '%s'\n", FindTask(NULL)->tc_Node.ln_Name));

	fssm = (struct FileSysStartupMsg *)BADDR(deviceNode->dn_Startup);

   	D(bug("[RunHandler] devicenode=%08lx path='%b' devicename '%b' unit %d dosname '%b' handler=%x seg=%08lx startup=%08lx\n",
            deviceNode,
            bpath,
            fssm->fssm_Device,
            fssm->fssm_Unit,
            deviceNode->dn_Name,
            deviceNode->dn_Handler,
            deviceNode->dn_SegList,
            deviceNode->dn_Startup));

        /* start it up */
        process = CreateNewProcTags(
        NP_Entry, (IPTR)BADDR(deviceNode->dn_SegList)+sizeof(IPTR),
		NP_Name,  AROS_BSTR_ADDR(deviceNode->dn_Name), /* GB: always NUL terminated */
		NP_StackSize, deviceNode->dn_StackSize,
		NP_Priority,  deviceNode->dn_Priority,
		TAG_DONE);
  
        D(bug("[RunHandler] started, process structure is 0x%08x\n", process));
        reply_port = CreateMsgPort();
        if (!reply_port) {
            FreeVec(BADDR(bpath));
            return NULL;
        }

        /* build the startup packet */
        dp = (struct DosPacket *) AllocDosObject(DOS_STDPKT, NULL);
        if (!dp) {
            DeleteMsgPort(reply_port);
            FreeVec(BADDR(bpath));
            return NULL;
        }
        dp->dp_Arg1 = (SIPTR)bpath;
        dp->dp_Arg2 = (SIPTR)deviceNode->dn_Startup;
        dp->dp_Arg3 = (SIPTR)MKBADDR(deviceNode);
        dp->dp_Port = reply_port;

        /* A handler can add volumes during startup, so we have to be fully functional before it
           replies the startup packet */

        D(bug("[RunHandler] sending startup packet port=%x\n", &(process->pr_MsgPort)));
        PutMsg(&(process->pr_MsgPort), dp->dp_Link);
        WaitPort(reply_port);
        GetMsg(reply_port);

        DeleteMsgPort(reply_port);

        if (dp->dp_Res1 == DOSFALSE) {
            D(bug("[RunHandler] handler failed startup [%d]\n", dp->dp_Res2));
            process = NULL;
        } else {
            D(bug("[RunHandler] '%b' now online\n", deviceNode->dn_Name));
        }

        FreeDosObject(DOS_STDPKT, dp);

	return process ? &process->pr_MsgPort : NULL;

	AROS_LIBFUNC_EXIT
}
