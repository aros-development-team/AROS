/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/clip.h>
#include <graphics/gfxbase.h>
#include <graphics/layers.h>

#include "basicfuncs.h"

#define DEF_MAXX CR->bounds.MaxX
#define DEF_MINX CR->bounds.MinX
#define DEF_MAXY CR->bounds.MaxY
#define DEF_MINY CR->bounds.MinY
#define DEF_X0   R->MinX
#define DEF_X1   R->MaxX
#define DEF_Y0   R->MinY
#define DEF_Y1   R->MaxY


#define DEF_DO_THE_BLIT(CR_New, LAYER, FRIEND_BM)            \
  (CR_New)->BitMap = AllocBitMap(                            \
     (CR_New)->bounds.MaxX - (CR_New)->bounds.MinX + 1 + 16, \
     (CR_New)->bounds.MaxY - (CR_New)->bounds.MinY + 1,      \
     bm_old->Depth,                                          \
     0,                                                      \
     (FRIEND_BM));                                           \
                                                             \
  if ((CR_New)->BitMap !=NULL )                              \
  {                                                          \
    BltBitMap(                                               \
      bm_old,                                                \
      (CR_New)->bounds.MinX - DEF_MINX + ALIGN_OFFSET(DEF_MINX),  \
      (CR_New)->bounds.MinY - DEF_MINY,                      \
      (CR_New)->BitMap,                                      \
      ALIGN_OFFSET((CR_New)->bounds.MinX),                   \
      0,                                                     \
      (CR_New)->bounds.MaxX - (CR_New)->bounds.MinX + 1,     \
      (CR_New)->BitMap->Rows,                                \
      0x0c0,                                                 \
      0xff,                                                  \
      NULL                                                   \
    );                                                       \
    (CR_New) -> lobs = (LAYER);                              \
  }
  


#define DEF_DO_THE_BLIT_CR(X0, Y0, X1, Y1, FRIEND_BM)        \
  CR->BitMap = AllocBitMap(                                  \
     (X1) - (X0) + 1 + 16,                                   \
     (Y1) - (Y0) + 1,                                        \
     bm_old->Depth,                                          \
     0,                                                      \
     (FRIEND_BM));                                           \
                                                             \
  if (CR->BitMap != NULL)                                    \
    BltBitMap(                                               \
      bm_old,                                                \
      (X0) - DEF_MINX + (DEF_MINX & 0x0F),                   \
      (Y0) - DEF_MINY,                                       \
      CR->BitMap,                                            \
      (X0) & 0x0F,                                           \
      0,                                                     \
      (X1) - (X0) + 1,                                       \
      CR->BitMap->Rows,                                      \
      0x0c0,                                                 \
      0xff,                                                  \
      NULL                                                   \
    );

extern struct GfxBase * GfxBase;
extern struct LayersBase * LayersBase;

/*
   This is how case 0 looks like:

   none of the coords x0,y0,x1 and y1 are inside the
   cliprect that is behind it

    x0     x1

                      this is how the cliprect
                      behind has to be split
 y0 nnnnnnnnn
    nnnnnnnnn
    nnonononn   MinY        nnnnn  CR
    nnnononnn               nnnnn
    nnonononn               nnnnn
    nnnononnn               nnnnn
    nnonononn               nnnnn
    nnnononnn   MaxY        nnnnn
    nnnnnnnnn
 y1 nnnnnnnnn

      M   M
      i   a
      n   x
      X   X

*/




struct ClipRect * Case_0(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{

  if (NULL != CR->BitMap)
  {
    /*
      All data are already backed up. I only need to set the lobs-pointer
      to the new layer, if this one is farther in the front than the
      previous layer that was hiding me. This is done for all types
      of layers.
    */
    if (CR->lobs->priority < newlayer->priority)
      CR->lobs = newlayer;
  }
  else
  {
  /* get one new bitmap structure, if there is none. But only
     get it if the new layer is in front of the passive layer */
    if (newlayer->priority > passivelayer->priority)
    {
      /* we have to back it up now because it will be hidden by the
         new layer.
      */
      /* only for non-simple layers */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        ULONG AllocBitMapFlag = 0;
        struct BitMap * DestBM;
        LONG DestX, DestY;

        if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
          AllocBitMapFlag = BMF_CLEAR;

        /* if it doesn't have a superbitmap I allocate memory */ 
        if (0 == (passivelayer->Flags & LAYERSUPER) )
        {

          CR -> BitMap = AllocBitMap(
             DEF_MAXX - DEF_MINX + 1 + 16,
             DEF_MAXY - DEF_MINY + 1,
             display_bm->Depth,
             AllocBitMapFlag,
             display_bm);

          /* out of memory */
          if (NULL == CR -> BitMap)
            return NULL;

          DestBM = CR->BitMap; 
          DestX  = DEF_MINX & 0x0f;
          DestY  = 0;
        } /* if */
        else
        {
          /* it has a superbitmap */
          DestX  = CR->bounds.MinX - 
                       passivelayer->bounds.MinX - passivelayer->Scroll_X;
          DestY  = CR->bounds.MinY -
                       passivelayer->bounds.MinY - passivelayer->Scroll_Y;
          DestBM = passivelayer->SuperBitMap;
          CR -> BitMap = DestBM;

        } /* else */
      
        /* 
           and back up the information for the part with the n's
           from the info found in the display bitmap.
        */
        if (0 == AllocBitMapFlag)
           BltBitMap(
            display_bm,             /* SrcBitMap = Display BitMap */ 
            DEF_MINX,               /* SrcX - no optimization !!! */
            DEF_MINY,               /* SrcY */
            DestBM,                 /* DestBitMap */
            DestX,                  /* DestX - optimized */
            DestY,                  /* DestY */
            DEF_MAXX - DEF_MINX + 1,/* SizeX */
            DEF_MAXY - DEF_MINY + 1,/* SizeY */
            0x0c0,                  /* Vanilla Copy */
            0xff,                   /* Mask */
            NULL
          );
      } /* if (not a simple layer) */
    
      /* 
       * Only update the lobs entry in the CR if the lobs pointer is NULL
       * or if the newlayer is further in the front than the previous layer
       * hiding this cliprect.
       */
      if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
        CR -> lobs = newlayer;
    }
  } /* else */
  return CR;
}

