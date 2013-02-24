#include <asm/io.h>
#include <exec/types.h>

struct pio_data
{
    port_t ioBase;
    port_t ioAlt;
};

extern const APTR bus_FuncTable[];
extern const APTR pio_FuncTable[];
