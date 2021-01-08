/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Strings for the F1 key. In a real AmigaOS keymap, these would have come after
** the HiKeyMap, but we do it this way to avoid prototyping
**
** String descriptors are byte arrays and work like this:
**
** sizeofstring,offset_from_start_array_to_start_of_string
** sizeofstring,offset_from_start_array_to_start_of_string
** ..
** ..
** string1
** string2
** ..
** ..
**
** The number of strings depends on the qualifier flags 
** set in the keymap type.
*/

STATIC CONST UBYTE f1_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'0','~',
    0x9B,'1','0','~'
};

STATIC CONST UBYTE f2_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'1','~',
    0x9B,'1','1','~'
};

STATIC CONST UBYTE f3_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'2','~',
    0x9B,'1','2','~'
};

STATIC CONST UBYTE f4_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'3','~',
    0x9B,'1','3','~'
};

STATIC CONST UBYTE f5_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'4','~',
    0x9B,'1','4','~'
};

STATIC CONST UBYTE f6_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'5','~',
    0x9B,'1','5','~'
};

STATIC CONST UBYTE f7_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'6','~',
    0x9B,'1','6','~'
};

STATIC CONST UBYTE f8_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'7','~',
    0x9B,'1','7','~'
};

STATIC CONST UBYTE f9_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'8','~',
    0x9B,'1','8','~'
};

STATIC CONST UBYTE f10_descr[] =
{
    3,4,
    4,7,
    
    0x9B,'9','~',
    0x9B,'1','9','~'
};

STATIC CONST UBYTE f11_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'2','0','~',
    0x9B,'3','0','~'
};

STATIC CONST UBYTE f12_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'2','1','~',
    0x9B,'3','1','~'
};

STATIC CONST UBYTE insert_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','0','~',
    0x9B,'5','0','~'
};

STATIC CONST UBYTE pageup_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','1','~',
    0x9B,'5','1','~'
};

STATIC CONST UBYTE pagedown_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','2','~',
    0x9B,'5','2','~'
};

STATIC CONST UBYTE pausebreak_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','3','~',
    0x9B,'5','3','~'
};

STATIC CONST UBYTE home_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','4','~',
    0x9B,'5','4','~'
};

STATIC CONST UBYTE end_descr[] =
{
    4,4,
    4,8,
    
    0x9B,'4','5','~',
    0x9B,'5','5','~'
};

STATIC CONST UBYTE up_descr[] =
{
    2,4,
    2,6,
    
    0x9B,'A',
    0x9B,'T'
};

STATIC CONST UBYTE down_descr[] =
{
    2,4,
    2,6,
    
    0x9B,'B',
    0x9B,'S'
};

STATIC CONST UBYTE left_descr[] =
{
    2,4,
    3,6,
    
    0x9B,'D',
    0x9B,' ','A'
};

STATIC CONST UBYTE right_descr[] =
{
    2,4,
    3,6,
    
    0x9B,'C',
    0x9B,' ','@'
};

STATIC CONST UBYTE tab_descr[] =
{
    1,4,
    2,5,
    
    0x9,
    0x9B,'Z'
};

STATIC CONST UBYTE help_descr[] =
{
    3,2,
    
    0x9B,'?','~'
};
