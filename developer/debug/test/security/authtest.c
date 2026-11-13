/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AuthTest - exercise security.library's authentication process:
          the per-user hold-back, the remote lock-out, login tickets,
          account locking and the root-only parts of secVerifyUserA() and
          secAuthControlA(). Results go to a report file and the debug
          output, like SecTest.

    Usage: AuthTest REPORT/A,USER/A,PHASE/A
        REPORT - file the results are written to (created)
        USER   - a user without a password to test with
        PHASE  - ROOT (run as root; Security.config LOGINDELAYMAX 1,
                 MAXTRIES 5), USER (run as that user afterwards) or LOG
                 (write the security.library and pam.library entries of
                 log.resource to the report)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <utility/hooks.h>
#include <proto/log.h>
#include <resources/log.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/security.h>

#include <string.h>

/* the Yubico OTP maker of the yubikey plugin, to make codes for a test token */
#include "yubiotp.h"

#define DEBUG 1
#include <aros/debug.h>

const TEXT version[] = "$VER: AuthTest 45.1 (15.10.2026)";

#define TEMPLATE "REPORT/A,USER/A,PHASE/A"

struct Library *secBase;
static BPTR report;
static CONST_STRPTR testuser;

static void Report(CONST_STRPTR what, BOOL ok, LONG detail)
{
    FPrintf(report, "%-56s : %s (%ld)\n", what, ok ? "OK" : "FAILED", detail);
    bug("[AuthTest] %-56s : %s (%ld)\n", what, ok ? "OK" : "FAILED", (long)detail);
}

static ULONG Verify(CONST_STRPTR user, CONST_STRPTR pwd, CONST_STRPTR host, struct secLoginTicket *ticket)
{
    struct TagItem tags[] =
    {
        { secT_UserID,                                  (IPTR)user      },
        { secT_Password,                                (IPTR)pwd       },
        { secT_Service,                                 (IPTR)"authtest" },
        { host ? secT_RemoteHost : TAG_IGNORE,          (IPTR)host      },
        { ticket ? secT_Ticket : TAG_IGNORE,            (IPTR)ticket    },
        { TAG_DONE,                                     0               }
    };
    return secVerifyUserA(tags);
}

/* Ticks a call takes */
static LONG Ticks(struct DateStamp *a, struct DateStamp *b)
{
    return (b->ds_Days - a->ds_Days) * 24 * 60 * 50 + (b->ds_Minute - a->ds_Minute) * 60 * 50 + (b->ds_Tick - a->ds_Tick);
}

static BOOL Clear(CONST_STRPTR user)
{
    struct TagItem tags[] = { { secT_AuthClear, TRUE }, { user ? secT_UserID : TAG_IGNORE, (IPTR)user }, { TAG_DONE, 0 } };
    return secAuthControlA(tags);
}

/* A child process tries the parent's ticket */
static struct secLoginTicket shared;
static volatile ULONG childresult;
static struct Task *parent;

static void ChildEntry(void)
{
    struct TagItem tags[] = { { secT_Ticket, (IPTR)&shared }, { TAG_DONE, 0 } };

    childresult = secLoginA(tags);
    Signal(parent, SIGBREAKF_CTRL_F);
}

/* ------------------------------------------------------------------------ */
/* Hardware tokens: a Yubico OTP token registered for the test user         */

static const UBYTE KeyAES[16]  = { 0x10,0x21,0x32,0x43,0x54,0x65,0x76,0x87,0x98,0xa9,0xba,0xcb,0xdc,0xed,0xfe,0x0f };
static const UBYTE KeyPriv[6]  = { 0x01,0x02,0x03,0x04,0x05,0x06 };
static const UBYTE OtherPriv[6] = { 0x06,0x05,0x04,0x03,0x02,0x01 };
#define KEYPUB      "cccccccccccb"
#define KEYSECRET   "102132435465768798a9bacbdcedfe0f" "010203040506"   /* AES key, private id */

/* password + the code the key would type (public id + token) */
static void MakeOTP(CONST_STRPTR password, CONST_STRPTR pubid, const UBYTE *priv, ULONG counter, STRPTR out)
{
    strcpy(out, password);
    strcat(out, pubid);
    yubiotp_make(KeyAES, priv, counter, 1234, 0x4242, out + strlen(out));
}

