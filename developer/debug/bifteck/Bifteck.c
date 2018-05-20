/*
      Copyright © 2009-2018, The AROS Development Team. All rights reserved.
      $Id$

      Bifteck -- Retrieves memory-stored debug output.
*/

#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

struct Args
{
    TEXT *to;
};

struct LogBlock
{
    struct MinNode node;
    ULONG length;    /* number of data bytes that follow */
};

struct LogData
{
    struct SignalSemaphore lock;
    struct MinList buffers;
    struct LogBlock *block;
    ULONG block_pos;
    APTR pool;
};

static TEXT GetLogChar(struct LogData *data, struct LogBlock **block,
    ULONG *pos);

const TEXT template[] = "TO/K";
const TEXT version_string[] = "$VER: Bifteck 41.2 (30.8.2012)";
static const TEXT data_name[] = "bifteck";


LONG main(VOID)
{
    struct RDArgs *read_args;
    LONG error = 0, result = RETURN_OK;
    BPTR output;
    struct Args args = {NULL};
    struct LogData *data;
    struct LogBlock *block;
    ULONG pos = 0;
    TEXT ch, old_ch = '\n';

    /* Parse arguments */

    read_args = ReadArgs(template, (SIPTR *)&args, NULL);

    /* Get buffer */

    Forbid();
    data = (struct LogData *)FindSemaphore(data_name);
    Permit();

    if (read_args != NULL && data != NULL)
    {
        ObtainSemaphore(&data->lock);
        block = (struct LogBlock *)data->buffers.mlh_Head;

        /* Get destination */

        if (args.to != NULL)
            output = Open(args.to, MODE_NEWFILE);
        else
            output = Output();

        /* Type debug log */

        if (output != (BPTR)NULL)
        {
            while ((ch = GetLogChar(data, &block, &pos)) != '\0' && error == 0)
            {
                FPutC(output,ch);
                if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                    error = ERROR_BREAK;
                old_ch = ch;
            }
            error = IoErr();

            if (old_ch != '\n')
                FPutC(output, '\n');
        }
        else
        {
            result = RETURN_FAIL;
            error = IoErr();
        }

        /* Close the destination file */

        if (args.to != NULL && output != BNULL)
            Close(output);

        ReleaseSemaphore(&data->lock);
    }
    else
    {
        result = RETURN_FAIL;
        if (read_args != NULL && data == NULL)
        {
            PutStr("Debug data not found. "
                "Add \"debug=memory\" to boot options.\n");
            error = ERROR_OBJECT_NOT_FOUND;
        }
        else
            error = IoErr();
    }

    FreeArgs(read_args);

    /* Print any error message and exit */

    if (result == RETURN_OK)
        SetIoErr(0);
    else
        PrintFault(error, NULL);

    return result;
}


static TEXT GetLogChar(struct LogData *data, struct LogBlock **block,
    ULONG *pos)
{
    TEXT ch = '\0';

    /* Move on to next block if necessary */

    if (*pos == (*block)->length)
    {
        *block = (struct LogBlock *)(*block)->node.mln_Succ;
        *pos = 0;
    }

    /* Retrieve a character if the current block is valid and we're not past
       the used portion of the last block. Assume that block_pos is updated
       atomically */

    if ((*block)->node.mln_Succ != NULL
        && (*block != data->block || *pos < data->block_pos))
        ch = ((UBYTE *)*block)[sizeof(struct LogBlock) + (*pos)++];

    return ch;
}


