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

/* Accents */
#define ISOASCII_DIAERESIS      "\xA8"  /* diaeresis (umlaut) accent   */
#define ISOASCII_MACRON         "\xAF"  /* macron accent               */
#define ISOASCII_ACUTE          "\xB4"  /* acute accent                */
#define ISOASCII_CEDILLA        "\xB8"  /* cedilla accent              */

/* ISO 8859-1 letters (HTML entity naming; case of the name matches the case of the letter) */
#define ISOASCII_Agrave         "\xC0"  /* capital A grave             */
#define ISOASCII_Aacute         "\xC1"  /* capital A acute             */
#define ISOASCII_Acirc          "\xC2"  /* capital A circumflex        */
#define ISOASCII_Atilde         "\xC3"  /* capital A tilde             */
#define ISOASCII_Auml           "\xC4"  /* capital A diaeresis         */
#define ISOASCII_Aring          "\xC5"  /* capital A ring              */
#define ISOASCII_AElig          "\xC6"  /* capital AE ligature         */
#define ISOASCII_Ccedil         "\xC7"  /* capital C cedilla           */
#define ISOASCII_Egrave         "\xC8"  /* capital E grave             */
#define ISOASCII_Eacute         "\xC9"  /* capital E acute             */
#define ISOASCII_Ecirc          "\xCA"  /* capital E circumflex        */
#define ISOASCII_Euml           "\xCB"  /* capital E diaeresis         */
#define ISOASCII_Igrave         "\xCC"  /* capital I grave             */
#define ISOASCII_Iacute         "\xCD"  /* capital I acute             */
#define ISOASCII_Icirc          "\xCE"  /* capital I circumflex        */
#define ISOASCII_Iuml           "\xCF"  /* capital I diaeresis         */
#define ISOASCII_ETH            "\xD0"  /* capital Icelandic eth       */
#define ISOASCII_Ntilde         "\xD1"  /* capital N tilde             */
#define ISOASCII_Ograve         "\xD2"  /* capital O grave             */
#define ISOASCII_Oacute         "\xD3"  /* capital O acute             */
#define ISOASCII_Ocirc          "\xD4"  /* capital O circumflex        */
#define ISOASCII_Otilde         "\xD5"  /* capital O tilde             */
#define ISOASCII_Ouml           "\xD6"  /* capital O diaeresis         */
#define ISOASCII_Oslash         "\xD8"  /* capital O slash             */
#define ISOASCII_Ugrave         "\xD9"  /* capital U grave             */
#define ISOASCII_Uacute         "\xDA"  /* capital U acute             */
#define ISOASCII_Ucirc          "\xDB"  /* capital U circumflex        */
#define ISOASCII_Uuml           "\xDC"  /* capital U diaeresis         */
#define ISOASCII_Yacute         "\xDD"  /* capital Y acute             */
#define ISOASCII_THORN          "\xDE"  /* capital Icelandic thorn     */
#define ISOASCII_szlig          "\xDF"  /* small sharp s               */
#define ISOASCII_agrave         "\xE0"  /* small a grave               */
#define ISOASCII_aacute         "\xE1"  /* small a acute               */
#define ISOASCII_acirc          "\xE2"  /* small a circumflex          */
#define ISOASCII_atilde         "\xE3"  /* small a tilde               */
#define ISOASCII_auml           "\xE4"  /* small a diaeresis           */
#define ISOASCII_aring          "\xE5"  /* small a ring                */
#define ISOASCII_aelig          "\xE6"  /* small ae ligature           */
#define ISOASCII_ccedil         "\xE7"  /* small c cedilla             */
#define ISOASCII_egrave         "\xE8"  /* small e grave               */
#define ISOASCII_eacute         "\xE9"  /* small e acute               */
#define ISOASCII_ecirc          "\xEA"  /* small e circumflex          */
#define ISOASCII_euml           "\xEB"  /* small e diaeresis           */
#define ISOASCII_igrave         "\xEC"  /* small i grave               */
#define ISOASCII_iacute         "\xED"  /* small i acute               */
#define ISOASCII_icirc          "\xEE"  /* small i circumflex          */
#define ISOASCII_iuml           "\xEF"  /* small i diaeresis           */
#define ISOASCII_eth            "\xF0"  /* small Icelandic eth         */
#define ISOASCII_ntilde         "\xF1"  /* small n tilde               */
#define ISOASCII_ograve         "\xF2"  /* small o grave               */
#define ISOASCII_oacute         "\xF3"  /* small o acute               */
#define ISOASCII_ocirc          "\xF4"  /* small o circumflex          */
#define ISOASCII_otilde         "\xF5"  /* small o tilde               */
#define ISOASCII_ouml           "\xF6"  /* small o diaeresis           */
#define ISOASCII_oslash         "\xF8"  /* small o slash               */
#define ISOASCII_ugrave         "\xF9"  /* small u grave               */
#define ISOASCII_uacute         "\xFA"  /* small u acute               */
#define ISOASCII_ucirc          "\xFB"  /* small u circumflex          */
#define ISOASCII_uuml           "\xFC"  /* small u diaeresis           */
#define ISOASCII_yacute         "\xFD"  /* small y acute               */
#define ISOASCII_thorn          "\xFE"  /* small Icelandic thorn       */
#define ISOASCII_yuml           "\xFF"  /* small y diaeresis           */

#endif /* AROS_ISOASCII_H */
