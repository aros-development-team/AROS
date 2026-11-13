
#ifndef INTERFACE_PIO_H
#define INTERFACE_PIO_H

#include <asm/io.h>
#include <exec/types.h>
/*
 * Standard Amiga Gayle Definitions
 */
#define GAYLE_BASE_1200         0xda0000                        /* 0xda0000.W, 0xda0004.B, 0xda0008.B ...               */
#define GAYLE_IRQ_1200          0xda9000
#define GAYLE_INT_1200          0xdaa000

#define GAYLE_BASE_4000         0xdd2022                        /* 0xdd2020.W, 0xdd2026.B, 0xdd202a.B ... (argh!)       */
#define GAYLE_IRQ_4000          0xdd3020

#define GAYLE_IRQ_IDE           0x80
#define GAYLE_INT_IDE           0x80

/*
 * Elbox FastATA Gayle Extensions
 */
#define GAYLE_BASE_FASTATA      (GAYLE_BASE_1200 + 0x2000)
#define GAYLE_BASE_FASTATA_PIO0 (0x0)                           /* the different PIO modes are handled at different ..  */
#define GAYLE_BASE_FASTATA_PIO3 (0x10000)                       /* ..  register offsets - see below for register def's  */
#define GAYLE_BASE_FASTATA_PIO4 (0x14000)
#define GAYLE_BASE_FASTATA_PIO5 (0x12000)
#define GAYLE_IRQ_FASTATA       (GAYLE_IRQ_1200)

#define GAYLE_FASTATA_PORTSIZE  0x1000

#define GAYLE_FASTATA_PIO_DATA	0x0
#define GAYLE_FASTATA_PIO_ERR   0x4
#define GAYLE_FASTATA_PIO_NSECT 0x8
#define GAYLE_FASTATA_PIO_SECT  0xC
#define GAYLE_FASTATA_PIO_LOCYL	0x10
#define GAYLE_FASTATA_PIO_HICYL	0x14
#define GAYLE_FASTATA_PIO_SEL   0x18
#define GAYLE_FASTATA_PIO_STAT  0x1C

#define GAYLE_FASTATA_LONGDATA  0x0
#define GAYLE_FASTATA_ERR       0x200
#define GAYLE_FASTATA_DATA      0x208
#define GAYLE_FASTATA_NSECT     0x400
#define GAYLE_FASTATA_SECT      0x600
#define GAYLE_FASTATA_LOCYL     0x800
#define GAYLE_FASTATA_HICYL     0xA00
#define GAYLE_FASTATA_SEL       0xC00
#define GAYLE_FASTATA_STAT      0xE00

struct pio_data
{
    UBYTE *dataport;
    UBYTE *port;
    UBYTE *altport;
};

extern const APTR bus_FuncTable[];
extern const APTR pio_FuncTable[];

#endif /* !INTERFACE_PIO_H */
