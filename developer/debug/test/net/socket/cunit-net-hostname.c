/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the host name change signal: a base that sets
    SBTC_SIG_HOSTNAME_CHANGE_MASK is signalled when the host's name changes
    (sethostname(), here miami.library's), and then reads it again with
    gethostname(). Setting the name it has already is no change and sends no
    signal; two changes before the consumer looks come as one signal, and
    gethostname() gives the last; the signal is not the address-change one
    (SBTC_SIG_ADDRESS_CHANGE_MASK), and an address change does not send it;
    each mask is set by its own tag, apart from the other.
    The HOSTNAME variable follows the name, as long as the name is.

    The test sets names of its own and gives the host its name back at the
    end. Usage:  cunit-net-hostname. Without miami.library the tests are
    skipped.
*/

#define __CONFIG_ROADSHOW__     /* SocketBaseTags() */
#include "socktest.h"

#include <proto/dos.h>
#include <defines/miami.h>      /* sethostname() alone: proto/miami.h and __CONFIG_ROADSHOW__ clash */
#include <bsdsocket/socketbasetags.h>
#include <net/if.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>
#include <sys/param.h>          /* MAXHOSTNAMELEN */

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

static ULONG hostmask, addrmask;
static char original[MAXHOSTNAMELEN + 1];

/* The stack took the mask (a stack without the signal refuses the tag). */
#define NEED_MASK() CU_ASSERT_FATAL(hostmask != 0)
#define NEED_MIAMI() do { NEED_MASK(); if (MiamiBase == NULL) { CU_PASS("no miami.library - skipping"); return; } } while (0)

/* Whether the signals in mask came since the last look, and clear them. */
static int signalled(ULONG mask)
{
    return (SetSignal(0, mask) & mask) != 0;
}

/* The host name as gethostname() gives it. */
static const char *name_now(void)
{
    static char buf[MAXHOSTNAMELEN + 1];

    buf[0] = '\0';
    gethostname(buf, sizeof(buf));
    return buf;
}

static int set_name(const char *name)
{
    return sethostname(name, strnlen(name, MAXHOSTNAMELEN));
}

/* Add (1) or delete (0) 127.77.0.1/32 on lo0: an address change, no ARP. */
static int lo_alias(int add)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    if (add) {
        struct in_aliasreq ifra;

        bzero(&ifra, sizeof(ifra));
        snprintf(ifra.ifra_name, sizeof(ifra.ifra_name), "lo0");
        ifra.ifra_addr.sin_len = ifra.ifra_mask.sin_len = sizeof(struct sockaddr_in);
        ifra.ifra_addr.sin_family = ifra.ifra_mask.sin_family = AF_INET;
        ifra.ifra_addr.sin_addr.s_addr = inet_addr("127.77.0.1");
        ifra.ifra_mask.sin_addr.s_addr = inet_addr("255.255.255.255");
        r = IoctlSocket(s, SIOCAIFADDR, (char *)&ifra);
    } else {
        struct ifreq ifr;
        struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;

        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "lo0");
        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = inet_addr("127.77.0.1");
        r = IoctlSocket(s, SIOCDIFADDR, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

int init_suite(void) { return SocketBase != NULL ? 0 : -1; }
int clean_suite(void)
{
    if (MiamiBase != NULL && original[0] != '\0')
        set_name(original);
    return 0;
}

/* The mask set is the mask read back. */
void testHOSTNAME_MASK(void)
{
    IPTR m = 0;

    NEED_MASK();
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_GETREF(SBTC_SIG_HOSTNAME_CHANGE_MASK), (IPTR)&m, TAG_END), 0);
    CU_ASSERT_EQUAL(m, hostmask);
}

/* Each mask is its own: a call that sets one leaves the other as it was. */
void testHOSTNAME_MASKS_APART(void)
{
    IPTR h = 0, a = 0;

    NEED_MASK();
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_SETVAL(SBTC_SIG_HOSTNAME_CHANGE_MASK), 0, TAG_END), 0);
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_GETREF(SBTC_SIG_ADDRESS_CHANGE_MASK), (IPTR)&a, TAG_END), 0);
    CU_ASSERT_EQUAL(a, addrmask);
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), 0, TAG_END), 0);
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_SETVAL(SBTC_SIG_HOSTNAME_CHANGE_MASK), hostmask, TAG_END), 0);
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), addrmask, TAG_END), 0);
    CU_ASSERT_EQUAL(SocketBaseTags(SBTM_GETREF(SBTC_SIG_HOSTNAME_CHANGE_MASK), (IPTR)&h,
        SBTM_GETREF(SBTC_SIG_ADDRESS_CHANGE_MASK), (IPTR)&a, TAG_END), 0);
    CU_ASSERT_EQUAL(h, hostmask);
    CU_ASSERT_EQUAL(a, addrmask);
}

