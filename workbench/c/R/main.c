/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/
#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "r.h"
#include "locale.h"

#define ARG_TEMPLATE "FILENAME,PROFILE/K,NOGUI/S,ARGUMENTS/F"
#define CMD_TMPLATE_SIZE (2000)

APTR poolmem;

enum
{
    ARG_FILENAME,
    ARG_PROFILE,
    ARG_NOGUI,
    ARG_ARGUMENTS,
    ARG_COUNT
};

// functions
static void clean_exit(struct Req *req, CONST_STRPTR s)
{
    LONG retval = RETURN_OK;

    if (s)
    {
        retval = RETURN_FAIL;
        PutStr(s);
    }
    if (req)
    {
        if (req->rda) FreeArgs(req->rda);
    }
    cleanup_gui();
    DeletePool(poolmem);
    exit(retval);
}

static struct Req *alloc_req(void)
{
    return AllocPooled(poolmem, sizeof (struct Req));
}

static BOOL handle_args(struct Req *req, int argc, char **argv)
{
    if (argc)
    {
        IPTR args[ARG_COUNT] = {0};
        
        req->rda = ReadArgs(ARG_TEMPLATE, args, NULL);
        if (!req->rda)
        {
            PrintFault(IoErr(), argv[0]);
            return FALSE;
        }
        
        req->filename = (STRPTR)args[ARG_FILENAME];
        req->profile = (STRPTR)args[ARG_PROFILE];
        req->nogui = args[ARG_NOGUI] ? TRUE : FALSE;
        req->arguments = (STRPTR)args[ARG_ARGUMENTS];
    }
    else
    {
        return FALSE;
        // FIXME: it should be possible to use R as default tool
        // of another command
    }
    return TRUE;
}


// return TRUE if name exists and is not a directory
static BOOL is_file(CONST_STRPTR name)
{
    BOOL retval = FALSE;

    if (name && name[0])
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        if (fib)
        {
            BPTR lock = Lock(name, SHARED_LOCK);
            if (lock)
            {
                if (Examine(lock, fib))
                {
                    if (fib->fib_DirEntryType < 0)
                    {
                        retval = TRUE;
                    }
                }
                UnLock(lock);
            }
            FreeDosObject(DOS_FIB, fib);
        }
    }
    return retval;
}


// search for the command. It must
// be an absolute path, exist in the current directory
// or exist in "C:".
static BOOL check_exist(struct Req *req)
{
    BOOL retval = FALSE;

    if (req->filename == NULL)
        return FALSE;

    if (strchr(req->filename, ':')) // absolute path
    {
        if (is_file(req->filename))
        {
            D(bug("[R] command found by absolute path\n"));
            retval = TRUE;
        }
    }
    else if (strchr(req->filename, '/') == NULL) // not in a sub-dir
    {
        if (is_file(req->filename)) // in current directory
        {
            D(bug("[R] command found in current directory\n"));
            retval = TRUE;
        }
        else // in C:
        {
            BPTR lock = Lock("C:", SHARED_LOCK);
            if (lock)
            {
                BPTR olddir = CurrentDir(lock);
                if (is_file(req->filename))
                {
                    D(bug("[R] command found in C:\n"));
                    retval = TRUE;
                }
                CurrentDir(olddir);
                UnLock(lock);
            }

        }
    }

    return retval;
}


