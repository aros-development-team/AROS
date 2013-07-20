#include <asm/io.h>
#include <resources/acpi.h>

#include "gas_io.h"

/*****************************************************************************

    NAME */
#include <proto/acpi.h>

	AROS_LH1I(IPTR, ACPI_ReadReg,

/*  SYNOPSIS */
	AROS_LHA(struct GENERIC_ACPI_ADDR *, reg, A0),

/*  LOCATION */
	struct ACPIBase *, ACPIBase, 3, Acpi)

/*  FUNCTION
	Read a value from the register specified by GAS

    INPUTS
	reg - a pointer to a generic address structure

    RESULT
	A value read from the register

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
    IPTR val = ReadRegInt(reg, size);

    if (reg->register_bit_width != FullSize[reg->size])
    {
    	/*
    	 * Partial bit field.
    	 * Mask out the needed bits and shift to the right.
    	 */
    	unsigned long mask = (1 << reg->register_bit_width) - 1;

	val = (val >> reg->register_bit_offset) & mask;
    }

    return val;

    AROS_LIBFUNC_EXIT
}

const unsigned char FullSize[] = {0, 8, 16, 32, 64};

unsigned char GetSize(struct GENERIC_ACPI_ADDR *reg)
{
    unsigned char size = reg->size;

    if (size == ACPI_SIZE_UNDEFINED)
    {
	/* Need to compute register size */
        unsigned char bits = reg->register_bit_offset + reg->register_bit_width;

	if (bits <= 8)
	    size = ACPI_SIZE_BYTE;
	else if (bits <= 16)
	    size = ACPI_SIZE_WORD;
	else if (bits <= 32)
	    size = ACPI_SIZE_DWORD;
	else
	    size = ACPI_SIZE_QUAD;
    }
    return size;
}

IPTR ReadRegInt(struct GENERIC_ACPI_ADDR *reg, unsigned char size)
{
    IPTR val = 0;

    switch (reg->address_space_id)
    {
    case ACPI_SPACE_MEM:
    	switch (size)
    	{
    	case ACPI_SIZE_BYTE:
    	    val = *((UBYTE *)(IPTR)reg->address);
    	    break;

    	case ACPI_SIZE_WORD:
    	    val = *((UWORD *)(IPTR)reg->address);
    	    break;

    	case ACPI_SIZE_DWORD:
    	    val = *((ULONG *)(IPTR)reg->address);
    	    break;

#if __WORDSIZE == 64
	case ACPI_SIZE_QUAD:
	    val = *((UQUAD *)(IPTR)reg->address);
	    break;
#endif
    	}
    	break;
    
    case ACPI_SPACE_IO:
    	switch (size)
    	{
    	case ACPI_SIZE_BYTE:
    	    val = inb(reg->address);
    	    break;

    	case ACPI_SIZE_WORD:
    	    val = inw(reg->address);
    	    break;

    	case ACPI_SIZE_DWORD:
    	    val = inl(reg->address);
    	    break;

    	/* ACPI_SIZE_QUAD - not supported ? */
    	}
	break;
    }
    
    return val;
}
