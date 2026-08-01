/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AUX: handler. Hands the startup packet to con-handler with a
          serial device underneath it instead of console.device.
*/

/*
  This implementation lifts con-handler, which can already drive any device
  that answers CMD_READ and CMD_WRITE.

  Line-oriented I/O, ACTION_WAIT_CHAR, ^C..^F handling and line editing are
  performed by con-handler.

  The handover is:
    - Find con-handler via FindSegment()
    - Point the startup packet at a FileSysStartupMsg naming the device
    - Set AUXF_DEVICE in dp_Arg2
    - Post the packet back to our own port
    - Call con-handler's entry point
    - Then con-handler starts up in this same process and picks up the
      packet as if it had been mounted directly.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

/*
 * Set in the startup BPTR of con-handler's startup packet to ask for a device
 * rather than a console window. A BPTR is a longword address shifted right by
 * two, so the top bits are free.
 */
#define AUXF_DEVICE             0x40000000

#define DEFAULT_DEVICE          "serial.device"

/*
 * con-handler registers itself under this name, so it can be found without
 * knowing where it was loaded from.
 */
#define CON_HANDLER_NAME        "con-handler"

/* A BSTR wants its length byte first and the whole thing longword aligned.
   One over the string size leaves room for that length byte. */
struct auxStartup
{
    struct FileSysStartupMsg fssm;
    UBYTE                    device[sizeof(DEFAULT_DEVICE) + 1];
};

static void buildDefaultStartup(struct auxStartup *startup,
                                struct ExecBase *SysBase)
{
    /* Length byte, then the string with its terminator: a BSTR that is also
       readable as a C string. CopyMem() rather than strcpy() because a
       handler cannot rely on a C library being reachable. */
    startup->device[0] = sizeof(DEFAULT_DEVICE) - 1;
    CopyMem(DEFAULT_DEVICE, &startup->device[1], sizeof(DEFAULT_DEVICE));

    startup->fssm.fssm_Unit    = 0;
    startup->fssm.fssm_Device  = MKBADDR(startup->device);
    startup->fssm.fssm_Environ = (BPTR)0;
    startup->fssm.fssm_Flags   = 0;
}

/*
 * Failure codes. A handler that swallows its startup packet without replying
 * wedges whoever opened it, so every path out of here has to reply, and saying
 * which step failed costs nothing. They are deliberately outside the range
 * dos.library knows, so Fault() prints the number rather than a wrong message.
 */
#define AUXERR_NO_DOS           2901    /* dos.library would not open */
#define AUXERR_NO_CON           2902    /* con-handler is not in the segment list */
#define AUXERR_CON_NOT_SYSTEM   2903    /* found, but not a resident segment */

LONG aux_handler(struct ExecBase *SysBase)
{
    struct DosLibrary *DOSBase;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct MsgPort *mp = &me->pr_MsgPort;
    struct Message *mn;
    struct DosPacket *dp;
    LONG err = AUXERR_NO_DOS;

    /*
     * con-handler is called rather than jumped to, so this frame stays live
     * underneath it for as long as the handler runs. That is what lets the
     * startup it is handed live on the stack.
     */
    struct auxStartup startup;

    WaitPort(mp);
    mn = GetMsg(mp);
    dp = (struct DosPacket *)mn->mn_Node.ln_Name;

    Forbid();

    if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36)))
    {
        struct Segment *seg = FindSegment(CON_HANDLER_NAME, NULL, CMD_SYSTEM);

        err = seg ? AUXERR_CON_NOT_SYSTEM : AUXERR_NO_CON;

        /* Only a resident con-handler will do: a loaded one would be
           unloaded out from under us when its user count drops. */
        if (seg && seg->seg_UC == CMD_SYSTEM)
        {
            struct FileSysStartupMsg *fssm;
            void (*conentry)(void);

            /* A seglist begins with the BPTR to the next segment. */
            conentry = (void (*)(void))((ULONG *)BADDR(seg->seg_Seg) + 1);

            /*
             * The mountlist may have supplied a startup naming any device and
             * unit, in which case it is used untouched. TypeOfMem() is how to
             * tell a real pointer from the small integers the built-in
             * mountlists use as a startup.
             */
            fssm = (struct FileSysStartupMsg *)BADDR(dp->dp_Arg2);

            if (!TypeOfMem(fssm))
            {
                struct DeviceNode *dn = (struct DeviceNode *)BADDR(dp->dp_Arg3);

                fssm = dn ? (struct FileSysStartupMsg *)BADDR(dn->dn_Startup)
                          : NULL;

                if (!TypeOfMem(fssm))
                {
                    buildDefaultStartup(&startup, SysBase);
                    fssm = &startup.fssm;
                }
            }

            dp->dp_Arg2 = (BPTR)((ULONG)MKBADDR(fssm) | AUXF_DEVICE);

            /* con-handler collects this once it starts up below. */
            PutMsg(mp, mn);

            Permit();
            CloseLibrary((struct Library *)DOSBase);

            conentry();

            return RETURN_OK;
        }

        CloseLibrary((struct Library *)DOSBase);
    }

    Permit();

    dp->dp_Res1 = DOSFALSE;
    dp->dp_Res2 = err;
    PutMsg(dp->dp_Port, mn);

    return RETURN_FAIL;
}
