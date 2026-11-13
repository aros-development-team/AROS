/*
 * This function is not written. The idea was to provide a quick drop-in
 * replacement for fork()/execve() pair. It was planned for use in dhclient
 * but was cancelled due to architectural change
 */

int ExecVEAsync(const char *filename, char *const argv[], char *const envp[])
{
    return 0;
}