/* A new name: the signal, and gethostname() and HOSTNAME give it. */
void testHOSTNAME_CHANGE(void)
{
    char var[MAXHOSTNAMELEN + 1];

    NEED_MIAMI();
    signalled(hostmask);
    CU_ASSERT_EQUAL(set_name("b42-one"), 0);
    CU_ASSERT(signalled(hostmask));
    CU_ASSERT_STRING_EQUAL(name_now(), "b42-one");
    var[0] = '\0';
    CU_ASSERT(GetVar("HOSTNAME", var, sizeof(var), GVF_GLOBAL_ONLY) >= 0);
    CU_ASSERT_STRING_EQUAL(var, "b42-one");
}

/* The name it has already: no signal. A length past the name's end (the
 * name ends at its NUL) is still the same name. */
void testHOSTNAME_SAME(void)
{
    static const char padded[16] = "b42-one";

    NEED_MIAMI();
    CU_ASSERT_EQUAL(set_name("b42-one"), 0);
    signalled(hostmask);
    CU_ASSERT_EQUAL(set_name("b42-one"), 0);
    CU_ASSERT_FALSE(signalled(hostmask));
    CU_ASSERT_EQUAL(sethostname(padded, sizeof(padded)), 0);
    CU_ASSERT_FALSE(signalled(hostmask));
    CU_ASSERT_STRING_EQUAL(name_now(), "b42-one");
}

/* Two changes before the consumer looks: one signal, the last name. */
void testHOSTNAME_TWO_CHANGES(void)
{
    NEED_MIAMI();
    signalled(hostmask);
    CU_ASSERT_EQUAL(set_name("b42-two"), 0);
    CU_ASSERT_EQUAL(set_name("b42-three"), 0);
    CU_ASSERT(signalled(hostmask));
    CU_ASSERT_FALSE(signalled(hostmask));
    CU_ASSERT_STRING_EQUAL(name_now(), "b42-three");
}

/* A name change is not an address change, nor the other way round. */
void testHOSTNAME_NOT_ADDRESS(void)
{
    NEED_MIAMI();
    signalled(hostmask | addrmask);
    CU_ASSERT_EQUAL(set_name("b42-four"), 0);
    CU_ASSERT(signalled(hostmask));
    CU_ASSERT_FALSE(signalled(addrmask));
    CU_ASSERT_EQUAL(lo_alias(1), 0);
    CU_ASSERT(signalled(addrmask));
    CU_ASSERT_FALSE(signalled(hostmask));
    CU_ASSERT_EQUAL(lo_alias(0), 0);
    CU_ASSERT(signalled(addrmask));
    CU_ASSERT_FALSE(signalled(hostmask));
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    BYTE hsig = -1, asig = -1;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping HOSTNAME tests\n");
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    if (MiamiBase == NULL)
        printf("no miami.library - skipping the HOSTNAME tests that set the name\n");
    hsig = AllocSignal(-1);
    asig = AllocSignal(-1);
    if (hsig != -1 && asig != -1) {
        hostmask = 1UL << hsig;
        addrmask = 1UL << asig;
        if (SocketBaseTags(SBTM_SETVAL(SBTC_SIG_HOSTNAME_CHANGE_MASK), hostmask,
                SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), addrmask, TAG_END) != 0)
            hostmask = 0;
    }
    snprintf(original, sizeof(original), "%s", name_now());

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("HOSTNAME_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "the mask set is the mask read", testHOSTNAME_MASK, ST_NONE) ||
        !st_add_test(p, "each mask is set apart from the other", testHOSTNAME_MASKS_APART, ST_NONE) ||
        !st_add_test(p, "a new name: the signal, gethostname(), HOSTNAME", testHOSTNAME_CHANGE, ST_NONE) ||
        !st_add_test(p, "the same name: no signal", testHOSTNAME_SAME, ST_NONE) ||
        !st_add_test(p, "two changes: one signal, the last name", testHOSTNAME_TWO_CHANGES, ST_NONE) ||
        !st_add_test(p, "a name change is not an address change", testHOSTNAME_NOT_ADDRESS, ST_V4))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-HOSTNAME");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    SocketBaseTags(SBTM_SETVAL(SBTC_SIG_HOSTNAME_CHANGE_MASK), 0,
        SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), 0, TAG_END);
    if (hsig != -1)
        FreeSignal(hsig);
    if (asig != -1)
        FreeSignal(asig);
    if (MiamiBase)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
