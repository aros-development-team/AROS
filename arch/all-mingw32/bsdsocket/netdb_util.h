char *CopyString(char *src, APTR pool);
struct protoent *CopyProtoEntry(struct PROTOENT *wsentry, APTR pool);
void FreeProtoEntry(struct protoent *entry, APTR pool);
