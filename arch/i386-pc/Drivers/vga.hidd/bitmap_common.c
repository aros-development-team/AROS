#include <exec/alerts.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

static VOID set_pixelformat(Object *bm, struct vga_staticdata *xsd)
{
    HIDDT_PixelFormat *pf = BM_PIXFMT(bm);
    
    pf->red_mask	= 0x000000ff;
    pf->green_mask	= 0x0000ff00;
    pf->blue_mask	= 0x00ff0000;
    
    pf->red_shift	= 24;
    pf->green_shift	= 16;
    pf->blue_shift	= 8;
}

/**************  BitMap::Set()  *********************************/
static VOID MNAME(set)(Class *cl, Object *o, struct pRoot_Set *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    struct TagItem *tag, *tstate;
    ULONG idx;
    
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Foreground :
		    data->fg = tag->ti_Data;
		    break;
		    
                case aoHidd_BitMap_Background :
		    data->bg = tag->ti_Data;
		    break;
		    
		case aoHidd_BitMap_DrawMode :		    
		    data->drmd = tag->ti_Data;
		    break;
            }
        }
    }
    
    /* Let supermethod take care of other attrs */
    DoSuperMethod(cl, o, (Msg)msg);
    
    return;
}

static HIDDT_Pixel MNAME(mapcolor)(Class *cl, Object *o, struct pHidd_BitMap_MapColor *msg)
{
    int i,f;
    struct bitmap_data *data = INST_DATA(cl, o);

    HIDDT_Pixel red	= msg->color->red >> 8;
    HIDDT_Pixel green	= msg->color->green >> 8;
    HIDDT_Pixel blue	= msg->color->blue >> 8;

    HIDDT_Pixel color = red | (green << 8) | (blue << 16);
    
    i = 0;
    f = 1;
    
    do
    {
	if (data->cmap[i] < 0x01000000)	/* Is empty? */
	{
	    f = 0;			/* Got color */
	    data->cmap[i] = 0x01000000 | red | (green << 8) | (blue << 16);
	    data->Regs->DAC[i*3] = red >> 2;
	    data->Regs->DAC[i*3+1] = green >> 2;
	    data->Regs->DAC[i*3+2] = blue >> 2;
#ifdef OnBitmap
	    ObtainSemaphore(&XSD(cl)->HW_acc);
	    vgaRestore(data->Regs);
	    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */
	}
	else if ((data->cmap[i] & 0xffffff) == color)
	{
	    if ((data->cmap[i] & 0xff000000) != 0xff000000)
	    {
		data->cmap[i] += 0x01000000;
	    }
	    f=0;
	}
	else i++;	    
    } while (f && (i<16));
    
    return i;
}