/*
   This is how case 1 looks like:

   only y1 is inside the
   cliprect that is behind it

    x0     x1

 y0 nnnnnnnn          this is how the cliprect
    nnnnnnnn          behind has to be split
    nnnnnnnn
    nnnnnnnn
    nnononon  MinY        nnnnn  CR
 y1 nnnononn              nnnnn
      ooooo               ooooo  CR_New1
      ooooo               ooooo
      ooooo               ooooo
      ooooo   MaxY        ooooo

      M   M
      i   a
      n   x
      X   X

*/

struct ClipRect * Case_1(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);

  /* 
     If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.
  */

  if (NULL != CR_New1)
  {
    /* Initialize New1, it will contain the lower part (o's) */
    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_Y1+1  /* y1+1 */;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_MAXY;

    CR_New1->lobs        = CR->lobs;

    /* The new ClipRect goes behind the old one in the list */
    CR_New1->Next = CR->Next;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures */

      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper" one (n's), ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_MINY, DEF_MAXX, DEF_Y1, display_bm);

          /* the "lower" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* 
             dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR->lobs = newlayer;

    }
    else
    { /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */
      if (newlayer->priority > passivelayer->priority)
      {
        /* only for non-simple layers */
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;       
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;
          /* 
             Are we supposed to get a blank bitmap, which is the case
             for cliprects that are not shown yet = layers that are added  
           */ 
          if (0 == (passivelayer->Flags & LAYERSUPER))
          {

            CR -> BitMap = AllocBitMap(
               DEF_MAXX - DEF_MINX + 1 + 16,
               DEF_Y1 - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }

          /* 
             and back up the information for the part with the n's
             from the info found in the display bitmap.
          */

          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */ 
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_MINX + 1,/* SizeX */
              DEF_Y1 - DEF_MINY + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL);
        } /* if (not a simple layer) */
        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      } /* if */
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper part (n's) */
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New1;
}

/****************************************************************/

/*
   This is how case 2 looks like:

   only x1 is inside the
   cliprect that is behind it

    x0    x1                this is how the cliprect
                            behind has to be split
 y0 nnnnnnn
    nnononoooo  MinY        nnnnnooo
    nnnononooo              nnnnnooo
    nnononoooo              nnnnnooo
    nnnononooo              nnnnnooo
    nnononoooo  MaxY        nnnnnooo
 y1 nnnnnnn
      M      M
      i      a
      n      x
      X      X

*/

/*
  Rectangle R is pointing to the rectangle with the n's.
  ClipRect CR has the rectagle with the o's.
*/


struct ClipRect * Case_2(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */
  if (NULL != CR_New1)
  {

    /* Initialize New1, it will contain the right part (o's) */
    CR_New1->bounds.MinX = DEF_X1+1;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_MAXY;
    
    CR_New1->lobs        = CR->lobs;

    /* The new ClipRect goes behind the old one in the list */
    CR_New1->Next = CR->Next;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* 
         get two new bitmap structures
         they have the same height, but different width, but only if
         it's not a simple layer
       */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "left" one (n's), ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_MINY, DEF_X1, DEF_MAXY, display_bm);

          /* the "right" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR-> lobs, display_bm);
          /* 
             Dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;
    }
    else
    { /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer*/
      /* the "left" one (n's) */
      if (newlayer->priority > passivelayer->priority)
      {
        /* only for non-simple layers */
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0 )
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
          {

            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_MINX + 1 + 16,
               DEF_MAXY - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
          }

          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1 - DEF_MINX + 1,  /* SizeX */
              DEF_MAXY - DEF_MINY + 1,/* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
             );
        } /* if (not a simple layer) */
        
        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;
      }
    }
    /* leave this code down here, otherwise the DEFs will go wrong */
    /* Change CR, it will contain the left part (n's) */
    CR->bounds.MaxX = DEF_X1;
  }

  return CR_New1;
}