static ULONG KeyAdmin(ULONG op, CONST_STRPTR user, CONST_STRPTR label, struct secLoginTicket *ticket, ULONG *count)
{
    static struct secKeyInfo list[4];
    struct TagItem tags[] =
    {
        { secT_KAOperation,     op                      },
        { secT_UserID,          (IPTR)user              },
        { ticket ? secT_Ticket : TAG_IGNORE, (IPTR)ticket },
        { secT_KeyType,         secKEY_OTP              },
        { secT_KeyScheme,       (IPTR)"yubico"          },
        { secT_KeyLabel,        (IPTR)label             },
        { secT_KeyPublicID,     (IPTR)KEYPUB            },
        { secT_KeySecret,       (IPTR)KEYSECRET         },
        { secT_KeyList,         (IPTR)list              },
        { secT_KeyListSize,     4                       },
        { count ? secT_KeyCount : TAG_IGNORE, (IPTR)count },
        { TAG_DONE,             0                       }
    };
    return secKeyAdminA(tags);
}

static BOOL HasKeys(CONST_STRPTR user)
{
    BOOL has = FALSE;
    struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { secT_HasKeys, (IPTR)&has }, { TAG_DONE, 0 } };

    secQueryUserA(tags);
    return has;
}

static void KeyTestsRoot(void)
{
    char otp[128];
    ULONG r, count = 0;

    r = KeyAdmin(secKA_ADD, testuser, "test", NULL, NULL);
    Report("key: register a Yubico OTP token for the user", r == secKAERR_OK, r);
    if (r != secKAERR_OK)
        return;
    Report("key: secQueryUserA() reports keys", HasKeys(testuser), 0);
    r = KeyAdmin(secKA_LIST, testuser, NULL, NULL, &count);
    Report("key: listed", r == secKAERR_OK && count == 1, count);
    r = KeyAdmin(secKA_ADD, testuser, "test", NULL, NULL);
    Report("key: the same label again refused", r == secKAERR_EXISTS, r);

    r = Verify(testuser, "", NULL, NULL);
    Report("key: the password alone no longer verifies", r == secVERIFY_FAILED, r);
    MakeOTP("", KEYPUB, KeyPriv, 0x0100, otp);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: password + a code from the key verifies", r == secVERIFY_OK, r);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: the same code again refused (replay)", r == secVERIFY_FAILED, r);
    MakeOTP("", KEYPUB, KeyPriv, 0x0101, otp);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: the next code (session counter) verifies", r == secVERIFY_OK, r);
    MakeOTP("", KEYPUB, KeyPriv, 0x0050, otp);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: a code with a lower counter refused", r == secVERIFY_FAILED, r);
    MakeOTP("", KEYPUB, KeyPriv, 0x0200, otp);
    otp[strlen(otp) - 3] = (otp[strlen(otp) - 3] == 'c') ? 'b' : 'c';
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: a damaged code refused", r == secVERIFY_FAILED, r);
    MakeOTP("", "cccccccccccd", KeyPriv, 0x0200, otp);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: a code of an unregistered key refused", r == secVERIFY_FAILED, r);
    MakeOTP("", KEYPUB, OtherPriv, 0x0200, otp);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: a code with the wrong private id refused", r == secVERIFY_FAILED, r);
    MakeOTP("", KEYPUB, KeyPriv, 0x0200, otp);
    r = Verify(testuser, otp, NULL, NULL);
    Report("key: a good code verifies again", r == secVERIFY_OK, r);
    {
        static struct secKeyInfo list[1];
        struct TagItem tags[] = { { secT_KAOperation, secKA_LIST }, { secT_UserID, (IPTR)testuser }, { secT_KeyList, (IPTR)list },
                                  { secT_KeyListSize, 1 }, { secT_KeyCount, (IPTR)&count }, { TAG_DONE, 0 } };
        r = secKeyAdminA(tags);
        Report("key: the counter was saved", r == secKAERR_OK && count == 1 && list[0].Counter == 0x0200, list[0].Counter);
    }
    r = KeyAdmin(secKA_REMOVE, testuser, "test", NULL, NULL);
    Report("key: removed", r == secKAERR_OK, r);
    Report("key: secQueryUserA() reports none", !HasKeys(testuser), 0);
    r = Verify(testuser, "", NULL, NULL);
    Report("key: the password alone verifies again", r == secVERIFY_OK, r);
    r = KeyAdmin(secKA_REMOVE, testuser, "test", NULL, NULL);
    Report("key: removing it again: not found", r == secKAERR_NOTFOUND, r);
}

