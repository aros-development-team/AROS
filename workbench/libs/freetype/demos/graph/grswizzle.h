#ifndef __gr_swizzle_h__
#define __gr_swizzle_h__

extern void
gr_swizzle_rect_rgb24( unsigned char*    read_buff,
                       int               read_pitch,
                       unsigned char*    write_buff,
                       int               write_pitch,
                       int               buff_width,
                       int               buff_height,
                       int               x,
                       int               y,
                       int               width,
                       int               height );

extern void
gr_swizzle_rect_rgb565( unsigned char*    read_buff,
                        int               read_pitch,
                        unsigned char*    write_buff,
                        int               write_pitch,
                        int               buff_width,
                        int               buff_height,
                        int               x,
                        int               y,
                        int               width,
                        int               height );

extern void
gr_swizzle_rect_xrgb32( unsigned char*    read_buff,
                        int               read_pitch,
                        unsigned char*    write_buff,
                        int               write_pitch,
                        int               buff_width,
                        int               buff_height,
                        int               x,
                        int               y,
                        int               width,
                        int               height );

#endif /* __gr_swizzle_h__ */