/****************************************************************/

/*
   This is how case 3 (=%0011) looks like:

   x1 and y1 are inside the
   cliprect that is behind it


     x0   x1              this is how the cliprect
 y0  nnnnnn               behind has to be split
     nnnnnn
     nnnonoooo  MinY      nnnooo
     nnnnonooo         CR nnnooo CR_New1
 y1  nnnonoooo            nnnooo
        oooooo            OOOOOO
        oooooo  MaxY      OOOOOO

        M    M             CR_New2
        i    a
        n    x
        X    X

*/



struct ClipRect * Case_3(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);

  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */
  if (NULL != CR_New1 && NULL != CR_New2)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_X1+1;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y1;
    
    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y1+1;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_MAXY;
    
    /* The new ClipRects go behind the old one in the list */
    CR_New2->Next = CR->Next;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */

      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper left" one (n's) */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_MINY, DEF_X1, DEF_Y1, display_bm);
          /* the "upper right" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm)
          /* the "lower" one (O's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
          CR_New2->BitMap = passivelayer->SuperBitMap;
        }


      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority) 
        CR -> lobs = newlayer;

    }
    else
    { /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */
      /* the "upper left" one (n's) */
      if (newlayer->priority > passivelayer->priority)
      {
        /* only for non-simple layers */
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER)) 
	  {

            CR->BitMap = AllocBitMap(
               DEF_X1 - DEF_MINX + 1 + 16,
               DEF_Y1 - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
	  {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }

          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */ 
              DestY,                  /* DestY */
              DEF_X1 - DEF_MINX + 1,  /* SizeX */
              DEF_Y1 - DEF_MINY + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MaxX = DEF_X1;
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New2;
}

/*
   This is how case 4 looks like:

   only y0 is inside the
   cliprect that is behind it



                    this is how the cliprect
                    behind has to be split
    x0     x1

      ooooo   MinY        ooooo
      ooooo               ooooo CR_New1
      ooooo               ooooo
 y0 nnnononn              nnnnn
    nnononon              nnnnn CR
    nnnononn  MaxY        nnnnn
    nnnnnnnn
    nnnnnnnn
 y1 nnnnnnnn

      M   M
      i   a
      n   x
      X   X

*/

/*
  Rectangle R is pointing to the rectangle with the n's.
  ClipRect CR has the rectagle with the o's.
*/

struct ClipRect * Case_4(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);

  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1)
  {
    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    CR_New1->lobs        = CR->lobs;

    /* The new ClipRect goes behind the old one in the list */
    CR_New1->Next = CR->Next;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures */

      if (0 == (passivelayer -> Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer -> Flags & LAYERSUPER))
        {
          /* the "upper" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);

          /* the "lower" one (n's), ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_Y0, DEF_MAXX, DEF_MAXY, display_bm );
         /* dispose the old bitmap structure as everything
            that was backed up there is now in the two other
            bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    { 
      /* the "lower" ones */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0 )
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
	  {

            CR->BitMap = AllocBitMap(
              DEF_MAXX - DEF_MINX + 1 + 16,
              DEF_MAXY - DEF_Y0 + 1,
              display_bm->Depth,
              AllocBitMapFlag,
              display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
	  {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }

          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
     
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_MINX + 1,/* SizeX */
              DEF_MAXY - DEF_Y0 + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer)*/
        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;


      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper part (n's) */
    CR->bounds.MinY = DEF_Y0;
  }

  return CR_New1;
}

/****************************************************************/

/*
   This is how case 5 (=%0101) looks like:

   y0 and y1 are inside the
   cliprect that is behind it

    x0         x1                  this is how the cliprect
                                 behind has to be split

       ooooooo     MinY             ooooooo CR_New1
 y0 nnnononononnn                   nnnnnnn
    nnnnonononnnn                   nnnnnnn CR
 y1 nnnononononnn                   nnnnnnn
       ooooooo     MaxY             ooooooo CR_New2



       M     M
       i     a
       n     x
       X     X

*/


struct ClipRect * Case_5(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */
  if (NULL != CR_New1 && NULL != CR_New2)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;
    
    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y1+1;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New2->Next = CR->Next;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);

          /* the "middle" one (n's), ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_Y0, DEF_MAXX, DEF_Y1, display_bm);

          /* the "lower" one (o's) */
          DEF_DO_THE_BLIT(CR_New2, CR-> lobs, display_bm);
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
          CR_New2->BitMap = passivelayer->SuperBitMap;
        }
      }

      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    { 
      /* the "middle" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER ))
          {
            CR ->BitMap = AllocBitMap(
               DEF_MAXX - DEF_MINX + 1 + 16,
               DEF_Y1 - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }

          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_MINX + 1,/* SizeX */
              DEF_Y1   - DEF_Y0   + 1,/* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */
    /* Change CR, it will contain the left part (o's) */
    CR->bounds.MinY = DEF_Y0;
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New2;
}


