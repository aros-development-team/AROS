#ifndef AMISSL_TAGS_H
#define AMISSL_TAGS_H

#include <utility/tagitem.h>

#define AmiSSL_SocketBase       (TAG_USER + 0x01)
#define AmiSSL_Version          (TAG_USER + 0x02) /* OBSOLETE */
#define AmiSSL_Revision         (TAG_USER + 0x03) /* OBSOLETE */
#define AmiSSL_VersionOverride  (TAG_USER + 0x04) /* OBSOLETE */
/* #define  AmiSSL_TCPStack     (TAG_USER + 0x05)    OBSOLETE */
#define AmiSSL_SSLVersionApp    (TAG_USER + 0x06)

#ifdef __amigaos4__
#define AmiSSL_ISocket          (TAG_USER + 0x07) /* Only accessible from ppc code */
#define AmiSSL_ISocketPtr       (TAG_USER + 0x08) /* Used by auto initializer to avoid dependancy on opening order */
#endif

#define AmiSSL_SocketBaseBrand  (TAG_USER + 0x09)
#define AmiSSL_MLinkLock        (TAG_USER + 0x0a)
#define AmiSSL_ErrNoPtr         (TAG_USER + 0x0b)

#endif /* !AMISSL_TAGS_H */
