/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: card_intern.h $

    Desc: Internal data structures for card.resource
    Lang: english
*/

#ifndef CARD_INTERN_H
#define CARD_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_INTERRUPTS_H
#include <exec/interrupts.h>
#endif
#ifndef RESOURCES_CARD_H
#include <resources/card.h>
#endif
#ifndef PROTO_CARDRES_H
#include <proto/cardres.h>
#endif

#include <aros/debug.h>

#define CARDDEBUG(x) x

#define ISMINE (handle == CardResource->ownedcard && !CardResource->removed)


#define CISTPL_NULL 0x00
#define CISTPL_DEVICE 0x01
#define CISTPL_LONGLINK_A 0x11
#define CISTPL_LONGLINK_C 0x12
#define CISTPL_LINKTARGET 0x13
#define CISTPL_NO_LINK 0x14
#define CISTPL_DEVICE_A 0x17
#define CISTPL_FORMAT 0x41
#define CISTPL_GEOMETRY 0x42
#ifndef CISTPL_AMIGAXIP
#define CISTPL_AMIGAXIP 0x91
#endif
#define CISTPL_END 0xff

#define DTYPE_SRAM 6
#define DTYPE_DRAM 7

/* Gayle hardware addresses */
#define GAYLE_BASE          0x00da8000

#define GAYLE_RAM           0x00600000
#define GAYLE_ATTRIBUTE     0x00a00000
#define GAYLE_IO            0x00a20000

#define GAYLE_RAMSIZE       0x00400000
#define GAYLE_ATTRIBUTESIZE 0x00020000
#define GAYLE_IOSIZE        0x00010000
#define GAYLE_IO_8BITODD    0x00a30000

#define GAYLE_RESET         0x00a40000

/* DA8000 GAYLE STATUS
 * Read current state of signals.
 * Bits 0 and 1 are normal read/write bits.
 */
#define GAYLE_CS_IDE	0x80	/* IDE int status */
#define GAYLE_CS_CCDET	0x40    /* credit card detect */
#define GAYLE_CS_BVD1	0x20    /* battery voltage detect 1 */
#define GAYLE_CS_SC	0x20    /* credit card status change */
#define GAYLE_CS_BVD2	0x10    /* battery voltage detect 2 */
#define GAYLE_CS_DA	0x10    /* digital audio */
#define GAYLE_CS_WR	0x08    /* write enable (1 == enabled) */
#define GAYLE_CS_BSY	0x04    /* credit card busy */
#define GAYLE_CS_IRQ	0x04    /* interrupt request */
#define GAYLE_CS_DAEN   0x02    /* enable digital audio */ 
#define GAYLE_CS_DIS    0x01    /* disable PCMCIA slot */ 

/* DA9000 GAYLE INTREQ
 * Bit is set if state changes.
 * Cleared by writing zero bit. Writing one does nothing.
 * Bits 0 and 1 are normal read/write bits.
 */
#define GAYLE_IRQ_IDE	    0x80
#define GAYLE_IRQ_CCDET	    0x40    /* credit card detect */
#define GAYLE_IRQ_BVD1	    0x20    /* battery voltage detect 1 */
#define GAYLE_IRQ_SC	    0x20    /* credit card status change */
#define GAYLE_IRQ_BVD2	    0x10    /* battery voltage detect 2 */
#define GAYLE_IRQ_DA	    0x10    /* digital audio */
#define GAYLE_IRQ_WR	    0x08    /* write enable (1 == enabled) */
#define GAYLE_IRQ_BSY	    0x04    /* credit card busy */
#define GAYLE_IRQ_IRQ	    0x04    /* interrupt request */
#define GAYLE_IRQ_RESET	    0x02    /* reset machine after CCDET change */ 
#define GAYLE_IRQ_BERR      0x01    /* generate bus error after CCDET change */ 

#define GAYLE_IRQ_IRQ_MASK (GAYLE_IRQ_IDE | GAYLE_IRQ_CCDET | GAYLE_IRQ_BVD1 | GAYLE_IRQ_BVD2 | GAYLE_IRQ_WR | GAYLE_IRQ_BSY)
/* Both RESET and BERR set = Reset inserted card */
#define GAYLE_IRQ_CARD_RESET_MASK (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR)

/* DAA000 GAYLE INTENA
 * Enable/disable interrupts and set interrupt level.
 * Normal read/write register.
 */
#define GAYLE_INT_IDE	    0x80    /* IDE interrupt enable */
#define GAYLE_INT_CCDET	    0x40    /* credit card detect change enable */
#define GAYLE_INT_BVD1	    0x20    /* battery voltage detect 1 change enable */
#define GAYLE_INT_SC	    0x20    /* credit card status change enable */
#define GAYLE_INT_BVD2	    0x10    /* battery voltage detect 2 change enable */
#define GAYLE_INT_DA	    0x10    /* digital audio change enable */
#define GAYLE_INT_WR	    0x08    /* write enable change enabled */
#define GAYLE_INT_BSY	    0x04    /* credit card busy */
#define GAYLE_INT_IRQ	    0x04    /* credit card interrupt request */
#define GAYLE_INT_BVD_LEV   0x02    /* BVD int level, 0=lev2,1=lev6 */ 
#define GAYLE_INT_BSY_LEV   0x01    /* BSY int level, 0=lev2,1=lev6 */ 

/* 0xDAB000 GAYLE_CONFIG
 * Speed and voltage configuration.
 * Read/write.
 */
#define GAYLE_CFG_0V            0x00
#define GAYLE_CFG_5V            0x01
#define GAYLE_CFG_12V           0x02
#define GAYLE_CFG_100NS         0x08
#define GAYLE_CFG_150NS         0x04
#define GAYLE_CFG_250NS         0x00
#define GAYLE_CFG_720NS         0x0c
 
#define GAYLE_CFG_VOLTAGE       0x03
#define GAYLE_CFG_SPEED         0x0c

struct GayleIO
{
    UBYTE status;	/* 0xda8000 */
    UBYTE pad1[0xfff];
    UBYTE intreq;	/* 0xda9000 */
    UBYTE pad2[0xfff];
    UBYTE intena;	/* 0xdaa000 */
    UBYTE pad3[0xfff];
    UBYTE config;	/* 0xdab000 */
};

struct CardResource
{
    struct Library crb_LibNode;
    struct Interrupt level2;
    struct Interrupt level6;
    struct List handles;
    struct CardHandle *ownedcard;
    struct timerequest *timerio;
    struct MsgPort *timermp;
    BOOL removed;
    BOOL disabled;
    ULONG changecount;
    struct Task *task;
    ULONG signalmask;
    UBYTE resetberr;
};

void pcmcia_reset(struct CardResource*);
void pcmcia_disable(void);
void pcmcia_enable(void);
void pcmcia_cardreset(struct CardResource*);
void pcmcia_newowner(struct CardResource*,BOOL);
void pcmcia_removeowner(struct CardResource*);
void pcmcia_enable_interrupts(void);
void pcmcia_clear_requests(struct CardResource*);
BOOL pcmcia_havecard(void);

#define CARDF_USED 0x80

#endif //CARD_INTERN_H
