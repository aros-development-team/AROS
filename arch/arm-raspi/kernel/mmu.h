/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define ENABLE_MMU              (1 << 0)
#define ENABLE_D_CACHE          (1 << 2)
#define ENABLE_I_CACHE          (1 << 12)

#define PAGE_TRANSLATIONFAULT   0
#define PAGE_SECONDLEVEL        (1 << 0)
#define PAGE_SECTION            (1 << 1)
#define PAGE_SUPERSECTION       (1 << 18) | PAGE_SECTION

#define PAGE_B_BIT              (1 << 2)
#define PAGE_C_BIT              (1 << 3)
#define PAGE_XN_BIT             (1 << 4)
#define PAGE_DOM_SHIFT          5
//#define PAGE_P_BIT              (1 << 9)
#define PAGE_FL_AP_SHIFT           10
#define PAGE_TEX_SHIFT          12
#define PAGE_FL_APX_BIT         (1 << 10)
#define PAGE_FL_S_BIT           (1 << 16)
#define PAGE_NG_BIT             (1 << 15)
#define PAGE_NS_BIT             (1 << 19)

#define PAGE_SL_AP_SHIFT        4
#define PAGE_SL_APX_BIT         (1 << 9)
#define PAGE_SL_S_BIT           (1 << 10)
