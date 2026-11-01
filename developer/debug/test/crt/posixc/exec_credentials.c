/* Verify that exec preserves distinct real/effective IDs and the umask. */
#include <proto/dos.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../test.h"

static mode_t saved_mask;
static uid_t saved_uid, saved_euid;
static gid_t saved_gid, saved_egid;
static int saved;

void cleanup(void)
{
    if (saved)
    {
        umask(saved_mask);
        setuid(saved_uid);
        seteuid(saved_euid);
        setgid(saved_gid);
        setegid(saved_egid);
        saved = 0;
    }
}

static int report(const char *label)
{
    mode_t mask = umask(077);
    uid_t uid = getuid(), euid = geteuid();
    gid_t gid = getgid(), egid = getegid();
    umask(mask);
    printf("%s mask=%03o uid=%lu euid=%lu gid=%lu egid=%lu\n", label,
        (unsigned)mask, (unsigned long)uid, (unsigned long)euid,
        (unsigned long)gid, (unsigned long)egid);
    fflush(stdout);
    return mask == 077 && uid == 123 && euid == 125 &&
        gid == 124 && egid == 126;
}

int main(int argc, char **argv)
{
    char program[1024], name[512];
    mode_t oldmask;
    uid_t uid, euid;
    gid_t gid, egid;
    pid_t child, waited = 0;
    int status = -1, error = 0;
    volatile int rc = 20;
    unsigned polls;
    if (argc == 2 && !strcmp(argv[1], "--credentials-child"))
        return report("EXEC_CREDENTIALS_CHILD") ? 0 : 21;
    if (argc != 1)
        return FAIL;
    if (!GetProgramName((STRPTR)name, sizeof(name)))
        return FAIL;
    if (snprintf(program, sizeof(program), "PROGDIR:%s", FilePart((CONST_STRPTR)name))
        >= (int)sizeof(program))
        return FAIL;
    uid = getuid();
    euid = geteuid();
    gid = getgid();
    egid = getegid();
    oldmask = umask(077);
    saved_mask = oldmask;
    saved_uid = uid;
    saved_euid = euid;
    saved_gid = gid;
    saved_egid = egid;
    saved = 1;
    if (setuid(123) != 0 || seteuid(125) != 0 || setgid(124) != 0 || setegid(126) != 0)
        goto restore;
    if (!report("EXEC_CREDENTIALS_PROGRAM"))
        goto restore;
    child = vfork();
    if (child == 0)
    {
        char *const args[] = {program, "--credentials-child", NULL};
        char *const env[] = {"CREDENTIAL_CONTROL=explicit", NULL};
        execve(program, args, env);
        _exit(123);
    }
    error = errno;
    if (child < 0)
    {
        printf("EXEC_CREDENTIALS_SPAWN errno=%d\n", error);
        goto restore;
    }
    for (polls = 0; polls < 300; ++polls)
    {
        waited = waitpid(child, &status, WNOHANG);
        error = errno;
        if (waited != 0)
            break;
        Delay(1);
    }
    printf("EXEC_CREDENTIALS_WAIT child=%ld waited=%ld status=%d errno=%d polls=%u\n",
        (long)child, (long)waited, status, error, polls);
    rc = waited == child && status == 0 ? 0 : 20;
restore:
    cleanup();
    printf("EXEC_CREDENTIALS rc=%d restored_mask=%03o uid=%lu euid=%lu gid=%lu egid=%lu\n",
        rc, (unsigned)oldmask, (unsigned long)getuid(), (unsigned long)geteuid(),
        (unsigned long)getgid(), (unsigned long)getegid());
    TEST(rc == 0);
    return OK;
}
