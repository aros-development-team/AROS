#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dostags.h>

#include <assert.h>

static BYTE sig;
static struct Task *t;

static void StartCommand(void)
{
  struct FileHandle *fh = Open("S:Startup-Sequence", MODE_OLDFILE);
    
  SystemTags("type in:", SYS_Input, (IPTR) fh);
    
  Signal(t, 1<<sig);  
}

int main(void)
{
    struct FileHandle *fh;
    
    SystemTags
    (
        "dir", 
        SYS_Asynch,        TRUE, 
        SYS_Input,  (IPTR) Open("NIL:", MODE_OLDFILE),
        SYS_Output, (IPTR) Open("RAW:////Dir/CLOSE/WAIT", MODE_NEWFILE), 
        TAG_DONE
    );
    
    SystemTags
    (
        "systemtagsslave", 
        SYS_Asynch, TRUE,
        SYS_Input,  SYS_DupStream,
        SYS_Output, SYS_DupStream, 
        TAG_DONE
    );
    
    fh = Open("S:Startup-Sequence", MODE_OLDFILE);
    assert(fh!=NULL);
    SystemTags("type in:", SYS_Input, (IPTR) fh, TAG_DONE);
    
    sig = AllocSignal(-1);
    t = FindTask(NULL);
    
    CreateNewProcTags(NP_Entry, (IPTR) StartCommand, NP_Input, (IPTR) Input(), NP_CloseInput, FALSE);
    
    Wait(1<<sig);
    FreeSignal(sig);
    
    return 0;
}
