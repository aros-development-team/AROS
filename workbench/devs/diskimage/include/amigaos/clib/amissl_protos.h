#ifndef CLIB_AMISSL_PROTOS_H
#define CLIB_AMISSL_PROTOS_H

#include <dos/dos.h>
#include <utility/tagitem.h>
#include <stdlib.h>
#include <amissl/amissl.h>

struct AmiSSLInitStruct;

void InternalInitAmiSSL(struct AmiSSLInitStruct *amisslinit);
long InitAmiSSLA(struct TagItem *tagList);
long InitAmiSSL(Tag tag1, ...);
long CleanupAmiSSLA(struct TagItem *tagList);
long CleanupAmiSSL(Tag tag1, ...);
long IsCipherAvailable(long cipher);
int UI_read_string_lib(UI *ui, UI_STRING *uis);
int UI_write_string_lib(UI *ui, UI_STRING *uis);

#endif /* !CLIB_AMISSL_PROTOS_H */
