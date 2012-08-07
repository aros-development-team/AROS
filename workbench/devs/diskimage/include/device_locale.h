#ifndef DEVICE_LOCALE_H
#define DEVICE_LOCALE_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
#ifndef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#endif
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif

#ifdef CATCOMP_BLOCK
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_OK_GAD 0
#define MSG_CANCEL_GAD 1
#define MSG_PASSWORD_REQ 2
#define MSG_NOZLIB 3
#define MSG_NOCAPSDEV 4
#define MSG_CANCELED 5
#define MSG_NOPASSWD 6
#define MSG_WRONGPASSWD 7
#define MSG_ZLIBERR 8
#define MSG_CAPSERR 9
#define MSG_XPKERR 10
#define MSG_XADERR 11
#define MSG_UNKNDISKIMGTYPE 12
#define MSG_UNKNCOMPMETHOD 13
#define MSG_EOF 14
#define MSG_BADDATA 15
#define MSG_BADCRC 16
#define MSG_BADCHECKSUM 17
#define MSG_REQ 18
#define MSG_REQVER 19
#define MSG_WRONGDAA 20
#define MSG_EXPATERR 21
#define MSG_BZLIBERR 22
#define MSG_UNKNENCRMETHOD 23

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_OK_GAD_STR "_Ok"
#define MSG_CANCEL_GAD_STR "_Cancel"
#define MSG_PASSWORD_REQ_STR "Enter password:"
#define MSG_NOZLIB_STR "z.library required"
#define MSG_NOCAPSDEV_STR "capsimage.device required"
#define MSG_CANCELED_STR "operation canceled"
#define MSG_NOPASSWD_STR "password required"
#define MSG_WRONGPASSWD_STR "wrong password"
#define MSG_ZLIBERR_STR "z.library error"
#define MSG_CAPSERR_STR "capsimage.device error"
#define MSG_XPKERR_STR "xpkmaster.library error"
#define MSG_XADERR_STR "xadmaster.library error"
#define MSG_UNKNDISKIMGTYPE_STR "unsupported disk image type/format"
#define MSG_UNKNCOMPMETHOD_STR "unsupported compression method"
#define MSG_EOF_STR "unexpected end of file"
#define MSG_BADDATA_STR "file contains bad data"
#define MSG_BADCRC_STR "CRC check on file data failed"
#define MSG_BADCHECKSUM_STR "checksum error"
#define MSG_REQ_STR "%s required"
#define MSG_REQVER_STR "%s v%ld or newer required"
#define MSG_WRONGDAA_STR "wrong DAA file (multi-file DAA)"
#define MSG_EXPATERR_STR "expat.library error"
#define MSG_BZLIBERR_STR "bz2.library error"
#define MSG_UNKNENCRMETHOD_STR "unsupported encryption method"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
    LONG         cca_ID;
    CONST_STRPTR cca_Str;
};

STATIC CONST struct CatCompArrayType CatCompArray[] =
{
    {MSG_OK_GAD,(CONST_STRPTR)MSG_OK_GAD_STR},
    {MSG_CANCEL_GAD,(CONST_STRPTR)MSG_CANCEL_GAD_STR},
    {MSG_PASSWORD_REQ,(CONST_STRPTR)MSG_PASSWORD_REQ_STR},
    {MSG_NOZLIB,(CONST_STRPTR)MSG_NOZLIB_STR},
    {MSG_NOCAPSDEV,(CONST_STRPTR)MSG_NOCAPSDEV_STR},
    {MSG_CANCELED,(CONST_STRPTR)MSG_CANCELED_STR},
    {MSG_NOPASSWD,(CONST_STRPTR)MSG_NOPASSWD_STR},
    {MSG_WRONGPASSWD,(CONST_STRPTR)MSG_WRONGPASSWD_STR},
    {MSG_ZLIBERR,(CONST_STRPTR)MSG_ZLIBERR_STR},
    {MSG_CAPSERR,(CONST_STRPTR)MSG_CAPSERR_STR},
    {MSG_XPKERR,(CONST_STRPTR)MSG_XPKERR_STR},
    {MSG_XADERR,(CONST_STRPTR)MSG_XADERR_STR},
    {MSG_UNKNDISKIMGTYPE,(CONST_STRPTR)MSG_UNKNDISKIMGTYPE_STR},
    {MSG_UNKNCOMPMETHOD,(CONST_STRPTR)MSG_UNKNCOMPMETHOD_STR},
    {MSG_EOF,(CONST_STRPTR)MSG_EOF_STR},
    {MSG_BADDATA,(CONST_STRPTR)MSG_BADDATA_STR},
    {MSG_BADCRC,(CONST_STRPTR)MSG_BADCRC_STR},
    {MSG_BADCHECKSUM,(CONST_STRPTR)MSG_BADCHECKSUM_STR},
    {MSG_REQ,(CONST_STRPTR)MSG_REQ_STR},
    {MSG_REQVER,(CONST_STRPTR)MSG_REQVER_STR},
    {MSG_WRONGDAA,(CONST_STRPTR)MSG_WRONGDAA_STR},
    {MSG_EXPATERR,(CONST_STRPTR)MSG_EXPATERR_STR},
    {MSG_BZLIBERR,(CONST_STRPTR)MSG_BZLIBERR_STR},
    {MSG_UNKNENCRMETHOD,(CONST_STRPTR)MSG_UNKNENCRMETHOD_STR},
};

