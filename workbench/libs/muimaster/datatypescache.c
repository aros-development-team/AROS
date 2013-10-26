/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.

    $Id$
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DT_V44_SUPPORT

#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>

//#define MYDEBUG 1 
#include "debug.h"
#include "support.h"
#include "muimaster_intern.h"
#include "datatypescache.h"

extern struct Library *MUIMasterBase;

static struct List dt_list;
static int dt_initialized;


struct LayerHookMsg
{
    STACKED struct Layer *layer;
    STACKED struct Rectangle bounds;
    STACKED LONG offsetx;
    STACKED LONG offsety;
};

struct BltMaskHook
{
    struct Hook hook;
    struct BitMap maskBitMap;
    struct BitMap *srcBitMap;
    LONG srcx, srcy;
    LONG destx, desty;
};

#define RECTSIZEX(r) ((r)->MaxX-(r)->MinX+1)
#define RECTSIZEY(r) ((r)->MaxY-(r)->MinY+1)

#define MOD(x,y) ((x)<0 ? (y)-((-(x))%(y)) : (x)%(y))

struct BackFillMsg
{
    struct Layer *Layer;
    struct Rectangle Bounds;
    LONG OffsetX;
    LONG OffsetY;
};

struct BackFillOptions
{
    WORD MaxCopyWidth;   // maximum width for the copy
    WORD MaxCopyHeight;  // maximum height for the copy
//      BOOL CenterX;    // center the tiles horizontally?
//      BOOL CenterY;    // center the tiles vertically?
    WORD OffsetX;      // offset to add
    WORD OffsetY;      // offset to add
    BOOL OffsetTitleY; // add the screen titlebar height to the vertical offset?
};

struct BackFillInfo
{
    struct Hook Hook;
    WORD Width;
    WORD Height;
    struct BitMap *BitMap;
    /*  struct Screen *Screen; */ /* Needed for centering */
    WORD CopyWidth;
    WORD CopyHeight;
    struct BackFillOptions Options;
    WORD OffsetX;
    WORD OffsetY;
};

#ifndef __AROS__

/* A BltBitMaskPort() replacement which blits masks for interleaved bitmaps
   correctly */
#ifndef _DCC
VOID MyBltMaskBitMap(CONST struct BitMap *srcBitMap, LONG xSrc, LONG ySrc,
    struct BitMap *destBitMap, LONG xDest, LONG yDest, LONG xSize,
    LONG ySize, struct BitMap *maskBitMap)
{
    BltBitMap(srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize,
        0x99, ~0, NULL);
    BltBitMap(maskBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize,
        ySize, 0xe2, ~0, NULL);
    BltBitMap(srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize,
        0x99, ~0, NULL);
}
#endif

ASM void HookFunc_BltMask(REG(a0, struct Hook *hook), REG(a1,
        struct LayerHookMsg *msg), REG(a2, struct RastPort *rp))
{
    struct BltMaskHook *h = (struct BltMaskHook *)hook;

    LONG width = msg->bounds.MaxX - msg->bounds.MinX + 1;
    LONG height = msg->bounds.MaxY - msg->bounds.MinY + 1;
    LONG offsetx = h->srcx + msg->offsetx - h->destx;
    LONG offsety = h->srcy + msg->offsety - h->desty;

#ifdef __SASC
    putreg(REG_A4, (long)hook->h_Data);
#endif

    MyBltMaskBitMap(h->srcBitMap, offsetx, offsety, rp->BitMap,
        msg->bounds.MinX, msg->bounds.MinY, width, height, &h->maskBitMap);
}

VOID MyBltMaskBitMapRastPort(struct BitMap *srcBitMap, LONG xSrc, LONG ySrc,
    struct RastPort *destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize,
    ULONG minterm, APTR bltMask)
{
    if (GetBitMapAttr(srcBitMap, BMA_FLAGS) & BMF_INTERLEAVED)
    {
        LONG src_depth = GetBitMapAttr(srcBitMap, BMA_DEPTH);
        struct Rectangle rect;
        struct BltMaskHook hook;

        /* Define the destination rectangle in the rastport */
        rect.MinX = xDest;
        rect.MinY = yDest;
        rect.MaxX = xDest + xSize - 1;
        rect.MaxY = yDest + ySize - 1;

        /* Initialize the hook */
        hook.hook.h_Entry = (HOOKFUNC) HookFunc_BltMask;
#ifdef __SASC
        hook.hook.h_Data = (void *)getreg(REG_A4);
#endif
        hook.srcBitMap = srcBitMap;
        hook.srcx = xSrc;
        hook.srcy = ySrc;
        hook.destx = xDest;
        hook.desty = yDest;

        /* Initialize a bitmap where all plane pointers points to the mask */
        InitBitMap(&hook.maskBitMap, src_depth, GetBitMapAttr(srcBitMap,
                BMA_WIDTH), GetBitMapAttr(srcBitMap, BMA_HEIGHT));
        while (src_depth)
            hook.maskBitMap.Planes[--src_depth] = bltMask;

        /* Blit onto the Rastport */
        DoHookClipRects(&hook.hook, destRP, &rect);
    }
    else
    {
        BltMaskBitMapRastPort(srcBitMap, xSrc, ySrc, destRP, xDest, yDest,
            xSize, ySize, minterm, bltMask);
    }
}

