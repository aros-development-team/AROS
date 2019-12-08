
#ifndef _AMIGACHIPSETBITMAP_H
#define _AMIGACHIPSETBITMAP_H

#include <hidd/gfx.h>

/* Use nominal screen height. Overscan is not supported yet. */
static inline WORD limitheight(struct amigavideo_staticdata *csd, WORD y, BOOL lace, BOOL maxlimit)
{
    if (lace)
        y /= 2;
    if (csd->palmode) {
        if (maxlimit && y > 311)
            y = 311;
        else if (!maxlimit && y > 256)
            y = 256;
    } else {
        if (maxlimit && y > 261)
            y = 261;
        else if (!maxlimit && y > 200)
            y = 200;
    }
    if (lace)
        y *= 2;
    return y;
}

static inline VOID setpalntsc(struct amigavideo_staticdata *csd)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    if (!csd->ecs_agnus)
        return;

    custom->beamcon0 = (csd->palmode) ? 0x0020 : 0x0000;
}

VOID resetmode(struct amigavideo_staticdata *);
BOOL setmode(struct amigavideo_staticdata *, struct amigabm_data*);
BOOL setbitmap(struct amigavideo_staticdata *, struct amigabm_data*);
VOID initcustom(struct amigavideo_staticdata *);

VOID setfmode(struct amigavideo_staticdata *, struct amigabm_data *);
VOID setspritepos(struct amigavideo_staticdata *, WORD, WORD, UBYTE, BOOL);
BOOL setsprite(OOP_Class *, OOP_Object *, WORD, WORD, struct pHidd_Gfx_SetCursorShape *);
VOID resetsprite(struct amigavideo_staticdata *);
VOID setspritevisible(struct amigavideo_staticdata *, BOOL);

BOOL setcolors(struct amigavideo_staticdata *, struct amigabm_data *, struct pHidd_BitMap_SetColors *);
VOID setscroll(struct amigavideo_staticdata *, struct amigabm_data*);

UWORD get_copper_list_length(struct amigavideo_staticdata *, UBYTE);
VOID setcopperlisttail(struct amigavideo_staticdata *, UWORD *, UWORD *, BOOL);
UWORD *populatebmcopperlist(struct amigavideo_staticdata *, struct amigabm_data *, struct copper2data *, UWORD *, BOOL);
VOID updatebmbplcon(struct amigavideo_staticdata *, struct amigabm_data *, struct copper2data *);
VOID setcopperscroll(struct amigavideo_staticdata *, struct amigabm_data *, BOOL);
VOID setcoppercolors(struct amigavideo_staticdata *, struct amigabm_data *, UBYTE *);

#endif
