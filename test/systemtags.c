#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/dostags.h>

int main(void)
{
  SystemTags("dir", SYS_Asynch, TRUE, SYS_Input, Open("NIL:", MODE_OLDFILE),
	     SYS_Output, Open("RAW:////Dir/CLOSE/WAIT", MODE_NEWFILE), TAG_DONE);
  SystemTags("systemtagsslave", SYS_Asynch, TRUE, SYS_Input, DupLock(Input()),
	     SYS_Output, DupLock(Output()), TAG_DONE);
}
