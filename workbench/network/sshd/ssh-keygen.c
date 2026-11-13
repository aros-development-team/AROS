/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * Author: Fabian Schmieder (@metaneutrons)
 *
 * AROS SSH Key Generator
 *   - RSA via mbedTLS 3.6.6
 *   - Ed25519 via libssh 0.12.0
 *
 * Usage: ssh-keygen [-t rsa|ed25519] [-f keyfile]
 *
 * Generates an SSH key pair and saves:
 *   - Private key: ENVARC:SSH/id_<type> (PEM format)
 *   - Public key:  ENVARC:SSH/id_<type>.pub (OpenSSH format)
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <dos/dos.h>

#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/base64.h>
#include <mbedtls/error.h>

#include <libssh/libssh.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char __attribute__((used)) verstag[] = "\0$VER: ssh-keygen 0.1 (19.8.2026)\r\n";

#define DEFAULT_RSA_FILE    "ENVARC:SSH/id_rsa"
#define DEFAULT_ED25519_FILE "ENVARC:SSH/id_ed25519"
#define CONFIG_DIR          "ENVARC:SSH"
#define RSA_KEY_BITS        2048

enum key_type {
    KEY_RSA,
    KEY_ED25519
};

/* --- Write OpenSSH public key format (RSA only, via mbedTLS) --- */
static BOOL write_pubkey_openssh(mbedtls_pk_context *pk, const char *pubkey_path)
{
    unsigned char der_buf[4096];
    int der_len;
    unsigned char b64_buf[8192];
    size_t b64_len = 0;
    BPTR fh;

    der_len = mbedtls_pk_write_pubkey_der(pk, der_buf, sizeof(der_buf));
    if (der_len < 0)
    {
        Printf("ssh-keygen: Failed to export public key (error %ld)\n", (long)der_len);
        return FALSE;
    }

    unsigned char *der_start = der_buf + sizeof(der_buf) - der_len;

    if (mbedtls_base64_encode(b64_buf, sizeof(b64_buf), &b64_len, der_start, der_len) != 0)
    {
        Printf("ssh-keygen: Base64 encoding failed\n");
        return FALSE;
    }

    fh = Open(pubkey_path, MODE_NEWFILE);
    if (!fh)
    {
        Printf("ssh-keygen: Cannot create '%s'\n", (IPTR)pubkey_path);
        return FALSE;
    }

    FPrintf(fh, "ssh-rsa %s aros@aros\n", (IPTR)b64_buf);
    Close(fh);

    return TRUE;
}

/* --- Generate RSA key pair via mbedTLS --- */
static int generate_rsa(const char *keyfile)
{
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char pers[] = "aros_ssh_keygen";
    unsigned char buf[16384];
    int ret;
    char pubkey_path[256];

    Printf("Generating RSA %ld-bit key pair...\n", (long)RSA_KEY_BITS);

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                 (const unsigned char *)pers, sizeof(pers) - 1);
    if (ret != 0)
    {
        Printf("ssh-keygen: RNG seed failed (error %ld)\n", (long)ret);
        goto fail;
    }

    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret != 0)
    {
        Printf("ssh-keygen: PK setup failed (error %ld)\n", (long)ret);
        goto fail;
    }

    ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random,
                               &ctr_drbg, RSA_KEY_BITS, 65537);
    if (ret != 0)
    {
        Printf("ssh-keygen: Key generation failed (error %ld)\n", (long)ret);
        goto fail;
    }

    /* Ensure config directory exists */
    BPTR lock = CreateDir(CONFIG_DIR);
    if (lock) UnLock(lock);

    /* Write private key (PEM) */
    memset(buf, 0, sizeof(buf));
    ret = mbedtls_pk_write_key_pem(&pk, buf, sizeof(buf));
    if (ret != 0)
    {
        Printf("ssh-keygen: PEM export failed (error %ld)\n", (long)ret);
        goto fail;
    }

    BPTR fh = Open(keyfile, MODE_NEWFILE);
    if (!fh)
    {
        Printf("ssh-keygen: Cannot create '%s'\n", (IPTR)keyfile);
        goto fail;
    }
    Write(fh, buf, strnlen((char *)buf, sizeof(buf)));
    Close(fh);

    Printf("Private key saved to: %s\n", (IPTR)keyfile);

    /* Write public key (OpenSSH format) */
    snprintf(pubkey_path, sizeof(pubkey_path), "%s.pub", keyfile);

    if (!write_pubkey_openssh(&pk, pubkey_path))
        goto fail;

    Printf("Public key saved to:  %s\n", (IPTR)pubkey_path);

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return RETURN_OK;