/* ---- challenge tokens (FIDO U2F through fidosim.device, LOADPLUGIN fido) ---- */

#define CHAL_LIST   4

AROS_UFH3(static IPTR, ChalStatusFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct secKeyStatus *, ks, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    ULONG *seen = (ULONG *)hook->h_Data;

    *seen |= 1UL << ks->Type;
    return TRUE;

    AROS_USERFUNC_EXIT
}

/* The first pass: the challenge and the user's tokens, if secVERIFY_KEYNEEDED */
static ULONG VerifyChal(CONST_STRPTR user, CONST_STRPTR pwd, CONST_STRPTR host, struct secLoginTicket *ticket,
                        UBYTE *challenge, struct secKeyInfo *list, ULONG *count)
{
    struct TagItem tags[] =
    {
        { secT_UserID,      (IPTR)user      },
        { secT_Password,    (IPTR)pwd       },
        { secT_Service,     (IPTR)"authtest" },
        { host ? secT_RemoteHost : TAG_IGNORE, (IPTR)host },
        { secT_Ticket,      (IPTR)ticket    },
        { secT_KeyChallenge,(IPTR)challenge },
        { secT_KeyList,     (IPTR)list      },
        { secT_KeyListSize, CHAL_LIST       },
        { secT_KeyCount,    (IPTR)count     },
        { TAG_DONE,         0               }
    };
    return secVerifyUserA(tags);
}

/* The token's answer (secKeyClientA) */
static ULONG Assert(struct secKeyInfo *list, ULONG count, UBYTE *challenge, UBYTE *assertion, ULONG *size, ULONG *which, ULONG *seen)
{
    struct Hook hook;
    struct TagItem tags[] =
    {
        { secT_KCOperation,     secKC_ASSERT        },
        { secT_KeyList,         (IPTR)list          },
        { secT_KeyListSize,     count               },
        { secT_KeyChallenge,    (IPTR)challenge     },
        { secT_KeyAssertion,    (IPTR)assertion     },
        { secT_KeyAssertionSize,(IPTR)size          },
        { secT_KeyIndex,        (IPTR)which         },
        { secT_StatusHook,      (IPTR)&hook         },
        { secT_KeyTimeout,      5                   },
        { TAG_DONE,             0                   }
    };

    memset(&hook, 0, sizeof(hook));
    hook.h_Entry = (HOOKFUNC)ChalStatusFunc;
    hook.h_Data = seen;
    *size = secKEYASSERTIONMAX;
    return secKeyClientA(tags);
}

/* The second pass: the answer with the ticket */
static ULONG VerifyAnswer(CONST_STRPTR user, struct secLoginTicket *ticket, const UBYTE *assertion, ULONG size, CONST_STRPTR publicid)
{
    struct TagItem tags[] =
    {
        { secT_UserID,          (IPTR)user      },
        { secT_Ticket,          (IPTR)ticket    },
        { secT_KeyAssertion,    (IPTR)assertion },
        { secT_KeyAssertionSize,size            },
        { secT_KeyPublicID,     (IPTR)publicid  },
        { secT_Service,         (IPTR)"authtest" },
        { TAG_DONE,             0               }
    };
    return secVerifyUserA(tags);
}

