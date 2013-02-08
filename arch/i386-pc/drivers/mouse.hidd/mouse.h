#ifndef _MOUSE_H
#define _MOUSE_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the mouse native HIDD.
    Lang: English.
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#include <hidd/mouse.h>

/* defines for buttonstate */

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MIDDLE_BUTTON   4

#define INTELLIMOUSE_SUPPORT        1

#define PS2_PROTOCOL_STANDARD       0
#define PS2_PROTOCOL_INTELLIMOUSE   1

/***** Mouse HIDD *******************/

struct mouse_staticdata
{
    OOP_AttrBase        hiddAttrBase;
    OOP_AttrBase        hiddMouseAB;

    OOP_Class           *mouseclass;
    OOP_Object          *mousehidd;
};

struct mousebase
{
    struct Library library;
    
    struct mouse_staticdata msd;
};

/* Object data */

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;

    UWORD buttonstate;

    struct Interrupt            irq;
    UBYTE                       mouse_data[5];
    UBYTE                       mouse_collected_bytes;
    UBYTE                       mouse_protocol;
    UBYTE                       mouse_packetsize;
    UBYTE                       expected_mouse_acks;
    UBYTE                       packetsize;
            
    struct pHidd_Mouse_Event    event;
};

/****************************************************************************************/

#define KBD_STATUS_OBF                  0x01    /* keyboard output buffer full */
#define KBD_STATUS_IBF                  0x02    /* keyboard input buffer full */
#define KBD_STATUS_MOUSE_OBF            0x20    /* Mouse output buffer full */
#define KBD_STATUS_GTO                  0x40    /* General receive/xmit timeout */
#define KBD_STATUS_PERR                 0x80    /* Parity error */

#define KBD_CTRLCMD_READ_MODE           0x20
#define KBD_CTRLCMD_WRITE_MODE          0x60
#define KBD_CTRLCMD_GET_VERSION         0xA1
#define KBD_CTRLCMD_MOUSE_DISABLE       0xA7
#define KBD_CTRLCMD_MOUSE_ENABLE        0xA8
#define KBD_CTRLCMD_TEST_MOUSE          0xA9
#define KBD_CTRLCMD_SELF_TEST           0xAA
#define KBD_CTRLCMD_KBD_TEST            0xAB
#define KBD_CTRLCMD_KBD_DISABLE         0xAD
#define KBD_CTRLCMD_KBD_ENABLE          0xAE
#define KBD_CTRLCMD_WRITE_AUX_OBUF      0xD3
#define KBD_CTRLCMD_WRITE_MOUSE         0xD4

#define KBD_OUTCMD_SET_RES              0xE8
#define KBD_OUTCMD_SET_SCALE11          0xE6
#define KBD_OUTCMD_SET_SCALE21          0xE7
#define KBD_OUTCMD_STATUS_REQUEST       0xE9
#define KBD_OUTCMD_SET_STREAM_MODE      0xEA
#define KBD_OUTCMD_READ_DATA            0xEB
#define KBD_OUTCMD_SET_REMOTE_MODE      0xF0
#define KBD_OUTCMD_GET_ID               0xF2
#define KBD_OUTCMD_SET_RATE             0xF3
#define KBD_OUTCMD_SET_STREAM           0xEA
#define KBD_OUTCMD_ENABLE               0xF4
#define KBD_OUTCMD_DISABLE              0xF5
#define KBD_OUTCMD_RESET                0xFF

#define KBD_STATUS_REG                  0x64
#define KBD_CONTROL_REG                 0x64
#define KBD_DATA_REG                    0x60

#define KBD_REPLY_POR                   0xAA    /* Power on reset */
#define KBD_REPLY_ACK                   0xFA    /* Command ACK */
#define KBD_REPLY_RESEND                0xFE    /* Command NACK, send the cmd again */

#define KBD_MODE_KBD_INT                0x01    /* Keyboard data generate IRQ1 */
#define KBD_MODE_MOUSE_INT              0x02    /* Mouse data generate IRQ12 */
#define KBD_MODE_SYS                    0x04    /* The system flag (?) */
#define KBD_MODE_NO_KEYLOCK             0x08    /* The keylock doesn't affect the keyboard if set */
#define KBD_MODE_DISABLE_KBD            0x10    /* Disable keyboard interface */
#define KBD_MODE_DISABLE_MOUSE          0x20    /* Disable mouse interface */
#define KBD_MODE_KCC                    0x40    /* Scan code conversion to PC format */
#define KBD_MODE_RFU                    0x80

/****************************************************************************************/

#ifdef inb
#undef inb
#endif
static inline unsigned char inb(unsigned short port)
{
    unsigned char  _v;

    __asm__ __volatile__
    ("inb %w1,%0"
     : "=a" (_v)
     : "Nd" (port)
    );

    return _v;
}

#ifdef outb
#undef outb
#endif
static inline void outb(unsigned char value, unsigned short port)
{
    __asm__ __volatile__
    ("outb %b0,%w1"
     :
     : "a" (value), "Nd" (port)
    );
}

/****************************************************************************************/

#define mouse_read_input()      inb(0x60) //KBD_DATA_REG)
#define mouse_read_status()     inb(0x64) //KBD_STATUS_REG)
#define mouse_write_output(val) outb(val, 0x60) //KBD_DATA_REG)
#define mouse_write_command(val)        outb(val, 0x64) //KBD_CONTROL_REG)

/****************************************************************************************/


#define MSD(cl)         (&((struct mousebase *)cl->UserData)->msd)

#endif /* _MOUSE_H */

