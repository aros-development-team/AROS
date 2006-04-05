#ifndef HIDD_KBD_H
#define HIDD_KBD_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the kbd HIDD.
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
#include <hidd/irq.h>
/****************************************************************************************/

#define KBD_STATUS_OBF 			0x01 	/* keyboard output buffer full */
#define KBD_STATUS_IBF 			0x02 	/* keyboard input buffer full */
#define KBD_STATUS_MOUSE_OBF		0x20	/* Mouse output buffer full */
#define KBD_STATUS_GTO			0x40    /* General receive/xmit timeout */
#define KBD_STATUS_PERR			0x80    /* Parity error */

#define KBD_CTRLCMD_READ_MODE		0x20
#define KBD_CTRLCMD_WRITE_MODE		0x60
#define KBD_CTRLCMD_GET_VERSION 	0xA1
#define KBD_CTRLCMD_MOUSE_DISABLE	0xA7
#define KBD_CTRLCMD_MOUSE_ENABLE	0xA8
#define KBD_CTRLCMD_TEST_MOUSE		0xA9
#define KBD_CTRLCMD_SELF_TEST		0xAA
#define KBD_CTRLCMD_KBD_TEST		0xAB
#define KBD_CTRLCMD_KBD_DISABLE		0xAD
#define KBD_CTRLCMD_KBD_ENABLE		0xAE
#define KBD_CTRLCMD_WRITE_AUX_OBFU	0xD3
#define KBD_CTRLCMD_WRITE_MOUSE		0xD4

#define KBD_OUTCMD_SET_LEDS		0xED
#define KBD_OUTCMD_SET_RATE		0xF3
#define KBD_OUTCMD_ENABLE		0xF4
#define KBD_OUTCMD_DISABLE		0xF5
#define KBD_OUTCMD_RESET		0xFF

#define KBD_STATUS_REG  		0x64
#define KBD_CONTROL_REG			0x64
#define KBD_DATA_REG			0x60

#define KBD_REPLY_POR			0xAA	/* Power on reset */
#define KBD_REPLY_ACK			0xFA	/* Command ACK */
#define KBD_REPLY_RESEND		0xFE	/* Command NACK, send the cmd again */

#define KBD_MODE_KBD_INT		0x01	/* Keyboard data generate IRQ1 */
#define KBD_MODE_MOUSE_INT		0x02	/* Mouse data generate IRQ12 */
#define KBD_MODE_SYS 			0x04	/* The system flag (?) */
#define KBD_MODE_NO_KEYLOCK		0x08	/* The keylock doesn't affect the keyboard if set */
#define KBD_MODE_DISABLE_KBD		0x10	/* Disable keyboard interface */
#define KBD_MODE_DISABLE_MOUSE		0x20	/* Disable mouse interface */
#define KBD_MODE_KCC 			0x40	/* Scan code conversion to PC format */
#define KBD_MODE_RFU			0x80

/****************************************************************************************/

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

/****************************************************************************************/

static inline void outb(unsigned char value, unsigned short port)
{
    __asm__ __volatile__
    ("outb %b0,%w1"
     :
     : "a" (value), "Nd" (port)
    );
}

/****************************************************************************************/

#define kbd_read_input() 	inb(KBD_DATA_REG)
#define kbd_read_status() 	inb(KBD_STATUS_REG)
#define kbd_write_output(val)	outb(val, KBD_DATA_REG)
#define kbd_write_command(val) 	outb(val, KBD_CONTROL_REG)

/****************************************************************************************/

/***** Kbd HIDD *******************/

/* IDs */
#define IID_Hidd_HwKbd		"hidd.kbd.hw"
#define CLID_Hidd_HwKbd		"hidd.kbd.hw"

/* Methods */
enum
{
    moHidd_Kbd_HandleEvent
};

struct pHidd_Kbd_HandleEvent
{
    OOP_MethodID 		mID;
    ULONG 			event;
};

/* misc */

struct abdescr
{
    STRPTR 			interfaceid;
    OOP_AttrBase 		*attrbase;
};

struct kbd_staticdata
{
    struct SignalSemaphore 	sema; /* Protexting this whole struct */

    OOP_Class 			*kbdclass;

    OOP_Object 			*irqhidd;
    OOP_Object 			*kbdhidd;
    
    OOP_AttrBase        hiddKbdAB;
    HIDDT_IRQ_Handler	*irq;
};

struct kbdbase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
    
    struct kbd_staticdata ksd;
};

struct kbd_data
{
    VOID    (*kbd_callback)(APTR, UWORD);
    APTR    callbackdata;

    ULONG   kbd_keystate;
    WORD    prev_amigacode;
    UWORD   prev_keycode;
};

/****************************************************************************************/

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase);
VOID releaseattrbases(struct abdescr *abd, struct Library *OOPBase);

/****************************************************************************************/

#define XSD(cl) 	(&((struct kbdbase *)cl->UserData)->ksd)

/****************************************************************************************/

#endif /* HIDD_KBD_H */
