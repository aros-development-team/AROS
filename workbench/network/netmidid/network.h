/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI_NETWORK_H
#define NETMIDI_NETWORK_H

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct Library;
/* bsdsocket.library's base, opened by netmidi_network_platform_open: the
 * inline stubs of whatever is linked with this use the one. */
extern struct Library *SocketBase;

/* The service's UDP sockets, each bound to a port of its own and known by
 * its number here. Which protocol speaks on which is not known here. */
#define NETMIDI_NETWORK_SOCKETS 4

struct netmidi_network_runtime {
    int socket[NETMIDI_NETWORK_SOCKETS];    /* -1 where there is none */
    int platform_opened;
};

/* What netmidi_network_wait reports: a bit to each socket that has a
 * datagram waiting, by its number, and this when the task was told to
 * stop. */
#define NETMIDI_NETWORK_STOP (1u << NETMIDI_NETWORK_SOCKETS)

void netmidi_network_init(struct netmidi_network_runtime *runtime);
/* This machine's name on the network, without its domain; after
 * netmidi_network_platform_open. Returns -1, with name empty, when it has
 * none. */
int netmidi_network_hostname(char *name, size_t size);
int netmidi_network_platform_open(struct netmidi_network_runtime *runtime);
/* A UDP socket bound to port on every interface. Returns its number, or
 * -1 when it cannot be had. */
int netmidi_network_open(struct netmidi_network_runtime *runtime,
                         uint16_t port);
void netmidi_network_close(struct netmidi_network_runtime *runtime);
void netmidi_network_close_socket(int fd);
int netmidi_network_send(const struct netmidi_network_runtime *runtime,
                         int number, const void *bytes, size_t length,
                         const struct sockaddr_in *destination);
int netmidi_network_receive(const struct netmidi_network_runtime *runtime,
                            int number, void *bytes, size_t capacity,
                            struct sockaddr_in *source,
                            socklen_t *source_length);
/* Fills the buffer from the system's random source: entropy.resource on
 * AROS. Returns -1 when there is none or it gave less than was asked. */
int netmidi_network_random(void *buffer, size_t length);
/* Waits up to timeout_us for a datagram on any socket, or, on AROS, for
 * wake_signal_bit or Ctrl-C. */
int netmidi_network_wait(const struct netmidi_network_runtime *runtime,
                         long timeout_us, int wake_signal_bit,
                         unsigned int *events);

#endif