// execute the command with "?" option and read the command template
static BOOL get_template(struct Req *req)
{
    BOOL retval = FALSE;

    BPTR input_fh = BNULL;
    BPTR output_fh = BNULL;

    TEXT out_file_name[30];
    TEXT in_file_name[30];
    TEXT *cmd = NULL;
    ULONG cmd_len = 0;

    LONG i;
    __unused LONG cmd_res = 0;

    if (req->filename == NULL)
    {
        goto cleanup;
    }

    cmd_len = strlen(req->filename) + 20;
    cmd = AllocPooled(poolmem, cmd_len);
    if (cmd == NULL)
    {
        goto cleanup;
    }

    for (i = 0; i < 20 && output_fh == BNULL; i++)
    {
        sprintf(out_file_name, "t:%08u.request.outfile", (unsigned int)i);
        output_fh = Open(out_file_name, MODE_NEWFILE); 
    }
    if (output_fh == BNULL)
    {
        goto cleanup;
    }

    for (i = 0; i < 20 && input_fh == BNULL; i++)
    {
        sprintf(in_file_name, "t:%08u.request.infile", (unsigned int)i);
        input_fh = Open(in_file_name, MODE_NEWFILE); 
    }
    if (input_fh == BNULL)
    {
        goto cleanup;
    }
    Close(input_fh);
    input_fh = Open(in_file_name, MODE_OLDFILE);
    if (input_fh == BNULL)
    {
        goto cleanup;
    }

    // append "*>NIL: ?" to the command
    strlcpy(cmd, req->filename, cmd_len);
    strlcat(cmd, " *>NIL: ?", cmd_len);

    // shut up DOS error message
    struct Process *me = (struct Process*)FindTask(NULL);
    APTR oldwin = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;

    // Execute the command
    cmd_res = Execute(cmd, input_fh, output_fh);
    D(bug("[R] Execute() returned: %d\n", cmd_res));

    // restore window ptr
    me->pr_WindowPtr = oldwin;

    req->cmd_template = AllocPooled(poolmem, CMD_TMPLATE_SIZE); // FIXME get mem size from file size
    if (req->cmd_template == NULL)
    {
        goto cleanup;
    }

    // go to the beginning of the output file and read the template
    Seek(output_fh, 0, OFFSET_BEGINNING);
    if (FGets(output_fh, req->cmd_template, CMD_TMPLATE_SIZE))
    {
        D(bug("[R] template read: %s\n", req->cmd_template));
        retval = TRUE;
    }

cleanup:
    if (input_fh)
    {
        Close(input_fh);
        DeleteFile(in_file_name);
    }
    if (output_fh)
    {
        Close(output_fh);
        DeleteFile(out_file_name);
    }

    FreePooled(poolmem, cmd, cmd_len);

    return retval;
}


static BOOL parse_template(struct Req *req)
{
    TEXT *chr;
    LONG len;
    LONG arg;

    if (req->cmd_template[0] == '\0')
        return FALSE;

    // count number of arguments
    for
    (
        req->arg_cnt = 1, chr = req->cmd_template;
        *chr != '\0' && req->arg_cnt < 50;
        chr++
    )
    {
        if (*chr == ',')
        {
            req->arg_cnt++;
        }
    }

    D(bug("[R/parse_template args found %d\n", req->arg_cnt));

    req->cargs = AllocPooled(poolmem, sizeof (struct CArg) * req->arg_cnt);
    if (req->cargs == NULL)
    {
        return FALSE;
    }

    for
    (
        arg = 0, chr = req->cmd_template;
        arg < req->arg_cnt;
        chr++
    )
    {
        // read name
        TEXT *name_start = chr;
        while (1)
        {
            if (isalnum(*chr))
            {
                chr++;
                continue;
            }
            else if (*chr == '=')
            {
                // we are only interested in the part after the "=".
                chr++;
                name_start = chr;
                continue;
            }
            break;
        }

        len = chr - name_start;
        if (len == 0)
            return FALSE;

        if (len >= 35)
            len = 35;

        req->cargs[arg].argname = AllocPooled(poolmem, len + 1);
        if (req->cargs[arg].argname == NULL)
        {
            return FALSE;
        }
        memcpy(req->cargs[arg].argname, name_start, len);
        req->cargs[arg].argname[len] = '\0';

        // read modifiers
        while (*chr == '/')
        {
            switch (*(chr + 1))
            {
                case 'A':
                    req->cargs[arg].a_flag = TRUE;
                    chr++;
                    break;
                case 'F':
                    req->cargs[arg].f_flag = TRUE;
                    chr++;
                    break;
                case 'K':
                    req->cargs[arg].k_flag = TRUE;
                    chr++;
                    break;
                case 'M':
                    req->cargs[arg].m_flag = TRUE;
                    chr++;
                    break;
                case 'N':
                    req->cargs[arg].n_flag = TRUE;
                    chr++;
                    break;
                case 'S':
                    req->cargs[arg].s_flag = TRUE;
                    chr++;
                    break;
                case 'T':
                    req->cargs[arg].t_flag = TRUE;
                    chr++;
                    break;
                default:
                    return FALSE;
                    break;
            }
            chr++;
        }
        arg++;
        if (*chr != ',')
            break;
    }
    return TRUE;
}


