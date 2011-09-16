extern const unsigned char FullSize[];

unsigned char GetSize(struct GENERIC_ACPI_ADDR *reg);
IPTR ReadRegInt(struct GENERIC_ACPI_ADDR *reg, unsigned char size);
