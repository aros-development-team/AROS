
#ifndef _AMIGABLITTER_H
#define _AMIGABLITTER_H

#define USE_BLITTER 1

BOOL blit_fillrect(struct amigavideo_staticdata*, struct BitMap* ,WORD,WORD,WORD,WORD,HIDDT_Pixel,HIDDT_DrawMode);
BOOL blit_copybox(struct amigavideo_staticdata*, struct BitMap*, struct BitMap*, WORD, WORD, WORD, WORD, WORD, WORD, HIDDT_DrawMode);
BOOL blit_copybox_mask(struct amigavideo_staticdata*, struct BitMap*, struct BitMap*, WORD, WORD, WORD, WORD, WORD, WORD, HIDDT_DrawMode, APTR);
BOOL blit_puttemplate(struct amigavideo_staticdata*, struct BitMap*, struct pHidd_BitMap_PutTemplate*);
BOOL blit_putpattern(struct amigavideo_staticdata*, struct BitMap*, struct pHidd_BitMap_PutPattern*);

#endif
