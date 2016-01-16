
/*
**
**  $VER: mpegmotionvector.c 1.11 (30.10.97)
**  mpegvideo.datatype 1.11
**
**  This file handles computation of the forward/backward
**  motion vectors.
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

/* project includes */
#include "mpegvideo.h"
#include "mpegproto.h"
#include "mpegutil.h"


/*
 *--------------------------------------------------------------
 *
 * ComputeVector --
 *
 *    Computes motion vector given parameters previously parsed
 *      and reconstructed.
 *
 * Results:
 *      Reconstructed motion vector info is put into recon_* parameters
 *      passed to this function. Also updated previous motion vector
 *      information.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


#define ComputeVector( recon_right_ptr, recon_down_ptr, recon_right_prev, recon_down_prev, f, full_pel_vector, motion_h_code, motion_v_code, motion_h_r, motion_v_r ) \
{                                                                               \
    int comp_h_r, comp_v_r;                                                     \
    int right_little, right_big, down_little,  down_big;                        \
    int max, min, new_vector;                                                   \
                                                                                \
    /* The following procedure for the reconstruction of motion vectors         \
     * is a direct and simple implementation of the instructions given          \
     * in the mpeg December 1991 standard draft.                                \
     */                                                                         \
                                                                                \
    comp_h_r = ((f == 1) || (motion_h_code == 0))?(0):(f - 1 - motion_h_r);     \
    comp_v_r = ((f == 1) || (motion_v_code == 0))?(0):(f - 1 - motion_v_r);     \
                                                                                \
    right_little = motion_h_code * f;                                           \
                                                                                \
    if( right_little == 0 )                                                     \
    {                                                                           \
      right_big = 0;                                                            \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      if( right_little > 0 )                                                    \
      {                                                                         \
        right_little = right_little - comp_h_r;                                 \
        right_big    = right_little - (f << 5);                                 \
      }                                                                         \
      else                                                                      \
      {                                                                         \
        right_little = right_little + comp_h_r;                                 \
        right_big    = right_little + (f << 5);                                 \
      }                                                                         \
    }                                                                           \
                                                                                \
    down_little = motion_v_code * f;                                            \
                                                                                \
    if( down_little == 0 )                                                      \
    {                                                                           \
      down_big = 0;                                                             \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      if( down_little > 0 )                                                     \
      {                                                                         \
        down_little = down_little - comp_v_r;                                   \
        down_big    = down_little - (f << 5);                                   \
      }                                                                         \
      else                                                                      \
      {                                                                         \
        down_little = down_little + comp_v_r;                                   \
        down_big    = down_little + (f << 5);                                   \
      }                                                                         \
    }                                                                           \
                                                                                \
    max = (f << 4) - 1;                                                         \
    min = -(f << 4);                                                            \
                                                                                \
    new_vector = recon_right_prev + right_little;                               \
                                                                                \
    if( (new_vector <= max) && (new_vector >= min) )                            \
    {                                                                           \
      *recon_right_ptr = recon_right_prev + right_little; /* just new_vector */ \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      *recon_right_ptr = recon_right_prev + right_big;                          \
    }                                                                           \
                                                                                \
    recon_right_prev = *recon_right_ptr;                                        \
                                                                                \
    if( full_pel_vector )                                                       \
    {                                                                           \
      *recon_right_ptr = *recon_right_ptr << 1;                                 \
    }                                                                           \
                                                                                \
    new_vector = recon_down_prev + down_little;                                 \
                                                                                \
    if( (new_vector <= max) && (new_vector >= min) )                            \
    {                                                                           \
      *recon_down_ptr = recon_down_prev + down_little; /* just new_vector */    \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      *recon_down_ptr = recon_down_prev + down_big;                             \
    }                                                                           \
    recon_down_prev = *recon_down_ptr;                                          \
                                                                                \
    if( full_pel_vector )                                                       \
    {                                                                           \
      *recon_down_ptr = *recon_down_ptr << 1;                                   \
    }                                                                           \
}


/*
 *--------------------------------------------------------------
 *
 * ComputeForwVector --
 *
 *    Computes forward motion vector by calling ComputeVector
 *      with appropriate parameters.
 *
 * Results:
 *    Reconstructed motion vector placed in recon_right_for_ptr and
 *      recon_down_for_ptr.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void ComputeForwVector( struct MPEGVideoInstData *mvid, int *recon_right_for_ptr, int *recon_down_for_ptr )
{
  Pict       *picture;
  Macroblock *mblock;

  picture = (&(curVidStream -> picture));
  mblock  = (&(curVidStream -> mblock));

  ComputeVector( recon_right_for_ptr, recon_down_for_ptr,
                 (mblock -> recon_right_for_prev),
                 (mblock -> recon_down_for_prev),
                 (picture -> forw_f), (picture -> full_pel_forw_vector),
                 (mblock -> motion_h_forw_code), (mblock -> motion_v_forw_code),
                 (mblock -> motion_h_forw_r), (mblock -> motion_v_forw_r) );
}


/*
 *--------------------------------------------------------------
 *
 * ComputeBackVector --
 *
 *    Computes backward motion vector by calling ComputeVector
 *      with appropriate parameters.
 *
 * Results:
 *    Reconstructed motion vector placed in recon_right_back_ptr and
 *      recon_down_back_ptr.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


void ComputeBackVector( struct MPEGVideoInstData *mvid, int *recon_right_back_ptr, int *recon_down_back_ptr )
{
  Pict       *picture;
  Macroblock *mblock;

  picture = (&(curVidStream -> picture));
  mblock  = (&(curVidStream -> mblock));

  ComputeVector( recon_right_back_ptr, recon_down_back_ptr,
                 (mblock -> recon_right_back_prev),
                 (mblock -> recon_down_back_prev),
                 (picture -> back_f), (picture -> full_pel_back_vector),
                 (mblock -> motion_h_back_code), (mblock -> motion_v_back_code),
                 (mblock -> motion_h_back_r), (mblock -> motion_v_back_r) );

}


