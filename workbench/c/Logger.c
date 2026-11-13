/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: Logger - disk based caching and buffering for log.resource
*/

/******************************************************************************

    NAME

        Logger

    SYNOPSIS

        FILE/A

    LOCATION

        C:

    FUNCTION

        Buffers system logs to a specified file.

    INPUTS

        SYSLOG   --  The file used to store the system logs.
        DEBUGLOG --  The file used to store the debug logs.

    RESULT


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Patches log resources log entry enumeration functions, to provide buffered versions
		that access the on disk log files. Caches currently logged entries in log.resource to disk,
		appending-to/replacing the existing log files.

    HISTORY

******************************************************************************/

/*
 * TODO:
 *  # - let each category be buffered to a separate log file if desired
 *  # - cx interface to view log file usage/stats
 *  # - command line option to have runtime stats logged
 *  # - command line options to control log file behaviour (overwrite, max size, etc)
 *  # - option to use a config file, to specify the used options ..
 *
 *  # - Use IFF as the log file format?
 *  # - Compress text block(s)?
 */

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/log.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <proto/commodities.h>

#include <defines/log_LVO.h>

#include <resources/log.h>
#include <libraries/commodities.h>

#include <aros/shcommands.h>

#include <string.h>

#include "Logger.h"

#define BASENAME		logger
#define LOGGER_STR		"Logger - AROS log.resource disk cache"

struct loggerLog {
	char	*logName;
	BPTR	logFile;
	UBYTE	*logBuffer;
	ULONG	logBufUsed;
};

struct Library 					*LogResBase = NULL;
static struct LogProviderNode 	loggerLogProvider;
static APTR 					loggerLogHandle = NULL;
static struct MsgPort 			*loggerMsgPort = NULL;
static APTR 					loggerEventListener = NULL;
//
struct loggerLog 				loggerSysLog = {
	.logName = NULL,
	.logFile = BNULL,
	.logBuffer = NULL,
	.logBufUsed = 0
};
struct loggerLog 				loggerDbgLog = {
	.logName = NULL,
	.logFile = BNULL,
	.logBuffer = NULL,
	.logBufUsed = 0
};

#if (0)
struct loggerLog			 *logSources[LOGB_Type_Debug + 1];
#endif

static struct NewBroker loggerBroker =
{
   NB_VERSION,
   NULL,
   NULL,
   NULL,
   NBU_NOTIFY | NBU_UNIQUE,
   0,
   -120,
   NULL,
   0
};
static CxObj *cxBroker, *cxCust;

void (*OrigLockEntries) ();
void (*OrigUnlockEntries) ();
void (*OrigNextEntry) ();

AROS_LH1(VOID, loggerLockEntries,
         AROS_LHA(ULONG, flags, D0),
         struct Library *, logResBase, LVOlogLockEntries, BASENAME)
{
    AROS_LIBFUNC_INIT

	if (OrigLockEntries)
	{
		AROS_CALL1(APTR, OrigLockEntries,
					AROS_LCA(ULONG,     	flags,			D0),
					struct Library *, logResBase);
	}

    AROS_LIBFUNC_EXIT
}

AROS_LH1(VOID, loggerUnlockEntries,
         AROS_LHA(ULONG, flags, D0),
         struct Library *, logResBase, 15, BASENAME)
{
    AROS_LIBFUNC_INIT

	if (OrigUnlockEntries)
	{
		AROS_CALL1(APTR, OrigUnlockEntries,
					AROS_LCA(ULONG,     	flags,			D0),
					struct Library *, logResBase);
	}

    AROS_LIBFUNC_EXIT
}

AROS_LH1(APTR, loggerNextEntry,
         AROS_LHA(APTR *, entryHandle, A0),
         struct Library *, logResBase, LVOlogNextEntry, BASENAME)
{
    AROS_LIBFUNC_INIT

	APTR nxtEntry;

