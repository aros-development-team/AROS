/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES
 * ARE DISCLAIMED.
 */

#ifndef IP_H
#define IP_H

#include <sys/types.h>
#include <sys/socket.h>

/* Global options shared by all sub-commands */
struct ipcmd_opts {
	int family;	/* AF_INET, AF_INET6, AF_LINK, or AF_UNSPEC */
	int stats;	/* -s: show statistics (increments for -s -s) */
	int detail;	/* -d: show details */
	int resolve;	/* -r: resolve names */
	int numeric;	/* -n: numeric output */
};

extern struct ipcmd_opts opts;

/* Sub-command entry points */
int do_iplink(int argc, char **argv);
int do_ipaddr(int argc, char **argv);
int do_iproute(int argc, char **argv);
int do_ipneigh(int argc, char **argv);
int do_ipmaddr(int argc, char **argv);
int do_ipmroute(int argc, char **argv);
int do_iprule(int argc, char **argv);
int do_ipnetns(int argc, char **argv);

/* Utility functions */

/*
 * Format a sockaddr as a printable string.  Returns a pointer to a
 * static buffer (not thread-safe).
 */
const char *format_sockaddr(const struct sockaddr *sa, char *buf, size_t len);

/*
 * Format a link-layer address (MAC) as xx:xx:xx:xx:xx:xx.
 */
const char *format_lladdr(const unsigned char *addr, int len,
    char *buf, size_t buflen);

/*
 * Format interface flags as a comma-separated string.
 */
void format_flags(unsigned int flags, char *buf, size_t buflen);

/*
 * Count prefix length from a network mask.
 */
int mask2prefix(const struct sockaddr *sa);

/*
 * Open a socket for ioctl operations.
 * Returns socket fd or -1 on error.
 */
int ip_socket(int family);

#endif /* IP_H */
