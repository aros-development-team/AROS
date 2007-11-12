#ifndef CODESETS_INTERFACE_DEF_H
#define CODESETS_INTERFACE_DEF_H
/*
** This file is machine generated from idltool
** Do not edit
*/ 

#include <exec/types.i>
#include <exec/exec.i>
#include <exec/interfaces.i>

STRUCTURE CodesetsIFace, InterfaceData_SIZE
	    FPTR ICodesets_Obtain
	    FPTR ICodesets_Release
	    FPTR ICodesets_Expunge
	    FPTR ICodesets_Clone
	    FPTR ICodesets_Reserved1
	    FPTR ICodesets_CodesetsConvertUTF32toUTF16
	    FPTR ICodesets_CodesetsConvertUTF16toUTF32
	    FPTR ICodesets_CodesetsConvertUTF16toUTF8
	    FPTR ICodesets_CodesetsIsLegalUTF8
	    FPTR ICodesets_CodesetsIsLegalUTF8Sequence
	    FPTR ICodesets_CodesetsConvertUTF8toUTF16
	    FPTR ICodesets_CodesetsConvertUTF32toUTF8
	    FPTR ICodesets_CodesetsConvertUTF8toUTF32
	    FPTR ICodesets_CodesetsSetDefaultA
	    FPTR ICodesets_CodesetsSetDefault
	    FPTR ICodesets_CodesetsFreeA
	    FPTR ICodesets_CodesetsFree
	    FPTR ICodesets_CodesetsSupportedA
	    FPTR ICodesets_CodesetsSupported
	    FPTR ICodesets_CodesetsFindA
	    FPTR ICodesets_CodesetsFind
	    FPTR ICodesets_CodesetsFindBestA
	    FPTR ICodesets_CodesetsFindBest
	    FPTR ICodesets_CodesetsUTF8Len
	    FPTR ICodesets_CodesetsUTF8ToStrA
	    FPTR ICodesets_CodesetsUTF8ToStr
	    FPTR ICodesets_CodesetsUTF8CreateA
	    FPTR ICodesets_CodesetsUTF8Create
	    FPTR ICodesets_CodesetsEncodeB64A
	    FPTR ICodesets_CodesetsEncodeB64
	    FPTR ICodesets_CodesetsDecodeB64A
	    FPTR ICodesets_CodesetsDecodeB64
	    FPTR ICodesets_CodesetsStrLenA
	    FPTR ICodesets_CodesetsStrLen
	    FPTR ICodesets_CodesetsIsValidUTF8
	    FPTR ICodesets_CodesetsFreeVecPooledA
	    FPTR ICodesets_CodesetsFreeVecPooled
	    FPTR ICodesets_CodesetsConvertStrA
	    FPTR ICodesets_CodesetsConvertStr
	    FPTR ICodesets_CodesetsListCreateA
	    FPTR ICodesets_CodesetsListCreate
	    FPTR ICodesets_CodesetsListDeleteA
	    FPTR ICodesets_CodesetsListDelete
	    FPTR ICodesets_CodesetsListAddA
	    FPTR ICodesets_CodesetsListAdd
	    FPTR ICodesets_CodesetsListRemoveA
	    FPTR ICodesets_CodesetsListRemove
	LABEL CodesetsIFace_SIZE

#endif
