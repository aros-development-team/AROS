/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc: Test of the DOS functions MatchFirst, MatchNext and MatchEnd.
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>

int main (void)
{
  BOOL error;
  int strlength=160;
  struct AnchorPath * AP = AllocVec(sizeof(struct AnchorPath) + strlength,MEMF_CLEAR);
  char * Pattern = AllocVec(80,MEMF_ANY);

  //dirlock = Lock("sys:",ACCESS_READ);
  //oldlock = CurrentDir(dirlock);

  AP->ap_BreakBits = SIGBREAKF_CTRL_C;
  AP->ap_Flags  = 0;//APF_DODIR;
  AP->ap_Strlen = strlength;

  printf("Give me a pattern to search for: ");
  /* the following line breaks AROS in MatchEnd() when calling FreeVec()
     the second time the program is run. I have no idea why, though. */
  scanf("%s",Pattern);
  printf("Pattern to search for: %s\n",Pattern);

  for(error = MatchFirst(Pattern,AP); error == 0;
      error = MatchNext(AP))
  {
    if (AP->ap_Flags & APF_DIDDIR)
    {
      AP->ap_Flags &= ~APF_DIDDIR;
      printf(" leaving \033[32m%s\033[39m\n",AP->ap_Buf);
    }
    else /* if dir then enter it! */
    {
      if (AP->ap_Info.fib_DirEntryType >= 0)
      {
        AP->ap_Flags |=APF_DODIR;
        printf("entering \033[33m%s\033[39m\n",AP->ap_Buf);
      }
      else
      {
        //BPTR fl = CurrentDir(AP->ap_Current->an_Lock);
        //(void)CurrentDir(fl);
        
        printf("         %s\n",AP->ap_Buf);
      }
    }
  }
  printf("error = %i \n",error);
  MatchEnd(AP);
  FreeVec(AP);
  FreeVec(Pattern);
  //CurrentDir(oldlock);

  //UnLock(dirlock);

    return 0;
}

