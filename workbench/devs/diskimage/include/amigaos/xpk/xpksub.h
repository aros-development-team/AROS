#ifndef XPK_XPKSUB_H
#define XPK_XPKSUB_H

/*
**	$VER: xpk/xpksub.h 4.10 (05.04.97) by SDI
**
**	(C) Copyright 1991-1996 by 
**          Urban Dominik Mueller, Bryan Ford,
**          Christian Schneider, Christian von Roques,
**	    Dirk Stöcker
**	    All Rights Reserved
*/

#ifndef XPK_XPK_H
#include <xpk/xpk.h>
#endif

/**************************************************************************
 *
 *                     The XpkInfo structure
 *
 */

/* Sublibs return this structure to xpkmaster when asked nicely
 * This is version 1 of XpkInfo. It's not #define'd because we don't want
 * it changing automatically with recompiles - you've got to actually update
 * your code when it changes. */
struct XpkInfo {
	UWORD   xi_XpkInfoVersion  ; /* Version number of this structure        */
	UWORD   xi_LibVersion      ; /* The version of this sublibrary          */
	UWORD   xi_MasterVersion   ; /* The required master lib version         */
	UWORD   xi_ModesVersion    ; /* Version number of mode descriptors      */
	STRPTR  xi_Name            ; /* Brief name of the packer, 20 char max   */
	STRPTR  xi_LongName        ; /* Full name of the packer   30 char max   */
	STRPTR  xi_Description     ; /* Short packer desc., 70 char max         */
	ULONG   xi_ID              ; /* ID the packer goes by (XPK format)      */
	ULONG	xi_Flags           ; /* Defined below                           */
	ULONG	xi_MaxPkInChunk    ; /* Max input chunk size for packing        */
	ULONG	xi_MinPkInChunk    ; /* Min input chunk size for packing        */
	ULONG	xi_DefPkInChunk    ; /* Default packing chunk size              */
	STRPTR  xi_PackMsg         ; /* Packing message, present tense          */
	STRPTR  xi_UnpackMsg       ; /* Unpacking message, present tense        */
	STRPTR  xi_PackedMsg       ; /* Packing message, past tense             */
	STRPTR  xi_UnpackedMsg     ; /* Unpacking message, past tense           */
	UWORD   xi_DefMode         ; /* Default mode number                     */
	UWORD   xi_Pad             ; /* for future use                          */
	struct XpkMode *xi_ModeDesc; /* List of individual descriptors          */
	ULONG   xi_Reserved[6]     ; /* Future expansion - set to zero          */
};

/* defines for Flags: see xpk.h, XPKIF_xxxxx */

/**************************************************************************
 *
 *                     The XpkSubParams structure
 *
 */

struct XpkSubParams {
	APTR	xsp_InBuf	; /* The input data               */
	ULONG	xsp_InLen	; /* The number of bytes to pack  */
	APTR	xsp_OutBuf	; /* The output buffer            */
	ULONG	xsp_OutBufLen	; /* The length of the output buf */
	ULONG	xsp_OutLen	; /* Number of bytes written      */
	ULONG	xsp_Flags	; /* Flags for master/sub comm.   */
	ULONG	xsp_Number	; /* The number of this chunk     */
	ULONG	xsp_Mode	; /* The packing mode to use      */
	STRPTR	xsp_Password	; /* The password to use          */
	UWORD	xsp_LibVersion	; /* SublibVersion used to pack   */
	UWORD	xsp_Pad		; /* Reserved; don't use          */
	ULONG	xsp_Arg[3]	; /* Reserved; don't use          */
	ULONG	xsp_Sub[4]	; /* Sublib private data          */
};

/*
 * xsp_LibVersion is the version number of the sublibrary used to pack
 * this chunk. It can be used to create backwards compatible sublibraries
 * with a totally different fileformat.
 */

#define XSF_STEPDOWN   1  /* May reduce pack eff. to save mem   */
#define XSF_PREVCHUNK  2  /* Previous chunk available on unpack */

#endif /* XPK_XPKSUB_H */
