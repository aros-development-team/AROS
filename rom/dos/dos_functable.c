/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:49  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef NULL
#define NULL ((void *)0)
#endif

void Dos_open();
void Dos_close();
void Dos_expunge();
void Dos_null();
void Dos_Open();
void Dos_Close();
void Dos_DupLock();
void Dos_Read();
void Dos_Write();
void Dos_Input();
void Dos_Output();
void Dos_Seek();
void Dos_Examine();
void Dos_CurrentDir();
void Dos_IoErr();
void Dos_LoadSeg();
void Dos_UnLoadSeg();
void Dos_Delay();
void Dos_IsInteractive();
void Dos_AllocDosObject();
void Dos_FreeDosObject();
void Dos_SelectInput();
void Dos_SelectOutput();
void Dos_FGetC();
void Dos_FPutC();
void Dos_UnGetC();
void Dos_FPuts();
void Dos_VFPrintf();
void Dos_Flush();
void Dos_OpenFromLock();
void Dos_NameFromLock();
void Dos_ExAll();
void Dos_SetIoErr();
void Dos_PrintFault();
void Dos_Cli();
void Dos_CreateNewProc();
void Dos_RunCommand();
void Dos_GetArgStr();
void Dos_MaxCli();
void Dos_AssignLock();
void Dos_LockDosList();
void Dos_UnLockDosList();
void Dos_AttemptLockDosList();
void Dos_RemDosEntry();
void Dos_AddDosEntry();
void Dos_FindDosEntry();
void Dos_MakeDosEntry();
void Dos_FreeDosEntry();
void Dos_DateToStr();
void Dos_ReadArgs();
void Dos_FindArg();
void Dos_ReadItem();
void Dos_StrToLong();
void Dos_FreeArgs();
void Dos_PutStr();
void Dos_VPrintf();

void *const Dos_functable[]=
{
    Dos_open, /* 1 */
    Dos_close,
    Dos_expunge,
    Dos_null,
    Dos_Open,
    Dos_Close,
    Dos_Read,
    Dos_Write,
    Dos_Input,
    Dos_Output, /* 10 */
    Dos_Seek,
    NULL,
    NULL,
    Dos_Open, /* Lock */
    Dos_Close, /* UnLock */
    Dos_DupLock,
    Dos_Examine,
    NULL,
    NULL,
    NULL, /* 20 */
    Dos_CurrentDir,
    Dos_IoErr,
    NULL,
    NULL,
    Dos_LoadSeg,
    Dos_UnLoadSeg,
    NULL,
    NULL,
    NULL,
    NULL, /* 30 */
    NULL,
    NULL,
    Dos_Delay,
    NULL,
    NULL,
    Dos_IsInteractive,
    NULL,
    Dos_AllocDosObject,
    Dos_FreeDosObject,
    NULL, /* 40 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Dos_SelectInput,
    Dos_SelectOutput, /* 50 */
    Dos_FGetC,
    Dos_FPutC,
    Dos_UnGetC,
    NULL,
    NULL,
    Dos_FPuts,
    NULL,
    NULL,
    Dos_VFPrintf,
    Dos_Flush, /* 60 */
    NULL,
    Dos_DupLock, /* DupLockFromFH */
    Dos_OpenFromLock,
    NULL,
    Dos_Examine, /* ExamineFH */
    NULL,
    Dos_NameFromLock,
    Dos_NameFromLock, /* NameFromFH */
    NULL,
    NULL, /* 70 */
    NULL,
    Dos_ExAll,
    NULL,
    NULL,
    NULL,
    NULL,
    Dos_SetIoErr,
    NULL,
    Dos_PrintFault,
    NULL, /* 80 */
    NULL,
    Dos_Cli,
    Dos_CreateNewProc,
    Dos_RunCommand,
    NULL,
    NULL,
    NULL,
    NULL,
    Dos_GetArgStr,
    NULL, /* 90 */
    NULL,
    Dos_MaxCli,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* 100 */
    NULL,
    Dos_AssignLock,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Dos_LockDosList,
    Dos_UnLockDosList, /* 110 */
    Dos_AttemptLockDosList,
    Dos_RemDosEntry,
    Dos_AddDosEntry,
    Dos_FindDosEntry,
    NULL,
    Dos_MakeDosEntry,
    Dos_FreeDosEntry,
    NULL,
    NULL,
    NULL, /* 120 */
    NULL,
    NULL,
    NULL,
    Dos_DateToStr,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* 130 */
    NULL,
    NULL,
    Dos_ReadArgs,
    Dos_FindArg,
    Dos_ReadItem,
    Dos_StrToLong,
    NULL,
    NULL,
    NULL,
    NULL, /* 140 */
    NULL,
    NULL,
    Dos_FreeArgs,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* 150 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Dos_PutStr,
    Dos_VPrintf,
    NULL, /* 160 */
    (void *)-1
};

const char Dos_end=0;
