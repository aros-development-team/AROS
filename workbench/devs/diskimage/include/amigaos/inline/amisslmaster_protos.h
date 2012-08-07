#ifndef _VBCCINLINE_AMISSLMASTER_H
#define _VBCCINLINE_AMISSLMASTER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

LONG __InitAmiSSLMaster(__reg("a6") struct Library *, __reg("d0") LONG APIVersion, __reg("d1") LONG AllowUserStructs)="\tjsr\t-30(a6)";
#define InitAmiSSLMaster(APIVersion, AllowUserStructs) __InitAmiSSLMaster(AmiSSLMasterBase, (APIVersion), (AllowUserStructs))

struct Library * __OpenAmiSSL(__reg("a6") struct Library *)="\tjsr\t-36(a6)";
#define OpenAmiSSL() __OpenAmiSSL(AmiSSLMasterBase)

void __CloseAmiSSL(__reg("a6") struct Library *)="\tjsr\t-42(a6)";
#define CloseAmiSSL() __CloseAmiSSL(AmiSSLMasterBase)

struct Library * __OpenAmiSSLCipher(__reg("a6") struct Library *, __reg("d0") LONG Cipher)="\tjsr\t-48(a6)";
#define OpenAmiSSLCipher(Cipher) __OpenAmiSSLCipher(AmiSSLMasterBase, (Cipher))

void __CloseAmiSSLCipher(__reg("a6") struct Library *, __reg("a0") struct Library * CipherBase)="\tjsr\t-54(a6)";
#define CloseAmiSSLCipher(CipherBase) __CloseAmiSSLCipher(AmiSSLMasterBase, (CipherBase))

#endif /*  _VBCCINLINE_AMISSLMASTER_H  */
