#include <cybergraphx/cybergraphics.h>
#include <aros/symbolsets.h>

ADD2LIBS("cybergraphics.library", 39, LIBSET_CYBERGRAPHICS_PRI, struct Library *, CyberGfxBase, NULL, NULL);