/*
   This is how case 6 (=%0110) looks like:

   x1 and y0 are inside the
   cliprect that is behind it


     x0   x1              this is how the cliprect
                          behind has to be split

        oooooo  MinY      oooooo
        oooooo            oooooo CR_New1
 y0  nnnonoooo            nnnOOO
     nnnnonooo         CR nnnOOO  CR_New2
     nnnonoooo  MaxY      nnnOOO
     nnnnnn
     nnnnnn
 y1  nnnnnn
        M    M
        i    a
        n    x
        X    X

*/


struct ClipRect * Case_6(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
    }
 
    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    CR_New2->bounds.MinX = DEF_X1+1;
    CR_New2->bounds.MinY = DEF_Y0;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New2->Next = CR->Next;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper " one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "lower left" one (o's), Cliprect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_Y0, DEF_X1, DEF_MAXY, display_bm);
          /* the "lower right" one (O's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
          CR_New2->BitMap = passivelayer->SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    {
      /* the "lower left" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;
        
          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
	  {

            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_MINX + 1 + 16,
               DEF_MAXY - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;
  
            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
	  {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }

          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1 - DEF_MINX + 1,  /* SizeX */
              DEF_MAXY - DEF_Y0 + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MinY = DEF_Y0;
    CR->bounds.MaxX = DEF_X1;
  }

  return CR_New2;
}

/*
   This is how case 7 (=%0111) looks like:

   x1 and y0 and y1 are inside the
   cliprect that is behind it


     x0   x1              this is how the cliprect
                          behind has to be split

        oooooo  MinY      oooooo  CR_New1
 y0  nnnnonooo            nnnOOO
     nnnonoooo         CR nnnOOO  CR_New2
 y1  nnnnonooo            nnnOOO
        onoooo  MaxY      oooooo  CR_New3



        M    M
        i    a
        n    x
        X    X

*/

struct ClipRect * Case_7(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2, * CR_New3;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);
  CR_New3 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2 && NULL != CR_New3)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
      CR_New3->lobs = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    CR_New2->bounds.MinX = DEF_X1+1;
    CR_New2->bounds.MinY = DEF_Y0;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_Y1;

    CR_New3->bounds.MinX = DEF_MINX;
    CR_New3->bounds.MinY = DEF_Y1+1;
    CR_New3->bounds.MaxX = DEF_MAXX;
    CR_New3->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New3->Next = CR->Next;
    CR_New2->Next = CR_New3;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper " one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "middle left" one (o's), Cliprect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_MINX, DEF_Y0, DEF_X1, DEF_Y1, display_bm);
          /* the "middle right" one (O's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* the "lower" one (o's) */
          DEF_DO_THE_BLIT(CR_New3, CR->lobs, display_bm);

          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures  
           */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1->BitMap = passivelayer->SuperBitMap;
          CR_New2->BitMap = passivelayer->SuperBitMap;
          CR_New3->BitMap = passivelayer->SuperBitMap;
        }
      }
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    { /* the "lower left" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
	  { 

            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_MINX + 1 + 16,
               DEF_Y1 - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_MINX & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = CR->bounds.MinX - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }

          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_MINX,               /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1 - DEF_MINX + 1,  /* SizeX */
              DEF_Y1 - DEF_Y0   + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;


      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MinY = DEF_Y0;
    CR->bounds.MaxX = DEF_X1;
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New3;
}


/****************************************************************/

/*
   This is how case 8 looks like:

   only x0 is inside the
   cliprect that is behind it

         x0     x1               this is how the cliprect
                                 behind has to be split
 y0      nnnnnnnn
      ooonononnnn     MinY          ooonnnnn
      oooonononnn                   ooonnnnn
      ooonononnnn           CR_New1 ooonnnnn CR
      oooonononnn                   ooonnnnn
      ooonononnnn     MaxY          ooonnnnn
 y1      nnnnnnnn

      M      M
      i      a
      n      x
      X      X

*/


struct ClipRect * Case_8(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);

  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1)
  {

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_X0-1;
    CR_New1->bounds.MaxY = DEF_MAXY;

    CR_New1->lobs        = CR->lobs;

    CR_New1->Next = CR->Next;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same height, but different width
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "left" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);

          /* the "right" one (n's) , ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_MINY, DEF_MAXX, DEF_MAXY, display_bm);
    
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
           */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    { /* the "right" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER ))
	  {

            CR ->BitMap = AllocBitMap(
               DEF_MAXX - DEF_X0 + 1 + 16,
               DEF_MAXY - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else 
          {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
          }

          /* 
             and back up the information for the part with the n's
             from the info found in the display bitmap, if the new layer
             is infront of the passive layer and if the cliprect of the 
             passive layer is already displayed.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_X0 + 1,  /* SizeX */
              DEF_MAXY - DEF_MINY + 1,/* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;


      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */
    /* Change CR, it will contain the left part (n's) */
     CR->bounds.MinX = DEF_X0;
  }
  return CR_New1;
}

/****************************************************************/