static ULONG EnrolChal(CONST_STRPTR user, CONST_STRPTR label, struct secLoginTicket *ticket)
{
    ULONG seen = 0;
    struct Hook hook;
    struct TagItem tags[] =
    {
        { secT_KAOperation,     secKA_ADD               },
        { secT_UserID,          (IPTR)user              },
        { ticket ? secT_Ticket : TAG_IGNORE, (IPTR)ticket },
        { secT_KeyType,         secKEY_CHALLENGE        },
        { secT_KeyScheme,       (IPTR)"u2f"             },
        { secT_KeyLabel,        (IPTR)label             },
        { secT_KeyEnrol,        TRUE                    },
        { secT_StatusHook,      (IPTR)&hook             },
        { secT_KeyTimeout,      5                       },
        { TAG_DONE,             0                       }
    };

    memset(&hook, 0, sizeof(hook));
    hook.h_Entry = (HOOKFUNC)ChalStatusFunc;
    hook.h_Data = &seen;
    return secKeyAdminA(tags);
}

/* A full two-pass verification; the ticket is the login ticket on secVERIFY_OK */
static ULONG LoginChal(CONST_STRPTR user, CONST_STRPTR pwd, struct secLoginTicket *ticket)
{
    static struct secKeyInfo list[CHAL_LIST];
    static UBYTE assertion[secKEYASSERTIONMAX];
    UBYTE challenge[secKEYCHALLENGESIZE];
    ULONG r, count = 0, size, which = 0, seen = 0;

    r = VerifyChal(user, pwd, NULL, ticket, challenge, list, &count);
    if (r != secVERIFY_KEYNEEDED)
        return r;
    if (Assert(list, count, challenge, assertion, &size, &which, &seen) != secKCERR_OK)
        return secVERIFY_FAILED;
    return VerifyAnswer(user, ticket, assertion, size, list[which].PublicID);
}

