#ifndef HARDWARE_EFI_CONSOLE_H
#define HARDWARE_EFI_CONSOLE_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: EFI firmware console interface
    Lang: english
*/

#include <exec/types.h>

struct EFI_InputKey
{
    UWORD Code;		/* Raw scan code     */
    UWORD Char;		/* Unicode character */
};

/* Simple text input interface */
struct EFI_SimpleIn
{
    SIPTR (*Reset)(struct EFI_SimpleInput *This, UBYTE Extended);
    SIPTR (*ReadKey)(struct EFI_SimpleInput *This, struct EFI_InputKey *Key);
    APTR    WaitEvent;								/* 'Wait for key' event descriptor */
};

struct EFI_TextMode
{
    LONG MaxMode;		/* Maximum mode number			 */
    LONG Mode;			/* Current mode number			 */
    LONG Attribute;		/* Text and background color (EGA-alike) */
    LONG Col;			/* Current output coordinates		 */
    LONG Row;
    UBYTE CursorVisible;	/* Boolean value			 */
};

/* Simple text output interface */
struct EFI_SimpleOut
{
    SIPTR (*Reset)(struct EFI_SimpleInput *This, UBYTE Extended);
    SIPTR (*OutString)(struct EFI_SimpleInput *This, UWORD *String);
    SIPTR (*TestString)(struct EFI_SimpleInput *This, UWORD *String);
    SIPTR (*QueryMode)(struct EFI_SimpleInput *This, IPTR Mode, IPTR *Cols, IPTR *Rows);
    SIPTR (*SetMode)(struct EFI_SimpleInput *This, IPTR Mode);
    SIPTR (*SetAttribute)(struct EFI_SimpleInput *This, IPTR Attr);
    SIPTR (*ClearScreen)(struct EFI_SimpleInput *This);
    SIPTR (*SetCursorPos)(struct EFI_SimpleInput *This, IPTR Col, IPTR Row);
    SIPTR (*EnableCursor)(struct EFI_SimpleInput *This, UBYTE Enable);
    struct EFI_TextMode *Mode;
};

#endif
