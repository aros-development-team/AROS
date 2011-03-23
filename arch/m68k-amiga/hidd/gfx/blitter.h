
#ifndef _AMIGABLITTER_H
#define _AMIGABLITTER_H

#define USE_BLITTER 1

BOOL blit_fillrect(struct amigavideo_staticdata*, struct amigabm_data* ,WORD,WORD,WORD,WORD,HIDDT_Pixel,HIDDT_DrawMode);
BOOL blit_copybox(struct amigavideo_staticdata*, struct amigabm_data*, struct amigabm_data*, WORD, WORD, WORD, WORD, WORD, WORD, HIDDT_DrawMode);
BOOL blit_puttemplate(struct amigavideo_staticdata*, struct amigabm_data*, struct pHidd_BitMap_PutTemplate*);
#endif
