/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level support routines for i8042 controller.
    Lang: English.
*/

#include <proto/exec.h>

#include "i8042_kbd.h"
#include "i8042_common.h"

void kbd_usleep(struct IORequest* tmr, LONG usec)
{
    tmr->io_Command = TR_ADDREQUEST;
    ((struct timerequest*)tmr)->tr_time.tv_secs = 0;
    ((struct timerequest*)tmr)->tr_time.tv_micro = usec * 1000;
    DoIO(tmr);
}


void kbd_write_cmd(struct IORequest* tmr, int cmd)
{
    kb_wait(tmr, 100);
    kbd_write_command(KBD_CTRLCMD_WRITE_MODE);
    kb_wait(tmr, 100);
    kbd_write_output(cmd);
}

/*
 * Aux (mouse) port uses longer delays, this is necessary
 * in order to detect IntelliMouse properly
 */
void aux_write_ack(struct IORequest* tmr, int val)
{
    kb_wait(tmr, 1000);
    kbd_write_command(KBD_CTRLCMD_WRITE_MOUSE);
    kb_wait(tmr, 1000);
    kbd_write_output(val);
    kbd_wait_for_input(tmr);
}

void aux_write_noack(struct IORequest* tmr, int val)
{
    kb_wait(tmr, 1000);
    kbd_write_command(KBD_CTRLCMD_WRITE_MOUSE);
    kb_wait(tmr, 1000);
    kbd_write_output(val);
}

void kbd_write_output_w(struct IORequest* tmr, int val)
{
    kb_wait(tmr, 100);
    kbd_write_output(val);
}

void kbd_write_command_w(struct IORequest* tmr, int val)
{
    kb_wait(tmr, 100);
    kbd_write_command(val);
}

int kbd_read_data(void)
{
    LONG        retval = KBD_NO_DATA;
    UBYTE       status;
    
    status = kbd_read_status();
    if (status & KBD_STATUS_OBF)
    {
        UBYTE   data = kbd_read_input();
        
        retval = data;
        if (status & (KBD_STATUS_GTO | KBD_STATUS_PERR))
            retval = KBD_BAD_DATA;
    }
    
    return retval;
}

int kbd_clear_input(void)
{
    int maxread = 100, code, lastcode = KBD_NO_DATA;
    UBYTE status;

    do
    {
        status = kbd_read_status();
        if ((code = kbd_read_data()) == KBD_NO_DATA)
            break;
        if (!(status & KBD_STATUS_MOUSE_OBF))
            lastcode = code;
    } while (--maxread);

    return lastcode;
}

