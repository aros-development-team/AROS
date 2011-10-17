/*
    Copyright © 2009-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: functions for memory-stored RawIOInit/RawPutChar
    Lang: English
*/

#include <proto/exec.h>
#include <proto/kernel.h>

#include <string.h>

#include "exec_intern.h"

#define BLOCK_SIZE (256 * 1024)
#define TASK_PRIORITY 0
#define STACK_SIZE 4096


static const TEXT name[] = "bifteck";
static const TEXT data_missed_msg[] = "<*** GAP IN DATA ***>\n";

struct LogBlock
{
    struct MinNode node;
    ULONG length;            /* number of data bytes that follow */
};

struct LogData
{
    struct SignalSemaphore lock;
    struct MinList buffers;
    struct LogBlock *block;
    ULONG block_pos;
    APTR pool;
};

void Putc(char);
static void LogTask();

static struct LogData *data;
static struct Task *task;
static BOOL data_missed = FALSE;
static struct LogBlock *next_block = NULL;


/*****i***********************************************************************

    NAME */
	AROS_LH0(void, MemoryRawIOInit,

/*  LOCATION */
	struct ExecBase *, SysBase, 84, Exec)

/*  FUNCTION
	This is a private function. It initializes raw IO. After you
	have called this function, you can use (!RawMayGetChar()) and
	RawPutChar().

    INPUTS
	None.

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawPutChar(), RawMayGetChar()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL success = TRUE;
    APTR stack;

    /* Initialise data struct, memory pool and semaphore */

    data = AllocMem(sizeof(struct LogData), MEMF_PUBLIC | MEMF_CLEAR);
    if (data == NULL)
        success = FALSE;

    if (success)
    {
        data->pool = CreatePool(MEMF_PUBLIC, BLOCK_SIZE, BLOCK_SIZE);
        NEWLIST(&data->buffers);
        InitSemaphore(&data->lock);
        data->lock.ss_Link.ln_Name = (char *)name;
        if (data->pool == NULL)
            success = FALSE;
    }


    if (success)
    {
       /* Allocate initial buffer */

        data->block = (struct LogBlock *)AllocPooled(data->pool, BLOCK_SIZE);
        data->block->length = BLOCK_SIZE - sizeof(struct LogBlock);
        if (data->block == NULL)
            success = FALSE;
    }

    if (success)
    {
       /* Add semaphore/data to public list along with initial buffer */

        AddTail((struct List *)&data->buffers, (struct Node *)data->block);
        AddSemaphore(&data->lock);
    }

    if (success)
    {
        /* Create a task that will allocate buffers as needed */

        task =
            AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
        stack = AllocMem(STACK_SIZE, MEMF_PUBLIC);
        if (task == NULL || stack == NULL)
            success = FALSE;
    }

    if (success)
    {
        /* Initialise and start task */

        task->tc_Node.ln_Type = NT_TASK;
        task->tc_Node.ln_Pri = TASK_PRIORITY;
        task->tc_Node.ln_Name = (char *)name;
        task->tc_SPUpper = stack + STACK_SIZE;
        task->tc_SPLower = stack;
        task->tc_SPReg = stack + STACK_SIZE;
        NEWLIST(&task->tc_MemEntry);
        task->tc_UserData = data;

        if (AddTask(task, LogTask, NULL) == NULL)
            success = FALSE;
    }

    if (success)
    {
        /* Signal task to allocate the second buffer */

        Signal(task, SIGF_SINGLE);
    }

    if (!success)
    {
    }

    AROS_LIBFUNC_EXIT
}


/*****i***********************************************************************

    NAME */
	AROS_LH1(void, MemoryRawPutChar,

/*  SYNOPSIS */
	AROS_LHA(UBYTE, chr, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 86, Exec)

/*  FUNCTION
	Emits a single character.

    INPUTS
	chr - The character to emit

    RESULT
	None.

    NOTES
	This function is for very low level debugging only.

    EXAMPLE

    BUGS

    SEE ALSO
	RawIOInit(), RawPutChar(), RawMayGetChar()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct LogBlock *new_block;

    /* Write to screen/serial as well */
    KrnPutChar(chr);

    /* Check if we need to move on to a new block */

    if (data->block_pos == data->block->length)
    {
        /* Move on to the next block if it exists */

        Disable();
        if ((new_block = next_block) != NULL)
            next_block = NULL;
        Enable();

        if (new_block != NULL)
        {
            data->block = new_block;
            data->block_pos = 0;

            /* If data was missed, add a warning to the log */

            if (data_missed)
            {
                CopyMem(data_missed_msg,
                    (UBYTE *)data->block + sizeof(struct LogBlock),
                    strlen(data_missed_msg));
                data->block_pos += strlen(data_missed_msg);
                data_missed = FALSE;
            }

            /* Ask for another new block, which will hopefully be allocated
             * by the time we need it */

            Signal(task, SIGF_SINGLE);
        }
    }

    /* Store character in buffer if there's space */

    if (data->block_pos != data->block->length)
    {
        if (chr && (chr != '\03'))
	{
            ((UBYTE *)data->block)[sizeof(struct LogBlock) + data->block_pos++] = chr;
	}
    }
    else
        data_missed = TRUE;

    AROS_LIBFUNC_EXIT
}


static void LogTask()
{
    struct LogBlock *block;

    /* Add a new buffer block to the list each time we're signalled */

    while (Wait(SIGF_SINGLE))
    {
        block = (struct LogBlock *)AllocPooled(data->pool, BLOCK_SIZE);
        if (block != NULL)
        {
            block->length = BLOCK_SIZE - sizeof(struct LogBlock);

            Disable();
            next_block = block;
            Enable();

            ObtainSemaphore(&data->lock);
            AddTail((struct List *)&data->buffers, (struct Node *)block);
            ReleaseSemaphore(&data->lock);
        }
    }
}

