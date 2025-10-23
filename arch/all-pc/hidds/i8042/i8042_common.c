/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: Low-level support routines for i8042 controller.
*/

#include <proto/exec.h>

#include "i8042_kbd.h"
#include "i8042_common.h"

void i8042_delay(struct IORequest* tmr, LONG usec)
{
    tmr->io_Command = TR_ADDREQUEST;
    ((struct timerequest*)tmr)->tr_time.tv_secs = 0;
    ((struct timerequest*)tmr)->tr_time.tv_micro = usec;
    DoIO(tmr);
}


void i8042_write_mode(struct IORequest* tmr, int cmd)
{
    i8042_wait_for_write_ready(tmr, 100);
    i8042_write_command_port(KBD_CTRLCMD_WRITE_MODE);
    i8042_wait_for_write_ready(tmr, 100);
    i8042_write_data_port(cmd);
}

/*
 * Aux (mouse) port uses longer delays, this is necessary
 * in order to detect IntelliMouse properly
 */
void i8042_mouse_write_ack(struct IORequest* tmr, int val)
{
    i8042_wait_for_write_ready(tmr, 1000);
    i8042_write_command_port(KBD_CTRLCMD_WRITE_MOUSE);
    i8042_wait_for_write_ready(tmr, 1000);
    i8042_write_data_port(val);
    i8042_wait_for_input(tmr);
}

void i8042_mouse_write_noack(struct IORequest* tmr, int val)
{
    i8042_wait_for_write_ready(tmr, 1000);
    i8042_write_command_port(KBD_CTRLCMD_WRITE_MOUSE);
    i8042_wait_for_write_ready(tmr, 1000);
    i8042_write_data_port(val);
}

void i8042_write_data_with_wait(struct IORequest* tmr, int val)
{
    i8042_wait_for_write_ready(tmr, 100);
    i8042_write_data_port(val);
}

void i8042_write_command_with_wait(struct IORequest* tmr, int val)
{
    i8042_wait_for_write_ready(tmr, 100);
    i8042_write_command_port(val);
}

int i8042_read_data(void)
{
    LONG        retval = KBD_NO_DATA;
    UBYTE       status;

    status = i8042_read_status_port();
    if (status & KBD_STATUS_OBF) {
        UBYTE   data = i8042_read_data_port();

        retval = data;
        if (status & (KBD_STATUS_GTO | KBD_STATUS_PERR))
            retval = KBD_BAD_DATA;
    }

    return retval;
}

int i8042_kbd_clear_input(void)
{
    int maxread = 100, lastcode = KBD_NO_DATA;
    UBYTE status;

    do {
        int code;
        status = i8042_read_status_port();
        if ((code = i8042_read_data()) == KBD_NO_DATA)
            break;
        if (!(status & KBD_STATUS_MOUSE_OBF))
            lastcode = code;
    } while (--maxread);

    return lastcode;
}