	if (entryHandle)
	{
		nxtEntry = *entryHandle;
		if (OrigNextEntry)
		{
			AROS_CALL1(APTR, OrigNextEntry,
						AROS_LCA(APTR *,     	&nxtEntry,			A0),
						struct Library *, logResBase);
		}
		else
		{
			nxtEntry = (APTR)LOGEntry_Last;
		}
		*entryHandle = nxtEntry;
	}
	else
		nxtEntry = (APTR)LOGEntry_Last;

	return nxtEntry;

    AROS_LIBFUNC_EXIT
}

void FlushLogBuffer(struct loggerLog *log)
{
    if (log->logFile && log->logBufUsed > 0)
    {
		Printf("[Logger] %s: flushing %u bytes to %s\n", __func__, log->logBufUsed, log->logName);
		Seek(log->logFile, 0, OFFSET_END); // Append

        Write(log->logFile, log->logBuffer, log->logBufUsed);
        log->logBufUsed = 0;
    }
}

void BufferLogEntry(APTR logentry)
{
    struct DateStamp	*leDateStamp = NULL;
    char 				*leOrigin = NULL;
    char 				*leComponent = NULL;
    char 				*leSubComponent = NULL;
    char 				*leLogTag = NULL;
    char 				*leEntry = NULL;
    ULONG 				leEventID = 0;
    ULONG 				leFlags = 0;
    struct TagItem 		tags[] = {
        { LOGMA_Flags,        (IPTR)&leFlags		},
        { LOGMA_DateStamp,    (IPTR)&leDateStamp	},
        { LOGMA_EventID,      (IPTR)&leEventID		},
        { LOGMA_Origin,       (IPTR)&leOrigin		},
        { LOGMA_Component,    (IPTR)&leComponent	},
        { LOGMA_SubComponent, (IPTR)&leSubComponent	},
        { LOGMA_LogTag,       (IPTR)&leLogTag		},
        { LOGMA_Entry,        (IPTR)&leEntry		},
        { TAG_DONE,           0 }
    };
	struct DateStamp	emptyDateStamp = { 0 };
	struct loggerLog	*entryLog;

    if (!logentry || (((struct Node *)logentry)->ln_Type != NT_LOGENTRY))
        return;

    logGetEntryAttrs(logentry, tags);
    if ((leFlags & LOGM_Flag_TypeMask) == LOGF_Flag_Type_Debug)
	{
		if (!loggerDbgLog.logFile)
			return;
		entryLog = &loggerDbgLog;
	}
	else
	{
		if (!loggerSysLog.logBuffer || !loggerSysLog.logFile)
			return;
		entryLog = &loggerSysLog;
	}

    struct DiskLogEntryHeader hdr = {
        .Flags         = leFlags,
        .Stamp         = (leDateStamp) ? *leDateStamp : emptyDateStamp ,
        .EventID       = leEventID,
        .OriginLen     = (leOrigin) ? strlen(leOrigin) + 1 : 0,
        .ComponentLen  = (leComponent) ? strlen(leComponent) + 1 : 0,
        .SubComponentLen = (leSubComponent) ? strlen(leSubComponent) + 1 : 0,
        .LogTagLen     = (leLogTag) ? strlen(leLogTag) + 1 : 0,
        .EntryLen      = (leEntry) ? strlen(leEntry) + 1 : 0,
    };

    ULONG needed = sizeof(hdr) +
                   hdr.OriginLen + hdr.ComponentLen +
                   hdr.SubComponentLen + hdr.LogTagLen +
                   hdr.EntryLen;

    if (entryLog->logBufUsed + needed > DISKLOG_BUFFER_SIZE)
        FlushLogBuffer(entryLog);  // flush early if overflow

    if (needed > DISKLOG_BUFFER_SIZE)
        return; // skip if leEntry is too large to ever fit

    UBYTE *p = entryLog->logBuffer + entryLog->logBufUsed;
    memcpy(p, &hdr, sizeof(hdr)); p += sizeof(hdr);
	if (hdr.OriginLen > 0) {
		memcpy(p, leOrigin, hdr.OriginLen);
		p += hdr.OriginLen;
	}
	if (hdr.ComponentLen > 0) {
		memcpy(p, leComponent, hdr.ComponentLen);
		p += hdr.ComponentLen;
	}
	if (hdr.SubComponentLen > 0) {
		memcpy(p, leSubComponent, hdr.SubComponentLen);
		p += hdr.SubComponentLen;
	}
	if (hdr.LogTagLen > 0) {
		memcpy(p, leLogTag, hdr.LogTagLen);
		p += hdr.LogTagLen;
	}
	if (hdr.EntryLen > 0) {
		memcpy(p, leEntry, hdr.EntryLen);
		p += hdr.EntryLen;
	}

    entryLog->logBufUsed += needed;
}

