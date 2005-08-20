/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: COM mouse driver.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <asm/io.h>
#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <hidd/serial.h>

#include <devices/inputevent.h>

#include <stdio.h>
#include <strings.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

#define HiddMouseAB	(MSD(cl)->hiddMouseAB)

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/* Prototypes */

ULONG   mouse_RingHandler(UBYTE *, ULONG, ULONG, struct mouse_data *);
int     mouse_CheckRing(struct mouse_data *);
int     mouse_GetFromRing(struct mouse_data *, char *);
int     mouse_Select(struct mouse_data *, ULONG);
void    mouse_FlushInput(struct mouse_data *);
int     mouse_DetectPNP(struct mouse_data *, OOP_Object *);
void    handle_events(UBYTE proto, struct mouse_data *);

/* mouse_usleep - sleep for usec microseconds */
#warning: Incompatible with BOCHS busy loop! Change to precise timer.device!

#define TIMER_RPROK 3599597124UL

static ULONG usec2tick(ULONG usec)
{
    ULONG ret;
    asm volatile("movl $0,%%eax; divl %2":"=eax"(ret):"edx"(usec),"m"(TIMER_RPROK));
    return ret;
}

void mouse_usleep(LONG usec)
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

/***** Test procedure ***********************************************/

int test_mouse_com(OOP_Class *cl, OOP_Object *o)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    int i=0;

    data->u.com.shidd = OpenLibrary("serial.hidd",0);
    if (data->u.com.shidd != NULL)
    {
        data->u.com.serial = OOP_NewObject(NULL, CLID_Hidd_Serial, NULL);
        if (data->u.com.serial != NULL)
        {
            /*
                As we got serial object, we go now through all units searching
                for mouse.

                Because we don't have timed IO operations yet, we will use busy
                loops. Be carefull with BOCHS!! It will not understand this!!
            */
#warning: Chang busy loop for BOCHS!!!!

            /* Allocate ring buffer */

            data->u.com.rx = AllocMem(sizeof(struct Ring), MEMF_CLEAR);
            data->u.com.mouse_inth_state = 0;  /* initialize to init state */

            if (data->u.com.rx)
            {
                for (i=0; i<4; i++)
                {
                    /* Alloc New unit for us */
                    data->u.com.unit = HIDD_Serial_NewUnit(data->u.com.serial, i);
                    if (data->u.com.unit != NULL)
                    {
                        int proto;

                        D(bug("Checking for mouse on serial port %d\n", i));

                        /* Install RingBuffer interrupt */
                        HIDD_SerialUnit_Init(data->u.com.unit, mouse_RingHandler, data, NULL, NULL);

                        /* Try to get mouse protocol in PnP way */
                        if ((proto = mouse_DetectPNP(data, data->u.com.unit)) >= 0)
                        {
                            /* We got protocol */
                            data->u.com.mouse_protocol = proto;
                            switch (proto)
                            {
                                case P_MS:
                                    D(bug("Mouse: protocol: MicroSoft\n"));
                                    break;
                                case P_LOGI:
                                    D(bug("Mouse: protocol: Logitech\n"));
                                    break;
                                case P_LOGIMAN:
                                    D(bug("Mouse: protocol: Logitech MouseMan\n"));
                                    break;
                                default:
                                    D(bug("Mouse: protocol: %d\n", proto));
                            }
                            data->u.com.mouse_inth_state = 1;  /* initialize to event handling state */
                            return 1; /* report the found mouse */
                        }
                        else
                        {
                            D(bug("Mouse: no serial mouse detected!\n"));
                            /* No mouse? Dispose useless unit then */
                            HIDD_Serial_DisposeUnit(data->u.com.serial, data->u.com.unit);
                        }
                    }
                }
                FreeMem(data->u.com.rx, sizeof(struct Ring));
            }

            /* Found no serial mouse... Dispose serial object */
            OOP_DisposeObject(data->u.com.serial);
        }
        CloseLibrary(data->u.com.shidd);
    }
    return 0; /* Report no COM mouse */
}