#endif


static Object *LoadPicture(CONST_STRPTR filename, struct Screen *scr)
{
    Object *o;

    struct Process *myproc = (struct Process *)FindTask(NULL);
    APTR oldwindowptr = myproc->pr_WindowPtr;
    myproc->pr_WindowPtr = (APTR) - 1;

    o = NewDTObject((APTR) filename,
        DTA_GroupID, GID_PICTURE,
        OBP_Precision, PRECISION_EXACT,
        PDTA_Screen, (IPTR) scr,
        PDTA_DestMode, PMODE_V43, PDTA_UseFriendBitMap, TRUE, TAG_DONE);

    myproc->pr_WindowPtr = oldwindowptr;
    D(bug("... picture=%lx\n", o));

    if (o)
    {
        struct BitMapHeader *bmhd;

        GetDTAttrs(o, PDTA_BitMapHeader, (IPTR) & bmhd, TAG_DONE);
        if (bmhd->bmh_Masking == mskHasAlpha)
        {
            if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) >= 15)
            {
                SetAttrs(o, PDTA_FreeSourceBitMap, FALSE,
                    PDTA_Remap, FALSE, TAG_DONE);
            }
        }

        struct FrameInfo fri = { 0 };

        D(bug("DTM_FRAMEBOX\n", o));
        DoMethod(o, DTM_FRAMEBOX, NULL, (IPTR) & fri, (IPTR) & fri,
            sizeof(struct FrameInfo), 0);

        if (fri.fri_Dimensions.Depth > 0)
        {
            D(bug("DTM_PROCLAYOUT\n", o));
            if (DoMethod(o, DTM_PROCLAYOUT, NULL, 1))
            {
                return o;
            }
        }
        DisposeDTObject(o);
    }
    return NULL;
}

/*
void dt_init(void)
{
}

void dt_cleanup(void)
{
}
*/

char *allocPath(const char *str)
{
    const char *s0, *s1;
    char *s;
    int l;

    s = NULL;
    s0 = str;

    s1 = PathPart(str);
    if (s1)
    {
        for (l = 0; s0 != s1; s0++, l++);
        s = AllocVec(l + 1, MEMF_CLEAR);
        if (s)
            strncpy(s, str, l);
    }
    return s;
}

void freeString(char *str)
{
    if (str)
        FreeVec(str);
}

char *SkipChars(char *v)
{
    char *c;

    c = strstr(v, "=");
    return ++c;
}

int GetInt(char *v)
{
    char *c;
    c = SkipChars(v);
    return atol(c);
}

BOOL GetBool(char *v, char *id)
{
    if (strstr(v, id))
        return TRUE;
    else
        return FALSE;
}

void GetIntegers(char *v, int *v1, int *v2)
{
    char *c;
    char va1[32], va2[32];
    int cnt;
    c = SkipChars(v);
    if (c)
    {
        cnt = sscanf(c, "%s %s", va1, va2);
        if (cnt == 1)
        {
            *v1 = -1;
            *v2 = atol(va1);
        }
        else if (cnt == 2)
        {
            *v1 = atol(va1);
            *v2 = atol(va2);
        }
    }
}

/* Function:   Create a new Image with the specified dimensions
 * Input:      UWORD   w, h:
    *                          width and height ofthe wished Image
 * Output:     NewImage:
    *                          Pointer to the Created image or NULL
    * Bugs:	   Not known yet
    * NOTES:      Function will only return non-NULL if all allocations could be done
    *             so you have not to check something inside the NewImage structure
*/
struct NewImage *NewImageContainer(UWORD w, UWORD h)
{

    struct NewImage *ni;

    ni = AllocVec(sizeof(struct NewImage), MEMF_ANY | MEMF_CLEAR);
    if (ni)
    {
        ni->w = w;
        ni->h = h;
        ni->data = AllocVec(w * h * 4, MEMF_ANY | MEMF_CLEAR);
        if (ni->data == NULL)
        {
            FreeVec(ni);
            ni = NULL;
        }
    }
    return ni;
}

