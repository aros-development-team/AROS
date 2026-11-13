#ifndef HARDWARE_EFI_CONSOLE_H
#define HARDWARE_EFI_CONSOLE_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EFI firmware console interface
    Lang: english
*/

#include <exec/types.h>
#include <hardware/efi/tables.h>

struct EFI_InputKey
{
    UWORD Code;		/* Raw scan code     */
    UWORD Char;		/* Unicode character */
};

/* Simple text input interface */
struct EFI_SimpleIn
{
    __eficall SIPTR (*Reset)(struct EFI_SimpleIn *This, UBYTE Extended);
    __eficall SIPTR (*ReadKey)(struct EFI_SimpleIn *This, struct EFI_InputKey *Key);
    APTR WaitEvent;		/* Usable only by boot services */
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
    __eficall SIPTR (*Reset)(struct EFI_SimpleOut *This, UBYTE Extended);
    __eficall SIPTR (*OutString)(struct EFI_SimpleOut *This, UWORD *String);
    __eficall SIPTR (*TestString)(struct EFI_SimpleOut *This, UWORD *String);
    __eficall SIPTR (*QueryMode)(struct EFI_SimpleOut *This, IPTR Mode, IPTR *Cols, IPTR *Rows);
    __eficall SIPTR (*SetMode)(struct EFI_SimpleOut *This, IPTR Mode);
    __eficall SIPTR (*SetAttribute)(struct EFI_SimpleOut *This, IPTR Attr);
    __eficall SIPTR (*ClearScreen)(struct EFI_SimpleOut *This);
    __eficall SIPTR (*SetCursorPos)(struct EFI_SimpleOut *This, IPTR Col, IPTR Row);
    __eficall SIPTR (*EnableCursor)(struct EFI_SimpleOut *This, UBYTE Enable);

    struct EFI_TextMode *Mode;	/* Represents current settings */
};

#endif
