#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/dostags.h>

int main(void)
{
  SystemTags("dir", SYS_Asynch, TRUE, SYS_Input, Open("NIL:", MODE_OLDFILE),
	     SYS_Output, Open("RAW:////Dir/CLOSE/WAIT", MODE_NEWFILE), TAG_DONE);
  SystemTags("systemtagsslave", SYS_Asynch, TRUE, SYS_Input, SYS_DupStream,
	     SYS_Output, SYS_DupStream, TAG_DONE);
}
