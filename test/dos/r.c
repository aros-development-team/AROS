#include <proto/dos.h>
#include <proto/exec.h>

#include <aros/debug.h>

TEXT buffer[100];

/*
 * A tool like "R" http://www.geit.de/eng_r.html requires that we can
 * call a command such that it prints the template and then stops.
 */

int main(void)
{
    // in the real "R" we would check for an unused file name first
    BPTR input = Open("t:00000001.request.infile", MODE_NEWFILE);
    BPTR output = Open("t:00000001.request.outfile", MODE_NEWFILE);

    // shut up DOS error message
    struct Process *me = (struct Process*)FindTask(NULL);
    APTR oldwin = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;

    // execute the command. The purpose of "*>NIL:" is to
    // trigger an error
    Execute("dir *>NIL: ?", input, output);

    // restore window ptr
    me->pr_WindowPtr = oldwin;

    Seek(output, 0, OFFSET_BEGINNING);

    // read the template
    if (FGets(output, buffer, sizeof buffer))
    {
        bug("*****\n%s*****\n", buffer);
    }
    else
    {
        bug("%s\n", "-----");
    }

    Close(input);
    Close(output);

    return 0;
}
