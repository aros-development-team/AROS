/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: COM mouse driver.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <hidd/serial.h>

#include <devices/inputevent.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

#define HiddMouseAB	(MSD(cl)->hiddMouseAB)

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/* Prototypes */

ULONG mouse_RingHandler(UBYTE *, ULONG, ULONG, struct mouse_data *);
int mouse_CheckRing(struct mouse_data *);
UBYTE mouse_GetFromRing(struct mouse_data *);

/* Misc functions */

#define inb(port) \
    ({  char __value;   \
    __asm__ __volatile__ ("inb $" #port ",%%al":"=a"(__value)); \
    __value;    })

#define outb(val,port) \
    ({  char __value=(val); \
    __asm__ __volatile__ ("outb %%al,$" #port::"a"(__value)); })

/* aros_usleep - sleep for usec microseconds */
#warning: Incompatible with BOCHS busy loop! Change to precise timer.device!
void aros_usleep(ULONG usec)
{
    ULONG hz;
    int step;
    int latch;
    
    while (usec)
    {
        /*
         * If we want to wait longer than 50000 usec, then we have to do it
         * in several steps
         */
	
        step = (usec > 50000) ? 50000 : usec;
        hz = 1000000 / step;
        
        latch = (1193180 + (hz >> 1)) / hz;

        /* Do the timer like cpu.c file */

        outb((inb(0x61) & ~0x02) | 0x01, 0x61);
        outb(0xb0, 0x43);           /* binary, mode 0, LSB/MSB, Ch 2 */
        outb(latch & 0xff, 0x42); /* LSB of count */
        outb(latch >> 8, 0x42);   /* MSB of count */
        
        /* Speaker counter will start now. Just wait till it finishes */
        do {} while ((inb(0x61) & 0x20) == 0);
        
        /* Decrease wait counter */
	    usec -= step;
    }
}

/***** Test procedure ***********************************************/

int test_mouse_com(OOP_Class *cl, OOP_Object *o)
{
    OOP_Object      *serial;
    OOP_Object      *unit;
    
    struct Library  *shidd;
    
    int i=0;
    
    if ((shidd = OpenLibrary("serial.hidd",0)))
    {
        if ((serial = OOP_NewObject(NULL, CLID_Hidd_Serial, NULL)))
        {
            struct mouse_data *data = OOP_INST_DATA(cl, o);
        
            /*
                As we got serial object, we go now through all units searching
                for mouse.

                Because we don't have timed IO operations yet, we will use busy
                loops. Be carefull with BOCHS!! It will not understand this!!
            */
#warning: Chang busy loop for BOCHS!!!!

            /* Allocate ring buffer */

            data->u.com.rx = AllocMem(sizeof(struct Ring), MEMF_CLEAR);

            if (data->u.com.rx)
            {
                for (i=0; i<4; i++)
                {
                    /* Alloc New unit for us */
                    if ((unit = HIDD_Serial_NewUnit(serial, i++)))
                    {
                        /* Install RingBuffer interrupt */
                        HIDD_SerialUnit_Init(unit, mouse_RingHandler, data, NULL, NULL);

                    
                        /* Try to detect mouse type. Use PNP first. */

                    
                        /* No mouse? Dispose useless unit then */
                        HIDD_Serial_DisposeUnit(serial, unit);
                    }
                }
                FreeMem(data->u.com.rx, sizeof(struct Ring));
            }
            
            /* Found no serial mouse... Dispose serial object */
            OOP_DisposeObject(serial);
        }
        CloseLibrary(shidd);
    }    

    return 0; /* Report no COM mouse */
}

/*****  *************************************************************/

ULONG mouse_RingHandler(UBYTE *buf, ULONG len, ULONG unit, struct mouse_data *data)
{
    struct Ring *r = data->u.com.rx;
    while (len--)
    {
        r->ring[r->top++] = *buf++;

        if (r->top >= RingSize) r->top = 0;
    }
    
    return 0;
}

/* 
 * Check whether there is some data in ring. Return nozero value if there
 * is anything to get
 */
int mouse_CheckRing(struct mouse_data *data)
{
    return data->u.com.rx->top - data->u.com.rx->ptr;
}

/*
 * Get one byte from ring buffer. Returns 0 if there is nothing to get
 */
UBYTE mouse_GetFromRing(struct mouse_data *data)
{
    struct Ring *r = data->u.com.rx;
    UBYTE result = 0;
    
    if (r->top != r->ptr)
    {
        result = r->ring[r->ptr++];

        if (r->ptr >= RingSize) r->ptr = 0;
    }

    return result;
}