/* Function:   Remove all Memory used by an Image
 * Input:      NewImage ni:
    *                          Pointer to an Image to be deallocated
    * Bugs:	   Not known
*/
void DisposeImageContainer(struct NewImage *ni)
{

    if (ni)
    {
        if (ni->data)
        {
            FreeVec(ni->data);
        }
        if (ni->o != NULL)
            DisposeDTObject(ni->o);
        FreeVec(ni);
    }
}

/* Function:   Load an Image from a file
 * Input:      char   name:
    *                          Filename of the Image to load
 * Output:     NewImage:
    *                          Pointer to the Created image or NULL
    * Bugs:	   Not known yet
    * NOTES:      Function will only return non-NULL if all allocations could be done
    *             so you have not to check something inside the NewImage struct.
    *             This function uses DataTypes for loading images, so be sure to have
    *             the specific DataTypes installed
*/
struct NewImage *GetImageFromFile(char *name, struct Screen *scr)
{

    struct BitMapHeader *bmhd = NULL;
    struct NewImage *ni;

    struct pdtBlitPixelArray pa;
    Object *pic;
    UWORD w, h;
    ULONG a, depth;
    UBYTE mask;
    ni = NULL;

    pic = NewDTObject(name, DTA_SourceType, DTST_FILE,
        DTA_GroupID, GID_PICTURE,
        PDTA_Remap, FALSE, PDTA_DestMode, PMODE_V43, TAG_DONE);
    if (pic)
    {
        get(pic, PDTA_BitMapHeader, &bmhd);
        if (bmhd)
        {
            w = bmhd->bmh_Width;
            h = bmhd->bmh_Height;
            mask = bmhd->bmh_Masking;
            ni = NewImageContainer(w, h);
            if (ni)
            {
                pa.MethodID = PDTM_READPIXELARRAY;
                pa.pbpa_PixelData = (APTR) ni->data;
                pa.pbpa_PixelFormat = PBPAFMT_ARGB;
                pa.pbpa_PixelArrayMod = w * 4;
                pa.pbpa_Left = 0;
                pa.pbpa_Top = 0;
                pa.pbpa_Width = w;
                pa.pbpa_Height = h;
                DoMethodA(pic, (Msg) & pa);
                if (mask != mskHasAlpha)
                {
#if !AROS_BIG_ENDIAN
                    for (a = 0; a < (w * h); a++)
                        ni->data[a] |= 0x000000ff;
#else
                    for (a = 0; a < (w * h); a++)
                        ni->data[a] |= 0xff000000;
#endif
                }
                if (scr != NULL)
                {
                    depth = (ULONG) GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH);

                    if (depth < 15)
                        ni->o = LoadPicture(name, scr);
                    if (ni->o != NULL)
                    {
                        GetDTAttrs(ni->o, PDTA_DestBitMap,
                            (IPTR) & ni->bitmap, TAG_DONE);
                        if (ni->bitmap == NULL)
                            GetDTAttrs(ni->o, PDTA_BitMap,
                                (IPTR) & ni->bitmap, TAG_DONE);

                        if (ni->bitmap)
                            GetDTAttrs(ni->o, PDTA_MaskPlane,
                                (IPTR) & ni->mask, TAG_DONE);
                    }
                }
            }
        }
        DisposeDTObject(pic);
    }
    return ni;


}

