#include <proto/dos.h>
#include <proto/alib.h>
#include <dos/dos.h>

#include <stdio.h>

#include "systemtags.h"

#define DEBUG 0
#include <aros/debug.h>

int main(int argc, char *argv[])
{
    struct STData *data;
    BPTR input, output;
    UBYTE buf[80];

    if (argc == 2)
    {
        sscanf(argv[1], "%p", &data);

        D(bug("[systemtags_slave]Input: %p, Output: %p\n",
              data->input, data->output
        ));

        input = SelectInput(data->input);
        output = SelectOutput(data->output);
    }
    else
    {
        /* Flush() will remove command line arguments from Input() */
        Flush(Input());
    }

    FPuts(Output(), "Type something (+return):\n");
    FGets(Input(), buf, 80);
    FPrintf(Output(), "Got: '%s'\n", buf);

    if (argc == 2)
    {
        SelectInput(input);
        SelectOutput(output);
    }

    return 0;
}
