/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library password hashing
*/
#ifndef _SECURITY_CRYPTO_H
#define _SECURITY_CRYPTO_H

#include <exec/types.h>
#include <libraries/security.h>

struct SecurityBase;

/* Length of an ACrypt hash (the classic MuFS format) */
#define secACRYPT_LEN           11

extern ULONG RegisterEncryptionHandler(struct SecurityBase *secBase, struct plugin_ops *ops);
extern ULONG UnRegisterEncryptionHandler(struct SecurityBase *secBase, struct plugin_ops *ops);

/* Hash 'pwd' with ACrypt using 'setting' as salt, into buffer[>= 12] */
extern STRPTR Encrypt(STRPTR buffer, CONST_STRPTR pwd, CONST_STRPTR setting);
/* Encrypt a password for storage in the database (buffer >= MaxPwdLen()+1) */
extern BOOL EncryptPassword(struct SecurityBase *secBase, STRPTR buffer, CONST_STRPTR userid, CONST_STRPTR pwd);
/* Maximum length of a stored hash */
extern ULONG MaxPwdLen(struct SecurityBase *secBase);
/* Is this string a hash we can verify? */
extern BOOL IsValidPasswordHash(struct SecurityBase *secBase, CONST_STRPTR hash);
/* Verify a supplied password against a stored hash */
extern BOOL verifypass(struct SecurityBase *secBase, CONST_STRPTR userid, CONST_STRPTR thepass, CONST_STRPTR suppliedpass);

#endif /* _SECURITY_CRYPTO_H */