static APTR SafeNextEntry(APTR *current, APTR *next)
{
	if (!*next)
	{
		*current = AROS_CALL1(APTR, OrigNextEntry, AROS_LCA(APTR *, current, A0), struct Library *, LogResBase);
		*next = *current;
	}
	else
	{
		*current = *next;
	}
	if (*next != (APTR)LOGEntry_Last)
			*next = AROS_CALL1(APTR, OrigNextEntry, AROS_LCA(APTR *, next, A0), struct Library *, LogResBase);
	return *current;
}

static void loggerCXAction(CxMsg *msg,CxObj *obj)
{
#if (0)
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        switch(ie->ie_Code)
        {
            case SELECTDOWN:
				{
                }
                break;

            case SELECTUP:
                {
                }
                break;

            case IECODE_NOBUTTON:
                {
                }
                break;

        } /* switch(ie->ie_Code) */

    } /* if (ie->ie_Class == IECLASS_RAWMOUSE) */
    else if (ie->ie_Class == IECLASS_TIMER)
    {
    }
#endif
}


static void loggerCXInit(struct MsgPort *cxPort)
{
#if (0)
    if (!(cxport = CreateMsgPort()))
    {
        Cleanup(_(MSG_CANT_CREATE_MSGPORT));
    }
#endif
    loggerBroker.nb_Port = cxPort;
//    cxmask = 1L << cxport->mp_SigBit;

    if ((cxBroker = CxBroker(&loggerBroker, 0)))
    {
		if ((cxCust = CxCustom(loggerCXAction, 0)))
		{
			AttachCxObj(cxBroker, cxCust);
			ActivateCxObj(cxBroker, 1);
		}
	}
}

AROS_SH2H(Logger, 45.0, "Buffers system logs to a specified file\n",
AROS_SHAH(STRPTR, , SYSLOG, /A, NULL, "File to store system logs in\n"),
AROS_SHAH(STRPTR, , DEBUGLOG, , NULL, "File to store debug logs in\n"))
{
    AROS_SHCOMMAND_INIT

    LONG result = RETURN_FAIL;

    loggerSysLog.logName = SHArg(SYSLOG);
    loggerDbgLog.logName = SHArg(DEBUGLOG);
#if (0)
	loggerBroker.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);
#endif

	PutStr(LOGGER_STR "\n");

	if (!loggerSysLog.logName)
	{
		PrintFault(ERROR_REQUIRED_ARG_MISSING, "Logger");
		return RETURN_FAIL;
	}

	loggerSysLog.logFile = Open(loggerSysLog.logName, MODE_READWRITE);
	if (!loggerSysLog.logFile)
	{
		loggerSysLog.logFile = Open(loggerSysLog.logName, MODE_NEWFILE); // Create if not exist
		if (!loggerSysLog.logFile)
		{
			PrintFault(IoErr(), "Logger");
			return RETURN_FAIL;
		}
	}

#if (0)
	logSources[LOGB_Type_Error] = &loggerSysLog;
	logSources[LOGB_Type_Crit] = logSources[LOGB_Type_Error];
	logSources[LOGB_Type_Warn] = logSources[LOGB_Type_Error];
	logSources[LOGB_Type_Information] = logSources[LOGB_Type_Error];
	logSources[LOGB_Type_Verbose] = logSources[LOGB_Type_Error];
	logSources[LOGB_Type_Debug] = &loggerDbgLog;
