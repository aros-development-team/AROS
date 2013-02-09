/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level routines for i8042 controller.
    Lang: English.
*/

#include "kbd.h"
#include "kbd_common.h"

#define TIMER_RPROK 3599597124UL

int kbd_wait_for_input(void);

static ULONG usec2tick(ULONG usec)
{
    ULONG ret;
    ULONG prok = TIMER_RPROK;
    asm volatile("movl $0,%%eax; divl %2":"=a"(ret):"d"(usec),"m"(prok));
    return ret;
}

void kbd_usleep(LONG usec)
{
    int oldtick, tick;
    usec = usec2tick(usec);

    outb(0x80, 0x43);
    oldtick = inb(0x42);
    oldtick += inb(0x42) << 8;

    while (usec > 0)
    {
        outb(0x80, 0x43);
        tick = inb(0x42);
        tick += inb(0x42) << 8;

        usec -= (oldtick - tick);
        if (tick > oldtick) usec -= 0x10000;
        oldtick = tick;
    }
}

unsigned char handle_kbd_event(void)
{
    unsigned char status = kbd_read_status();
    unsigned int work = 10000;
                
    while (status & KBD_STATUS_OBF)
    {
        kbd_read_input();

        status = kbd_read_status();
        if(!work--)
        {
            //printf(KERN_ERR "pc_keyb: controller jammed (0x%02X).\n",status);
            break;
        }
    }
    return status;
}

/*
 * Wait until we can write to a peripheral again. Any input that comes in
 * while we're waiting is discarded.
 */
void kb_wait(ULONG timeout)
{
    do
    {
        unsigned char status = handle_kbd_event();
        if (! (status & KBD_STATUS_IBF))
            return;
        
        kbd_usleep(1000);
        timeout--;
    } while (timeout);
}

void kbd_write_cmd(int cmd)
{
    kb_wait(100);
    kbd_write_command(KBD_CTRLCMD_WRITE_MODE);
    kb_wait(100);
    kbd_write_output(cmd);
}

/*
 * Aux (mouse) port uses longer delays, this is necessary
 * in order to detect IntelliMouse properly
 */
void aux_write_ack(int val)
{
    kb_wait(1000);
    kbd_write_command(KBD_CTRLCMD_WRITE_MOUSE);
    kb_wait(1000);
    kbd_write_output(val);
    kbd_wait_for_input();
}

void aux_write_noack(int val)
{
    kb_wait(1000);
    kbd_write_command(KBD_CTRLCMD_WRITE_MOUSE);
    kb_wait(1000);
    kbd_write_output(val);
}

void kbd_write_output_w(int data)
{
    kb_wait(100);
    kbd_write_output(data);
}

void kbd_write_command_w(int data)
{
    kb_wait(100);
    kbd_write_command(data);
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

int kbd_wait_for_input(void)
{
    ULONG timeout = 100;

    do
    {
        int retval = kbd_read_data();
        if (retval >= 0)
            return retval;
        kbd_usleep(1000);
    } while(--timeout);
    return -1;
}

