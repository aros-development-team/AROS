/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

#include "aros_resource.h"

APTR LoadSpeechResource(struct DosLibrary *DOSBase, CONST_STRPTR path,
                        struct SCResource *resource)
{
    BPTR file = BNULL;
    LONG length;
    APTR bytes = NULL, workspace = NULL, compact = NULL;
    size_t workspace_size;
    struct SCResource compact_resource;

    if (DOSBase == NULL)
        return NULL;
    file = Open(path, MODE_OLDFILE);
    if (file == BNULL || Seek(file, 0, OFFSET_END) == -1)
        goto out;
    length = Seek(file, 0, OFFSET_BEGINNING);
    if (length < 12 || (ULONG)length > SC_RESOURCE_MAX_SIZE)
        goto out;
    workspace_size = SCResourceWorkspaceSize((size_t)length);
    bytes = AllocVec((ULONG)length, MEMF_ANY);
    workspace = AllocVec(workspace_size, MEMF_ANY);
    if (bytes == NULL || workspace == NULL ||
        Read(file, bytes, length) != length ||
        SCResourceDecode(bytes, (size_t)length, workspace, workspace_size,
                         resource) != SC_OK)
    {
        if (workspace != NULL)
            FreeVec(workspace);
        workspace = NULL;
        memset(resource, 0, sizeof(*resource));
    }
    else if (resource->workspace_used != 0 &&
             resource->workspace_used < workspace_size)
    {
        compact = AllocVec(resource->workspace_used, MEMF_ANY);
        if (compact != NULL &&
            SCResourceDecode(bytes, (size_t)length, compact,
                             resource->workspace_used,
                             &compact_resource) == SC_OK)
        {
            FreeVec(workspace);
            workspace = compact;
            compact = NULL;
            *resource = compact_resource;
        }
    }
out:
    if (compact != NULL)
        FreeVec(compact);
    if (bytes != NULL)
        FreeVec(bytes);
    if (file != BNULL)
        Close(file);
    return workspace;
}