#endif /* CATCOMP_ARRAY */


/****************************************************************************/


#ifdef CATCOMP_BLOCK

STATIC CONST UBYTE CatCompBlock[] =
{
    "\x00\x00\x00\x00\x00\x04"
    MSG_OK_GAD_STR "\x00"
    "\x00\x00\x00\x01\x00\x08"
    MSG_CANCEL_GAD_STR "\x00"
    "\x00\x00\x00\x02\x00\x10"
    MSG_PASSWORD_REQ_STR "\x00"
    "\x00\x00\x00\x03\x00\x14"
    MSG_NOZLIB_STR "\x00\x00"
    "\x00\x00\x00\x04\x00\x1A"
    MSG_NOCAPSDEV_STR "\x00"
    "\x00\x00\x00\x05\x00\x14"
    MSG_CANCELED_STR "\x00\x00"
    "\x00\x00\x00\x06\x00\x12"
    MSG_NOPASSWD_STR "\x00"
    "\x00\x00\x00\x07\x00\x10"
    MSG_WRONGPASSWD_STR "\x00\x00"
    "\x00\x00\x00\x08\x00\x10"
    MSG_ZLIBERR_STR "\x00"
    "\x00\x00\x00\x09\x00\x18"
    MSG_CAPSERR_STR "\x00\x00"
    "\x00\x00\x00\x0A\x00\x18"
    MSG_XPKERR_STR "\x00"
    "\x00\x00\x00\x0B\x00\x18"
    MSG_XADERR_STR "\x00"
    "\x00\x00\x00\x0C\x00\x24"
    MSG_UNKNDISKIMGTYPE_STR "\x00\x00"
    "\x00\x00\x00\x0D\x00\x20"
    MSG_UNKNCOMPMETHOD_STR "\x00\x00"
    "\x00\x00\x00\x0E\x00\x18"
    MSG_EOF_STR "\x00\x00"
    "\x00\x00\x00\x0F\x00\x18"
    MSG_BADDATA_STR "\x00\x00"
    "\x00\x00\x00\x10\x00\x1E"
    MSG_BADCRC_STR "\x00"
    "\x00\x00\x00\x11\x00\x10"
    MSG_BADCHECKSUM_STR "\x00\x00"
    "\x00\x00\x00\x12\x00\x0C"
    MSG_REQ_STR "\x00"
    "\x00\x00\x00\x13\x00\x1A"
    MSG_REQVER_STR "\x00"
    "\x00\x00\x00\x14\x00\x20"
    MSG_WRONGDAA_STR "\x00"
    "\x00\x00\x00\x15\x00\x14"
    MSG_EXPATERR_STR "\x00"
    "\x00\x00\x00\x16\x00\x12"
    MSG_BZLIBERR_STR "\x00"
    "\x00\x00\x00\x17\x00\x1E"
    MSG_UNKNENCRMETHOD_STR "\x00"
};

#endif /* CATCOMP_BLOCK */


/****************************************************************************/



#endif /* DEVICE_LOCALE_H */
