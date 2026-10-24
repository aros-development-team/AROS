/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef TEST_NETINET_IN_H
#define TEST_NETINET_IN_H
#include <sys/socket.h>
struct sockaddr_in { uint8_t sin_len, sin_family; uint16_t sin_port; uint32_t sin_addr; char sin_zero[8]; };
struct sockaddr_in6 { uint8_t sin6_len, sin6_family; uint16_t sin6_port; uint32_t sin6_flowinfo; uint8_t sin6_addr[16]; uint32_t sin6_scope_id; };
#endif
