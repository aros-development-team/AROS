/*
 * $Id$
 *
 * Sashimi -- intercepts raw serial debugging output on your own machine
 *
 * Written by Olaf `Olsen' Barthel <olsen@sourcery.han.de>
 * Public Domain
 *
 * :ts=4
 */

/****************************************************************************/

#define __TIMER_NOLIBBASE__

#include <exec/execbase.h>
#include <exec/memory.h>

#include <devices/timer.h>

#include <dos/dosextens.h>
#include <dos/rdargs.h>

#include <proto/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/libcall.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

/****************************************************************************/

STRPTR Version = "$VER: Sashimi 1.6 (20.6.99)\r\n";

/****************************************************************************/

#define OK        (0)
#define NOT        !
#define BUSY    NULL
#define ZERO    ((BPTR)0UL)

#define min(a,b)   (((a) < (b)) ? (a) : (b))

#define NO_ALLOCABS 1

/****************************************************************************/

enum
{
    MODE_Regular,
    MODE_Recovery
};

/****************************************************************************/

#define MILLION 1000000

/****************************************************************************/

STATIC struct Device * TimerBase;

/****************************************************************************/

typedef LONG    SWITCH;
typedef LONG *    NUMBER;
typedef STRPTR    KEY;

STATIC struct
{
    /* Startup options */
    KEY        Recover;    /* Recover any old Sashimi buffer still in memory */
    SWITCH    On;            /* Ignored */
    NUMBER    BufferK;    /* Buffer size, to the power of two */
    NUMBER    BufferSize;    /* Buffer size in bytes */
    SWITCH    NoPrompt;    /* Do not show the initial prompt message */
    SWITCH    Quiet;        /* Do not produce any output at all */
    SWITCH    AskExit;    /* Ask whether to exit the program */
    SWITCH    AskSave;    /* Ask for a file to save the buffer to when exiting */
    SWITCH    TimerOn;    /* Check the circular buffer every 1/10 of a second */
    SWITCH    Console;    /* Open a console window for I/O */
    KEY        Window;        /* Console window specifier */

    /* Runtime options */
    SWITCH    Off;        /* Turn Sashimi off */
    SWITCH    Save;        /* Save the circular buffer contents */
    KEY        SaveAs;        /* Save the circular buffer contents under a specific name */
    SWITCH    Empty;        /* Empty the circular buffer */
} ShellArguments;

STATIC const STRPTR ShellTemplate =
    "RECOVER/K,"
    "ON/S,"
    "BUFK/N,"
    "BUFFERSIZE/N,"
    "NOPROMPT/S,"
    "QUIET/S,"
    "ASKEXIT/S,"
    "ASKSAVE/S,"
    "TIMERON/S,"
    "CONSOLE/S,"
    "WINDOW/K,"
    "OFF/S,"
    "SAVE/S,"
    "SAVEAS/K,"
    "EMPTY/S";

/****************************************************************************/

/* Eat me */
#define COOKIE 0x08021999

struct SashimiResource
{
    struct Library    sr_Library;            /* Global link */
    UWORD            sr_Pad;                /* Long word alignment */

    ULONG            sr_Cookie;            /* Magic marker */
    APTR            sr_PointsToCookie;    /* Points back to cookie */
    ULONG            sr_CreatedWhen;        /* When exactly was this data structure created? */

    struct Task *    sr_Owner;            /* Current owner of the patches */
    LONG            sr_OwnerSigBit;
    ULONG            sr_OwnerSigMask;    /* Signal mask to send when a new line is in the buffer. */

    ULONG            sr_FIFOTotalSize;    /* Number of bytes allocated for the buffer */
    ULONG            sr_FIFOReadIndex;    /* Read index counter */
    ULONG            sr_FIFOWriteIndex;    /* Write index counter */
    ULONG            sr_FIFOBytesStored;    /* Number of bytes in the FIFO */
    BOOL            sr_FIFOOverrun;        /* TRUE if the write index counter has
                                         * overrun the read index counter.
                                         */
    BOOL            sr_FIFOWrapped;        /* TRUE if the write index counter has
                                         * wrapped around the circular buffer.
                                         */
    UBYTE            sr_FIFO[1];            /* The message buffer */
};

STATIC const STRPTR SashimiResourceName = "sashimi.resource";
STATIC struct SashimiResource * GlobalSashimiResource;