void dispose_mouse_seriell(OOP_Class *cl, OOP_Object *o) {
struct mouse_data *data = OOP_INST_DATA(cl, o);

	HIDD_Serial_DisposeUnit(data->u.com.serial, data->u.com.unit);
	FreeMem(data->u.com.rx, sizeof(struct Ring));
	OOP_DisposeObject(data->u.com.serial);
	CloseLibrary(data->u.com.shidd);
}

/******************************************************************/

#undef SysBase
#define SysBase (*(struct ExecBase **)4L)

/* serial PnP ID string */
typedef struct {
    int revision;       /* PnP revision, 100 for 1.00 */
    char *eisaid;       /* EISA ID including mfr ID and product ID */
    char *serial;       /* serial No, optional */
    char *class;        /* device class, optional */
    char *compat;       /* list of compatible drivers, optional */
    char *description;  /* product description, optional */
    int neisaid;        /* length of the above fields... */
    int nserial;
    int nclass;
    int ncompat;
    int ndescription;
} pnpid_t;

/* symbol table entry */
typedef struct {
    char *name;
    int val;
} symtab_t;

static const __attribute__((section(".text"))) symtab_t pnpprod[] = {
    { "KML0001",        P_THINKING },   /* Kensignton ThinkingMouse */
    { "MSH0001",        P_IMSERIAL },   /* MS IntelliMouse */
    { "MSH0004",        P_IMSERIAL },   /* MS IntelliMouse TrackBall */
    { "KYEEZ00",        P_MS },         /* Genius EZScroll */
    { "KYE0001",        P_MS },         /* Genius PnP Mouse */
    { "KYE0003",        P_IMSERIAL },   /* Genius NetMouse */
    { "LGI800C",        P_IMSERIAL },   /* Logitech MouseMan (4 button model) */
    { "LGI8050",        P_IMSERIAL },   /* Logitech MouseMan+ */
    { "LGI8051",        P_IMSERIAL },   /* Logitech FirstMouse+ */
    { "LGI8001",        P_LOGIMAN },    /* Logitech serial */

    { "PNP0F00",        P_BM },         /* MS bus */
    { "PNP0F01",        P_MS },         /* MS serial */
    { "PNP0F02",        P_BM },         /* MS InPort */
    { "PNP0F03",        P_PS2 },        /* MS PS/2 */
    /*
     * EzScroll returns PNP0F04 in the compatible device field; but it
     * doesn't look compatible... XXX
     */
    { "PNP0F04",        P_MSC },        /* MouseSystems */
    { "PNP0F05",        P_MSC },        /* MouseSystems */
    { "PNP0F08",        P_LOGIMAN },    /* Logitech serial */
    { "PNP0F09",        P_MS },         /* MS BallPoint serial */
    { "PNP0F0A",        P_MS },         /* MS PnP serial */
    { "PNP0F0B",        P_MS },         /* MS PnP BallPoint serial */
    { "PNP0F0C",        P_MS },         /* MS serial comatible */
    { "PNP0F0D",        P_BM },         /* MS InPort comatible */
    { "PNP0F0E",        P_PS2 },        /* MS PS/2 comatible */
    { "PNP0F0F",        P_MS },         /* MS BallPoint comatible */
    { "PNP0F11",        P_BM },         /* MS bus comatible */
    { "PNP0F12",        P_PS2 },        /* Logitech PS/2 */
    { "PNP0F13",        P_PS2 },        /* PS/2 */
    { "PNP0F15",        P_BM },         /* Logitech bus */
    { "PNP0F17",        P_LOGIMAN },    /* Logitech serial compat */
    { "PNP0F18",        P_BM },         /* Logitech bus compatible */
    { "PNP0F19",        P_PS2 },        /* Logitech PS/2 compatible */
    { NULL,             -1 },
};