#endif

	Printf("using system log file '%s'\n", loggerSysLog.logName);

	if (loggerDbgLog.logName)
	{
		loggerDbgLog.logFile = Open(loggerDbgLog.logName, MODE_READWRITE);
		if (!loggerDbgLog.logFile)
			loggerDbgLog.logFile = Open(loggerDbgLog.logName, MODE_NEWFILE); // Create if not exist
		if (loggerDbgLog.logFile)
		{
			loggerDbgLog.logBuffer = AllocVec(DISKLOG_BUFFER_SIZE, MEMF_CLEAR);
			if (!loggerDbgLog.logBuffer)
			{
				PutStr("Failed to allocate debug buffer\n");
				Close(loggerDbgLog.logFile);
				loggerDbgLog.logFile = NULL;
			}
			else
				Printf("using debug log file '%s'\n", loggerDbgLog.logName);
			
		}
	}

    loggerSysLog.logBuffer = AllocVec(DISKLOG_BUFFER_SIZE, MEMF_CLEAR);
    if (!loggerSysLog.logBuffer)
	{
		PutStr("Failed to allocate buffer\n");
		Close(loggerSysLog.logFile);
		return RETURN_FAIL;	
	}

    LogResBase = OpenResource("log.resource");
    if (LogResBase)
    {
        loggerLogProvider.lpn_Node.ln_Name = "Logger";
		if ((loggerMsgPort = CreateMsgPort()) != NULL)
		{
			loggerMsgPort->mp_Node.ln_Pri = -127; /* We pickup entries last! */
			if ((loggerEventListener = logAddListener(loggerMsgPort, LOGF_Flag_Type_All | EHMF_ALLEVENTS)) != NULL)
			{
				if ((loggerLogHandle = logInitialise(&loggerLogProvider)) != NULL)
				{
					logLockEntries(LLF_WRITE); // Lock access to the log, while we initialize

					OrigLockEntries = (void (*) ()) SetFunction ((struct Library*)LogResBase, -(LVOlogLockEntries * LIB_VECTSIZE), 
										  (ULONG (*) ()) AROS_SLIB_ENTRY(loggerLockEntries, BASENAME, LVOlogLockEntries));
					OrigUnlockEntries = (void (*) ()) SetFunction ((struct Library*)LogResBase, -(LVOlogUnlockEntries * LIB_VECTSIZE), 
										  (ULONG (*) ()) AROS_SLIB_ENTRY(loggerUnlockEntries, BASENAME, LVOlogUnlockEntries));
					OrigNextEntry = (void (*) ()) SetFunction ((struct Library*)LogResBase, -(LVOlogNextEntry * LIB_VECTSIZE), 
										  (ULONG (*) ()) AROS_SLIB_ENTRY(loggerNextEntry, BASENAME, LVOlogNextEntry));

					logAddEntry(LOGF_Flag_Type_Information, loggerLogHandle, "", __func__, 0,
								LOGGER_STR "\nInitializing subsystem");

				}
				else
				{
					logRemListener(loggerEventListener);
					loggerEventListener = NULL;
				}
			}
			else
			{
				if ((loggerLogHandle = logInitialise(&loggerLogProvider)) != NULL)
				{
					logAddEntry(LOGF_Flag_Type_Error, loggerLogHandle, "", __func__, 0,
								LOGGER_STR "\nFailed to add event listener");
				}
			}
		}

		if (loggerEventListener)
		{
            struct LogResBroadcastMsg *lrbMsg = NULL;
			BOOL running = TRUE;

			if (loggerDbgLog.logFile)
			{
				logAddEntry(LOGF_Flag_Type_Information, loggerLogHandle, "", __func__, 0,
						LOGGER_STR "\nDisk based log cacheing & buffering started\nusing system log file: %s\ndebug log file: %s", loggerSysLog.logName, loggerDbgLog.logName);
			}
			else
			{
				logAddEntry(LOGF_Flag_Type_Information, loggerLogHandle, "", __func__, 0,
						LOGGER_STR "\nDisk based log cacheing & buffering started\nusing system log file: %s", loggerSysLog.logName);
			}
			PutStr("spooling current log entries from log.resource...\n");

            // Dump all the pending entries..
			{
				APTR lognode = LOGEntry_First, nextnode = NULL;
				while ((lognode = SafeNextEntry(&lognode, &nextnode)) && (lognode != (APTR)LOGEntry_Last))
				{
					BufferLogEntry(lognode);
					logRemEntry(lognode);
				}
			}
			logUnlockEntries(LLF_WRITE); // UnLock access to the log, and go live..

			PutStr("Buffered logging online.\n");
//			loggerCXInit(loggerMsgPort);

			struct timerequest * timerIO = (struct timerequest *)CreateIORequest(loggerMsgPort, sizeof(struct timerequest));
			if (timerIO && (!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)timerIO, 0)))
			{
				timerIO->tr_node.io_Command = TR_ADDREQUEST;
				timerIO->tr_time.tv_secs = 5;
				timerIO->tr_time.tv_micro = 0;
				SendIO((struct IORequest *)timerIO);
			
				// Start listening for new events to output..
				while (running)
				{
					ULONG signals = Wait((1 << loggerMsgPort->mp_SigBit) | SIGBREAKF_CTRL_C);
					if (signals & SIGBREAKF_CTRL_C)
					{
						PutStr("CTRL-C Received.\n");
						running = FALSE;
						continue;
					}
					else if (signals & (1 << loggerMsgPort->mp_SigBit))
						while ((lrbMsg = (struct LogResBroadcastMsg *)GetMsg(loggerMsgPort)) != NULL)
						{
							if ((struct IORequest *)lrbMsg == (struct IORequest *)timerIO)
							{
								// Timer reply: flush the log buffer
								WaitIO((struct IORequest *)timerIO);  // Confirm it's done
								FlushLogBuffer(&loggerSysLog);
								if (loggerDbgLog.logFile)
									FlushLogBuffer(&loggerDbgLog);
								timerIO->tr_node.io_Command = TR_ADDREQUEST;
								timerIO->tr_time.tv_secs = 5;
								timerIO->tr_time.tv_micro = 0;
								SendIO((struct IORequest *)timerIO);
							}
							else
							{
								if (lrbMsg->lrbm_MsgType == EHMF_ADDENTRY)
								{
									logLockEntries(LLF_WRITE);
									BufferLogEntry(lrbMsg->lrbm_Target);
									logRemEntry(lrbMsg->lrbm_Target);
									logUnlockEntries(LLF_WRITE);
								}
								logDeleteBroadcast(loggerEventListener, lrbMsg);
							}
						}
				}
				if (!CheckIO((struct IORequest *)timerIO))  // Is it still pending?
					AbortIO((struct IORequest *)timerIO);

				WaitIO((struct IORequest *)timerIO);        // Wait for completion
				CloseDevice((struct IORequest *)timerIO);   // Close the device
			}
			if (timerIO)
				DeleteIORequest((struct IORequest *)timerIO);

			SetFunction ((struct Library*)LogResBase, -(LVOlogLockEntries * LIB_VECTSIZE), 
						  (ULONG (*) ()) OrigLockEntries);
			SetFunction ((struct Library*)LogResBase, -(LVOlogUnlockEntries * LIB_VECTSIZE), 
						  (ULONG (*) ()) OrigUnlockEntries);
			SetFunction ((struct Library*)LogResBase, -(LVOlogNextEntry * LIB_VECTSIZE), 
						  (ULONG (*) ()) OrigNextEntry);
			logRemListener(loggerEventListener);
		}

		if (loggerLogHandle)
		{
			logAddEntry(LOGF_Flag_Type_Information, loggerLogHandle, "", __func__, 0,
					LOGGER_STR "\nClosing");
			logFinalise(loggerLogHandle);
		}

		if (loggerMsgPort)
			DeleteMsgPort(loggerMsgPort);
	}

	PutStr("Closing...\n");
	
	if (loggerDbgLog.logFile)
	{
		FlushLogBuffer(&loggerDbgLog);
		Close(loggerDbgLog.logFile);
	}
	if (loggerSysLog.logFile)
	{
		FlushLogBuffer(&loggerSysLog);
		Close(loggerSysLog.logFile);
	}

    return result;

    AROS_SHCOMMAND_EXIT
}
