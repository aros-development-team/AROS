#include <dos/dosasl.h>
#include <string.h>

/* Function needed by MatchFirst/Next */

BOOL writeFullPath(struct AnchorPath * AP)
{
  struct AChain * AC = AP->ap_Base;
  BOOL end = FALSE;
  char * LastPos = (char *)&AP->ap_Buf;
  int copied = 0;
  
  while (FALSE == end)
  {
    int len = strlen(AC->an_Info.fib_FileName);
    if (copied+len > AP->ap_Strlen)
    {
      return FALSE;
    } 
    strcpy(&LastPos[copied], AC->an_Info.fib_FileName);
    copied += len;
    
    if (AC != AP->ap_Current)
    {
      /* also add a '/' */
      if (copied+1 > AP->ap_Strlen)
      {
        return FALSE;
      }
      LastPos[copied]='/';
      copied++;
    }
    else
    {
      if (copied+1 > AP->ap_Strlen)
      {
        return FALSE;
      }
      LastPos[copied]='\0';
      end = TRUE;
    }
      
    AC = AC->an_Child;
  }
  return TRUE;
}