int mouse_pnpgets(struct mouse_data *data, OOP_Object *unit, char *buf)
{
    int     i,tmpavail;
    char    c;

    struct TagItem stags[] = {
        { TAG_DATALENGTH,   7 },
        { TAG_STOP_BITS,    1 },
        { TAG_PARITY_OFF,   1 },
        { TAG_DONE,         0 }};

    struct TagItem mcr[] = {
        { TAG_SET_MCR,      0 },
        { TAG_DONE,         0 }};

    /* Try to detect mouse according to XF86 sources. */
    HIDD_SerialUnit_SetBaudrate(unit, 1200);
    HIDD_SerialUnit_SetParameters(unit, stags);
    
    /* Set DTR=1, RTS=0 */
//    mcr[0].ti_Data = 1;
    /* Set DTR=0, RTS=0 */
    HIDD_SerialUnit_SetParameters(unit, mcr);
    mouse_usleep(200000);

    /* wait for response */
    mouse_FlushInput(data);
    mcr[0].ti_Data = 3;     /* DTR=1, RTS=1 */
    HIDD_SerialUnit_SetParameters(unit, mcr);
    /* Try to read data. Mouse has to respond if PNP */
    tmpavail = mouse_Select(data, 300000);
    if (!tmpavail)
        goto connect_idle;

    /* Collect PnP COM device ID */
    i = 0;
    mouse_usleep(200000);   /* the mouse must send `Begin ID' within 200msec */
    while (mouse_GetFromRing(data, &c))
    {
        D(bug("Mouse: nopnp protocol detection %ld\n", c));

        /* we may see "M", or "M3..." before `Begin ID' */
        if ((i == 0) && ((c == 77) || (c == 79)))
        {
            buf[0] = c;
            break;
        }
        if ((c == 0x08) || (c == 0x28))     /* Begin ID */
        {
            D(bug("Mouse: yeah, we got a begin ID: %lx\n", c));
            buf[i++] = c;
            break;
        }
    }

    /* we haven't seen `Begin ID' in time... */
    if(i <= 0)
    {
        if(buf[0] != 0)
            return 1;

        goto connect_idle;
    }

    ++c;    /* make it `End ID' */
    for (;;)
    {
        if (!mouse_Select(data, 300000))
            break;

        mouse_GetFromRing(data, &buf[i]);
        if (buf[i++] == c)      /* End ID */
            break;
        if (i >= 256)
            break;
    }

    if (buf[i - 1] != c)
        goto connect_idle;

    return i;

connect_idle:
    D(bug("connect_idle\n"));
    return 0;
}

static int mouse_pnpparse(pnpid_t *id, char *buf, int len)
{
    char s[3];
    int offset;
    int sum = 0;
    int i, j;

    id->revision = 0;
    id->eisaid = NULL;
    id->serial = NULL;
    id->class = NULL;
    id->compat = NULL;
    id->description = NULL;
    id->neisaid = 0;
    id->nserial = 0;
    id->nclass = 0;
    id->ncompat = 0;
    id->ndescription = 0;

    offset = 0x28 - buf[0];

    /* calculate checksum */
    for (i = 0; i < len - 3; ++i)
    {
        sum += buf[i];
        buf[i] += offset;
    }
    sum += buf[len - 1];
    for (; i < len; ++i)
        buf[i] += offset;
//    D(bug("Mouse: PnP ID string: '%*.*s'\n", len, len, buf));

    D(bug("Mouse: PnP ID string: '%s'\n", buf));

    /* revision */
    buf[1] -= offset;
    buf[2] -= offset;
    id->revision = ((buf[1] & 0x3f) << 6) | (buf[2] & 0x3f);
    D(bug("Mouse: PnP rev %d.%02d\n", id->revision / 100, id->revision % 100));

    /* EISA vender and product ID */
    id->eisaid = &buf[3];
    id->neisaid = 7;

    D(bug("Mouse: EISA vendor/product ID: %07s\n", id->eisaid));

    /* option strings */
    i = 10;
    if (buf[i] == '\\')
    {
        /* device serial # */
        for (j = ++i; i < len; ++i)
        {
            if (buf[i] == '\\')
                break;
        }
        if (i >= len)
            i -= 3;
        if (i - j == 8)
        {
            id->serial = &buf[j];
            id->nserial = 8;
        }
    }
    if (buf[i] == '\\')
    {
        /* PnP class */
        for (j = ++i; i < len; ++i)
        {
            if (buf[i] == '\\')
            break;
        }
        if (i >= len)
            i -= 3;
        if (i > j + 1)
        {
            id->class = &buf[j];
            id->nclass = i - j;
        }
    }

    if (buf[i] == '\\')
    {
        /* compatible driver */
        for (j = ++i; i < len; ++i)
        {
            if (buf[i] == '\\')
                break;
        }
        /*
         * PnP COM spec prior to v0.96 allowed '*' in this field,
         * it's not allowed now; just igore it.
         */
        if (buf[j] == '*')
            ++j;
        if (i >= len)
            i -= 3;
        if (i > j + 1)
        {
            id->compat = &buf[j];
            id->ncompat = i - j;
        }
        D(bug("Mouse: compat: %d\n", id->compat));
    }

    if (buf[i] == '\\')
    {
        /* product description */
        for (j = ++i; i < len; ++i)
        {
            if (buf[i] == ';')
                break;
        }
        if (i >= len)
            i -= 3;
        if (i > j + 1)
        {
            id->description = &buf[j];
            id->ndescription = i - j;
        }
        D(bug("Mouse: product description: %s\n", id->description));
    }

    /* checksum exists if there are any optional fields */
    if ((id->nserial > 0) || (id->nclass > 0)
        || (id->ncompat > 0) || (id->ndescription > 0))
    {
        sprintf(s, "%02X", sum & 0x0ff);
        D(bug("Mouse: optional fields ?: %s\n", s));
        if (strncmp(s, &buf[len - 3], 2) != 0)
        {
        }
    }
    return TRUE;
}

