#include <intuition/intuitionbase.h>
#include <aros/symbolsets.h>

ADD2LIBS("intuition.library", 39, LIBSET_INTUITION_PRI, struct IntuitionBase *, IntuitionBase, NULL, NULL);
