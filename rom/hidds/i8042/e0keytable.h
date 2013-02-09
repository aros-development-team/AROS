/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#define NUM_E0KEYS 94

static const __attribute__((section(".text"))) WORD e0_keytable[NUM_E0KEYS] =
{
    NOKEY   	    , /* 000 (0x00) */
    NOKEY   	    , /* 001 (0x01) */
    NOKEY   	    , /* 002 (0x02) */
    NOKEY   	    , /* 003 (0x03) */
    NOKEY   	    , /* 004 (0x04) */
    NOKEY   	    , /* 005 (0x05) */
    NOKEY   	    , /* 006 (0x06) */
    NOKEY   	    , /* 007 (0x07) */
    NOKEY   	    , /* 008 (0x08) */
    NOKEY   	    , /* 009 (0x09) */
    NOKEY   	    , /* 010 (0x0A) */
    NOKEY   	    , /* 011 (0x0B) */
    NOKEY   	    , /* 012 (0x0C) */
    NOKEY   	    , /* 013 (0x0D) */
    NOKEY   	    , /* 014 (0x0E) */
    NOKEY   	    , /* 015 (0x0F) */
    NOKEY   	    , /* 016 (0x10) */
    NOKEY   	    , /* 017 (0x11) */
    NOKEY   	    , /* 018 (0x12) */
    NOKEY   	    , /* 019 (0x13) */
    NOKEY   	    , /* 020 (0x14) */
    NOKEY   	    , /* 021 (0x15) */
    NOKEY   	    , /* 022 (0x16) */
    NOKEY   	    , /* 023 (0x17) */
    NOKEY   	    , /* 024 (0x18) */
    NOKEY   	    , /* 025 (0x19) */
    NOKEY   	    , /* 026 (0x1A) */
    NOKEY   	    , /* 027 (0x1B) */
    RAWKEY_KP_ENTER , /* 028 (0x1C) K_KP_Enter */
    RAWKEY_CONTROL  , /* 029 (0x1D) K_RCtrl */
    NOKEY   	    , /* 030 (0x1E) */
    NOKEY   	    , /* 031 (0x1F) */
    NOKEY   	    , /* 032 (0x20) */
    NOKEY   	    , /* 033 (0x21) */
    NOKEY   	    , /* 034 (0x22) */
    NOKEY   	    , /* 035 (0x23) */
    NOKEY   	    , /* 036 (0x24) */
    NOKEY   	    , /* 037 (0x25) */
    NOKEY   	    , /* 038 (0x26) */
    NOKEY   	    , /* 039 (0x27) */
    NOKEY   	    , /* 040 (0x28) */
    NOKEY   	    , /* 041 (0x29) */
    NOKEY   	    , /* 042 (0x2A) */
    NOKEY   	    , /* 043 (0x2B) */
    NOKEY   	    , /* 044 (0x2C) */
    NOKEY   	    , /* 045 (0x2D) */
    NOKEY   	    , /* 046 (0x2E) */
    NOKEY   	    , /* 047 (0x2F) */
    NOKEY   	    , /* 048 (0x30) */
    NOKEY   	    , /* 049 (0x31) */
    NOKEY   	    , /* 050 (0x32) */
    NOKEY   	    , /* 051 (0x33) */
    NOKEY   	    , /* 052 (0x34) */
    0x5B,  /* 053 (0x35) K_KP_Divide */
    NOKEY   	    , /* 054 (0x36) */
    NOKEY   	    , /* 055 (0x37) */
    RAWKEY_RALT     ,  /* 056 (0x38) K_RAlt */
    NOKEY   	    , /* 057 (0x39) */
    NOKEY   	    , /* 058 (0x3A) */
    NOKEY   	    , /* 059 (0x3B) */
    NOKEY   	    , /* 060 (0x3C) */
    NOKEY   	    , /* 061 (0x3D) */
    NOKEY   	    , /* 062 (0x3E) */
    NOKEY   	    , /* 063 (0x3F) */
    NOKEY   	    , /* 064 (0x40) */
    NOKEY   	    , /* 065 (0x41) */
    NOKEY   	    , /* 066 (0x42) */
    NOKEY   	    , /* 067 (0x43) */
    NOKEY   	    , /* 068 (0x44) */
    NOKEY   	    , /* 069 (0x45) */
    NOKEY   	    , /* 070 (0x46) */
    RAWKEY_HOME     , /* 071 (0x47) K_Home */
    RAWKEY_UP	    , /* 072 (0x48) K_Up */
    RAWKEY_PAGEUP   , /* 073 (0x49) K_PgUp */
    NOKEY   	    , /* 074 (0x4A) */
    RAWKEY_LEFT     , /* 075 (0x4B) K_Left */
    NOKEY   	    , /* 076 (0x4C) */
    RAWKEY_RIGHT    , /* 077 (0x4D) K_Right */
    NOKEY   	    , /* 078 (0x4E) */
    RAWKEY_END	    , /* 079 (0x4F) K_End */
    RAWKEY_DOWN     , /* 080 (0x50) K_Down */
    RAWKEY_PAGEDOWN , /* 081 (0x51) K_PgDown */
    RAWKEY_INSERT   , /* 082 (0x52) K_Insert */
    RAWKEY_DELETE   , /* 083 (0x53) K_Del */
    NOKEY   	    , /* 084 (0x54) */
    NOKEY   	    , /* 085 (0x55) */
    NOKEY   	    , /* 086 (0x56) */
    NOKEY   	    , /* 087 (0x57) */
    NOKEY   	    , /* 088 (0x58) */
    NOKEY   	    , /* 089 (0x59) */
    NOKEY   	    , /* 090 (0x5A) */
    RAWKEY_LAMIGA   , /* 091 (0x5B) K_LMeta */
    RAWKEY_RAMIGA   , /* 092 (0x5C) K_RMeta */
    RAWKEY_RAMIGA     /* 093 (0x5D) K_Menu */
};
