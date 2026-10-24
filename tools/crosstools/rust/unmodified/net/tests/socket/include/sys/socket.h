/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef TEST_SYS_SOCKET_H
#define TEST_SYS_SOCKET_H
#include <stdint.h>
#define AF_INET 2
#define AF_INET6 28
struct sockaddr { uint8_t sa_len, sa_family; char sa_data[14]; };
struct sockaddr_storage { uint8_t ss_len, ss_family; char pad1[6]; int64_t align; char pad2[112]; };
struct iovec;
struct msghdr {
    void *msg_name;
    uint32_t msg_namelen;
    struct iovec *msg_iov;
    int msg_iovlen;
    void *msg_control;
    uint32_t msg_controllen;
    int msg_flags;
};
#endif
