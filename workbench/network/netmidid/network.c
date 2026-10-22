/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/


#include "network.h"

#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <dos/dos.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <proto/bsdsocket.h>
#include <proto/entropy.h>
#include <proto/exec.h>
#include <resources/entropy.h>
struct Library *SocketBase;
struct Library *EntropyBase;

int netmidi_network_random(void *buffer, size_t length)
{
    /* the base the call's inline uses; resources are not closed */
    if (!EntropyBase)
        EntropyBase = OpenResource((CONST_STRPTR)ENTROPYNAME);
    if (!EntropyBase || !buffer)
        return -1;
    return GetEntropy(buffer, (ULONG)length) == (LONG)length ? 0 : -1;
}

void netmidi_network_close_socket(int fd)
{
    if (fd < 0)
        return;
    CloseSocket(fd);
}

void netmidi_network_init(struct netmidi_network_runtime *runtime)
{
    int i;

    memset(runtime, 0, sizeof(*runtime));
    for (i = 0; i < NETMIDI_NETWORK_SOCKETS; ++i)
        runtime->socket[i] = -1;
}

int netmidi_network_platform_open(struct netmidi_network_runtime *runtime)
{
    if (!runtime)
        return -1;
    SocketBase = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    if (!SocketBase)
        return -1;
    runtime->platform_opened = 1;
    return 0;
}

static int open_udp(uint16_t port)
{
    struct sockaddr_in address;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return -1;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(fd, (const struct sockaddr *)&address, sizeof(address)) < 0) {
        netmidi_network_close_socket(fd);
        return -1;
    }
    return fd;
}

int netmidi_network_open(struct netmidi_network_runtime *runtime,
                         uint16_t port)
{
    int i;

    if (!runtime || !runtime->platform_opened || !port)
        return -1;
    for (i = 0; i < NETMIDI_NETWORK_SOCKETS; ++i)
        if (runtime->socket[i] < 0) {
            runtime->socket[i] = open_udp(port);
            return runtime->socket[i] < 0 ? -1 : i;
        }
    return -1;
}

void netmidi_network_close(struct netmidi_network_runtime *runtime)
{
    int i;

    if (!runtime)
        return;
    for (i = NETMIDI_NETWORK_SOCKETS - 1; i >= 0; --i) {
        netmidi_network_close_socket(runtime->socket[i]);
        runtime->socket[i] = -1;
    }
    if (runtime->platform_opened && SocketBase) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
    runtime->platform_opened = 0;
}

static int socket_fd(const struct netmidi_network_runtime *runtime,
                     int number)
{
    if (!runtime || number < 0 || number >= NETMIDI_NETWORK_SOCKETS)
        return -1;
    return runtime->socket[number];
}

int netmidi_network_send(const struct netmidi_network_runtime *runtime,
                         int number, const void *bytes, size_t length,
                         const struct sockaddr_in *destination)
{
    int fd = socket_fd(runtime, number);
    if (fd < 0 || !bytes || !destination)
        return -1;
    return sendto(fd, bytes, (int)length, 0,
                  (const struct sockaddr *)destination,
                  sizeof(*destination)) == (int)length ? 0 : -1;
}

int netmidi_network_receive(const struct netmidi_network_runtime *runtime,
                            int number, void *bytes, size_t capacity,
                            struct sockaddr_in *source,
                            socklen_t *source_length)
{
    int fd = socket_fd(runtime, number);
    if (fd < 0 || !bytes || !source || !source_length)
        return -1;
    return recvfrom(fd, bytes, (int)capacity, 0,
                    (struct sockaddr *)source, source_length);
}

int netmidi_network_wait(const struct netmidi_network_runtime *runtime,
                         long timeout_us, int wake_signal_bit,
                         unsigned int *events)
{
    struct timeval timeout;
    fd_set read_set;
    int maximum = -1;
    int ready;
    int i;
    ULONG signal_mask = SIGBREAKF_CTRL_C;

    if (!runtime || !events)
        return -1;
    *events = 0;
    FD_ZERO(&read_set);
    for (i = 0; i < NETMIDI_NETWORK_SOCKETS; ++i)
        if (runtime->socket[i] >= 0) {
            FD_SET(runtime->socket[i], &read_set);
            if (runtime->socket[i] > maximum)
                maximum = runtime->socket[i];
        }
    timeout.tv_sec = timeout_us / 1000000;
    timeout.tv_usec = timeout_us % 1000000;
    if (wake_signal_bit >= 0)
        signal_mask |= 1UL << wake_signal_bit;
    ready = WaitSelect(maximum + 1, &read_set, NULL, NULL, &timeout,
                       &signal_mask);
    if (signal_mask & SIGBREAKF_CTRL_C)
        *events |= NETMIDI_NETWORK_STOP;
    if (ready > 0)
        for (i = 0; i < NETMIDI_NETWORK_SOCKETS; ++i)
            if (runtime->socket[i] >= 0 &&
                FD_ISSET(runtime->socket[i], &read_set))
                *events |= 1u << i;
    return ready < 0 ? -1 : 0;
}

int netmidi_network_hostname(char *name, size_t size)
{
    size_t i;

    if (!name || !size)
        return -1;
    name[0] = 0;
    if (!SocketBase)
        return -1;
    if (gethostname(name, size) != 0) {
        name[0] = 0;
        return -1;
    }
    name[size - 1] = 0;
    for (i = 0; name[i]; ++i)
        if (name[i] == '.') {
            name[i] = 0;
            break;
        }
    return name[0] ? 0 : -1;
}
