/*
    Copyright  2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_DATATYPESCACHE_H
#define _MUI_DATATYPESCACHE_H

/* struct dt_node; */

/*void dt_init(void);*/
/*void dt_cleanup(void);*/
struct dt_node *dt_load_picture(CONST_STRPTR filename, struct Screen *scr);
void dt_dispose_picture(struct dt_node *node);

int dt_width(struct dt_node *node);
int dt_height(struct dt_node *node);
void dt_put_on_rastport(struct dt_node *node, struct RastPort *rp, int x, int y);
void dt_put_on_rastport_tiled(struct dt_node *node, struct RastPort *rp, int x1, int y1, int x2, int y2, int xoffset, int yoffset);
void dt_put_on_rastport_quicktiled(struct RastPort *rp, struct dt_node *node, UWORD x, UWORD y, UWORD w, UWORD h);

#endif
