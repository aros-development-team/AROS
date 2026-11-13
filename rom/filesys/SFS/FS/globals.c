#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "asmsupport.h"
#include "globals.h"


#define TIMEOUT         (1)   /* Timeout in seconds */
#define FLUSHTIMEOUT    (20)  /* Flush timeout in seconds */


void initGlobals()
{
    globals->is_LittleEndian = FALSE;
    globals->inhibitnestcounter = 0;
    globals->block_defragptr = 2;
    globals->is_casesensitive = FALSE;
    globals->has_recycled = TRUE;
    globals->locklist = NULL;
    globals->notifyrequests = NULL;
    globals->activitytimeractive = FALSE;
    globals->pendingchanges = FALSE;
    globals->timerreset = FALSE;
    globals->max_name_length = MAX_NAME_LENGTH;
    globals->activity_timeout = FLUSHTIMEOUT;
    globals->inactivity_timeout = TIMEOUT;
    globals->retries = MAX_RETRIES;
    globals->scsidirect = FALSE;
    globals->does64bit = FALSE;
    globals->newstyledevice = FALSE;
    globals->deviceopened = FALSE;
    globals->msgport = NULL;
    globals->ioreq = NULL;
    globals->ioreq2 = NULL;
    globals->ioreqchangeint = NULL;
    globals->cmdread = CMD_READ;
    globals->cmdwrite = CMD_WRITE;
    globals->blocks_maxtransfer = 1048576;
    globals->mask_mask = -1;
    globals->bufmemtype = MEMF_PUBLIC;
    globals->transactionpool = 0;
    globals->compressbuffer = 0;
    globals->transactionnestcount = 0;
    globals->iocache_lruhead = NULL;
    globals->iocache_lines = 8;
    globals->iocache_copyback = TRUE;
    globals->iocache_readonwrite = FALSE;
    globals->templockedobjectnode = 0;
    globals->internalrename = FALSE;
    globals->defrag_maxfilestoscan = 512;
    globals->debugreqs=TRUE;

    globals->mask_debug = 0xffff;
}
