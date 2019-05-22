
extern STRPTR Crypt(struct SecurityBase *secBase, STRPTR buffer, STRPTR key, STRPTR setting);
extern STRPTR Encrypt(STRPTR buffer, STRPTR pwd, STRPTR userid);
extern ULONG MaxPwdLen(struct SecurityBase *secBase);
extern BOOL verifypass(struct SecurityBase *secBase, STRPTR userid, STRPTR thepass, STRPTR suppliedpass);
