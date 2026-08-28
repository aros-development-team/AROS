/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    mbedTLS configuration for AROS.
    Minimal TLS 1.2/1.3 client with RSA + ECDSA + AES-GCM.
    No threads, no filesystem (certs embedded or loaded via DOS).
*/
#ifndef MBEDTLS_AROS_CONFIG_H
#define MBEDTLS_AROS_CONFIG_H

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_HAVE_TIME_DATE
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY

/* No filesystem — certs loaded programmatically */
/* #undef MBEDTLS_FS_IO */

/* No threads — single-threaded operation */
/* #undef MBEDTLS_THREADING_C */

/* Entropy — use our custom source (RNG200 hardware or timer jitter) */
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* DRBG */
#define MBEDTLS_CTR_DRBG_C

/* Networking — we provide custom net callbacks via bsdsocket.library */
#define MBEDTLS_NET_C
#define MBEDTLS_NET_ALT

/* TLS */
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_TLS1_3
/* TLS 1.3 derives its key schedule through PSA and keeps the peer cert */
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_SSL_KEEP_PEER_CERTIFICATE

/* X.509 */
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_BASE64_C

/* Public key */
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED

/* Symmetric ciphers */
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CCM_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_CHACHAPOLY_C
#define MBEDTLS_POLY1305_C

/* Hashes */
#define MBEDTLS_MD_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA384_C
#define MBEDTLS_SHA512_C

/* Key exchange */
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

/* mbedTLS has no millisecond clock for AROS - see mbedtls_platform_aros.c */
#define MBEDTLS_PLATFORM_MS_TIME_ALT

/* AROS declares inet_pton() but does not expose it to a plain compile, and a
 * certificate parser should not need the network stack - use mbedTLS's own. */
#define MBEDTLS_TEST_SW_INET_PTON

/* Misc */
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21
#define MBEDTLS_ERROR_C
#define MBEDTLS_HKDF_C
#define MBEDTLS_VERSION_C

#endif /* MBEDTLS_AROS_CONFIG_H */