static void ChalTestsRoot(void)
{
    static struct secKeyInfo list[CHAL_LIST];
    static UBYTE assertion[secKEYASSERTIONMAX], old[secKEYASSERTIONMAX];
    struct secLoginTicket ticket;
    UBYTE challenge[secKEYCHALLENGESIZE];
    ULONG r, count = 0, size = 0, oldsize, which = 0, seen = 0, counter;

    r = EnrolChal(testuser, "u2f", NULL);
    Report("chal: enrol a U2F token for the user (fidosim.device)", r == secKAERR_OK, r);
    if (r != secKAERR_OK)
        return;
    r = KeyAdmin(secKA_LIST, testuser, NULL, NULL, &count);
    Report("chal: listed", r == secKAERR_OK && count == 1, count);

    r = Verify(testuser, "", NULL, NULL);
    Report("chal: a verification without a ticket fails (no second pass possible)", r == secVERIFY_FAILED, r);
    r = VerifyChal(testuser, "", NULL, &ticket, challenge, list, &count);
    Report("chal: the first pass asks for the token (KEYNEEDED)", r == secVERIFY_KEYNEEDED && count == 1, r);
    Report("chal: ... and lists it", count == 1 && list[0].Type == secKEY_CHALLENGE && !strcmp(list[0].Scheme, "u2f"), list[0].Type);
    r = Assert(list, count, challenge, assertion, &size, &which, &seen);
    Report("chal: the token answers (secKeyClientA)", r == secKCERR_OK && size > 5 && which == 0, r);
    Report("chal: the status hook saw 'touch'", (seen & (1UL << secKS_TOUCH)) != 0, seen);
    counter = ((ULONG)assertion[1] << 24) | ((ULONG)assertion[2] << 16) | ((ULONG)assertion[3] << 8) | assertion[4];
    r = VerifyAnswer(testuser, &ticket, assertion, size, list[0].PublicID);
    Report("chal: the answer completes the verification", r == secVERIFY_OK, r);
    {
        struct TagItem tags[] = { { secT_DiscardTicket, (IPTR)&ticket }, { TAG_DONE, 0 } };
        Report("chal: the ticket became a login ticket (discard ok)", secVerifyUserA(tags) == secVERIFY_OK, 0);
    }
    r = KeyAdmin(secKA_LIST, testuser, NULL, NULL, &count);
    {
        static struct secKeyInfo l2[1];
        struct TagItem tags[] = { { secT_KAOperation, secKA_LIST }, { secT_UserID, (IPTR)testuser }, { secT_KeyList, (IPTR)l2 },
                                  { secT_KeyListSize, 1 }, { secT_KeyCount, (IPTR)&count }, { TAG_DONE, 0 } };
        r = secKeyAdminA(tags);
        Report("chal: the token's counter was saved", r == secKAERR_OK && l2[0].Counter == counter, l2[0].Counter);
    }

    /* replay: the old answer against a new challenge */
    memcpy(old, assertion, size);
    oldsize = size;
    r = VerifyChal(testuser, "", NULL, &ticket, challenge, list, &count);
    if (r == secVERIFY_KEYNEEDED)
        r = VerifyAnswer(testuser, &ticket, old, oldsize, list[0].PublicID);
    Report("chal: an old answer is refused", r == secVERIFY_FAILED, r);
    r = VerifyAnswer(testuser, &ticket, old, oldsize, list[0].PublicID);
    Report("chal: ... and the ticket is gone after that", r == secVERIFY_FAILED, r);

    /* a wrong password is not told until the token answered */
    r = VerifyChal(testuser, "wrong", NULL, &ticket, challenge, list, &count);
    Report("chal: a wrong password still asks for the token", r == secVERIFY_KEYNEEDED, r);
    r = Assert(list, count, challenge, assertion, &size, &which, &seen);
    if (r == secKCERR_OK)
        r = VerifyAnswer(testuser, &ticket, assertion, size, list[0].PublicID);
    Report("chal: ... and fails with the token's answer", r == secVERIFY_FAILED, r);

    /* an answer to somebody else's challenge: a fresh first pass, then an answer made for another */
    r = VerifyChal(testuser, "", NULL, &ticket, challenge, list, &count);
    if (r == secVERIFY_KEYNEEDED)
    {
        challenge[0] ^= 0xff;
        r = Assert(list, count, challenge, assertion, &size, &which, &seen);
        if (r == secKCERR_OK)
            r = VerifyAnswer(testuser, &ticket, assertion, size, list[0].PublicID);
    }
    Report("chal: an answer to a different challenge is refused", r == secVERIFY_FAILED, r);

    /* a damaged answer */
    r = VerifyChal(testuser, "", NULL, &ticket, challenge, list, &count);
    if (r == secVERIFY_KEYNEEDED)
    {
        r = Assert(list, count, challenge, assertion, &size, &which, &seen);
        if (r == secKCERR_OK)
        {
            assertion[size - 1] ^= 0x55;
            r = VerifyAnswer(testuser, &ticket, assertion, size, list[0].PublicID);
        }
    }
    Report("chal: a damaged answer is refused", r == secVERIFY_FAILED, r);

    /* the incomplete ticket cannot be redeemed */
    r = VerifyChal(testuser, "", NULL, &ticket, challenge, list, &count);
    if (r == secVERIFY_KEYNEEDED)
    {
        struct TagItem lt[] = { { secT_Ticket, (IPTR)&ticket }, { secT_Quiet, TRUE }, { TAG_DONE, 0 } };
        r = secLoginA(lt);
        Report("chal: a ticket still waiting for the token cannot log in", r == secOWNER_NOBODY, r);
        {
            struct TagItem tags[] = { { secT_DiscardTicket, (IPTR)&ticket }, { TAG_DONE, 0 } };
            secVerifyUserA(tags);
        }
    }
    else
        Report("chal: a ticket still waiting for the token cannot log in", FALSE, r);

    r = Verify(testuser, "", "somehost", NULL);
    Report("chal: from the network the token cannot be used: refused", r == secVERIFY_FAILED, r);

    r = LoginChal(testuser, "", &ticket);
    Report("chal: a full two-pass verification", r == secVERIFY_OK, r);
    {
        struct TagItem tags[] = { { secT_DiscardTicket, (IPTR)&ticket }, { TAG_DONE, 0 } };
        secVerifyUserA(tags);
    }
    r = KeyAdmin(secKA_REMOVE, testuser, "u2f", NULL, NULL);
    Report("chal: removed", r == secKAERR_OK, r);
    r = Verify(testuser, "", NULL, NULL);
    Report("chal: the password alone verifies again", r == secVERIFY_OK, r);
}

