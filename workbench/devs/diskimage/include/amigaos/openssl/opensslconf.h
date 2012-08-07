#ifndef OPENSSL_OPENSSLCONF_H
#define OPENSSL_OPENSSLCONF_H

#ifndef AMISSL
#define AMISSL
#endif /* !AMISSL */

#ifndef OPENSSL_NO_ENGINE
#define OPENSSL_NO_ENGINE
#endif /* !OPENSSL_NO_ENGINE */

#ifndef OPENSSL_NO_FP_API
#define OPENSSL_NO_FP_API
#endif /* !OPENSSL_NO_FP_API */

#ifndef OPENSSL_THREADS
#define OPENSSL_THREADS
#endif /* !OPENSSL_THREADS */

#ifndef OPENSSL_EXPORT_VAR_AS_FUNCTION
#define OPENSSL_EXPORT_VAR_AS_FUNCTION
#endif /* !OPENSSL_EXPORT_VAR_AS_FUNCTION */

#ifndef OPENSSL_NO_KRB5
#define OPENSSL_NO_KRB5
#endif /* !OPENSSL_NO_KRB5 */

#ifndef OPENSSLDIR
#define OPENSSLDIR "AmiSSL:"
#endif /* !OPENSSLDIR */

#undef OPENSSL_UNISTD
#define OPENSSL_UNISTD <unistd.h>

#ifndef THIRTY_TWO_BIT
#define THIRTY_TWO_BIT
#endif /* !THIRTY_TWO_BIT */

#ifndef DES_LONG
#define DES_LONG unsigned long
#endif /* !DES_LONG */

#ifndef IDEA_INT
#define IDEA_INT unsigned int
#endif /* !IDEA_INT */

#ifndef MD2_INT
#define MD2_INT unsigned int
#endif /* !MD2_INT */

#ifndef RC2_INT
#define RC2_INT unsigned int
#endif /* !RC2_INT */

#ifndef RC4_INT
#define RC4_INT unsigned int
#endif /* !RC4_INT */

#endif /* !OPENSSL_OPENSSLCONF_H */