/****************************************************************************/

#define LVORawIOInit	 (-84 * LIB_VECTSIZE)
#define LVORawMayGetChar (-85 * LIB_VECTSIZE)
#define LVORawPutChar	 (-86 * LIB_VECTSIZE)

/****************************************************************************/

STATIC APTR OldRawIOInit;
STATIC APTR OldRawMayGetChar;
STATIC APTR OldRawPutChar;

/****************************************************************************/

AROS_LH0(void, NewRawIOInit,
    struct ExecBase *, SysBase, 84, Exec)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    /* Nothing happens here */

    AROS_LIBFUNC_EXIT
}

AROS_LH0(LONG, NewRawMayGetChar,
    struct ExecBase *, SysBase, 85, Exec)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    
    /* We always return sort of a confirmation. */
    return('y');
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************/

STATIC LONG
GetCharsInFIFO(struct SashimiResource * sr)
{
    LONG result;

    Disable();

    if(sr->sr_FIFOWrapped)
        result = sr->sr_FIFOTotalSize;
    else
        result = sr->sr_FIFOWriteIndex;

    Enable();

    return(result);
}

STATIC VOID
EmptyFIFO(struct SashimiResource * sr)
{
    Disable();

    sr->sr_FIFOReadIndex    = 0;
    sr->sr_FIFOWriteIndex    = 0;
    sr->sr_FIFOBytesStored    = 0;
    sr->sr_FIFOOverrun        = FALSE;
    sr->sr_FIFOWrapped        = FALSE;

    Enable();
}

STATIC LONG
ReadFIFOChars(struct SashimiResource * sr,UBYTE * buffer,LONG maxChars)
{
    LONG result = 0;

    Disable();

    if(sr->sr_FIFOBytesStored > 0)
    {
        LONG howMany;

        if(maxChars > sr->sr_FIFOBytesStored)
            maxChars = sr->sr_FIFOBytesStored;

        do
        {
            /* Find out how many characters can be read
             * from the FIFO without overrunning the
             * end of it. We don't read more than these
             * few characters in one go.
             */
            howMany = min(maxChars,sr->sr_FIFOTotalSize - sr->sr_FIFOReadIndex);

            memcpy(buffer,&sr->sr_FIFO[sr->sr_FIFOReadIndex],howMany);

            result        += howMany;
            buffer        += howMany;
            maxChars    -= howMany;

            sr->sr_FIFOReadIndex = (sr->sr_FIFOReadIndex + howMany) % sr->sr_FIFOTotalSize;
        }
        while(maxChars > 0);

        /* Subtract the number of characters we
         * read in the loop.
         */
        sr->sr_FIFOBytesStored -= result;
    }

    Enable();

    return(result);
}

STATIC VOID
StoreFIFOChar(struct SashimiResource * sr,UBYTE c)
{
    sr->sr_FIFO[sr->sr_FIFOWriteIndex] = c;
    sr->sr_FIFOWriteIndex = (sr->sr_FIFOWriteIndex + 1) % sr->sr_FIFOTotalSize;

    /* If the buffer wraps around, remember it. */
    if(sr->sr_FIFOWriteIndex == 0)
        sr->sr_FIFOWrapped = TRUE;

    /* Check if the circular buffer was overrun */
    sr->sr_FIFOBytesStored++;
    if(sr->sr_FIFOBytesStored > sr->sr_FIFOTotalSize)
    {
        sr->sr_FIFOOverrun = TRUE;

        /* Move the read index to the same position as
         * the write index and retain only as many
         * bytes as would fit into the FIFO.
         */
        sr->sr_FIFOReadIndex = sr->sr_FIFOWriteIndex;
        sr->sr_FIFOBytesStored = sr->sr_FIFOTotalSize;
    }
}

/****************************************************************************/

AROS_LH1(void, NewRawPutChar,
    AROS_LHA(UBYTE, c, D0),
    struct ExecBase *, SysBase, 86, Exec)
{    
    AROS_LIBFUNC_INIT

    /* Do not store NUL bytes. */
    if(c != '\0')
    {
        STATIC ULONG Position = 0;

        Disable();

        /* Filter out extra <cr> characters. */
        if(c != '\r' || Position > 0)
        {
            struct SashimiResource * sr = GlobalSashimiResource;

            /* Store another byte in the buffer */
            StoreFIFOChar(sr,c);

            /* Notify Sashimi every time there is an end of line
             * character in the stream.
             */
            if(c == '\n' || c == '\r')
                Signal(sr->sr_Owner,sr->sr_OwnerSigMask);
        }

        if(c == '\r' || c == '\n')
            Position = 0;
        else
            Position++;

        Enable();
    }

