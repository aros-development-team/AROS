/* Test of the DOS functions MatchFirst MatchNext and MatchEnd.
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>

void main (void)
{
  BPTR dirlock,oldlock;
  BOOL error;
  BOOL success;
  int strlength=60;
  struct AnchorPath * AP = AllocVec(sizeof(struct AnchorPath) + strlength,MEMF_CLEAR);

  dirlock = Lock("libs:",ACCESS_READ);
  oldlock = CurrentDir(dirlock);

    AP->ap_BreakBits = SIGBREAKF_CTRL_C;
    AP->ap_Flags  = 0;//APF_DODIR;
    AP->ap_Strlen = strlength;

  error = MatchFirst("#?",AP);
  printf("error: %i \t %s\t(%i)\n",error,AP->ap_Base->an_String,strlen(AP->ap_Base->an_String));
  printf("  Matchfirst found %s\n",AP->ap_Info.fib_FileName);
/*
  printf("  Matchfirst found %s\n",AP->ap_Buf);
*/
  AP->ap_Flags |= APF_DODIR;
  while (0 ==(success = MatchNext(AP)))
  {
    printf("error %i \t Matchnext found %s \t Flags:%x\n",error,AP->ap_Info.fib_FileName,AP->ap_Flags);
/*
    printf("         \t Matchnext found %s\n",AP->ap_Buf);
*/
    if (AP->ap_Flags & APF_DIDDIR)
    {
      AP->ap_Flags &=~APF_DIDDIR;
    }
    else /* if dir then enter it! */
      if (AP->ap_Info.fib_DirEntryType > 0)
        AP->ap_Flags |=APF_DODIR;
  }
  printf("error = %i \n",success);
  MatchEnd(AP);
  FreeVec(AP);

  CurrentDir(oldlock);
printf("unlocking dirlock...\n");
  UnLock(dirlock);
printf("end of prog.\n");
}