/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <aros/debug.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <string.h>

static ULONG failures;
static void check(BOOL okay, CONST_STRPTR message)
{
    if (!okay) { bug("AFS IO FAIL: %s (IoErr %ld)\n", message, (long)IoErr()); failures++; }
}

/* Supply a private mounted test volume; existing user directories are untouched. */
int main(int argc, char **argv)
{
    BPTR root = BNULL, dir = BNULL, previous = BNULL, file = BNULL;
    UBYTE expected[2049], actual[2049];
    ULONG i;
    LONG bytes;
    if (argc != 2) { bug("AFS IO: supply a private test volume\n"); return 20; }
    root = Lock(argv[1], ACCESS_READ);
    if (!root) { check(FALSE,"mount test volume"); goto done; }
    previous = CurrentDir(root);
    dir = Lock("__afs_rom_size_test", ACCESS_READ);
    if (dir) { UnLock(dir); dir = BNULL; check(FALSE,"test directory must not exist"); goto restore; }
    dir = CreateDir("__afs_rom_size_test");
    if (!dir) { check(FALSE,"create directory"); goto restore; }
    UnLock(dir); dir = BNULL;
    for (i=0;i<sizeof(expected);i++) expected[i]=(UBYTE)(i*37+11);
    file = Open("__afs_rom_size_test/data", MODE_NEWFILE);
    if (!file) { check(FALSE,"create file"); goto cleanup; }
    check(Write(file,expected,1537)==1537,"write across block boundaries");
    Close(file); file = BNULL;
    file = Open("__afs_rom_size_test/data", MODE_READWRITE);
    if (!file) { check(FALSE,"reopen for update"); goto cleanup; }
    check(Seek(file,0,OFFSET_END)==0,"seek to end");
    check(Write(file,expected+1537,512)==512,"append another block");
    check(Seek(file,0,OFFSET_BEGINNING)==2049,"seek back to beginning");
    bytes=Read(file,actual,sizeof(actual));
    check(bytes==sizeof(actual) && memcmp(actual,expected,sizeof(actual))==0,"read back exact bytes");
    check(Read(file,actual,1)==0,"end of file");
    Close(file); file=BNULL;
    check(Rename("__afs_rom_size_test/data","__afs_rom_size_test/renamed"),"rename file");
    file=Open("__afs_rom_size_test/renamed",MODE_OLDFILE);
    check(file && Read(file,actual,sizeof(actual))==sizeof(actual) && memcmp(actual,expected,sizeof(actual))==0,"renamed file preserves data");
    if(file){Close(file);file=BNULL;}
cleanup:
    if(file){Close(file);file=BNULL;}
    DeleteFile("__afs_rom_size_test/data");
    DeleteFile("__afs_rom_size_test/renamed");
    check(DeleteFile("__afs_rom_size_test"),"remove empty directory");
restore:
    CurrentDir(previous);
done:
    if(root)UnLock(root);
    bug("AFS IO TEST: %lu failures\n",(unsigned long)failures);
    return failures ? 20 : 0;
}