    AROS_LIBFUNC_EXIT
}

/****************************************************************************/

STATIC VOID
RemovePatches(VOID)
{
    APTR res;

    /* We disable the interrupts because the raw I/O routines can
     * be called from within interrupt code.
     */
    Disable();

    /* For every patch planted, remove it and check whether the code
     * had been patched before. If it has, restore the patch. Note that
     * this is not bullet proof :(
     */
    res = SetFunction(&SysBase->LibNode,LVORawIOInit,OldRawIOInit);
    if(res != AROS_SLIB_ENTRY(NewRawIOInit,Exec))
        SetFunction(&SysBase->LibNode,LVORawIOInit,res);

    res = SetFunction(&SysBase->LibNode,LVORawMayGetChar,OldRawMayGetChar);
    if(res != AROS_SLIB_ENTRY(NewRawMayGetChar,Exec))
        SetFunction(&SysBase->LibNode,LVORawMayGetChar,res);

    res = SetFunction(&SysBase->LibNode,LVORawPutChar,OldRawPutChar);
    if(res != AROS_SLIB_ENTRY(NewRawPutChar,Exec))
        SetFunction(&SysBase->LibNode,LVORawPutChar,res);

    Enable();
}

STATIC VOID
InstallPatches(VOID)
{
    /* We disable the interrupts because the raw I/O routines can
     * be called from within interrupt code.
     */
    Disable();

    OldRawIOInit        = SetFunction(&SysBase->LibNode,LVORawIOInit,        AROS_SLIB_ENTRY(NewRawIOInit,Exec));
    OldRawMayGetChar    = SetFunction(&SysBase->LibNode,LVORawMayGetChar,    AROS_SLIB_ENTRY(NewRawMayGetChar,Exec));
    OldRawPutChar       = SetFunction(&SysBase->LibNode,LVORawPutChar,       AROS_SLIB_ENTRY(NewRawPutChar,Exec));

    Enable();
}

/****************************************************************************/

STATIC VOID
FreeSashimiResource(struct SashimiResource * sr)
{
    if(sr != NULL)
    {
        FreeSignal(sr->sr_OwnerSigBit);

        /* Destroy the markers */
        sr->sr_Cookie            = 0;
        sr->sr_PointsToCookie    = NULL;

#if NO_ALLOCABS
        FreeMem(sr,sizeof(*sr) + sr->sr_FIFOTotalSize-1 + sizeof(struct MemChunk));
#else
        FreeMem(sr,sizeof(*sr) + sr->sr_FIFOTotalSize-1);
#endif
    }
}

STATIC LONG
RemoveSashimiResource(struct SashimiResource * sr)
{
    LONG error = OK;

    if(sr != NULL)
    {
        Forbid();

        /* Allow the resource to be removed only if
         * there are no customers using it.
         */
        if(sr->sr_Library.lib_OpenCnt == 0)
            RemResource(sr);
        else
            error = ERROR_OBJECT_IN_USE;

        Permit();
    }

    return(error);
}

