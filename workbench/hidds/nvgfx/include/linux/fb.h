/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_FB_H_
#define _LINUX_FB_H_

struct fb_info;
struct fb_var_screeninfo;
struct fb_fix_screeninfo;
struct fb_ops;
struct fb_videomode;
struct fb_monspecs;
#define KHZ2PICOS(a)            (1000000000UL/(a))
#define PICOS2KHZ(a)            (1000000000UL/(a))
#define FBINFO_DEFAULT          0
#define FB_BLANK_UNBLANK        0
#define FB_BLANK_NORMAL         1
#define FB_BLANK_POWERDOWN      4

#endif /* _LINUX_FB_H_ */