static void ChalTestsUser(void)
{
    struct secLoginTicket ticket;
    ULONG r, count = 0;

    r = Verify(testuser, "", NULL, &ticket);
    if (r == secVERIFY_OK)
        r = EnrolChal(testuser, "myu2f", &ticket);
    Report("chal: a user enrols their own token with a ticket", r == secKAERR_OK, r);
    if (r != secKAERR_OK)
        return;
    r = LoginChal(testuser, "", &ticket);
    Report("chal: the user's two-pass verification", r == secVERIFY_OK, r);
    if (r == secVERIFY_OK)
        r = KeyAdmin(secKA_REMOVE, testuser, "myu2f", &ticket, NULL);
    Report("chal: ... whose ticket removes the token again", r == secKAERR_OK, r);
    r = KeyAdmin(secKA_LIST, testuser, NULL, NULL, &count);
    Report("chal: none left", r == secKAERR_OK && count == 0, count);
}

static void KeyTestsUser(void)
{
    struct secLoginTicket ticket;
    char otp[128];
    ULONG r, count = 0;

    r = KeyAdmin(secKA_ADD, testuser, "mine", NULL, NULL);
    Report("key: a user cannot register without a ticket", r == secKAERR_NOTALLOWED, r);
    r = Verify(testuser, "", NULL, &ticket);
    if (r == secVERIFY_OK)
        r = KeyAdmin(secKA_ADD, testuser, "mine", &ticket, NULL);
    Report("key: a user registers their own key with a ticket", r == secKAERR_OK, r);
    if (r != secKAERR_OK)
        return;
    r = KeyAdmin(secKA_LIST, testuser, NULL, NULL, &count);
    Report("key: a user lists their own keys", r == secKAERR_OK && count == 1, count);
    MakeOTP("", KEYPUB, KeyPriv, 0x0300, otp);
    r = Verify(testuser, otp, NULL, &ticket);
    if (r == secVERIFY_OK)
        r = KeyAdmin(secKA_ADD, "root", "mine", &ticket, NULL);
    Report("key: their ticket does not let them change root's keys", r == secKAERR_NOTALLOWED, r);
    MakeOTP("", KEYPUB, KeyPriv, 0x0301, otp);
    r = Verify(testuser, otp, NULL, &ticket);
    if (r == secVERIFY_OK)
        r = KeyAdmin(secKA_REMOVE, testuser, "mine", &ticket, NULL);
    Report("key: a user removes their own key with a ticket", r == secKAERR_OK, r);
    Report("key: none left", !HasKeys(testuser), 0);
}