/* name/val mapping */

static symtab_t *gettoken(symtab_t *tab, char *s, int len)
{
    int i;

    for (i = 0; tab[i].name != NULL; ++i)
    {
        if (strncmp(tab[i].name, s, len) == 0)
            break;
    }
    return &tab[i];
}

static symtab_t *mouse_pnpproto(pnpid_t *id)
{
    symtab_t *t;
    int i, j;

    if (id->nclass > 0)
    if (strncmp(id->class, "MOUSE", id->nclass) != 0)
        /* this is not a mouse! */
        return NULL;

    if (id->neisaid > 0)
    {
        t = gettoken(pnpprod, id->eisaid, id->neisaid);
        if (t->val != -1)
            return t;
    }

    /*
     * The 'Compatible drivers' field may contain more than one
     * ID separated by ','.
     */
    if (id->ncompat <= 0)
        return NULL;
    for (i = 0; i < id->ncompat; ++i)
    {
        for (j = i; id->compat[i] != ','; ++i)
            if (i >= id->ncompat)
                break;
        if (i > j)
        {
            t = gettoken(pnpprod, id->compat + j, i - j);
            if (t->val != -1)
                return t;
        }
    }

    return NULL;
}

int mouse_DetectPNP(struct mouse_data *data, OOP_Object *unit)
{
    char buf[256];
    int len;
    pnpid_t pnpid;
    symtab_t *t;

    len = mouse_pnpgets(data, unit, buf);

    if (len == 1)
    {
        if(buf[0] == 77)
            return 0;
        if(buf[0] == 79)
            return 1;
	   
	/* stegerg: checkme! Added this return -1, because if this is
	   not there, then below the "return (t->val)" is used to leave
	   the function, with t pointing to random address */
	    
	return -1;
    }
    else if(len > 1)
    {
        if(!mouse_pnpparse(&pnpid, buf, len))
            return -1;
        if ((t = mouse_pnpproto(&pnpid)) == NULL)
            return -1;
    }
    else if(len < 1)
        return -1;

    D(bug("Mouse: protocol: %d\n", t->val));

    return (t->val);
}

ULONG mouse_RingHandler(UBYTE *buf, ULONG len, ULONG unit, struct mouse_data *data)
{
    struct Ring *r = data->u.com.rx;

    while (len--)
    {
        r->ring[r->top++] = *buf++;

        if (r->top >= RingSize) r->top = 0;
    }

    if(data->u.com.mouse_inth_state >= 1)
        handle_events(data->u.com.mouse_protocol, data);

    return 0;
}

