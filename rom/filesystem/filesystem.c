#include <aros/symbolsets.h>
#include <resources/filesysres.h>
#include <proto/alib.h>
#include <proto/exec.h>

static int FileSystemInit(struct FileSysResource *FileSystemBase)
{
    FileSystemBase->fsr_Creator = "AROS Development Team";
    NewList(&FileSystemBase->fsr_FileSysEntries);

    return TRUE;
}

ADD2INITLIB(FileSystemInit, 0);
