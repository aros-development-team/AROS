/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* !!!! ONLY USE THE BELOW MACROS IF YOU ARE 100% SURE 
   THAT IT IS A HIDD BITMAP AND NOT ONE THE USER
   HAS CREATED BY HAND !!!. You can use IS_HIDD_BM(bitmap) to test
   if it is a HIDD bitmap
*/

#define HIDD_BM_OBJ(bitmap)		((OOP_Object *)(bitmap)->Planes[0]) 
#define HIDD_BM_COLMAP(bitmap)		((OOP_Object *)(bitmap)->Planes[2])
#define HIDD_BM_COLMOD(bitmap)		((HIDDT_ColorModel)(bitmap)->Planes[3])
#define HIDD_BM_PIXTAB(bitmap)		((HIDDT_Pixel *)(bitmap)->Planes[4])
