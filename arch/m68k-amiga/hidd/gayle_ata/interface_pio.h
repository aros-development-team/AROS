#include <asm/io.h>
#include <exec/types.h>

#define GAYLE_BASE_4000 0xdd2022 /* 0xdd2020.W, 0xdd2026.B, 0xdd202a.B ... (argh!) */
#define GAYLE_IRQ_4000  0xdd3020

#define GAYLE_BASE_1200 0xda0000 /* 0xda0000.W, 0xda0004.B, 0xda0008.B ... */
#define GAYLE_IRQ_1200  0xda9000
#define GAYLE_INT_1200  0xdaa000

#define GAYLE_IRQ_IDE 0x80
#define GAYLE_INT_IDE 0x80

struct pio_data
{
    UBYTE *dataport;
    UBYTE *port;
    UBYTE *altport;
};

extern const APTR bus_FuncTable[];
extern const APTR pio_FuncTable[];