BOOL ReadPropConfig(struct dt_node * data, struct Screen * scr)
{

    char buffer[256];
    char *line, *v;
    BPTR file;

    file = Open(data->filename, MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "ContainerTop ")) == line)
                {
                    GetIntegers(v, &data->ContainerTop_o,
                        &data->ContainerTop_s);
                }
                else if ((v = strstr(line, "ContainerVertTile ")) == line)
                {
                    GetIntegers(v, &data->ContainerVertTile_o,
                        &data->ContainerVertTile_s);
                }
                else if ((v = strstr(line, "KnobTop ")) == line)
                {
                    GetIntegers(v, &data->KnobTop_o, &data->KnobTop_s);
                }
                else if ((v = strstr(line, "KnobTileTop ")) == line)
                {
                    GetIntegers(v, &data->KnobTileTop_o,
                        &data->KnobTileTop_s);
                }
                else if ((v = strstr(line, "KnobVertGripper ")) == line)
                {
                    GetIntegers(v, &data->KnobVertGripper_o,
                        &data->KnobVertGripper_s);
                }
                else if ((v = strstr(line, "KnobTileBottom ")) == line)
                {
                    GetIntegers(v, &data->KnobTileBottom_o,
                        &data->KnobTileBottom_s);
                }
                else if ((v = strstr(line, "KnobBottom ")) == line)
                {
                    GetIntegers(v, &data->KnobBottom_o,
                        &data->KnobBottom_s);
                }
                else if ((v = strstr(line, "ContainerBottom ")) == line)
                {
                    GetIntegers(v, &data->ContainerBottom_o,
                        &data->ContainerBottom_s);
                }
                else if ((v = strstr(line, "ContainerLeft ")) == line)
                {
                    GetIntegers(v, &data->ContainerLeft_o,
                        &data->ContainerLeft_s);
                }
                else if ((v = strstr(line, "ContainerHorTile ")) == line)
                {
                    GetIntegers(v, &data->ContainerHorTile_o,
                        &data->ContainerHorTile_s);
                }
                else if ((v = strstr(line, "KnobLeft ")) == line)
                {
                    GetIntegers(v, &data->KnobLeft_o, &data->KnobLeft_s);
                }
                else if ((v = strstr(line, "KnobTileLeft ")) == line)
                {
                    GetIntegers(v, &data->KnobTileLeft_o,
                        &data->KnobTileLeft_s);
                }
                else if ((v = strstr(line, "KnobHorGripper ")) == line)
                {
                    GetIntegers(v, &data->KnobHorGripper_o,
                        &data->KnobHorGripper_s);
                }
                else if ((v = strstr(line, "KnobTileRight ")) == line)
                {
                    GetIntegers(v, &data->KnobTileRight_o,
                        &data->KnobTileRight_s);
                }
                else if ((v = strstr(line, "KnobRight ")) == line)
                {
                    GetIntegers(v, &data->KnobRight_o, &data->KnobRight_s);
                }
                else if ((v = strstr(line, "ContainerRight ")) == line)
                {
                    GetIntegers(v, &data->ContainerRight_o,
                        &data->ContainerRight_s);
                }
            }
        }
        while (line);
        Close(file);
    }
    STRPTR path = allocPath(data->filename);
    if (path)
    {
        BPTR lock = Lock(path, ACCESS_READ);
        if (lock)
        {
            BPTR oldcd = CurrentDir(lock);
            data->img_verticalcontainer =
                GetImageFromFile("Container/Vertical", scr);
            data->img_verticalknob = GetImageFromFile("Knob/Vertical", scr);
            data->img_horizontalcontainer =
                GetImageFromFile("Container/Horizontal", scr);
            data->img_horizontalknob =
                GetImageFromFile("Knob/Horizontal", scr);
            data->img_up = GetImageFromFile("ArrowUp/default", scr);
            data->img_down = GetImageFromFile("ArrowDown/default", scr);
            data->img_left = GetImageFromFile("ArrowLeft/default", scr);
            data->img_right = GetImageFromFile("ArrowRight/default", scr);

            CurrentDir(oldcd);
            UnLock(lock);

        }
        freeString(path);
    }

    if (data->img_horizontalcontainer && data->img_horizontalknob
        && data->img_verticalcontainer && data->img_verticalknob
        && data->img_up && data->img_down && data->img_left
        && data->img_right)
        return TRUE;
    return FALSE;
}

void FreePropConfig(struct dt_node *data)
{
    DisposeImageContainer(data->img_verticalcontainer);
    DisposeImageContainer(data->img_verticalknob);
    DisposeImageContainer(data->img_horizontalcontainer);
    DisposeImageContainer(data->img_horizontalknob);
    DisposeImageContainer(data->img_up);
    DisposeImageContainer(data->img_down);
    DisposeImageContainer(data->img_left);
    DisposeImageContainer(data->img_right);

}

BOOL ReadFrameConfig(CONST_STRPTR filename, struct dt_frame_image *fi,
    struct Screen *scr)
{

    char buffer[256];
    char *line, *v;
    BPTR file;

    fi->noalpha = FALSE;

    file = Open(filename, MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "TileLeft ")) == line)
                {
                    fi->tile_left = GetInt(v);
                }
                else if ((v = strstr(line, "TileTop ")) == line)
                {
                    fi->tile_top = GetInt(v);
                }
                else if ((v = strstr(line, "TileRight ")) == line)
                {
                    fi->tile_right = GetInt(v);
                }
                else if ((v = strstr(line, "TileBottom ")) == line)
                {
                    fi->tile_bottom = GetInt(v);
                }
                else if ((v = strstr(line, "InnerLeft ")) == line)
                {
                    fi->inner_left = GetInt(v);
                }
                else if ((v = strstr(line, "InnerTop ")) == line)
                {
                    fi->inner_top = GetInt(v);
                }
                else if ((v = strstr(line, "InnerRight ")) == line)
                {
                    fi->inner_right = GetInt(v);
                }
                else if ((v = strstr(line, "InnerBottom ")) == line)
                {
                    fi->inner_bottom = GetInt(v);
                }
                else if ((v = strstr(line, "NoAlpha ")) == line)
                {
                    fi->noalpha = GetBool(v, "Yes");
                }
            }
        }
        while (line);
        Close(file);
    }

    STRPTR path = allocPath(filename);
    if (path)
    {
        BPTR lock = Lock(path, ACCESS_READ);
        if (lock)
        {
            BPTR oldcd = CurrentDir(lock);
            fi->img_up = GetImageFromFile("up/default", scr);
            fi->img_down = GetImageFromFile("down/default", scr);

            CurrentDir(oldcd);
            UnLock(lock);

        }
        freeString(path);
    }
    if (fi->img_up && fi->img_down)
        return TRUE;
    return FALSE;
}