// create the command line from the selected options
static void execute_command(struct Req *req)
{
    ULONG i;
    CONST_STRPTR str;
    TEXT *cmd;

    ULONG cmd_size = strlen(req->filename) + 5;
    for (i = 0; i < req->arg_cnt; i++)
    {
        cmd_size += strlen(req->cargs[i].argname) + 5;
        if (!req->cargs[i].s_flag && !req->cargs[i].t_flag)
        {
            cmd_size += strlen(get_gui_string(&req->cargs[i])) + 5;
        }
    }

    cmd = AllocPooled(poolmem, cmd_size);
    if (cmd == NULL)
    {
        return;
    }

    strcpy(cmd, req->filename);

    for (i = 0; i < req->arg_cnt; i++)
    {
        if (req->cargs[i].s_flag || req->cargs[i].t_flag)
        {
            if (get_gui_bool(&req->cargs[i]))
            {
                strcat(cmd, " ");
                strcat(cmd, req->cargs[i].argname);
            }
        }
        else if (req->cargs[i].n_flag)
        {
            str = get_gui_string(&req->cargs[i]);
            if (str[0] != '\0')
            {
                strcat(cmd, " ");
                strcat(cmd, req->cargs[i].argname);
                strcat(cmd, " ");
                strcat(cmd, str);
            }
        }
        else
        {
            BOOL quote = FALSE;
            str = get_gui_string(&req->cargs[i]);
            if (str[0] != '\0')
            {
                // do we have a space character in the string?
                // if yes: quote it.
                // For /M the quotes are already set by the GUI
                if (!req->cargs[i].m_flag && strchr(str, ' ') && str[0] != '\"')
                {
                    quote = TRUE;
                }                    
                strcat(cmd, " ");
                strcat(cmd, req->cargs[i].argname);
                strcat(cmd, " ");
                if (quote)
                {
                    strcat(cmd, "\"");
                }
                strcat(cmd, str);
                if (quote)
                {
                    strcat(cmd, "\"");
                }
            }
        }
    }

    D(bug("[R] executing command %s\n", cmd));
    LONG result = System
    (
        cmd,
        NULL
    );
    if (result)
    {
        Printf(_(MSG_ERROR_RETURN), req->filename, result);
    }
}


int main(int argc, char **argv)
{
    poolmem = CreatePool(MEMF_ANY | MEMF_CLEAR, 2000, 2000);
    if (poolmem == NULL)
        clean_exit(NULL, _(MSG_ERROR_POOL));

    struct Req *req = alloc_req();
    if (req == NULL)
        clean_exit(req, _(MSG_ERROR_STRUCT));

    D(bug("[R/main] req %p\n", req));

    if (! handle_args(req, argc, argv))
        clean_exit(req, _(MSG_ERROR_ARGS));

    if (! check_exist(req))
        clean_exit(req, _(MSG_ERROR_NOTFOUND));

    if (! get_template(req))
        clean_exit(req, _(MSG_ERROR_TMPLT_GET));

    if (! parse_template(req))
        clean_exit(req, _(MSG_ERROR_TMPLT_PARSE));

    if (! create_gui(req))
        clean_exit(req, _(MSG_ERROR_GUI));

    if (handle_gui(req))
    {
        execute_command(req);
    }

    clean_exit(req, NULL);

    return RETURN_OK;
}