void handle_events(UBYTE proto, struct mouse_data *data)
{
//    static UBYTE inbuf[3];
    struct pHidd_Mouse_Event *e = &data->u.com.event;
    UWORD buttonstate;
    char c;

    D(bug("Mouse: handling events, proto: %ld\n", proto));

    while (mouse_GetFromRing(data, &c))
    {
        D(bug("Mouse: handling events, c: %d\n", c));

        data->u.com.mouse_data[data->u.com.mouse_collected_bytes++] = c;

        D(bug("mouse_data: %d, colb: %d\n", data->u.com.mouse_data[data->u.com.mouse_collected_bytes],data->u.com.mouse_collected_bytes));

        if (data->u.com.mouse_collected_bytes == 3)
        {
            data->u.com.mouse_collected_bytes = 0;
            while (!(data->u.com.mouse_data[0] & 0x40))
            {
                data->u.com.mouse_data[0] = data->u.com.mouse_data[1];
                data->u.com.mouse_data[1] = data->u.com.mouse_data[2];

#if 0
                if (length)
                {
                    inbuf[2] = *data++;
                    length--;
                }
                else return 0;
#endif
            }

/*
    microsoft serial mouse protocol:

        D7      D6      D5      D4      D3      D2      D1      D0

1.      X       1       LB      RB      Y7      Y6      X7      X6
2.      X       0       X5      X4      X3      X2      X1      X0
3.      X       0       Y5      Y4      Y3      Y2      Y1      Y0

*/

            //mousedata = (struct mouse_data *)userdata;

            D(bug("event 1\n"));
            e->x = (char)(((data->u.com.mouse_data[0] & 0x03) << 6) | (data->u.com.mouse_data[1] & 0x3f));
            D(bug("subevent 1\n"));
            e->y = (char)(((data->u.com.mouse_data[0] & 0x0c) << 4) | (data->u.com.mouse_data[2] & 0x3f));
            D(bug("subevent 1\n"));
            if (e->x || e->y)
            {
                D(bug("subevent 2\n"));
                e->button = vHidd_Mouse_NoButton;
                D(bug("subevent 3\n"));
                e->type = vHidd_Mouse_Motion;

                D(bug("subevent 4\n"));
                data->mouse_callback(data->callbackdata, e);
            }
            D(bug("event 2\n"));

            buttonstate  = ((data->u.com.mouse_data[0] & 0x20) >> 5); /* left  button bit goes to bit 0 in button state */
            buttonstate |= ((data->u.com.mouse_data[0] & 0x10) >> 3); /* right button bit goes to bit 1 in button state */

            if((buttonstate & LEFT_BUTTON) != (data->buttonstate & LEFT_BUTTON))
            {
                e->button = vHidd_Mouse_Button1;
                e->type = (buttonstate & LEFT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                data->mouse_callback(data->callbackdata, e);
            }
            D(bug("event 3\n"));

            if((buttonstate & RIGHT_BUTTON) != (data->buttonstate & RIGHT_BUTTON))
            {
                e->button = vHidd_Mouse_Button2;
                e->type = (buttonstate & RIGHT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                data->mouse_callback(data->callbackdata, e);
            }

            D(bug("event 4\n"));

            data->buttonstate = buttonstate;
        }
    }
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
 * Clears ring buffer so it can get new data
 */
void mouse_FlushInput(struct mouse_data *data)
{
        data->u.com.rx->ptr = data->u.com.rx->top;
}

/*
 * Get one byte from ring buffer. Returns 0 if there is nothing to get
 */
int mouse_GetFromRing(struct mouse_data *data, char *c)
{
    struct Ring *r = data->u.com.rx;

    if (r->top != r->ptr)
    {
        *c = r->ring[r->ptr++];

        if (r->ptr >= RingSize) r->ptr = 0;

        return 1;
    }

    return 0;
}

/*
 * Select version for Ring handling.
 *
 * This functions waits for data present in Ring buffer for usec. Returns
 * non-zero if there is something in buffer, 0 otherwise.
 */
#warning: Incompatible with BOCHS busy loop! Change to precise timer.device!
int mouse_Select(struct mouse_data *data, ULONG usec)
{
    ULONG hz;
    int step;
    int latch;
    int avail = 0;
    struct Ring *r = data->u.com.rx;

    while (usec && !avail)
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
        do {
            avail = r->top - r->ptr;
        } while (((inb(0x61) & 0x20) == 0) && !avail);

        /* Decrease wait counter */
        usec -= step;
    }
    avail = r->top - r->ptr;
    return avail;
}

