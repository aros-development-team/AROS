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

struct	  NewImage
{
    UWORD   w;
    UWORD   h;
    ULONG  *data;
    Object *o;
    struct  BitMap *bitmap;
    APTR    mask;
};

#define MODE_DEFAULT    0
#define MODE_PROP       1

struct dt_frame_image
{
    Object *o;
    struct NewImage *img_up;
    struct NewImage *img_down;
    short  tile_left;
    short  tile_top;
    short  tile_right;
    short  tile_bottom;
    short  inner_left;
    short  inner_top;
    short  inner_right;
    short  inner_bottom;
    BOOL   noalpha;
};

struct dt_node
{
    struct MinNode node;
    char *filename;
    Object *o;
    int width, height;
    struct Screen *scr;
    int count;
    struct BackFillInfo *bfi;
    UBYTE mask;
    UWORD mode;
    struct NewImage *img_verticalcontainer;
    struct NewImage *img_verticalknob;
    struct NewImage *img_horizontalcontainer;
    struct NewImage *img_horizontalknob;
    struct NewImage *img_up;
    struct NewImage *img_down;
    struct NewImage *img_left;
    struct NewImage *img_right;

    int              ContainerTop_o, ContainerTop_s;
    int              ContainerVertTile_o, ContainerVertTile_s;
    int              ContainerBottom_o, ContainerBottom_s;
    int              KnobTop_o, KnobTop_s;
    int              KnobTileTop_o, KnobTileTop_s;
    int              KnobVertGripper_o, KnobVertGripper_s;
    int              KnobTileBottom_o, KnobTileBottom_s;
    int              KnobBottom_o, KnobBottom_s;
    int              ContainerLeft_o, ContainerLeft_s;
    int              ContainerHorTile_o, ContainerHorTile_s;
    int              ContainerRight_o, ContainerRight_s;
    int              KnobLeft_o, KnobLeft_s;
    int              KnobTileLeft_o, KnobTileLeft_s;
    int              KnobHorGripper_o, KnobHorGripper_s;
    int              KnobTileRight_o, KnobTileRight_s;
    int              KnobRight_o, KnobRight_s;

};

void DisposeImageContainer(struct NewImage *ni);
struct NewImage *NewImageContainer(UWORD w, UWORD h);

struct dt_node *dt_load_picture(CONST_STRPTR filename, struct Screen *scr);
void dt_dispose_picture(struct dt_node *node);

int dt_width(struct dt_node *node);
int dt_height(struct dt_node *node);
void dt_put_on_rastport(struct dt_node *node, struct RastPort *rp, int x, int y);
void dt_put_mim_on_rastport(struct dt_node *node, struct RastPort *rp, int x, int y, int state);

void dt_put_on_rastport_tiled(struct dt_node *node, struct RastPort *rp, int x1, int y1, int x2, int y2, int xoffset, int yoffset);
void dt_put_on_rastport_quicktiled(struct RastPort *rp, struct dt_node *node, UWORD x, UWORD y, UWORD w, UWORD h);
struct dt_frame_image * load_custom_frame(CONST_STRPTR filename, struct Screen *scr);
void dispose_custom_frame(struct dt_frame_image * fi);

#if AROS_BIG_ENDIAN
#define GET_A(rgb) ((rgb >> 24) & 0xff)
#define GET_R(rgb) ((rgb >> 16) & 0xff)
#define GET_G(rgb) ((rgb >> 8) & 0xff)
#define GET_B(rgb) (rgb & 0xff)
#define SET_ARGB(a, r, g, b) a << 24 | r << 16 | g << 8 | b
#else
#define GET_A(rgb) (rgb & 0xff)
#define GET_R(rgb) ((rgb >> 8) & 0xff)
#define GET_G(rgb) ((rgb >> 16) & 0xff)
#define GET_B(rgb) ((rgb >> 24) & 0xff)
#define SET_ARGB(a, r, g, b) b << 24 | g << 16 | r << 8 | a
#endif

#endif