void FreeFrameConfig(struct dt_frame_image *fi)
{
    if (fi != NULL)
    {
        DisposeImageContainer(fi->img_up);
        DisposeImageContainer(fi->img_down);
    }
}

void dispose_custom_frame(struct dt_frame_image *fi)
{
    if (fi != NULL)
    {
        FreeFrameConfig(fi);
        FreeVec(fi);
    }
}

struct dt_frame_image *load_custom_frame(CONST_STRPTR filename,
    struct Screen *scr)
{
    struct dt_frame_image *fi =
        AllocVec(sizeof(struct dt_frame_image), MEMF_ANY);

    if (fi)
    {
        /* special configuration image for prop gadgets */
        if (Stricmp(FilePart(filename), "frame.config") == 0)
        {
            if (ReadFrameConfig(filename, fi, scr))
            {
                return fi;
            }
            else
            {
                FreeFrameConfig(fi);
                FreeVec(fi);
                return NULL;
            }
        }
        FreeVec(fi);
    }
    return NULL;
}

struct dt_node *dt_load_picture(CONST_STRPTR filename, struct Screen *scr)
{
    struct dt_node *node;
    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    if (!dt_initialized)
    {
        NewList(&dt_list);
        dt_initialized = 1;
    }

    node = List_First(&dt_list);
    while (node)
    {
        if (!Stricmp(filename, node->filename) && scr == node->scr)
        {
            node->count++;
            ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
            return node;
        }
        node = Node_Next(node);
    }

