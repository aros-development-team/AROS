/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dosasl.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

void ReadAll(BPTR lock)
{
  struct FileInfoBlock * FIB = AllocVec(sizeof(struct FileInfoBlock), MEMF_CLEAR);
  BOOL success;
  int count = 1;

  success = Examine(lock, FIB);
  if (FIB->fib_DirEntryType < 0)
    success = FALSE;
  if (success)
    success = ExNext(lock, FIB);
  while (success)
  {
    printf("%s",FIB->fib_FileName);
    if (FIB->fib_DirEntryType > 0)
    {
      printf(" (Dir)\n");
      count++;
      if (count > 1)
      {
        char * name = AllocVec(1024,0);
        BPTR tmplock;
        NameFromLock(lock,name,1024);
        AddPart(name,FIB->fib_FileName,1024);
        printf("Entering %s\n",name);
        tmplock = Lock(name , ACCESS_READ);
        ReadAll(tmplock);
        NameFromLock(lock,name,1024);
        printf("Returning to %s\n",name);
        UnLock(tmplock);
        FreeVec(name);
      }
    }
    else
    {
      printf("\n");
    }
    success = ExNext(lock,FIB);
  }
  FreeVec(FIB);
}

int main(int argc, char *argv[])
{
  BPTR lock;

  printf("Trying to scan %s \n",argv[1]);
  lock = Lock(argv[1],ACCESS_READ);
  if (lock)
  {
    ReadAll(lock);
    UnLock(lock);
  }
  else
    printf("no such directory/assign!\n");

    return 0;
}
