
#ifndef _AMIGACHIPSETBITMAP_H
#define _AMIGACHIPSETBITMAP_H

#include <hidd/graphics.h>

void resetmode(struct amigavideo_staticdata *data);
BOOL setmode(struct amigavideo_staticdata *data, struct amigabm_data*);
BOOL setbitmap(struct amigavideo_staticdata *data, struct amigabm_data*);
void initcustom(struct amigavideo_staticdata *data);

void setspritepos(struct amigavideo_staticdata *data, WORD x, WORD y);
BOOL setsprite(struct amigavideo_staticdata *data, WORD w, WORD h, struct pHidd_Gfx_SetCursorShape *shape);
void resetsprite(struct amigavideo_staticdata *data);
void setspritevisible(struct amigavideo_staticdata *data, BOOL visible);

BOOL setcolors(struct amigavideo_staticdata *data, struct pHidd_BitMap_SetColors *msg, BOOL visible);
void setscroll(struct amigavideo_staticdata *data, struct amigabm_data*);
#endif