STATIC LONG
AddSashimiResource(ULONG bufferSize,struct SashimiResource ** resourcePtr)
{
    struct SashimiResource * sr;
    LONG error = OK;

    /* We will do something really tricky; to increase our chances of
     * allocating an old Sashimi buffer to be recovered, we allocate
     * the amount of memory needed plus the size of a memory chunk.
     * Then we release that buffer again and reallocate it with the
     * size of the memory chunk trailing behind it.
     */

    Forbid();

    sr = AllocMem(sizeof(struct MemChunk) + sizeof(*sr) + bufferSize-1,MEMF_ANY|MEMF_PUBLIC);
#if !NO_ALLOCABS
    if(sr != NULL)
    {
        FreeMem(sr,sizeof(struct MemChunk) + sizeof(*sr) + bufferSize-1);
        sr = AllocAbs(sizeof(*sr) + bufferSize-1,(BYTE *)sr + sizeof(struct MemChunk));
    }
#endif
    Permit();

    if(sr != NULL)
    {
        struct timeval now;

        GetSysTime(&now);

        memset(sr,0,sizeof(*sr)-1);

        sr->sr_Library.lib_Node.ln_Name    = (char *)SashimiResourceName;
        sr->sr_Library.lib_Node.ln_Type    = NT_RESOURCE;
        sr->sr_Owner                    = FindTask(NULL);
        sr->sr_FIFOTotalSize            = bufferSize;
        sr->sr_Cookie                    = COOKIE;
        sr->sr_PointsToCookie            = &sr->sr_Cookie;
        sr->sr_CreatedWhen                = now.tv_secs;

        sr->sr_OwnerSigBit = AllocSignal(-1);
        if(sr->sr_OwnerSigBit != -1)
        {
            sr->sr_OwnerSigMask = (1UL << sr->sr_OwnerSigBit);

            Forbid();

            /* Do not add the resource if it has already been installed. */
            if(OpenResource((STRPTR)SashimiResourceName) == NULL)
                AddResource(sr);
            else
                error = ERROR_OBJECT_EXISTS;

            Permit();
        }
        else
        {
            error = ERROR_NO_FREE_STORE;
        }
    }
    else
    {
        error = ERROR_NO_FREE_STORE;
    }

    if(error != OK)
    {
        FreeSashimiResource(sr);
        sr = NULL;
    }

    (*resourcePtr) = sr;

    return(error);
}

/****************************************************************************/

STATIC VOID
CloseSashimiResource(struct SashimiResource * sr)
{
    if(sr != NULL)
    {
        Forbid();

        sr->sr_Library.lib_OpenCnt--;

        Permit();
    }
}

STATIC struct SashimiResource *
OpenSashimiResource(VOID)
{
    struct SashimiResource * sr;

    Forbid();

    sr = OpenResource((STRPTR)SashimiResourceName);
    if(sr != NULL)
        sr->sr_Library.lib_OpenCnt++;

    Permit();

    return(sr);
}

/****************************************************************************/

STATIC LONG
SaveBuffer(const STRPTR name,struct SashimiResource * sr,LONG mode)
{
    LONG error = OK;
    STRPTR buffer;

    /* We allocate a temporary buffer to store the circular
     * buffer data in.
     */
    buffer = AllocVec(sr->sr_FIFOTotalSize,MEMF_ANY|MEMF_PUBLIC);
    if(buffer != NULL)
    {
        LONG bytesInBuffer;
        BOOL wrapped;
        BOOL overrun;
        BPTR file;

        if(mode == MODE_Regular)
        {
            /* Stop interrupts and multitasking for a tick. */
            Disable();
        }

        wrapped = sr->sr_FIFOWrapped;
        overrun = sr->sr_FIFOOverrun;

        if(wrapped)
        {
            LONG oldBytes = sr->sr_FIFOTotalSize - sr->sr_FIFOWriteIndex;

            /* Unwrap the buffer; first copy the old data (following the
             * write index) then the newer data.
             */
            memcpy(buffer,&sr->sr_FIFO[sr->sr_FIFOWriteIndex],oldBytes);
            memcpy(&buffer[oldBytes],sr->sr_FIFO,sr->sr_FIFOWriteIndex);

            bytesInBuffer = sr->sr_FIFOTotalSize;
        }
        else
        {
            memcpy(buffer,sr->sr_FIFO,sr->sr_FIFOWriteIndex);

            bytesInBuffer = sr->sr_FIFOWriteIndex;
        }

        if(mode == MODE_Regular)
        {
            /* Start interrupts and multitasking again. */
            Enable();
        }

        /* Write the buffer contents. */
        file = Open((STRPTR)name,MODE_NEWFILE);
        if(file != ZERO)
        {
            if(mode == MODE_Recovery)
            {
                if(FPrintf(file,"RECOVERY WARNING - Data may have been damaged\n") < 0)
                    error = IoErr();
            }

            if(error == OK && overrun)
            {
                if(FPrintf(file,"BUFFER WAS OVERRUN - Data may have been lost\n") < 0)
                    error = IoErr();
            }

            if(error == OK && wrapped)
            {
                if(FPrintf(file,"BUFFER WRAPPED - This is the most recent captured data\n\n") < 0)
                    error = IoErr();
            }

            /* FPrintf() is a buffered I/O routine, this is why we need to flush the
             * output buffer here. Otherwise, it would be flushed after the Write()
             * command below is finished and the file is closed. This is not what
             * we want as that would have the effect of adding the messages above
             * to the end of the file.
             */
            if(error == OK)
                Flush(file);

            if(error == OK)
            {
                if(Write(file,buffer,bytesInBuffer) != bytesInBuffer)
                    error = IoErr();
            }

            Close(file);
        }
        else
        {
            error = IoErr();
        }

        FreeVec(buffer);
    }
    else
    {
        error = ERROR_NO_FREE_STORE;
    }

    return(error);
}

