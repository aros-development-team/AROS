#ifndef AROS_ISOASCII_H
#define AROS_ISOASCII_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ISO 8859-1 symbol escapes for use in string literals.
    Lang: english

    Embedding characters above 0x7F directly in source files makes the
    file encoding significant and triggers clang's
    -Winvalid-source-encoding warning. These macros expand to string
    literals containing the ISO 8859-1 code point via a hex escape, so
    sources stay plain ASCII. String literal concatenation splices them
    into surrounding text:

        MUIA_Application_Copyright, (IPTR)(ISOASCII_COPYRIGHT " 2026 Me")
*/

#define ISOASCII_NBSP           "\xA0"  /* no-break space              */
#define ISOASCII_INVEXCLAM      "\xA1"  /* inverted exclamation mark   */
#define ISOASCII_CENT           "\xA2"  /* cent sign                   */
#define ISOASCII_POUND          "\xA3"  /* pound sign                  */
#define ISOASCII_CURRENCY       "\xA4"  /* currency sign               */
#define ISOASCII_YEN            "\xA5"  /* yen sign                    */
#define ISOASCII_BROKENBAR      "\xA6"  /* broken bar                  */
#define ISOASCII_SECTION        "\xA7"  /* section sign                */
#define ISOASCII_COPYRIGHT      "\xA9"  /* copyright sign              */
#define ISOASCII_ORDFEMININE    "\xAA"  /* feminine ordinal indicator  */
#define ISOASCII_LGUILLEMET     "\xAB"  /* left angle quotation mark   */
#define ISOASCII_NOTSIGN        "\xAC"  /* not sign                    */
#define ISOASCII_SOFTHYPHEN     "\xAD"  /* soft hyphen                 */
#define ISOASCII_REGISTERED     "\xAE"  /* registered trade mark sign  */
#define ISOASCII_DEGREE         "\xB0"  /* degree sign                 */
#define ISOASCII_PLUSMINUS      "\xB1"  /* plus-minus sign             */
#define ISOASCII_SUPER2         "\xB2"  /* superscript two             */
#define ISOASCII_SUPER3         "\xB3"  /* superscript three           */
#define ISOASCII_MICRO          "\xB5"  /* micro sign                  */
#define ISOASCII_PARAGRAPH      "\xB6"  /* pilcrow (paragraph) sign    */
#define ISOASCII_MIDDLEDOT      "\xB7"  /* middle dot                  */
#define ISOASCII_SUPER1         "\xB9"  /* superscript one             */
#define ISOASCII_ORDMASCULINE   "\xBA"  /* masculine ordinal indicator */
#define ISOASCII_RGUILLEMET     "\xBB"  /* right angle quotation mark  */
#define ISOASCII_QUARTER        "\xBC"  /* vulgar fraction one quarter */
#define ISOASCII_HALF           "\xBD"  /* vulgar fraction one half    */
#define ISOASCII_THREEQUARTERS  "\xBE"  /* vulgar fraction 3 quarters  */
#define ISOASCII_INVQUESTION    "\xBF"  /* inverted question mark      */
#define ISOASCII_MULTIPLY       "\xD7"  /* multiplication sign         */
#define ISOASCII_DIVIDE         "\xF7"  /* division sign               */

#endif /* AROS_ISOASCII_H */