static VOID MNAME(unmappixel)(Class *cl, Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{
    int i,f;
    struct bitmap_data *data = INST_DATA(cl, o);

    HIDDT_Pixel red	= msg->color->red >> 8;
    HIDDT_Pixel green	= msg->color->green >> 8;
    HIDDT_Pixel blue	= msg->color->blue >> 8;

    HIDDT_Pixel color = red | (green << 8) | (blue << 16);
    
    i = 0;
    f = 1;

    do
    {
	if ((data->cmap[i] & 0xffffff) == color)	/* Find color */
	{
	    f = 0;					/* Got color */
	    if (data->cmap[i] & 0xff000000)		/* Dealloc it if used */
	    {
		data->cmap[i] -= 0x01000000;
	    }
	}
	else i++;	    
    } while (f && (i<16));
}

/*********  BitMap::PutPixel()  ***************************/

static VOID MNAME(putpixel)(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    HIDDT_Pixel fg;

    unsigned char mask,pix,notpix;
    int size,i;
    ULONG depth;
    unsigned char *ptr;
#ifdef OnBitmap
    unsigned char *ptr2;
#endif /* OnBitmap */
     
    GetAttr(o, aHidd_BitMap_Depth, &depth);
    fg = msg->pixel;
    
    ptr = (char *)(data->VideoData + (msg->x + (msg->y * data->width)) / 8);
#ifdef OnBitmap
    ptr2 = (char *)(0xa0000 + (msg->x + (msg->y * data->width)) / 8);
#endif /* OnBitmap */
    pix = 128 >> (msg->x % 8);
    size = (data->width * data->height) / 8;
    notpix = ~pix;
    mask = 1;
#ifdef OnBitmap
    ObtainSemaphore(&XSD(cl)->HW_acc);
    outw(0x3ce,0x0000);
    outw(0x3ce,0x0001);
    outw(0x3ce,0x0005);
#endif /* OnBitmap */
    
    for (i=0; i < depth; i++)
    {
#ifdef OnBitmap
	outw(0x3c4,(UWORD)((UWORD)(256 << i) | 0x02));
#endif /* OnBitmap */
	*ptr = *ptr & notpix;
	if (fg & mask)
	    *ptr = *ptr | pix;
#ifdef OnBitmap
	*ptr2 = *ptr;
#endif /* OnBitmap */
	ptr = (char *)(ptr + size);
	mask = mask << 1;
    }
#ifdef OnBitmap
    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */
    return;
}

/*********  BitMap::GetPixel()  *********************************/
static HIDDT_Pixel MNAME(getpixel)(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel;
    struct bitmap_data *data = INST_DATA(cl, o);
    
    unsigned char pix;
    int size,i;
    ULONG depth;
    unsigned char *ptr;

    GetAttr(o, aHidd_BitMap_Depth, &depth);

    pixel = 0;
    size = (data->width * data->height) / 8;
    ptr = (char *)(data->VideoData + (msg->x + (msg->y * data->width)) / 8);
    pix = 128 >> (msg->x % 8);

    for (i=0; i < depth; i++)
    {
        if ((*ptr & pix)!=0)
    	    pixel |= (1 << i);
	ptr = (char *)(ptr + size);
    }

    /* Get pen number from colortab */
    return pixel;
}

/*********  BitMap::DrawPixel()  ***************************/

static VOID MNAME(drawpixel)(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    HIDDT_Pixel fg;

    unsigned char mask,pix,notpix;
    int size,i;
    ULONG depth;
    unsigned char *ptr;
#ifdef OnBitmap
    unsigned char *ptr2;
#endif /* OnBitmap */
     
    GetAttr(o, aHidd_BitMap_Foreground, &fg);
    GetAttr(o, aHidd_BitMap_Depth, &depth);
    
    ptr = (char *)(data->VideoData + (msg->x + (msg->y * data->width)) / 8);
#ifdef OnBitmap
    ptr2 = (char *)(0xa0000 + (msg->x + (msg->y * data->width)) / 8);
#endif /* OnBitmap */
    pix = 128 >> (msg->x % 8);
    size = (data->width * data->height) / 8;
    notpix = ~pix;
    mask = 1;
#ifdef OnBitmap
    ObtainSemaphore(&XSD(cl)->HW_acc);
    outw(0x3ce,0x0000);
    outw(0x3ce,0x0001);
    outw(0x3ce,0x0005);
#endif /* OnBitmap */

    for (i=0; i < depth; i++)
    {
#ifdef OnBitmap
	outw(0x3c4,(mask << 8) | 2);
#endif /* OnBitmap */
	*ptr = *ptr & notpix;
	if ((fg & (1 << i))!=0)
	{
	    *ptr = *ptr | pix;
	}
#ifdef OnBitmap
	*ptr2 = *ptr;
#endif /* OnBitmap */
	ptr = (char *)(ptr + size);
	mask <<= 1;
    }
#ifdef OnBitmap
    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */
    return;
}

/*** BitMap::Get() *******************************************/

static VOID MNAME(get)(Class *cl, Object *o, struct pRoot_Get *msg)
{
//    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG idx;
    if (IS_VGABM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    default:
	    	DoSuperMethod(cl, o, (Msg)msg);
		break;
	}
    }
    else
    {
    	DoSuperMethod(cl, o, (Msg)msg);
    }

    return;
}




