/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function DoCollision()
    Lang: english
*/
#include <graphics/rastport.h>
#include <graphics/gels.h>

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
            int dy = _CurVSprite->Y - CurVSprite->Y;
            UWORD *  collmask =  CurVSprite->CollMask;
            UWORD * _collmask = _CurVSprite->CollMask;
            unsigned short  wordsperline; 
            unsigned short _wordsperline;
            int i = 0, _i = 0;
            int offset    , _offset = 0;
            int _scroll = 0;
            int line = 0;
            /*
             * I will hold the CurVSprite's collision mask still
             * and adjust/shift the collision mask of _CurVSprite.
             * If any and'ing results with non-zero, there is a
             * collision.
             */
            if (CurVSprite->Flags & VSPRITE)
              wordsperline = CurVSprite->Width >> 4; // should be 1
            else
              wordsperline = CurVSprite->Width;

            if (_CurVSprite->Flags & VSPRITE)
              _wordsperline = _CurVSprite->Width >> 4; // should be 1
            else
              _wordsperline = _CurVSprite->Width;
            
            if (dy > 0)
            {
               /*
                * _CurVSprite is lower than CurVSprite
                */
               i =  wordsperline * dy;
               line = _CurVSprite->Y;
            }
            else
            {
              _i = _wordsperline * dy;
              line = CurVSprite->Y;
            }
            
            offset = _CurVSprite->X - CurVSprite->X;
            if (offset < 0)
            {
              _offset = (-offset) >> 4;
              _scroll = (-offset) & 0xf;              
              offset = 0;
            }
            else
            {
              /*
               * _CurVSprite is further to the right than 
               * CurVSprite.
               */
              _scroll = -(offset & 0xf);
              offset >>= 4;
            }
            
            /*
             * _scroll > 0 means shift the bits to the RIGHT!
             */
            while ((line < ( CurVSprite->Height+ CurVSprite->Y)) &&
                   (line < (_CurVSprite->Height+_CurVSprite->Y))   )
            {
              int  curindex =  offset;
              int _curindex = _offset;
              while ( curindex <  wordsperline &&
                     _curindex < _wordsperline)
              {
                if (_scroll > 0)
                {
                  if (collmask[curindex+i] & (_collmask[_curindex+_i] >> _scroll) )
                  {
                    collision = TRUE;
                    break;
                  }
                  curindex++;
                  if (curindex > wordsperline)
                    break;
                  
                  if (collmask[curindex+i] & (_collmask[_curindex+_i] << (16-_scroll)))
                  {
                    collision = TRUE;
                    break;
                  }  
                  _curindex++;
                }
                else
                if (0 == _scroll)
                {
                  if (collmask[curindex+i] & _collmask[_curindex+_i])
                  {
                    collision = TRUE;
                    break;
                  }
                  _curindex++;
                   curindex++;
                }
                else
                {
                  if (collmask[curindex+i] & (_collmask[_curindex+_i] << (-_scroll)))
                  {
                    collision = TRUE;
                    break;
                  }
                  _curindex++;
                  if (_curindex > _wordsperline)
                    break;
                  
                  if (collmask[curindex+i] & (_collmask[_curindex+_i] >> (16+_scroll)))
                  {
                    collision = TRUE;
                    break;
                  }  
                  curindex++;
                }
              }
               i+= wordsperline;
              _i+=_wordsperline;
              line++;
            }

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