static void RootPhase(void)
{
    struct DateStamp t0, t1;
    struct secLoginTicket ticket, forged;
    ULONG r, owner, count = 0;
    LONG tk, i;
    char longhost[120];

    r = Verify(testuser, "", NULL, NULL);
    Report("verify, right (empty) password", r == secVERIFY_OK, r);

    /* hold-back after failures (LOGINDELAYMAX 1) */
    Clear(testuser);
    DateStamp(&t0);
    r = Verify(testuser, "wrong", NULL, NULL);
    DateStamp(&t1);
    Report("verify, wrong password: failed at once", r == secVERIFY_FAILED && Ticks(&t0, &t1) < 40, Ticks(&t0, &t1));
    DateStamp(&t0);
    r = Verify(testuser, "wrong", NULL, NULL);
    DateStamp(&t1);
    tk = Ticks(&t0, &t1);
    Report("verify again: held back about a second", r == secVERIFY_FAILED && tk >= 35 && tk < 150, tk);
    DateStamp(&t0);
    r = Verify(testuser, "", NULL, NULL);
    DateStamp(&t1);
    tk = Ticks(&t0, &t1);
    Report("right password after failures: held back, then OK", r == secVERIFY_OK && tk >= 35, tk);
    DateStamp(&t0);
    r = Verify(testuser, "", NULL, NULL);
    DateStamp(&t1);
    Report("success cleared the hold-back", r == secVERIFY_OK && Ticks(&t0, &t1) < 40, Ticks(&t0, &t1));

    /* local failures never lock out */
    for (i = 0; i < 6; i++)
        Verify(testuser, "wrong", NULL, NULL);
    r = Verify(testuser, "", NULL, NULL);
    Report("6 local failures: no lock-out", r == secVERIFY_OK, r);

    /* remote lock-out (MAXTRIES 5). The test user has no password and an
     * empty password is never accepted from a remote host, so these
     * compare LOCKED with FAILED */
    Clear(NULL);
    r = Verify(testuser, "", "host-b", NULL);
    Report("empty password from a remote host refused", r == secVERIFY_FAILED, r);
    Clear(NULL);
    for (i = 0; i < 5; i++)
        Verify(testuser, "wrong", "host-a", NULL);
    r = Verify(testuser, "wrong", "host-a", NULL);
    Report("5 failures from host-a: host-a locked out", r == secVERIFY_LOCKED, r);
    r = Verify(testuser, "wrong", "host-b", NULL);
    Report("host-b not locked out", r == secVERIFY_FAILED, r);
    r = Verify(testuser, "", NULL, NULL);
    Report("local not locked out", r == secVERIFY_OK, r);

    memset(longhost, 'x', sizeof(longhost) - 1);
    longhost[sizeof(longhost) - 1] = '\0';
    for (i = 0; i < 5; i++)
        Verify(testuser, "wrong", longhost, NULL);
    r = Verify(testuser, "wrong", longhost, NULL);
    Report("119-character host name locked out too", r == secVERIFY_LOCKED, r);

    {
        struct secAuthRecord recs[16];
        struct TagItem tags[] = { { secT_AuthList, (IPTR)recs }, { secT_AuthListSize, 16 }, { secT_AuthCount, (IPTR)&count }, { TAG_DONE, 0 } };
        BOOL ok = secAuthControlA(tags);
        Report("secAuthControlA() lists the records", ok && count >= 2, count);
    }
    Report("secAuthControlA() clears them", Clear(NULL), 0);
    r = Verify(testuser, "wrong", "host-a", NULL);
    Report("host-a no longer locked out", r == secVERIFY_FAILED, r);

    /* tickets */
    Clear(NULL);
    r = Verify(testuser, "", NULL, &ticket);
    Report("verify with a ticket", r == secVERIFY_OK, r);
    {
        struct TagItem tags[] = { { secT_Ticket, (IPTR)&ticket }, { TAG_DONE, 0 } };
        owner = secLoginA(tags);
        Report("secLoginA() with the ticket", owner != secOWNER_NOBODY && (owner >> 16) != secROOT_UID, owner);
        {
            struct TagItem out[] = { { secT_Quiet, TRUE }, { TAG_DONE, 0 } };
            owner = secLogoutA(out);
            Report("logged out again: root", (owner >> 16) == secROOT_UID, owner);
        }
        owner = secLoginA(tags);
        Report("the same ticket a second time refused", owner == secOWNER_NOBODY, owner);
    }
    memset(&forged, 0x5a, sizeof(forged));
    {
        struct TagItem tags[] = { { secT_Ticket, (IPTR)&forged }, { TAG_DONE, 0 } };
        owner = secLoginA(tags);
        Report("a made-up ticket refused", owner == secOWNER_NOBODY, owner);
    }
    r = Verify(testuser, "", NULL, &shared);
    {
        struct TagItem ptags[] = { { NP_Entry, (IPTR)ChildEntry }, { NP_Name, (IPTR)"AuthTest child" }, { TAG_DONE, 0 } };

        parent = FindTask(NULL);
        childresult = 0xdead;
        SetSignal(0, SIGBREAKF_CTRL_F);
        if (r == secVERIFY_OK && CreateNewProc(ptags))
        {
            Wait(SIGBREAKF_CTRL_F);
            Report("another task cannot use this task's ticket", childresult == secOWNER_NOBODY, childresult);
        }
        else
            Report("another task cannot use this task's ticket", FALSE, r);
    }
    {
        struct TagItem tags[] = { { secT_DiscardTicket, (IPTR)&shared }, { TAG_DONE, 0 } };
        Report("discard a ticket", secVerifyUserA(tags) == secVERIFY_OK, 0);
    }

    KeyTestsRoot();
    ChalTestsRoot();

    /* account locking */
    {
        struct TagItem lock[] = { { secT_LockAccount, (IPTR)testuser }, { TAG_DONE, 0 } };
        struct TagItem unlock[] = { { secT_UnlockAccount, (IPTR)testuser }, { TAG_DONE, 0 } };
        BOOL locked = FALSE;
        struct TagItem q[] = { { secT_UserID, (IPTR)testuser }, { secT_AccountLocked, (IPTR)&locked }, { TAG_DONE, 0 } };

        Report("lock the account", secAuthControlA(lock), 0);
        secQueryUserA(q);
        Report("secQueryUserA() reports it locked", locked, locked);
        Clear(NULL);
        r = Verify(testuser, "", NULL, NULL);
        Report("a locked account does not verify", r == secVERIFY_FAILED, r);
        Report("unlock the account", secAuthControlA(unlock), 0);
        Clear(NULL);
        r = Verify(testuser, "", NULL, NULL);
        Report("unlocked again: verifies", r == secVERIFY_OK, r);
    }

    /* hashed tokens need HASHEDLOGIN, even for root */
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)testuser }, { secT_PasswordHash, (IPTR)"abcdefghijk" }, { TAG_DONE, 0 } };
        r = secVerifyUserA(tags);
        Report("hashed token without HASHEDLOGIN: not supported", r == secVERIFY_NOTSUPPORTED, r);
    }
    Clear(NULL);
}

