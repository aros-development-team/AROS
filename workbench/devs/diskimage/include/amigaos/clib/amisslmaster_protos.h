#ifndef CLIB_AMISSLMASTER_PROTOS_H
#define CLIB_AMISSLMASTER_PROTOS_H

LONG InitAmiSSLMaster(LONG APIVersion,LONG UsesOpenSSLStructs);
struct Library *OpenAmiSSL(void);
void CloseAmiSSL(void);
struct Library *OpenAmiSSLCipher(LONG Cipher);
void CloseAmiSSLCipher(struct Library *CipherBase);

#endif /* CLIB_AMISSLMASTER_PROTOS_H */
