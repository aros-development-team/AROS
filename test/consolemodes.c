#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/filesystem.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char **argv) {
    BPTR in, out;
    TEXT something[64];
    int i;
    struct IOFileSys iofs;
    struct FileHandle *fh;

    in = Input();
    out = Output();

    SetMode(in, 0);

    Printf("in normal (cooked) mode\n");
    Printf("type something: ");
    Flush(out);

    FGets(out, something, 64);
    *(strchr(something, '\n')) = '\0';

    Printf("you typed: %s\n", something);

    SetMode(in, 1);

    Printf("in raw mode\n");
    Printf("type something: ");
    Flush(out);

    something[63] = '\0';
    i = 0;
    while (i < 63) {
        WaitForChar(in, 0);
        Read(in, &(something[i]), 1);
        if (something[i] == 0x0d) {
            something[i] = '\0';
            break;
        }
        if (! isprint(something[i]))
            continue;
        i++;
    }

    Printf("\nyou typed: %s\n", something);

    fh = (struct FileHandle *) in;

    iofs.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs.IOFS.io_Message.mn_ReplyPort = &(((struct Process *) FindTask(NULL))->pr_MsgPort);
    iofs.IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit = fh->fh_Unit;
    iofs.IOFS.io_Command = FSA_CONSOLE_MODE;
    iofs.IOFS.io_Flags = 0;

    iofs.io_Union.io_CONSOLE_MODE.io_ConsoleMode = FCM_NOECHO;

    DoIO(&(iofs.IOFS));

    Printf("in cooked mode with no echoing\n");
    Printf("type something: ");
    Flush(out);

    FGets(out, something, 64);
    *(strchr(something, '\n')) = '\0';

    Printf("you typed: %s\n", something);

    SetMode(in, 0);

    Printf("restored normal (cooked) mode\n");

    return 0;
}
