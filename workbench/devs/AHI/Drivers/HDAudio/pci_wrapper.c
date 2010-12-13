#include <exec/types.h>

#if defined(__AROS__)
#include "pci_aros.c"
#elif defined(__AMIGAOS4__)
#include "pci_aos4.c"
#else
#include "pci_openpci.c"
#endif