    if ((node =
        (struct dt_node *)AllocVec(sizeof(struct dt_node), MEMF_CLEAR)))
    {

        if ((node->filename = StrDup(filename)))
        {
            /* create the datatypes object */
            D(bug("loading %s\n", filename));

            /* special configuration image for prop gadgets */
            if ((Stricmp(FilePart(filename), "prop.config") == 0)
                || (Stricmp(FilePart(filename), "config") == 0))
            {
                if (ReadPropConfig(node, scr))
                {

                    node->mode = MODE_PROP;
                    node->scr = scr;
                    node->count = 1;
                    AddTail((struct List *)&dt_list, (struct Node *)node);
                    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
                    return node;
                }
                else
                {
                    FreePropConfig(node);
                }
            }
            else
            {
                if ((node->o = LoadPicture(filename, scr)))
                {
                    struct BitMapHeader *bmhd;
                    GetDTAttrs(node->o, PDTA_BitMapHeader, (IPTR) & bmhd,
                        TAG_DONE);
                    D(bug("picture %lx\n", node->o));

                    if (bmhd)
                    {
                        node->width = bmhd->bmh_Width;
                        node->height = bmhd->bmh_Height;
                        node->mask = bmhd->bmh_Masking;
                        D(bug("picture %lx = %ldx%ld\n", node->o,
                                node->width, node->height));
                    }
                    node->scr = scr;
                    node->count = 1;
                    AddTail((struct List *)&dt_list, (struct Node *)node);
                    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
                    return node;
                }
            }
            FreeVec(node->filename);
        }
        FreeVec(node);
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    return NULL;
}

void dt_dispose_picture(struct dt_node *node)
{
    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    if (node && node->count)
    {
        node->count--;
        if (!node->count)
        {
            Remove((struct Node *)node);
            if (node->bfi != NULL)
            {
                if (node->bfi->BitMap != NULL)
                    FreeBitMap(node->bfi->BitMap);
                FreeVec(node->bfi);
            }
            if (node->mode == MODE_PROP)
                FreePropConfig(node);
            else
                DisposeDTObject(node->o);
            FreeVec(node->filename);
            FreeVec(node);
        }
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
}

int dt_width(struct dt_node *node)
{
    if (node)
        return node->width;
    else
        return 0;
}

int dt_height(struct dt_node *node)
{
    if (node)
        return node->height;
    else
        return 0;
}

void dt_put_on_rastport(struct dt_node *node, struct RastPort *rp, int x,
    int y)
{
    struct BitMap *bitmap = NULL;
    struct pdtBlitPixelArray pa;
    ULONG *img;
    Object *o;
    BOOL doAlpha = TRUE;

    o = node->o;
    if (NULL == o)
        return;

#ifdef __mc68000
    /* WritePixelArrayAlpha is insanely expensive on slow
     * m68k machines in planar graphics modes
     */
    doAlpha = GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8;
#endif

    if (doAlpha && node->mask == mskHasAlpha)
    {
        img =
            (ULONG *) AllocVec(dt_width(node) * dt_height(node) * 4,
            MEMF_ANY);
        if (img)
        {
            pa.MethodID = PDTM_READPIXELARRAY;
            pa.pbpa_PixelData = (UBYTE *) img;
            pa.pbpa_PixelFormat = PBPAFMT_ARGB;
            pa.pbpa_PixelArrayMod = dt_width(node) * 4;
            pa.pbpa_Left = 0;
            pa.pbpa_Top = 0;
            pa.pbpa_Width = dt_width(node);
            pa.pbpa_Height = dt_height(node);
            DoMethodA(o, (Msg) & pa);
            WritePixelArrayAlpha(img, 0, 0, dt_width(node) * 4, rp, x, y,
                dt_width(node), dt_height(node), 0xffffffff);
            FreeVec((APTR) img);
        }
    }
    else
    {
        GetDTAttrs(o, PDTA_DestBitMap, (IPTR) & bitmap, TAG_DONE);
        if (NULL == bitmap)
            GetDTAttrs(o, PDTA_BitMap, (IPTR) & bitmap, TAG_DONE);

        if (bitmap)
        {
            APTR mask = NULL;

            GetDTAttrs(o, PDTA_MaskPlane, (IPTR) & mask, TAG_DONE);
            if (mask)
            {
#ifndef __AROS__
                MyBltMaskBitMapRastPort(bitmap, 0, 0, rp, x, y,
                    dt_width(node), dt_height(node), 0xe0, (PLANEPTR) mask);
#else
                BltMaskBitMapRastPort(bitmap, 0, 0, rp, x, y,
                    dt_width(node), dt_height(node), 0xe0, (PLANEPTR) mask);
#endif
            }
            else
                BltBitMapRastPort(bitmap, 0, 0, rp, x, y,
                    dt_width(node), dt_height(node), 0xc0);
        }
    }
}

void dt_put_mim_on_rastport(struct dt_node *node, struct RastPort *rp,
    int x, int y, int state)
{
    struct BitMap *bitmap = NULL;
    struct pdtBlitPixelArray pa;
    ULONG *img;

    Object *o;

    o = node->o;
    if (NULL == o)
        return;
    int width = dt_width(node) >> 1;
    if (node->mask == mskHasAlpha)
    {
        img =
            (ULONG *) AllocVec(dt_width(node) * dt_height(node) * 4,
            MEMF_ANY);
        if (img)
        {

            int height = dt_height(node);
            pa.MethodID = PDTM_READPIXELARRAY;
            pa.pbpa_PixelData = (UBYTE *) img;
            pa.pbpa_PixelFormat = PBPAFMT_ARGB;
            pa.pbpa_PixelArrayMod = width * 4;
            pa.pbpa_Left = state * width;
            pa.pbpa_Top = 0;
            pa.pbpa_Width = width;
            pa.pbpa_Height = height;
            DoMethodA(o, (Msg) & pa);
            WritePixelArrayAlpha(img, 0, 0, width * 4, rp, x, y, width,
                height, 0xffffffff);
            FreeVec((APTR) img);
        }
    }
    else
    {
        GetDTAttrs(o, PDTA_DestBitMap, (IPTR) & bitmap, TAG_DONE);
        if (NULL == bitmap)
            GetDTAttrs(o, PDTA_BitMap, (IPTR) & bitmap, TAG_DONE);


        if (bitmap)
        {
            APTR mask = NULL;

            GetDTAttrs(o, PDTA_MaskPlane, (IPTR) & mask, TAG_DONE);
            if (mask)
            {
#ifndef __AROS__
                MyBltMaskBitMapRastPort(bitmap, width * state, 0, rp, x, y,
                    width, dt_height(node), 0xe0, (PLANEPTR) mask);
#else
                BltMaskBitMapRastPort(bitmap, width * state, 0, rp, x, y,
                    width, dt_height(node), 0xe0, (PLANEPTR) mask);
#endif
            }
            else
                BltBitMapRastPort(bitmap, width * state, 0, rp, x, y,
                    width, dt_height(node), 0xc0);

        }

    }
}

static void CopyTiledBitMap(struct BitMap *Src, WORD SrcOffsetX,
    WORD SrcOffsetY, WORD SrcSizeX, WORD SrcSizeY, struct BitMap *Dst,
    struct Rectangle *DstBounds)
{
    WORD FirstSizeX;  // the width of the rectangle to blit as the first column
    WORD FirstSizeY;  // the height of the rectangle to blit as the first row
    WORD SecondMinX;  // the left edge of the second column
    WORD SecondMinY;  // the top edge of the second column
    WORD SecondSizeX; // the width of the second column
    WORD SecondSizeY; // the height of the second column
    WORD Pos;         // used as starting position in the "exponential" blit
    WORD Size;        // used as bitmap size in the "exponential" blit

    // the width of the first tile, this is either the rest of the tile right
    // to SrcOffsetX or the width of the dest rect, if the rect is narrow
    FirstSizeX = MIN(SrcSizeX - SrcOffsetX, RECTSIZEX(DstBounds));

    // the start for the second tile (if used)
    SecondMinX = DstBounds->MinX + FirstSizeX;

    // the width of the second tile (we want the whole tile to be SrcSizeX
    // pixels wide, if we use SrcSizeX-SrcOffsetX pixels for the left part
    // we'll use SrcOffsetX for the right part)
    SecondSizeX = MIN(SrcOffsetX, DstBounds->MaxX - SecondMinX + 1);

    // the same values are calculated for y direction
    FirstSizeY = MIN(SrcSizeY - SrcOffsetY, RECTSIZEY(DstBounds));
    SecondMinY = DstBounds->MinY + FirstSizeY;
    SecondSizeY = MIN(SrcOffsetY, DstBounds->MaxY - SecondMinY + 1);

    // blit the first piece of the tile
    BltBitMap(Src, SrcOffsetX, SrcOffsetY, Dst, DstBounds->MinX,
        DstBounds->MinY, FirstSizeX, FirstSizeY, 0xC0, -1, NULL);

    // if SrcOffset was 0 or the dest rect was to narrow, we won't need a
    // second column
    if (SecondSizeX > 0)
        BltBitMap(Src, 0, SrcOffsetY, Dst, SecondMinX, DstBounds->MinY,
            SecondSizeX, FirstSizeY, 0xC0, -1, NULL);

    // is a second row necessary?
    if (SecondSizeY > 0)
    {
        BltBitMap(Src, SrcOffsetX, 0, Dst, DstBounds->MinX, SecondMinY,
            FirstSizeX, SecondSizeY, 0xC0, -1, NULL);
        if (SecondSizeX > 0)
            BltBitMap(Src, 0, 0, Dst, SecondMinX, SecondMinY, SecondSizeX,
                SecondSizeY, 0xC0, -1, NULL);
    }

    // this loop generates the first row of the tiles
    for (Pos = DstBounds->MinX + SrcSizeX, Size =
        MIN(SrcSizeX, DstBounds->MaxX - Pos + 1); Pos <= DstBounds->MaxX;)
    {
        BltBitMap(Dst, DstBounds->MinX, DstBounds->MinY, Dst, Pos,
            DstBounds->MinY, Size, MIN(SrcSizeY, RECTSIZEY(DstBounds)),
            0xC0, -1, NULL);
        Pos += Size;
        Size = MIN(Size << 1, DstBounds->MaxX - Pos + 1);
    }

    // this loop blit the first row down several times to fill the whole
    // dest rect
    for (Pos = DstBounds->MinY + SrcSizeY, Size =
        MIN(SrcSizeY, DstBounds->MaxY - Pos + 1); Pos <= DstBounds->MaxY;)
    {
        BltBitMap(Dst, DstBounds->MinX, DstBounds->MinY, Dst,
            DstBounds->MinX, Pos, RECTSIZEX(DstBounds), Size, 0xC0, -1,
            NULL);
        Pos += Size;
        Size = MIN(Size << 1, DstBounds->MaxY - Pos + 1);
    }
}

AROS_UFH3S(void, WindowPatternBackFillFunc,
    AROS_UFHA(struct Hook *, Hook, A0),
    AROS_UFHA(struct RastPort *, RP, A2),
    AROS_UFHA(struct BackFillMsg *, BFM, A1))
{
    AROS_USERFUNC_INIT

    WORD OffsetX;    // the offset within the tile in x direction
    WORD OffsetY;    // the offset within the tile in y direction

    // get the data for our backfillhook
    struct BackFillInfo *BFI = (struct BackFillInfo *)Hook;

#ifdef __SASC
    putreg(12, (long)Hook->h_Data);
#endif

    // The first tile normally isn't totally visible => calculate the offset
    // (offset 0 would mean that the left edge of the damage rectangle
    // coincides with the left edge of a tile)
    OffsetX = BFM->Bounds.MinX - BFI->Options.OffsetX;
//      if (BFI->Options.CenterX) // horizontal centering?
//              OffsetX -= (BFI->Screen->Width-BFI->Width)/2;

    // The same values are calculated for y direction
    OffsetY = BFM->Bounds.MinY - BFI->Options.OffsetY;

/*
	if (BFI->Options.OffsetTitleY) // shift the tiles down?
		OffsetY -= BFI->Screen->BarHeight+1;
*/

//    if (BFI->Options.CenterY) // horizontal centering?
//        OffsetY -= (BFI->Screen->Height - BFI->Height)/2;

    CopyTiledBitMap(BFI->BitMap, MOD(OffsetX + BFI->OffsetX, BFI->Width),
        MOD(OffsetY + BFI->OffsetY, BFI->Height), BFI->CopyWidth,
        BFI->CopyHeight, RP->BitMap, &BFM->Bounds);

    AROS_USERFUNC_EXIT
}

static void CalculateCopySizes(struct BackFillInfo *BFI)
{
    BFI->CopyWidth =
        (BFI->Width >
        BFI->Options.MaxCopyWidth) ? BFI->Width : BFI->Options.
        MaxCopyWidth - BFI->Options.MaxCopyWidth % BFI->Width;
    BFI->CopyHeight =
        (BFI->Height >
        BFI->Options.MaxCopyHeight) ? BFI->Height : BFI->Options.
        MaxCopyHeight - BFI->Options.MaxCopyHeight % BFI->Height;
}

/*
**********************************************************/
void dt_put_on_rastport_tiled(struct dt_node *node, struct RastPort *rp,
    int x1, int y1, int x2, int y2, int xoffset, int yoffset)
{
    struct Screen *scr = node->scr;
    struct BitMap *bitmap;
    Object *o;

    o = node->o;
    if (!o)
        return;

    GetDTAttrs(o, PDTA_DestBitMap, (IPTR) & bitmap, TAG_DONE);
    if (NULL == bitmap)
        GetDTAttrs(o, PDTA_BitMap, (IPTR) & bitmap, TAG_DONE);
    if (NULL == bitmap)
        return;

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    if (!node->bfi)
    {
        struct BackFillInfo *bfi =
            (struct BackFillInfo *)AllocVec(sizeof(struct BackFillInfo),
            MEMF_CLEAR);
        if (bfi)
        {
            LONG depth = GetBitMapAttr(bitmap, BMA_DEPTH);
            bfi->Hook.h_Entry = (HOOKFUNC) WindowPatternBackFillFunc;
#ifdef __SASC
            bfi->Hook.h_Data = (APTR) getreg(12);       /* register A4 */
#endif

            bfi->Options.MaxCopyWidth = 256;
            bfi->Options.MaxCopyHeight = 256;
//          bfi->Options.CenterX = FALSE; /* center the tiles horizontally? */
//          bfi->Options.CenterY = FALSE; /* center the tiles vertically? */
            bfi->Options.OffsetX = 0;   /* offset to add */
            bfi->Options.OffsetY = 0;   /* offset to add */

            /* add the screen titlebar height to the vertical offset? */
            bfi->Options.OffsetTitleY = TRUE;
            bfi->Width = dt_width(node);
            bfi->Height = dt_height(node);

            CalculateCopySizes(bfi);

            if ((bfi->BitMap =
                    AllocBitMap(bfi->CopyWidth, bfi->CopyHeight, depth,
                        BMF_INTERLEAVED | BMF_MINPLANES,
                        scr->RastPort.BitMap)))
            {
                struct Rectangle CopyBounds;
                CopyBounds.MinX = 0;
                CopyBounds.MinY = 0;
                CopyBounds.MaxX = bfi->CopyWidth - 1;
                CopyBounds.MaxY = bfi->CopyHeight - 1;

                CopyTiledBitMap(bitmap, 0, 0, bfi->Width, bfi->Height,
                    bfi->BitMap, &CopyBounds);
            }
        }
        node->bfi = bfi;
    }

    if (node->bfi)
    {
        struct BackFillInfo *bfi = node->bfi;
        struct Rectangle rect;

        rect.MinX = x1;
        rect.MinY = y1;
        rect.MaxX = x2;
        rect.MaxY = y2;

        if (rp->Layer)
        {
            LockLayer(0, rp->Layer);
            xoffset -= rp->Layer->bounds.MinX;
            yoffset -= rp->Layer->bounds.MinY;
        }

        bfi->OffsetX = xoffset;
        bfi->OffsetY = yoffset;

        DoHookClipRects((struct Hook *)bfi, rp, &rect);

        if (rp->Layer)
        {
            UnlockLayer(rp->Layer);
        }
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
}
