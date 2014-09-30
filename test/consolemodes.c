/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char **argv) {
    BPTR in, out;
    TEXT something[64];
    int i;

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

    /* TODO: Switch to cooked mode */

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
