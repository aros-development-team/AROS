
#include <exec/types.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>

void STDARGS myGT_SetGadgetAttrs (struct Gadget *, struct Window *, struct Requester *, Tag,...);
struct Gadget * STDARGS myCreateGadget (ULONG, struct Gadget *, struct NewGadget *, Tag,...);
void STDARGS myDrawBevelBox (struct RastPort *, long, long, long, long, Tag,...);