static VOID MNAME(copybox)(Class *cl, Object *o, struct pHidd_BitMap_CopyBox *msg)
{
  /*
    Attributes of msg:
      srcX, srcY, dest, destX, destY, width, height
    Object o is source BitMap object.
   */
  struct bitmap_data * data_src = INST_DATA(cl, o);
  struct bitmap_data * data_dst = INST_DATA(cl, msg->dest);

  unsigned char * ptr_src = (char *)(data_src->VideoData + (msg->srcX + (msg->srcY * data_src->width)) / 8);
  unsigned char * ptr_dst = (char *)(data_dst->VideoData + (msg->destX + (msg->destY * data_dst->width)) / 8);
  unsigned char * ptr_dst_v = (char *)(0xa0000 + (msg->destX + (msg->destY * data_dst->width)) / 8 );
  
  ULONG size_src = (data_src->width * data_src->height) >> 3;
  ULONG size_dst = (data_dst->width * data_dst->height) >> 3;
  
  ULONG depth;
  ULONG depth2;
  UWORD mask = 0x100;

  ULONG shift_src = msg->srcX & 7;
  
  ULONG shift_dst = msg->destX & 7;
  UBYTE shiftmask_dst = 0;
  
  LONG w;
  ULONG h = 0;
  UBYTE pix;
  ULONG i = 0;
  ULONG j = 0;
 
  int d;

  if (shift_dst)
    shiftmask_dst = ((BYTE)0x80 >> (shift_dst - 1));
  
  GetAttr(o, aHidd_BitMap_Depth, &depth);

  if (-1 == data_dst->disp)
  {
    ObtainSemaphore(&XSD(cl)->HW_acc);
    outw(0x3ce,0x0000);
    outw(0x3ce,0x0001);
    outw(0x3ce,0x0005);
  }

  for (d=0; d < depth; d++)
  {
    if (-1 == data_dst->disp)
    {
      outw(0x3c4, mask | 2);
      mask = mask << 1;
    }

    while (h < msg->height)
    {
      i = h * data_src->width >> 3;
      j = h * data_dst->width >> 3;
      /*
      ** Read 8 pixels
      */
      w = msg->width;
      pix = ptr_src[i++];
      if (0 != shift_src)
        pix = (pix << shift_src) | (ptr_src[i] >> (8 - shift_src));
      
      if (w >= 8)
      {
        do
        {
          /* Now distribute these 8 bits to the destination */
          if (0 != shift_dst)
          {
            ptr_dst[j] = (ptr_dst[j] & shiftmask_dst) | (pix >> shift_dst);
            j++;
            ptr_dst[j] = (pix << (8 - shift_dst)) | (ptr_dst[j] & ~shiftmask_dst);
            if (-1 == data_dst->disp)
            {
              /* also write it to video mem! */
              ptr_dst_v[j-1] = ptr_dst[j-1];
              ptr_dst_v[j  ] = ptr_dst[j  ];
            } 
          }
          else
	  { 
            ptr_dst[j] = pix;
            if (-1 == data_dst->disp)
              ptr_dst_v[j] = pix;
            j++;
          }
          w-=8;
          pix = ptr_src[i++];
          if (0 != shift_src)
            pix = (pix << shift_src) | (ptr_src[i] >> (8 - shift_src));
        }
        while (w >= 8);
      }
      
      /* There are still some pixels to distribute but less than 8 */
      if (w > 0)
      {
        WORD pixmask = ((WORD)0xff80 >> (w-1));
        pix &= pixmask;
        pixmask ^= 0xff;
        if (0 != shift_dst)
        {
          ptr_dst[j] = (ptr_dst[j] & (pixmask >> shift_dst)) | 
                       (pix >> shift_dst);
          if (-1 == data_dst->disp)
            ptr_dst_v[j] = ptr_dst[j];
          w = w - (8 - shift_dst);
          if (w > 0)
          {
            j++;
            pixmask = (pixmask << 8) | 0xff;
            pixmask = pixmask << (8 - shift_dst);
            pixmask >>= 8;
            ptr_dst[j] = (pix << (8 - shift_dst)) | 
                         (ptr_dst[j] & pixmask);
            if (-1 == data_dst->disp)
              ptr_dst_v[j] = ptr_dst[j];
          }
        }
        else
        {
          ptr_dst[j] = pix | (ptr_dst[j] & pixmask);
          if (-1 == data_dst->disp)
            ptr_dst_v[j] = ptr_dst[j];
        }
      }
      h++;
    } /* while (h  < msg->height) */
    ptr_src = (char *)(ptr_src + size_src);
    ptr_dst = (char *)(ptr_dst + size_dst);
  } /* for () */

  if (-1 == data_dst->disp)
    ReleaseSemaphore(&XSD(cl)->HW_acc);
}