/*
   This is how case 9 (=%1001) looks like:

   x0 and y1 are inside the
   cliprect that is behind it

         x0     x1               this is how the cliprect
 y0      nnnnnnnn                behind has to be split
         nnnnnnnn
      ooonononnnn     MinY          ooonnnnn
      oooonononnn                   ooonnnnn
 y1   ooonononnnn           CR_New1 ooonnnnn CR
      oooooooo                      OOOOOOOO
      oooooooo        MaxY          OOOOOOOO
                                      CR_New2

      M      M
      i      a
      n      x
      X      X

*/

struct ClipRect * Case_9(struct Rectangle * R,
                         struct ClipRect * CR,
                         struct BitMap * display_bm,
                         struct Layer * newlayer,
                         struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
    }
  
    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_X0-1;
    CR_New1->bounds.MaxY = DEF_Y1;

    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y1+1;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New2->Next = CR->Next;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper " one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "lower left" one (o's), Cliprect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_MINY, DEF_MAXX, DEF_Y1, display_bm);
          /* the "lower right" one (O's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
    
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
        }
      }
       
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    { /* the "lower left" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
 	  {

            CR ->BitMap = AllocBitMap(
               DEF_MAXX - DEF_X0 + 1 + 16,
               DEF_Y1 - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else
 	  {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }


          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_X0 + 1,  /* SizeX */
              DEF_Y1 - DEF_MINY + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer)*/ 

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */
    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MaxY = DEF_Y1;
  }
  return CR_New2;
}


/****************************************************************/

/*
   This is how case 10 (=%1010) looks like:

   x0 and x1 are inside the
   cliprect that is behind it

         x0  x1                  this is how the cliprect
 y0      nnnnn                   behind has to be split
         nnnnn
      ooonononooo     MinY          ooonnnnnooo
      oooononoooo                   ooonnnnnooo
      ooonononooo         CR_New1   ooonnnnnooo  CR_New2
      oooononoooo                   ooonnnnnooo
      ooonononooo     MaxY          ooonnnnnooo
         nnnnn
 y1      nnnnn                          CR

      M         M
      i         a
      n         x
      X         X

*/

struct ClipRect * Case_10(struct Rectangle * R,
                          struct ClipRect * CR,
                          struct BitMap * display_bm,
                          struct Layer * newlayer,
                          struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs        = CR->lobs;
      CR_New2->lobs        = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_X0-1;
    CR_New1->bounds.MaxY = DEF_MAXY;

    CR_New2->bounds.MinX = DEF_X1+1;
    CR_New2->bounds.MinY = DEF_MINY;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New2->Next = CR->Next;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same height, but different width
      */
      if (0 == (passivelayer ->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer -> Flags & LAYERSUPER))
        {
          /* the "left" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "middle" one (n's), ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_MINY, DEF_X1, DEF_MAXY, display_bm);
          /* the "right" one (o's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
    
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
           */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if not a simple layer */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    {
      /* the "middle" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (NULL == CR->BitMap && newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
             AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
          {
            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_X0 + 1 + 16,
               DEF_MAXY - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);
 
            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else
	  {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }


          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1 - DEF_X0 + 1,    /* SizeX */
              DEF_MAXY - DEF_MINY + 1,/* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* Change CR, it will contain the left part (o's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MaxX = DEF_X1;

  }

  return CR_New2;
}

/****************************************************************/

/*
   This is how case 11 (=%1011) looks like:

   x0, x1 and y1 are inside the
   cliprect that is behind it

         x0  x1                  this is how the cliprect
 y0      nnnnn                   behind has to be split
         nnnnn                           CR
      ooonononooo     MinY          ooonnnnnooo
      oooononoooo                   ooonnnnnooo
 y1   ooonononooo         CR_New1   ooonnnnnooo  CR_New2
      ooooooooooo                   OOOOOOOOOOO
      ooooooooooo     MaxY          OOOOOOOOOOO

                                        CR_New3

      M         M
      i         a
      n         x
      X         X

*/

struct ClipRect * Case_11(struct Rectangle * R,
                          struct ClipRect * CR,
                          struct BitMap * display_bm,
                          struct Layer * newlayer,
                          struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2, * CR_New3;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);
  CR_New3 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2 && NULL != CR_New3)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
      CR_New3->lobs = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_X0-1;
    CR_New1->bounds.MaxY = DEF_Y1;

    CR_New2->bounds.MinX = DEF_X1+1;
    CR_New2->bounds.MinY = DEF_MINY;
    CR_New2->bounds.MaxX = DEF_MAXX;
    CR_New2->bounds.MaxY = DEF_Y1;

    CR_New3->bounds.MinX = DEF_MINX;
    CR_New3->bounds.MinY = DEF_Y1+1;
    CR_New3->bounds.MaxX = DEF_MAXX;
    CR_New3->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New3->Next = CR->Next;
    CR_New2->Next = CR_New3;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer -> Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer -> Flags & LAYERSUPER))
        {
          /* the "upper left" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "upper middle " one (n's), Cliprect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_MINY, DEF_X1, DEF_Y1, display_bm);
          /* the "upper right" one (o's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* the "lower" one (O's) */
          DEF_DO_THE_BLIT(CR_New3, CR->lobs, display_bm);

          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
          CR_New3 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if (not a simple layer) */

      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;
    }
    else
    {
      /* the "upper middle" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
          {

            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_X0 + 1 + 16,
               DEF_Y1 - DEF_MINY + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;
            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else 
          {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = CR->bounds.MinY -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }


          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_MINY,               /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1 - DEF_X0   + 1,  /* SizeX */
              DEF_Y1 - DEF_MINY + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MaxX = DEF_X1;
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New3;
}