/****************************************************************************/

STATIC BOOL
Recover(const STRPTR fileName)
{
    struct SashimiResource * sr = NULL;
    APTR allocated = NULL;
    struct MemHeader * mh;
    ULONG * start;
    ULONG * end;
    BOOL success;

    Printf("Trying to recover old Sashimi buffer... ");
    Flush(Output());

    Forbid();

    /* Scan the system memory list. */
    for(mh = (struct MemHeader *)SysBase->MemList.lh_Head ;
        mh->mh_Node.ln_Succ != NULL ;
        mh = (struct MemHeader *)mh->mh_Node.ln_Succ)
    {
        start    = (ULONG *)mh->mh_Lower;
        end        = (ULONG *)mh->mh_Upper;

        do
        {
            /* First look for the cookie... */
            if(start[0] == COOKIE)
            {
                /* Then look for the pointer back to it. */
                if(start[1] == (ULONG)start)
                {
                    /* Unless we don't have a resource pointer
                     * yet, compare the creation times and take
                     * only the latest buffer.
                     */
                    if(sr == NULL || start[2] > sr->sr_CreatedWhen)
                        sr = (struct SashimiResource *)((ULONG)start - offsetof(struct SashimiResource,sr_Cookie));
                }
            }
        }
        while(++start != end);
    }

    /* Try to allocate the memory the old buffer occupies. */
    if(sr != NULL)
        allocated = AllocAbs(sizeof(*sr) + sr->sr_FIFOTotalSize-1 + sizeof(struct MemChunk),(BYTE *)sr - sizeof(struct MemChunk));

    Permit();

    if(sr != NULL)
    {
        LONG error = OK;
        LONG numBytes;

        if(sr->sr_FIFOWrapped)
            numBytes = sr->sr_FIFOTotalSize;
        else
            numBytes = sr->sr_FIFOWriteIndex;

        Printf("found something (%ld bytes).\n",numBytes);

        /* If there is anything worth saving, save it. */
        if(numBytes > 0)
        {
            error = SaveBuffer(fileName,sr,MODE_Recovery);
            if(error == OK)
                Printf("Recovered Sashimi buffer saved as \"%s\".\n",fileName);
            else
                PrintFault(error,fileName);
        }
        else
        {
            Printf("This is not worth saving.\n");
        }

        /* If everything went fine so far and
         * if we are the owner of the buffer,
         * mark it as invalid.
         */
        if(error == OK && allocated != NULL)
        {
            sr->sr_Cookie            = 0;
            sr->sr_PointsToCookie    = NULL;
        }

        success = TRUE;
    }
    else
    {
        Printf("sorry.\n");

        success = FALSE;
    }

    /* Release the buffer... */
    if(allocated != NULL)
        FreeMem(allocated,sizeof(*sr) + sr->sr_FIFOTotalSize-1 + sizeof(struct MemChunk));

    return(success);
}

/****************************************************************************/

