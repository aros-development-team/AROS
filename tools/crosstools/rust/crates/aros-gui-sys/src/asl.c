/* File requesters and message requesters for Rust on AROS. Contract: glue.h.

   A separate object from glue.c, so only programs that ask for files pull
   in asl.library. */
#include <stdint.h>
#include <string.h>
#include <exec/memory.h>
#include <libraries/asl.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/intuition.h>

#include "glue.h"

char *aros_gui_file_request(uintptr_t window, const char *title, const char *drawer,
                            const char *file, const char *pattern, int flags)
{
    struct FileRequester *fr;
    BOOL drawers = (flags & AROS_GUI_FR_DRAWERS) != 0;
    char *path = NULL;

    fr = AllocAslRequestTags(ASL_FileRequest,
        window ? ASLFR_Window : TAG_IGNORE, window,
        title ? ASLFR_TitleText : TAG_IGNORE, (IPTR)title,
        drawer ? ASLFR_InitialDrawer : TAG_IGNORE, (IPTR)drawer,
        file ? ASLFR_InitialFile : TAG_IGNORE, (IPTR)file,
        pattern ? ASLFR_InitialPattern : TAG_IGNORE, (IPTR)pattern,
        ASLFR_DoPatterns, pattern != NULL,
        ASLFR_DoSaveMode, (flags & AROS_GUI_FR_SAVE) != 0,
        ASLFR_DrawersOnly, drawers,
        TAG_DONE);
    if (!fr)
        return NULL;

    /* No file name typed is a cancel too, unless a drawer was asked for. */
    if (AslRequestTags(fr, TAG_DONE) && (drawers || fr->fr_File[0])) {
        ULONG len = strlen((char *)fr->fr_Drawer) + strlen((char *)fr->fr_File) + 2;

        path = AllocVec(len, MEMF_ANY);
        if (path) {
            strcpy(path, (char *)fr->fr_Drawer);
            if (!drawers)
                AddPart((STRPTR)path, fr->fr_File, len);
        }
    }
    FreeAslRequest(fr);
    return path;
}

int aros_gui_message(uintptr_t window, const char *title, const char *text,
                     const char *buttons)
{
    struct EasyStruct es = {
        sizeof(struct EasyStruct), 0,
        (CONST_STRPTR)title, (CONST_STRPTR)"%s", (CONST_STRPTR)buttons
    };
    IPTR args[1] = { (IPTR)text };

    return EasyRequestArgs((struct Window *)window, &es, NULL, (RAWARG)args);
}

void aros_gui_string_free(char *text)
{
    FreeVec(text);
}