static void UserPhase(void)
{
    ULONG r;

    /* host-a was locked out for the user in the root phase and cleared;
     * lock it again as root would - a user's host name is ignored anyway */
    r = Verify(testuser, "", "host-a", NULL);
    Report("a user's secT_RemoteHost is ignored", r == secVERIFY_OK, r);
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)testuser }, { secT_PasswordHash, (IPTR)"abcdefghijk" }, { TAG_DONE, 0 } };
        r = secVerifyUserA(tags);
        Report("a user may not verify a hashed token", r == secVERIFY_NOTSUPPORTED, r);
    }
    Report("a user may not list the records", !secAuthControlA(NULL), 0);
    {
        struct TagItem lock[] = { { secT_LockAccount, (IPTR)"root" }, { TAG_DONE, 0 } };
        Report("a user may not lock accounts", !secAuthControlA(lock), 0);
    }

    KeyTestsUser();
    ChalTestsUser();
}

static void LogPhase(void)
{
    APTR LogResBase = OpenResource("log.resource");
    APTR entry = (APTR)LOGEntry_First;
    ULONG n = 0;

    if (!LogResBase)
    {
        Report("log.resource present", FALSE, 0);
        return;
    }
    logLockEntries(LLF_READ);
    while ((entry = logNextEntry(&entry)) && entry != (APTR)LOGEntry_Last)
    {
        STRPTR comp = NULL, sub = NULL, text = NULL;
        ULONG flags = 0;
        struct TagItem tags[] =
        {
            { LOGMA_Component,      (IPTR)&comp     },
            { LOGMA_SubComponent,   (IPTR)&sub      },
            { LOGMA_Entry,          (IPTR)&text     },
            { LOGMA_Flags,          (IPTR)&flags    },
            { TAG_DONE,             0               }
        };

        logGetEntryAttrs(entry, tags);
        if (comp && (!strcmp(comp, "security.library") || !strcmp(comp, "pam.library")))
        {
            FPrintf(report, "%s [%s] %08lx: %s\n", comp, sub ? sub : (STRPTR)"", (ULONG)flags, text ? text : (STRPTR)"");
            n++;
        }
    }
    logUnlockEntries(LLF_READ);
    Report("security/pam entries in log.resource", n > 0, n);
}

int main(void)
{
    IPTR args[3] = { 0 };
    struct RDArgs *rda;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "AuthTest");
        return RETURN_FAIL;
    }
    if (!(secBase = OpenLibrary(SECURITYNAME, 45)) || secBase->lib_Revision < 14)
    {
        PutStr("AuthTest: needs security.library 45.14\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (!(report = Open((STRPTR)args[0], MODE_NEWFILE)))
    {
        CloseLibrary(secBase);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    testuser = (CONST_STRPTR)args[1];
    if (!Stricmp((STRPTR)args[2], "ROOT"))
        RootPhase();
    else if (!Stricmp((STRPTR)args[2], "LOG"))
        LogPhase();
    else
        UserPhase();

    Close(report);
    CloseLibrary(secBase);
    FreeArgs(rda);
    return RETURN_OK;
}
