#include <asm/io.h>
#include <resources/acpi.h>

#include "gas_io.h"

/*****************************************************************************

    NAME */
#include <proto/acpi.h>

AROS_LH2I(void, ACPI_WriteReg,

/*  SYNOPSIS */
	AROS_LHA(struct GENERIC_ACPI_ADDR *, reg, A0),
	AROS_LHA(IPTR, value, D0),

/*  LOCATION */
	struct ACPIBase *, ACPIBase, 4, Acpi)

/*  FUNCTION
	Write a value from the register specified by GAS

    INPUTS
	reg   - a pointer to a generic address structure
	value - a value to write to the register

    RESULT
	None

    NOTES
    	Only memory and I/O spaces are supported.
	64-bit registers are supported only in memory space and only on 64-bit platforms.
	The given GAS must address a complete register, not a data structure.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    unsigned char size = GetSize(reg);

    if (reg->register_bit_width != FullSize[reg->size])
    {
    	/*
    	 * Partial bit field.
    	 * Read original value from the register and replace the needed bits.
    	 */
    	unsigned long mask = ((1 << reg->register_bit_width) - 1) << reg->register_bit_offset;
    	unsigned long old = ReadRegInt(reg, size);

	value = (value << reg->register_bit_offset) & mask;
	value |= (old & ~mask);
    }

    switch (reg->address_space_id)
    {
    case ACPI_SPACE_MEM:
    	switch (size)
    	{
    	case ACPI_SIZE_BYTE:
    	    *((UBYTE *)(IPTR)reg->address) = value;
    	    break;

    	case ACPI_SIZE_WORD:
    	    *((UWORD *)(IPTR)reg->address) = value;
    	    break;

    	case ACPI_SIZE_DWORD:
    	    *((ULONG *)(IPTR)reg->address) = value;
    	    break;

#if __WORDSIZE == 64
	case ACPI_SIZE_QUAD:
	    *((UQUAD *)(IPTR)reg->address) = value;
	    break;
#endif
    	}
    	break;

    case ACPI_SPACE_IO:
    	switch (size)
    	{
    	case ACPI_SIZE_BYTE:
    	    outb(value, reg->address);
    	    break;

    	case ACPI_SIZE_WORD:
    	    outw(value, reg->address);
    	    break;

    	case ACPI_SIZE_DWORD:
    	    outl(value, reg->address);
    	    break;

    	/* ACPI_SIZE_QUAD - not supported ? */
    	}
	break;
    }

    AROS_LIBFUNC_EXIT
}
