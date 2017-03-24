/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <devices/prtbase.h>

#include <aros/shcommands.h>

TEXT const printertxt[] = 
        "aRIS\033c\n"
        "aRIN\033#1\n"
        "aIND\033DaIND\n"
        "aNEL\033EaNEL\n"
        "aRI\033MaRI\n"
        "\n"
        "aSGR: \033[3m italics\033[4m italic underline\033[1m italic underline bold\n"
        "      \033[23m underline bold\033[24m bold\033[22m normal\033[4m underline\n"
        "      \033[0m normal\n"
        "aSFC  \033[30mblack \033[31mred \033[32mgreen \033[33myellow \033[34mblue \033[35md. gray \033[36mcyan \033[37ml. gray\033[0m\n"
        "aSBC  \033[40m\033[39mblack \033[41m\033[30mred \033[42mgreen \033[43myellow \033[44mblue \033[45md. gray \033[46mcyan \033[47ml. gray\033[0m\n"
        "aSHOR elite  \033[2wis abcdefg on \033[1wthen off\n"
        "      cond.  \033[4wis abcdefg on \033[3wthen off\n"
        "      enla   \033[6wis abcdefg on \033[5wthen off\n"
        "aDEN  shadow \033[6\"zis on \033[5\"zthen off\n"
        "      double \033[4\"zis on \033[3\"zthen off\n"
        "      NLQ    \033[2\"zis on \033[1\"zthen off\n"
        "\n"
        "aSUS  super  \033[2vis on \033[1vthen off\n"
        "      subs   \033[4vis on \033[3vthen off\n\n"
        "      both   \033[2vsuper \033[4vboth \033[0vneither\n"
        "\n"
        "aPL   \033L partial line up, \033K partial line down\n"
        "\n"
        "aFNT  (test needs to be written)\n"
        "aPROP prop   \033[2pis on \033[1pthen off \033[0pthen to default\n"
        "aTSS  prop   \033[2p\033[5Eproportional offset of 5\033[0p\n"
        "aJFY  left   \033[5F\nleft justify this text\033[0F\n"
        "      right  \033[7F\nright justify this text\033[0F\n"
        "      full   \033[6F\nfull justify this text\033[0F\n"
        "      letter \033[3F\nletter justify this text\033[0F\n"
        "      center \033[1F\ncenter justify this text\033[0F\n"
        "aVER  1/8\"  \033[0zOne\n            Eighth\n            Inch\n            Spacing\n"
        "      1/6\"  \033[1zOne\n            Sixth\n            Inch\n            Spacing\n"
        "aSLPP (test needs to be written)\n"
        "a?MS  (test needs to be written)\n"
        ;

AROS_SH0(printertxt, 1.0)
{
    AROS_SHCOMMAND_INIT

    struct MsgPort *mp;
    LONG err = RETURN_FAIL;
    struct Library *DOSBase;

    if (!(DOSBase = OpenLibrary("dos.library", 0))) {
        return RETURN_FAIL;
    }

    if ((mp = CreateMsgPort())) {
        struct IORequest *io;
        if ((io = CreateIORequest(mp, sizeof(*io)))) {
            if (0 == OpenDevice("printer.device", 0, io, 0)) {
                Printf("Printing test page to printer.device unit 0...\n");
                struct IOStdReq *sio = (struct IOStdReq *)io;
                sio->io_Command = CMD_WRITE;
                sio->io_Data = (APTR)printertxt;
                sio->io_Length = -1;
                err = DoIO(io);
                if (err != 0) {
                    Printf("\tFAILED (%d)\n", err);
                    err = RETURN_FAIL;
                } else {
                    Printf("\tPASSED (please compare with printertxt.pdf)\n");
                    err = RETURN_OK;
                }
                CloseDevice(io);
            } else {
                Printf("Can't open printer.device unit 0\n");
            }
            DeleteIORequest(io);
        } else {
            Printf("Can't create IO request\n");
        }
        DeleteMsgPort(mp);
    } else {
        Printf("Can't create MsgPort\n");
    }

    CloseLibrary(DOSBase);

    return err;

    AROS_SHCOMMAND_EXIT
}