fail:
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return RETURN_FAIL;
}

/* --- Generate Ed25519 key pair via libssh --- */
static int generate_ed25519(const char *keyfile)
{
    ssh_key key = NULL;
    char pubkey_path[256];
    int rc;

    Printf("Generating Ed25519 key pair...\n");

    rc = ssh_pki_generate(SSH_KEYTYPE_ED25519, 0, &key);
    if (rc != SSH_OK)
    {
        Printf("ssh-keygen: Ed25519 key generation failed\n");
        return RETURN_FAIL;
    }

    /* Ensure config directory exists */
    BPTR lock = CreateDir(CONFIG_DIR);
    if (lock) UnLock(lock);

    /* Export private key */
    rc = ssh_pki_export_privkey_file(key, NULL, NULL, NULL, keyfile);
    if (rc != SSH_OK)
    {
        Printf("ssh-keygen: Cannot write private key to '%s'\n", (IPTR)keyfile);
        ssh_key_free(key);
        return RETURN_FAIL;
    }

    Printf("Private key saved to: %s\n", (IPTR)keyfile);

    /* Export public key */
    snprintf(pubkey_path, sizeof(pubkey_path), "%s.pub", keyfile);

    rc = ssh_pki_export_pubkey_file(key, pubkey_path);
    if (rc != SSH_OK)
    {
        Printf("ssh-keygen: Cannot write public key to '%s'\n", (IPTR)pubkey_path);
        ssh_key_free(key);
        return RETURN_FAIL;
    }

    Printf("Public key saved to:  %s\n", (IPTR)pubkey_path);

    ssh_key_free(key);
    return RETURN_OK;
}

/* --- Main --- */
int main(int argc, char **argv)
{
    const char *keyfile = NULL;
    enum key_type type = KEY_RSA;
    int i;

    /* Parse arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        {
            i++;
            if (strcmp(argv[i], "rsa") == 0)
                type = KEY_RSA;
            else if (strcmp(argv[i], "ed25519") == 0)
                type = KEY_ED25519;
            else
            {
                Printf("ssh-keygen: Unknown key type '%s'. Use 'rsa' or 'ed25519'.\n",
                       (IPTR)argv[i]);
                return RETURN_WARN;
            }
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            keyfile = argv[++i];
        }
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
        {
            Printf("AROS SSH Key Generator 0.1\n");
            return RETURN_OK;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "?") == 0)
        {
            Printf("AROS SSH Key Generator 0.1\n");
            Printf("Usage: ssh-keygen [-t rsa|ed25519] [-f keyfile]\n\n");
            Printf("Options:\n");
            Printf("  -t type    Key type: rsa (default) or ed25519\n");
            Printf("  -f file    Output file (default: ENVARC:SSH/id_<type>)\n");
            Printf("  -V         Display version\n");
            return RETURN_OK;
        }
        else
        {
            Printf("ssh-keygen: Unknown option '%s'\n", (IPTR)argv[i]);
            return RETURN_WARN;
        }
    }

    /* Default key file based on type */
    if (!keyfile)
        keyfile = (type == KEY_ED25519) ? DEFAULT_ED25519_FILE : DEFAULT_RSA_FILE;

    /* Check if key already exists */
    BPTR lock = Lock(keyfile, SHARED_LOCK);
    if (lock)
    {
        UnLock(lock);
        Printf("Key file '%s' already exists.\n", (IPTR)keyfile);
        Printf("Overwrite? (yes/no): ");
        Flush(Output());

        char answer[16] = {0};
        FGets(Input(), answer, sizeof(answer));

        if (answer[0] != 'y' && answer[0] != 'Y')
        {
            Printf("Aborted.\n");
            return RETURN_OK;
        }
    }

    switch (type)
    {
        case KEY_RSA:
            return generate_rsa(keyfile);
        case KEY_ED25519:
            return generate_ed25519(keyfile);
    }

    return RETURN_FAIL;
}
