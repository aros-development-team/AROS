
  GBLENDER_CHANNEL_VARS;

  int                   h         = blit->height;
  const unsigned char*  src_line  = blit->src_line;
  int                   src_pitch = blit->src_pitch;
  unsigned char*        dst_line  = blit->dst_line;

  gblender_use_channels( blender, 1 );

  GBLENDER_CHANNEL_VARS_SET(blender,r,g,b);

  do
  {
    const unsigned char*  src = src_line + blit->src_x;
    unsigned char*        dst = dst_line + blit->dst_x*GDST_INCR;
    int                   w   = blit->width;

    do
    {
      unsigned int   ab = GBLENDER_SHADE_INDEX(src[0]);
      unsigned int   ag = GBLENDER_SHADE_INDEX(src[src_pitch]);
      unsigned int   ar = GBLENDER_SHADE_INDEX(src[src_pitch << 1]);
      GBlenderPixel  aa = ((GBlenderPixel)ar << 16) | (ag << 8) | ab;

      if ( aa == 0 )
      {
        /* nothing */
      }
      else if ( aa == (((GBLENDER_SHADE_COUNT-1) << 16) |
                       ((GBLENDER_SHADE_COUNT-1) << 8)  |
                        (GBLENDER_SHADE_COUNT-1)        ) )
      {
        GDST_COPY(dst);
      }
      else
      {
        GBlenderPixel  back;
        int            pix_r, pix_g, pix_b;

        GDST_READ(dst,back);

        {
          unsigned int  back_r = (back >> 16) & 255;

          GBLENDER_LOOKUP_R( blender, back_r );

          pix_r = _grcells[ar];
        }

        {
          unsigned int  back_g = (back >> 8) & 255;

          GBLENDER_LOOKUP_G( blender, back_g );

          pix_g = _ggcells[ag];
        }

        {
          unsigned int  back_b = (back) & 255;

          GBLENDER_LOOKUP_B( blender, back_b );

          pix_b = _gbcells[ab];
        }

        GDST_STOREC(dst,pix_r,pix_g,pix_b);
      }

      src += 1;
      dst += GDST_INCR;
    }
    while (--w > 0);

    src_line += blit->src_pitch*3;
    dst_line += blit->dst_pitch;
  }
  while (--h > 0);

  GBLENDER_CHANNEL_CLOSE(blender);
