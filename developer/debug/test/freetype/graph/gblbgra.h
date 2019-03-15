
  int                   h        = blit->height;
  const unsigned char*  src_line = blit->src_line;
  unsigned char*        dst_line = blit->dst_line;

  gblender_use_channels( blender, 0 );

  do
  {
    const unsigned char*  src = src_line + blit->src_x * 4;
    unsigned char*        dst = dst_line + blit->dst_x * GDST_INCR;
    int                   w   = blit->width;

    do
    {
      unsigned int  a  = GBLENDER_SHADE_INDEX(src[3]);
      unsigned int  ra = src[3];

      unsigned int  b = src[0];
      unsigned int  g = src[1];
      unsigned int  r = src[2];


      if ( a == 0 )
      {
        /* nothing */
      }
      else if ( a == 255 )
      {
        dst[0] = (unsigned char)r;
        dst[1] = (unsigned char)g;
        dst[2] = (unsigned char)b;
      }
      else
      {
        unsigned int  ba = 255 - ra;
        unsigned int  br = dst[0];
        unsigned int  bb = dst[1];
        unsigned int  bg = dst[2];


        dst[0] = (unsigned char)(br * ba / 255 + r);
        dst[1] = (unsigned char)(bg * ba / 255 + g);
        dst[2] = (unsigned char)(bb * ba / 255 + b);
      }

      src += 4;
      dst += GDST_INCR;

    } while ( --w > 0 );

    src_line += blit->src_pitch;
    dst_line += blit->dst_pitch;

  } while ( --h > 0 );

