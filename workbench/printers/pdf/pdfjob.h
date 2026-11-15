#ifndef PDFJOB_H
#define PDFJOB_H

/*
 * Copyright (C) 2026, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Job file passed from the PDF printer driver to C:PDFRender.
 * Header "APDFJOB1", then records of an opcode byte and big endian ULONGs.
 * Coordinates are 1/100 point from the top left of the page.
 *
 *  P  width height                         start a page
 *  T  x y size style char(UBYTE)           one character (style 1 bold,
 *                                          2 italic, 4 underline)
 *  I  x y width height pixw pixh channels  image, followed by its rows
 *  E                                       end the page
 *  Z                                       end the document
 */

#define PDFJOB_MAGIC        "APDFJOB1"
#define PDFJOB_MAX_IMAGE    (32UL * 1024 * 1024)
#define PDFJOB_MAX_MEMORY   (128UL * 1024 * 1024)
#define PDFJOB_MAX_SPOOL    (256UL * 1024 * 1024)

#endif /* PDFJOB_H */