/****************************************************************/

/*
   This is how case 12 looks like:

   only x0 is inside the
   cliprect that is behind it

         x0     x1               this is how the cliprect
                                 behind has to be split
                                      CR_New1
      oooooooo        MinY          oooooooo
      oooooooo                      oooooooo
 y0   ooonononnnn           CR_New2 OOOnnnnn
      oooonononnn                   OOOnnnnn CR
      ooonononnnn     MaxY          OOOnnnnn
 y1      nnnnnnnn
         nnnnnnnn
      M      M
      i      a
      n      x
      X      X

*/


struct ClipRect * Case_12(struct Rectangle * R,
                          struct ClipRect * CR,
                          struct BitMap * display_bm,
                          struct Layer * newlayer,
                          struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */
  if (NULL != CR_New1 && NULL != CR_New2)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
      
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y0;
    CR_New2->bounds.MaxX = DEF_X0-1;
    CR_New2->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New2->Next = CR->Next;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same height, but different width
      */
      if (0 == (passivelayer -> Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer -> Flags & LAYERSUPER))
        {
          /* the "left" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "middle" one (n's), ClipRect CR will hold it */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_Y0, DEF_MAXX, DEF_MAXY, display_bm);
          /* the "right" one (o's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
          */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    {
      /* the "lower right" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
	  {

            CR ->BitMap = AllocBitMap(
               DEF_MAXX - DEF_X0 + 1 + 16,
               DEF_MAXY - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;
            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }


          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_X0 + 1,  /* SizeX */
              DEF_MAXY - DEF_Y0 + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* Change CR, it will contain the left part (o's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MinY = DEF_Y0;

  }

  return CR_New2;
}

/****************************************************************/

/*
   This is how case 13 (=%1101) looks like:

   x0,y0 and y1 are inside the
   cliprect that is behind it

           x0   x1               this is how the cliprect
                                 behind has to be split

       ooooooo     MinY             ooooooo CR_New1
   y0  ooooononnn                   OOOOnnn
       oooononnnn           CR_New2 OOOOnnn CR
   y1  ooooononnn                   OOOOnnn
       ooooooo     MaxY             ooooooo CR_New3



       M     M
       i     a
       n     x
       X     X

*/



struct ClipRect * Case_13(struct Rectangle * R,
                          struct ClipRect * CR,
                          struct BitMap * display_bm,
                          struct Layer * newlayer,
                          struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2, * CR_New3;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);
  CR_New3 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2 && NULL != CR_New3)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs        = CR->lobs;
      CR_New2->lobs        = CR->lobs;
      CR_New3->lobs        = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y0;
    CR_New2->bounds.MaxX = DEF_X0-1;
    CR_New2->bounds.MaxY = DEF_Y1;

    CR_New3->bounds.MinX = DEF_MINX;
    CR_New3->bounds.MinY = DEF_Y1+1;
    CR_New3->bounds.MaxX = DEF_MAXX;
    CR_New3->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New3->Next = CR->Next;
    CR_New2->Next = CR_New3;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer -> Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer -> Flags & LAYERSUPER))
        {
          /* the "upper" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "middle left" one (O's)*/
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* the "upper right" one (o's), Cliprect CR will hold it  */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_Y0, DEF_MAXX, DEF_Y1, display_bm);
          /* the "lower" one (o's) */
          DEF_DO_THE_BLIT(CR_New3, CR->lobs, display_bm);
    
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
           */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
          CR_New3 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    {
      /* the "middle right" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
     {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
	  { 

            CR ->BitMap = AllocBitMap(
               DEF_MAXX - DEF_X0 + 1 + 16,
               DEF_Y1 - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;

            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }


          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_MAXX - DEF_X0 + 1,  /* SizeX */
              DEF_Y1   - DEF_Y0 + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MinY = DEF_Y0;
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New3;
}

/****************************************************************/

/*
   This is how case 14 (=%1110) looks like:

   x0, y0 and x1 are inside the
   cliprect that is behind it

         x0  x1                  this is how the cliprect
                                 behind has to be split
                                      CR_New1
      ooooooooooo     MinY          ooooooooooo
      ooooooooooo                   ooooooooooo
 y0   ooonononooo         CR_New2   OOOnnnnnOOO  CR_New3
      oooononoooo                   OOOnnnnnOOO
      ooonononooo     MaxY          OOOnnnnnOOO
         nnnnn
 y1      nnnnn                          CR

      M         M
      i         a
      n         x
      X         X

*/

struct ClipRect * Case_14(struct Rectangle * R,
                          struct ClipRect * CR,
                          struct BitMap * display_bm,
                          struct Layer * newlayer,
                          struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2, * CR_New3;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);
  CR_New3 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's and the O's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2 && NULL != CR_New3)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
      CR_New3->lobs = CR->lobs;
    }

    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y0;
    CR_New2->bounds.MaxX = DEF_X0-1;
    CR_New2->bounds.MaxY = DEF_MAXY;

    CR_New3->bounds.MinX = DEF_X1+1;
    CR_New3->bounds.MinY = DEF_Y0;
    CR_New3->bounds.MaxX = DEF_MAXX;
    CR_New3->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New3->Next = CR->Next;
    CR_New2->Next = CR_New3;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same width, but different height
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* the "upper" one (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* the "lower left" one (O's)*/
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* the "lower middle" one (n's), Cliprect CR will hold it  */
          DEF_DO_THE_BLIT_CR(DEF_X0, DEF_Y0, DEF_X1, DEF_MAXY, display_bm);
          /* the "lower right" one (o's) */
          DEF_DO_THE_BLIT(CR_New3, CR->lobs, display_bm);
          /* dispose the old bitmap structure as everything
             that was backed up there is now in the two other
             bitmap structures
           */
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
          CR_New3 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if (not a simple layer) */ 

      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    {
      /* the "middle right" one (n's) */
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
          {

            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_X0 + 1 + 16,
               DEF_MAXY - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);
 
            /* out of memory */
            if (NULL == CR->BitMap)
              return NULL;
            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
	  }


          /* and back up the information for the part with the n's
             from the info found in the display bitmap.
          */
          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1   - DEF_X0 + 1,  /* SizeX */
              DEF_MAXY - DEF_Y0 + 1,  /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if not a simple layer */

        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }
    }

    /* leave this code down here, otherwise the DEFs will go wrong */

    /* Change CR, it will contain the upper left part (n's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MinY = DEF_Y0;
    CR->bounds.MaxX = DEF_X1;
  }

  return CR_New3;
}

/*
   This is how case 15 (=%1111) looks like:

   x0,x1,y0 and y1 is inside the
   cliprect that is behind it

       x0  x1                   CR_New1

     oooooooooo  MinY        oooooooooo
     oooooooooo              oooooooooo
 y0  oonononooo              OOnnnnnnOO
     ooonononoo     CR_New2  OOnnnnnnOO  CR_New3
 y1  oonononooo              OOnnnnnnOO
     oooooooooo              oooooooooo
     oooooooooo  MaxY        oooooooooo
                               CR_New4

     M        M
     i        a
     n        x
     X        X

*/





struct ClipRect * Case_15(struct Rectangle * R,
                          struct ClipRect * CR,
                          struct BitMap * display_bm,
                          struct Layer * newlayer,
                          struct Layer * passivelayer)
{
  struct ClipRect * CR_New1, * CR_New2, * CR_New3, * CR_New4;
  struct BitMap   * bm_old;

  /* Get a new structure for a 2nd and 3rd ClipRect */
  CR_New1 = _AllocClipRect(newlayer);
  CR_New2 = _AllocClipRect(newlayer);
  CR_New3 = _AllocClipRect(newlayer);
  CR_New4 = _AllocClipRect(newlayer);


  /* If the ClipRect with the o's already had some backed up pixels
     in a bitmap-structure, then we have to split up the info found
     there in a bitmap which contains the info about the n's and the
     other one about the o's.
     If there were no backed up pixels so far we have to back up
     the ones that are overlapped now. This is the case for the
     part with the n's.

  */

  if (NULL != CR_New1 && NULL != CR_New2 && NULL != CR_New3 &&
      NULL != CR_New4)
  {
    if (NULL != CR->lobs)
    {
      CR_New1->lobs = CR->lobs;
      CR_New2->lobs = CR->lobs;
      CR_New3->lobs = CR->lobs;
      CR_New4->lobs = CR->lobs;
    }
    /* initialize these structrues */

    /* CR_New1 for the upper one */
    CR_New1->bounds.MinX = DEF_MINX;
    CR_New1->bounds.MinY = DEF_MINY;
    CR_New1->bounds.MaxX = DEF_MAXX;
    CR_New1->bounds.MaxY = DEF_Y0-1;

    /* CR_New2 for the middle left one */
    CR_New2->bounds.MinX = DEF_MINX;
    CR_New2->bounds.MinY = DEF_Y0;
    CR_New2->bounds.MaxX = DEF_X0-1;
    CR_New2->bounds.MaxY = DEF_Y1;

    /* CR_New3 for the middle right one */
    CR_New3->bounds.MinX = DEF_X1+1;
    CR_New3->bounds.MinY = DEF_Y0;
    CR_New3->bounds.MaxX = DEF_MAXX;
    CR_New3->bounds.MaxY = DEF_Y1;

    /* CR_New4 for the lower one */
    CR_New4->bounds.MinX = DEF_MINX;
    CR_New4->bounds.MinY = DEF_Y1+1;
    CR_New4->bounds.MaxX = DEF_MAXX;
    CR_New4->bounds.MaxY = DEF_MAXY;

    /* The new ClipRects go behind the old one in the list */
    CR_New4->Next = CR->Next;
    CR_New3->Next = CR_New4;
    CR_New2->Next = CR_New3;
    CR_New1->Next = CR_New2;
    CR     ->Next = CR_New1;

    bm_old = CR->BitMap;
    if (NULL != bm_old)
    { /* get two new bitmap structures
         they have the same height, but different width
      */
      if (0 == (passivelayer->Flags & LAYERSIMPLE))
      {
        if (0 == (passivelayer->Flags & LAYERSUPER))
        {
          /* upper part (o's) */
          DEF_DO_THE_BLIT(CR_New1, CR->lobs, display_bm);
          /* middle left part (O's) */
          DEF_DO_THE_BLIT(CR_New2, CR->lobs, display_bm);
          /* middle part (n's) */
          DEF_DO_THE_BLIT_CR(DEF_X0,DEF_Y0,DEF_X1,DEF_Y1, display_bm);
          /* middle left part (O's) */
          DEF_DO_THE_BLIT(CR_New3, CR->lobs, display_bm);
          /* lower part (o's) */
          DEF_DO_THE_BLIT(CR_New4, CR->lobs, display_bm);
          /* 
            Dispose the old bitmap structure as everything
            that was backed up there is now in the two other
            bitmap structures
          */
       
          FreeBitMap(bm_old);
        }
        else
        {
          CR_New1 -> BitMap = passivelayer -> SuperBitMap;
          CR_New2 -> BitMap = passivelayer -> SuperBitMap;
          CR_New3 -> BitMap = passivelayer -> SuperBitMap;
          CR_New4 -> BitMap = passivelayer -> SuperBitMap;
        }
      } /* if (not a simple layer) */
      
      if (CR->lobs->priority < newlayer->priority)
        CR -> lobs = newlayer;

    }
    else
    { 
      /* get one new bitmap structure, if there is none. But only
         get it if the new layer is in front of the passive layer */ 
      if (newlayer->priority > passivelayer->priority)
      {
        if (0 == (passivelayer->Flags & LAYERSIMPLE))
        {
          ULONG AllocBitMapFlag = 0;
          struct BitMap * DestBM;
          LONG DestX, DestY;

          if ((CR->Flags & CR_NEEDS_NO_LAYERBLIT_DAMAGE) != 0)
            AllocBitMapFlag = BMF_CLEAR;

          if (0 == (passivelayer->Flags & LAYERSUPER))
	  {

            CR ->BitMap = AllocBitMap(
               DEF_X1 - DEF_X0 + 1 + 16,
               DEF_Y1 - DEF_Y0 + 1,
               display_bm->Depth,
               AllocBitMapFlag,
               display_bm);

            /* out of memory */
            if (NULL == CR->BitMap)
             return NULL;
 
            DestBM = CR->BitMap;
            DestX  = DEF_X0 & 0x0f;
            DestY  = 0;
          }
          else
          {
            /* it has a superbitmap */
            DestX  = DEF_X0 - 
                         passivelayer->bounds.MinX - passivelayer->Scroll_X;
            DestY  = DEF_Y0 -
                         passivelayer->bounds.MinY - passivelayer->Scroll_Y;
            DestBM = passivelayer->SuperBitMap;
            CR -> BitMap = DestBM;
         }

          /* 
             Back up the information for the part with the n's
             from the info found in the display bitmap, but only if
             it belongs to this cliprect.
          */

          if (0 == AllocBitMapFlag)
            BltBitMap(
              display_bm,             /* SrcBitMap = Display BitMap */
              DEF_X0,                 /* SrcX - no optimization !!! */
              DEF_Y0,                 /* SrcY */
              DestBM,                 /* DestBitMap */
              DestX,                  /* DestX - optimized */
              DestY,                  /* DestY */
              DEF_X1 - DEF_X0 + 1,    /* SizeX */
              DEF_Y1 - DEF_Y0 + 1,    /* SizeY */
              0x0c0,                  /* Vanilla Copy */
              0xff,                   /* Mask */
              NULL
            );
        } /* if (not a simple layer) */
        /* 
         * Only update the lobs entry in the CR if the lobs pointer is NULL
         * or if the newlayer is further in the front than the previous layer
         * hiding this cliprect.
         */
        if (NULL == CR->lobs || CR->lobs->priority < newlayer->priority )   
          CR -> lobs = newlayer;

      }      
    }

    /* Change CR, it will contain the left part (o's) */
    CR->bounds.MinX = DEF_X0;
    CR->bounds.MinY = DEF_Y0;
    CR->bounds.MaxX = DEF_X1;
    CR->bounds.MaxY = DEF_Y1;
  }

  return CR_New4;
}
