/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function DoCollision()
    Lang: english
*/
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, DoCollision,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 18, Graphics)

/*  FUNCTION

    INPUTS
        rp - pointer to RastPort

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct VSprite * CurVSprite, * _CurVSprite;
    int shift, _shift;
    
    if (NULL != rp->GelsInfo)
    {
      CurVSprite = rp->GelsInfo->gelHead->NextVSprite;
      if (CurVSprite->Flags & VSPRITE)
        shift = 1;
      else
        shift = 4;
      
      while (NULL != CurVSprite->NextVSprite)
      {
        _CurVSprite = CurVSprite->NextVSprite;
        
        /*
         * As long as they can overlap vertically..
         */
        while (NULL != _CurVSprite &&
               (CurVSprite->Y + CurVSprite->Height - 1) >= _CurVSprite->Y)
        {
          /*
           * Do these two overlap horizontally ???
           */
          if (_CurVSprite->Flags & VSPRITE)
            _shift = 1;
          else
            _shift = 4;
            
          if (!
               ((CurVSprite->X>_CurVSprite->X+(_CurVSprite->Width<<_shift)-1) ||
                (CurVSprite->X+(CurVSprite->Width<<shift)-1<_CurVSprite->X))
             )
          {
            /*
             * Must test the collision masks!!
             * Phew, this is not going to be easy.
             */
            int collision = FALSE;

#warning Not comparing collision masks!!!!!!!!!
            // put collision mask comparison here!
            collision = TRUE;
            
            if (TRUE == collision)
            {
              UWORD mask = CurVSprite->MeMask & _CurVSprite->HitMask;
              int i = 0;
              while (i < 16 && 0 != mask)
              {
                if (mask & 0x01)
                {
                  if (rp->GelsInfo->collHandler &&
                      rp->GelsInfo->collHandler->collPtrs[i])
                    rp->GelsInfo->collHandler->collPtrs[i]( CurVSprite,
                                                           _CurVSprite);
                  break;
                }
                i++;
                mask >>= 1;
              }
            }
          }
          
          _CurVSprite = _CurVSprite->NextVSprite;
        }
        CurVSprite = CurVSprite->NextVSprite;
      }
    }

    AROS_LIBFUNC_EXIT
} /* DoCollision */