int
main(int argc,char **argv)
{
    int result = RETURN_FAIL;

    /* Kickstart 2.04 and a Shell window are required. */
    if(argc > 0)
    {
        struct RDArgs * rdargs;

        rdargs = ReadArgs((STRPTR)ShellTemplate,(LONG *)&ShellArguments,NULL);
        if(rdargs != NULL)
        {
            /* Before anything else happens, check if
             * we should recover any old data.
             */
            if(ShellArguments.Recover != NULL)
            {
                if(Recover(ShellArguments.Recover))
                    result = RETURN_OK;
                else
                    result = RETURN_WARN;
            }
            else
            {
                struct SashimiResource * sr = NULL;
                struct MsgPort * timePort;
                struct timerequest * timeRequest = NULL;
                BOOL added = FALSE;
                BOOL opened = FALSE;
                LONG error = OK;
                BPTR oldOutput = ZERO;
                BPTR newOutput = ZERO;
                BPTR oldInput = ZERO;
                BPTR newInput = ZERO;
                struct MsgPort * oldConsoleTask = NULL;
                STRPTR saveFile;

                /* Fill in the save file name, we might need it later. */
                if(ShellArguments.SaveAs != NULL)
                    saveFile = ShellArguments.SaveAs;
                else
                    saveFile = "T:sashimi.out";

                /* Set up the timer.device interface. */
                timePort = CreateMsgPort();
                if(timePort != NULL)
                {
                    timeRequest = (struct timerequest *)CreateIORequest(timePort,sizeof(*timeRequest));
                    if(timeRequest != NULL)
                    {
                        if(OpenDevice(TIMERNAME,UNIT_VBLANK,(struct IORequest *)timeRequest,0) == OK)
                            TimerBase = timeRequest->tr_node.io_Device;
                        else
                            error = ERROR_NO_FREE_STORE; /* Misleading? */
                    }
                    else
                    {
                        error = ERROR_NO_FREE_STORE;
                    }
                }
                else
                {
                    error = ERROR_NO_FREE_STORE;
                }

                if(error == OK)
                {
                    /* Try to open the resource, and if that fails, create one. */
                    sr = OpenSashimiResource();
                    if(sr != NULL)
                    {
                        opened = TRUE;
                    }
                    else
                    {
                        ULONG bufferSize;
    
                        /* The default buffer size is 32K. */
                        bufferSize = 32 * 1024;
    
                        /* Check for a specific buffer size (power of two). */
                        if(ShellArguments.BufferK != NULL)
                            bufferSize = 1024 * (*ShellArguments.BufferK);
    
                        /* Check for a specific buffer size. */
                        if(ShellArguments.BufferSize != NULL)
                            bufferSize = (ULONG)(*ShellArguments.BufferSize);
    
                        /* Don't make the buffer too small. */
                        if(bufferSize < 4096)
                            bufferSize = 4096;
    
                        /* Add the resource to the public list. Note that
                         * the patches are not installed yet.
                         */
                        error = AddSashimiResource(bufferSize,&sr);
                        if(error == OK)
                            added = TRUE;
                    }
                }

                /* Did we get everything we wanted? */
                if(error != OK)
                {
                    PrintFault(error,"Sashimi");
                    result = RETURN_ERROR;
                }
                else
                {
                    if(opened)
                    {
                        /* Save the current circular buffer contents? */
                        if(ShellArguments.SaveAs != NULL || ShellArguments.Save)
                        {
                            LONG error;

                            error = SaveBuffer(saveFile,sr,MODE_Regular);
                            if(error == OK)
                                Printf("Sashimi buffer saved as \"%s\".\n",saveFile);
                            else
                                PrintFault(error,saveFile);
                        }

                        /* Empty the circular buffer? */
                        if(ShellArguments.Empty)
                        {
                            EmptyFIFO(sr);

                            Printf("Sashimi buffer cleared.\n");
                        }

                        /* Turn off Sashimi? */
                        if(ShellArguments.Off)
                        {
                            struct Task * owner;

                            Forbid();

                            /* We cannot tell Sashimi to quit
                             * if there is a single customer
                             * left, such as us. This is why
                             * we close the resource and
                             * signal Sashimi to quit.
                             */
                            owner = sr->sr_Owner;
                            CloseSashimiResource(sr);
                            sr = NULL;

                            Signal(owner,SIGBREAKF_CTRL_C);

                            Permit();
                        }
                    }

                    if(added && NOT ShellArguments.Off)
                    {
                        ULONG signalsReceived,signalsToWaitFor;
                        BOOL done;

                        /* Open a console window? */
                        if(ShellArguments.Console)
                        {
                            STRPTR consoleWindow;
                            LONG error = OK;

                            if(ShellArguments.Window != NULL)
                                consoleWindow = ShellArguments.Window;
                            else
                                consoleWindow = "CON:0/20/640/100/Sashimi  [Ctrl]+E=Empty  [Ctrl]+F=File  [Ctrl]+D=Reset console/AUTO/CLOSE/WAIT/INACTIVE";

                            /* Open the window and make it the default
                             * I/O stream.
                             */
                          #if 1
			    newInput = Open(consoleWindow,MODE_READWRITE);
			  #else
			    newInput = Open(consoleWindow,MODE_NEWFILE);
			  #endif
                            if(newInput != ZERO)
                            {
                          #if 1
			    	oldInput  = SelectInput(newInput);
				oldOutput = SelectOutput(newInput);
			  #else
			        oldConsoleTask = SetConsoleTask(((struct FileHandle *)BADDR(newInput))->fh_Type);
                                newOutput = Open("CONSOLE:",MODE_OLDFILE);
                                if(newOutput != ZERO)
                                {
                                    oldInput = SelectInput(newInput);
                                    oldOutput = SelectOutput(newOutput);
                                }
                                else
                                {
                                    error = IoErr();

                                    /* Return to the original console task. */
                                    SetConsoleTask(oldConsoleTask);
                                    oldConsoleTask = NULL;
                                }
                          #endif
			    }
                            else
                            {
                                error = IoErr();
                            }

                            if(error != OK)
                                PrintFault(error,consoleWindow);
                        }

                        /* Show the banner message. */
                        if(NOT ShellArguments.NoPrompt && NOT ShellArguments.Quiet)
                        {
                            struct Process * cli = (struct Process *)FindTask(NULL);
                            LONG maxCli,thisCli = 1,i;

                            /* Find our current CLI process number. */
                            maxCli = MaxCli();
                            for(i = 1 ; i <= maxCli ; i++)
                            {
                                if(FindCliProc(i) == cli)
                                {
                                    thisCli = i;
                                    break;
                                }
                            }

                            Printf("Sashimi installed ([Ctrl]+C or \"Break %ld\" to remove)\n",thisCli);
                        }

                        GlobalSashimiResource = sr;
                        InstallPatches();

                        signalsToWaitFor = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D |
                                           SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F |
                                           sr->sr_OwnerSigMask;

                        /* Start the timer. */
                        if(ShellArguments.TimerOn)
                        {
                            signalsToWaitFor |= (1UL << timePort->mp_SigBit);

                            timeRequest->tr_node.io_Command    = TR_ADDREQUEST;
                            timeRequest->tr_time.tv_secs    = 0;
                            timeRequest->tr_time.tv_micro    = MILLION / 10;

                            SendIO((struct IORequest *)timeRequest);
                        }

                        done = FALSE;
                        do
                        {
                            signalsReceived = Wait(signalsToWaitFor);

                            /* Check if we should test the buffer. */
                            if(ShellArguments.TimerOn)
                            {
                                if(signalsReceived & (1UL << timePort->mp_SigBit))
                                {
                                    signalsReceived |= sr->sr_OwnerSigMask;

                                    WaitIO((struct IORequest *)timeRequest);

                                    /* Restart the timer. */
                                    timeRequest->tr_node.io_Command    = TR_ADDREQUEST;
                                    timeRequest->tr_time.tv_secs    = 0;
                                    timeRequest->tr_time.tv_micro    = MILLION / 10;

                                    SendIO((struct IORequest *)timeRequest);
                                }
                            }

                            /* Check if we should test the buffer. */
                            if(signalsReceived & sr->sr_OwnerSigMask)
                            {
                                if(NOT ShellArguments.Quiet)
                                {
                                    UBYTE localBuffer[256];
                                    ULONG moreSignals;
                                    LONG filled;

                                    /* Try to empty the circular buffer. */
                                    while((filled = ReadFIFOChars(sr,localBuffer,sizeof(localBuffer))) > 0)
                                    {
                                        /* Check if there is a message for us. */
                                        moreSignals = SetSignal(0,SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_E|SIGBREAKF_CTRL_F);

                                        /* Save the circular buffer to a file? */
                                        if(moreSignals & SIGBREAKF_CTRL_F)
                                        {
                                            LONG error;

                                            error = SaveBuffer(saveFile,sr,MODE_Regular);
                                            if(error == OK)
                                                Printf("Sashimi buffer saved as \"%s\".\n",saveFile);
                                            else
                                                PrintFault(error,saveFile);
                                        }

                                        /* Empty the circular buffer? */
                                        if(moreSignals & SIGBREAKF_CTRL_E)
                                        {
                                            EmptyFIFO(sr);

                                            Printf("Sashimi buffer cleared.\n");
                                            filled = 0;
                                        }

                                        /* Stop Sashimi? */
                                        if(moreSignals & SIGBREAKF_CTRL_C)
                                        {
                                            signalsReceived |= SIGBREAKF_CTRL_C;
                                            break;
                                        }

                                        /* Write the buffer to the file. */
                                        if(filled > 0)
                                            Write(Output(),localBuffer,filled);
                                    }
                                }
                            }

                            /* Save current buffer to file. */
                            if(signalsReceived & SIGBREAKF_CTRL_F)
                            {
                                LONG error;

                                error = SaveBuffer(saveFile,sr,MODE_Regular);
                                if(error == OK)
                                    Printf("Sashimi buffer saved as \"%s\".\n",saveFile);
                                else
                                    PrintFault(error,saveFile);
                            }

                            /* Empty the buffer. */
                            if(signalsReceived & SIGBREAKF_CTRL_E)
                            {
                                EmptyFIFO(sr);

                                Printf("Sashimi buffer cleared.\n");
                            }

                            /* Reset the terminal. */
                            if(signalsReceived & SIGBREAKF_CTRL_D)
                            {
                                Printf("\033c");
                                Flush(Output());
                            }

                            /* Terminate the program. */
                            if(signalsReceived & SIGBREAKF_CTRL_C)
                            {
                                BOOL terminate = FALSE;

                                if(ShellArguments.AskExit)
                                {
                                    UBYTE buffer[4];

                                    Printf("\nSashimi: stop signal received -- really exit (y or n)? ");
                                    Flush(Output());

                                    buffer[0] = '\0';

                                    if(FGets(Input(),buffer,sizeof(buffer)-1) != NULL)
                                    {
                                        if(buffer[0] == 'y' || buffer[0] == 'Y')
                                            terminate = TRUE;
                                    }
                                }
                                else
                                {
                                    terminate = TRUE;
                                }

                                if(terminate)
                                {
                                    if(RemoveSashimiResource(sr) == OK)
                                    {
                                        Printf("Sashimi removed.\n");
                                        done = TRUE;
                                    }
                                }
                            }
                        }
                        while(NOT done);

                        RemovePatches();

                        /* Stop the timer. */
                        if(ShellArguments.TimerOn)
                        {
                            if(CheckIO((struct IORequest *)timeRequest) == BUSY)
                                AbortIO((struct IORequest *)timeRequest);

                            WaitIO((struct IORequest *)timeRequest);
                        }
 
                        /* Check if we should and could save the circular buffer. */
                        if(ShellArguments.AskSave && GetCharsInFIFO(sr) > 0)
                        {
                            UBYTE name[256];

                            Printf("Enter name to save the buffer, or hit [Return] to cancel: ");
                            Flush(Output());

                            name[0] = '\0';

                            if(FGets(Input(),name,sizeof(name)-1) != NULL)
                            {
                                LONG error;
                                int i;

                                for(i = strlen(name)-1 ; i >= 0 ; i--)
                                {
                                    if(name[i] == '\n')
                                        name[i] = '\0';
                                }

                                error = SaveBuffer(name,sr,MODE_Regular);
                                if(error == OK)
                                    Printf("Sashimi buffer saved as \"%s\".\n",name);
                                else
                                    PrintFault(error,name);
                            }
                        }

                        FreeSashimiResource(sr);
                        sr = NULL;
                    }

                    result = RETURN_OK;
                }

                /* Close the resource, if we opened it. */
                if(opened)
                    CloseSashimiResource(sr);

                /* Remove and free the resource if we added it. */
                if(added)
                {
                    RemoveSashimiResource(sr);
                    FreeSashimiResource(sr);
                }

                /* Clean up the timer.device interface. */
                if(timeRequest != NULL)
                {
                    if(timeRequest->tr_node.io_Device != NULL)
                        CloseDevice((struct IORequest *)timeRequest);

                    DeleteIORequest((struct IORequest *)timeRequest);
                }

                DeleteMsgPort(timePort);

                /* Reset and clean up the console I/O streams. */
                if(oldOutput != ZERO)
                    SelectOutput(oldOutput);

                if(oldInput != ZERO)
                    SelectInput(oldInput);

                if(newOutput != ZERO)
                    Close(newOutput);

                if(oldConsoleTask != NULL)
                    SetConsoleTask(oldConsoleTask);

                if(newInput != ZERO)
                    Close(newInput);
            }

            FreeArgs(rdargs);
        }
        else
        {
            PrintFault(IoErr(),"Sashimi");

            result = RETURN_ERROR;
        }
    }

    return(result);
}
