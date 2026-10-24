/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#include "socket_native.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int aros_rs_socket(int, int, int);
int aros_rs_poll(struct aros_socket_pollfd *, unsigned int, int);
_Static_assert(sizeof(struct aros_socket_pollfd)==8 && offsetof(struct aros_socket_pollfd,events)==4 && offsetof(struct aros_socket_pollfd,revents)==6, "pollfd C/libc ABI");
int aros_rs_bind(int, const struct sockaddr *, uint32_t);
int aros_rs_connect(int, const struct sockaddr *, uint32_t);
int aros_rs_listen(int, int);
int aros_rs_accept(int, struct sockaddr *, uint32_t *);
int aros_rs_getsockname(int, struct sockaddr *, uint32_t *);
int aros_rs_getpeername(int, struct sockaddr *, uint32_t *);
int aros_rs_getsockopt(int, int, int, void *, uint32_t *);
int aros_rs_setsockopt(int, int, int, const void *, uint32_t);
int aros_rs_shutdown(int, int);
int aros_rs_socket_clone(int);
int aros_rs_socket_nonblocking(int, int);
int aros_rs_socket_close(int);
int aros_rs_socket_wait_writable(int, int);
int aros_rs_socketpair(int, int, int, int *);
ssize_t aros_rs_sendmsg(int, const struct msghdr *, int);
ssize_t aros_rs_recvmsg(int, struct msghdr *, int);
ssize_t aros_rs_send(int, const void *, size_t, int);
ssize_t aros_rs_recv(int, void *, size_t, int);
ssize_t aros_rs_sendto(int, const void *, size_t, int, const struct sockaddr *, uint32_t);
ssize_t aros_rs_recvfrom(int, void *, size_t, int, struct sockaddr *, uint32_t *);

_Static_assert(AF_INET6 == 28 && sizeof(struct sockaddr_in) == 16 && sizeof(struct sockaddr_in6) == 28, "AROS network ABI");
_Static_assert(sizeof(struct aros_socket_request) == 64, "native request LP64 size");
_Static_assert(offsetof(struct aros_socket_request, result) == 24, "native result offset");
_Static_assert(offsetof(struct aros_socket_request, address_length) == 56, "native addrlen pointer offset");
static struct aros_socket_request expected;
static int calls, error_response, cases;
static int fcntl_response=-1, fcntl_error=EBADF;
int aros_fixture_fcntl(int fd,int command,...) {
    assert(fd>=0 && fd<=2 && command==F_GETFD);
    if(fcntl_response<0)errno=fcntl_error;
    return fcntl_response;
}
static ssize_t return_value;
static int address_copy;
static const void *original_address;
static unsigned char address_bytes[128];
static uint32_t address_size;
static IPTR return_length;
static int write_length, receive_data;

int aros_socket_native_call(struct aros_socket_request *r) {
    calls++;
    assert(r->version == AROS_SOCKET_ABI && r->operation == expected.operation);
    assert(r->fd == expected.fd && r->arg0 == expected.arg0 && r->arg1 == expected.arg1 && r->arg2 == expected.arg2);
    assert(r->result == 0 && r->buffer == expected.buffer && r->length == expected.length);
    if (address_copy) {
        assert(r->address && r->address != original_address && r->address_length);
        assert(*r->address_length == address_size);
        assert(!memcmp(r->address, address_bytes, address_size));
    } else {
        assert(r->address == expected.address && r->address_length == expected.address_length);
        if (r->address_length) assert(*r->address_length == address_size);
    }
    if (!error_response) {
        if (receive_data) { assert(r->length >= 3); memcpy(r->buffer, "xyz", 3); }
        if (!address_copy && r->address) {
            memset(r->address, 0x42, address_size);
            ((struct sockaddr *)r->address)->sa_family = AF_INET;
            ((struct sockaddr *)r->address)->sa_len = 16;
            *r->address_length = 16;
        }
        if (write_length) r->length = return_length;
    }
    r->result = return_value;
    return error_response;
}
static void reset(ULONG operation) {
    memset(&expected, 0, sizeof(expected)); expected.operation = operation;
    expected.fd = 41; calls = error_response = address_copy = write_length = receive_data = 0;
    original_address = NULL; address_size = 0; return_length = 0; return_value = 0;
    errno = EINTR;
}
static void result(ssize_t got) {
    assert(calls == 1);
    if (error_response) assert(got == -1 && errno == error_response);
    else assert(got == return_value && errno == EINTR);
    cases++;
}
static void invalid(ssize_t got, int error) { assert(got == -1 && errno == error && !calls); cases++; }
static void copied_address(const void *address, uint32_t size) {
    original_address = address; address_size = size; address_copy = 1;
    memcpy(address_bytes, address, size); address_bytes[0] = (unsigned char)size;
}
static void dispatch_case(int op, int error) {
    struct sockaddr_in input = {0, AF_INET, 0x1234, 0x01020304, {0}};
    struct sockaddr_storage output;
    char buffer[12] = "hello";
    uint32_t length = sizeof(output);
    reset((ULONG)op); error_response = error;
    switch (op) {
    case AROS_SOCKET_CREATE:
        expected.fd = 0; expected.arg0 = AF_INET6; expected.arg1 = 2; expected.arg2 = 17; return_value = 81;
        result(aros_rs_socket(AF_INET6, 2, 17)); break;
    case AROS_SOCKET_BIND:
    case AROS_SOCKET_CONNECT:
        copied_address(&input, sizeof(input));
        result(op == AROS_SOCKET_BIND ? aros_rs_bind(41, (void *)&input, sizeof(input)) : aros_rs_connect(41, (void *)&input, sizeof(input)));
        assert(input.sin_len == 0 && input.sin_family == AF_INET && input.sin_addr == 0x01020304); break;
    case AROS_SOCKET_LISTEN:
        expected.arg0 = 97; result(aros_rs_listen(41, 97)); break;
    case AROS_SOCKET_ACCEPT:
    case AROS_SOCKET_LOCALNAME:
    case AROS_SOCKET_PEERNAME:
        expected.address = &output; expected.address_length = &length; address_size = length;
        if (op == AROS_SOCKET_ACCEPT) return_value = 82;
        result(op == AROS_SOCKET_ACCEPT ? aros_rs_accept(41, (void *)&output, &length) :
               op == AROS_SOCKET_LOCALNAME ? aros_rs_getsockname(41, (void *)&output, &length) :
               aros_rs_getpeername(41, (void *)&output, &length));
        assert(length == (error ? sizeof(output) : 16));
        if (!error) assert(output.ss_family == AF_INET && output.ss_len == 16); break;
    case AROS_SOCKET_GETOPT:
        length = sizeof(buffer); expected.arg0 = -1; expected.arg1 = 0x1007;
        expected.buffer = buffer; expected.length = length; write_length = 1; return_length = 4;
        result(aros_rs_getsockopt(41, -1, 0x1007, buffer, &length)); assert(length == (error ? sizeof(buffer) : 4)); break;
    case AROS_SOCKET_SETOPT:
        expected.arg0 = 0xffff; expected.arg1 = 0x1008; expected.buffer = buffer; expected.length = sizeof(buffer);
        result(aros_rs_setsockopt(41, 0xffff, 0x1008, buffer, sizeof(buffer))); break;
    case AROS_SOCKET_SHUTDOWN:
        expected.arg0 = 2; result(aros_rs_shutdown(41, 2)); break;
    case AROS_SOCKET_CLONE:
        return_value = 91; result(aros_rs_socket_clone(41)); break;
    case AROS_SOCKET_CLOSE:
        result(aros_rs_socket_close(41)); break;
    case AROS_SOCKET_NONBLOCK:
        expected.arg0 = 1; result(aros_rs_socket_nonblocking(41, -9)); break;
    case AROS_SOCKET_SEND:
        expected.buffer = buffer; expected.length = sizeof(buffer); expected.arg0 = 0x123;
        return_value = 3; result(aros_rs_send(41, buffer, sizeof(buffer), 0x123)); break;
    case AROS_SOCKET_RECV:
        expected.buffer = buffer; expected.length = sizeof(buffer); expected.arg0 = 0x321;
        return_value = 3; receive_data = 1; result(aros_rs_recv(41, buffer, sizeof(buffer), 0x321));
        assert(!memcmp(buffer, error ? "hel" : "xyz", 3)); break;
    default: assert(0);
    }
}
static ssize_t destination(int op, const struct sockaddr *address, uint32_t length, char *buffer) {
    if (op == AROS_SOCKET_BIND) return aros_rs_bind(41, address, length);
    if (op == AROS_SOCKET_CONNECT) return aros_rs_connect(41, address, length);
    return aros_rs_sendto(41, buffer, 7, 19, address, length);
}
int main(void) {
    for (int op = 1; op <= AROS_SOCKET_NONBLOCK; op++) {
        dispatch_case(op, 0); dispatch_case(op, EACCES);
    }
    for (int family = 0; family < 2; family++) {
        struct sockaddr_in v4 = {0, AF_INET, 0x5678, 0x01020304, {0}};
        struct sockaddr_in6 v6 = {0}; v6.sin6_family = AF_INET6; v6.sin6_scope_id = 23;
        for (int i = 0; i < 16; i++) v6.sin6_addr[i] = (uint8_t)i;
        struct sockaddr *address = family ? (void *)&v6 : (void *)&v4;
        uint32_t length = family ? sizeof(v6) : sizeof(v4);
        unsigned char before[28]; memcpy(before, address, length);
        char buffer[7] = "abcdef";
        int operations[] = {AROS_SOCKET_BIND, AROS_SOCKET_CONNECT, AROS_SOCKET_SEND};
        for (unsigned i = 0; i < 3; i++) {
            for (int error = 0; error < 2; error++) {
                reset(operations[i]); copied_address(address, length); error_response = error ? EAGAIN : 0;
                if (operations[i] == AROS_SOCKET_SEND) {
                    expected.buffer = buffer; expected.length = sizeof(buffer); expected.arg0 = 19; return_value = 2;
                }
                result(destination(operations[i], address, length, buffer)); assert(!memcmp(before, address, length));
            }
            reset(operations[i]); invalid(destination(operations[i], NULL, length, buffer), EINVAL);
            reset(operations[i]); invalid(destination(operations[i], address, 1, buffer), EINVAL);
            reset(operations[i]); invalid(destination(operations[i], address, length - 1, buffer), EINVAL);
            reset(operations[i]); invalid(destination(operations[i], address, length + 1, buffer), EINVAL);
            reset(operations[i]); invalid(destination(operations[i], address, UINT32_MAX, buffer), EINVAL);
            address->sa_family = 30; /* Reject the Darwin IPv6 value. */
            reset(operations[i]); invalid(destination(operations[i], address, length, buffer), EAFNOSUPPORT);
            address->sa_family = family ? AF_INET6 : AF_INET;
        }
    }
    unsigned char one_byte = 0;
    struct { uint8_t length, family; } tiny = {0, AF_INET6};
    reset(AROS_SOCKET_BIND); invalid(aros_rs_bind(41, (void *)&one_byte, 1), EINVAL);
    reset(AROS_SOCKET_CONNECT); invalid(aros_rs_connect(41, (void *)&tiny, 2), EINVAL);
    char buffer[7] = {0}; struct sockaddr_storage address; uint32_t length;
    for (int op = 0; op < 4; op++) {
        reset(op == 0 ? AROS_SOCKET_ACCEPT : op == 1 ? AROS_SOCKET_LOCALNAME : op == 2 ? AROS_SOCKET_PEERNAME : AROS_SOCKET_RECV);
        length = sizeof(address);
        ssize_t got = op == 0 ? aros_rs_accept(41, NULL, &length) : op == 1 ? aros_rs_getsockname(41, NULL, &length) :
            op == 2 ? aros_rs_getpeername(41, NULL, &length) : aros_rs_recvfrom(41, buffer, sizeof(buffer), 0, NULL, &length);
        invalid(got, EINVAL);
        reset(expected.operation);
        got = op == 0 ? aros_rs_accept(41, (void *)&address, NULL) : op == 1 ? aros_rs_getsockname(41, (void *)&address, NULL) :
            op == 2 ? aros_rs_getpeername(41, (void *)&address, NULL) : aros_rs_recvfrom(41, buffer, sizeof(buffer), 0, (void *)&address, NULL);
        invalid(got, EINVAL);
    }
    reset(AROS_SOCKET_ACCEPT); return_value = 84; result(aros_rs_accept(41, NULL, NULL));
    for (int pair = 0; pair < 2; pair++) for (int error = 0; error < 2; error++) {
        reset(AROS_SOCKET_RECV); length = sizeof(address); expected.buffer = buffer; expected.length = sizeof(buffer);
        expected.arg0 = 5; return_value = 2; error_response = error ? EWOULDBLOCK : 0; receive_data = 1;
        if (pair) { expected.address = &address; expected.address_length = &length; address_size = length; }
        result(aros_rs_recvfrom(41, buffer, sizeof(buffer), 5, pair ? (void *)&address : NULL, pair ? &length : NULL));
        if (pair) assert(length == (error ? sizeof(address) : 16));
    }
    reset(AROS_SOCKET_GETOPT); invalid(aros_rs_getsockopt(41, 0, 0, buffer, NULL), EINVAL);
    reset(AROS_SOCKET_GETOPT); length = 7; expected.buffer = buffer; expected.length = length; write_length = 1; return_length = (IPTR)UINT32_MAX + 1;
    assert(aros_rs_getsockopt(41, 0, 0, buffer, &length) == -1 && errno == EOVERFLOW && length == 7 && calls == 1); cases++;
    reset(AROS_SOCKET_GETOPT); length = 7; expected.buffer = buffer; expected.length = length; write_length = 1; return_length = UINT32_MAX;
    result(aros_rs_getsockopt(41, 0, 0, buffer, &length)); assert(length == UINT32_MAX);
    reset(AROS_SOCKET_NONBLOCK); result(aros_rs_socket_nonblocking(41, 0));
    reset(AROS_SOCKET_SEND); return_value = 0; result(aros_rs_send(41, NULL, 0, 0));
    reset(AROS_SOCKET_RECV); return_value = 0; result(aros_rs_recv(41, NULL, 0, 0));
    reset(AROS_SOCKET_SEND); expected.buffer = buffer; expected.length = (size_t)UINT32_MAX + 7; return_value = (ssize_t)UINT32_MAX + 1;
    result(aros_rs_send(41, buffer, expected.length, 0)); /* no buffer dereference: size_t/ssize_t transport */
    int timeouts[] = {-1, 0, 23, INT_MAX};
    for (unsigned t = 0; t < sizeof(timeouts) / sizeof(timeouts[0]); t++) {
        for (int mask = 0; mask <= 7; mask++) {
            reset(AROS_SOCKET_POLL); expected.arg0 = AROS_SOCKET_POLL_WRITE;
            expected.arg1 = timeouts[t]; return_value = mask;
            int writable = aros_rs_socket_wait_writable(41, timeouts[t]);
            assert(calls == 1 && errno == EINTR);
            assert(writable == !!(mask & AROS_SOCKET_POLL_WRITE)); cases++;
        }
    }
    int poll_errors[] = {EINVAL, EINTR, EBADF};
    for (unsigned e = 0; e < sizeof(poll_errors) / sizeof(poll_errors[0]); e++) {
        reset(AROS_SOCKET_POLL); expected.arg0 = AROS_SOCKET_POLL_WRITE;
        expected.arg1 = 29; return_value = AROS_SOCKET_POLL_WRITE; error_response = poll_errors[e];
        result(aros_rs_socket_wait_writable(41, 29));
    }
    int descriptors[] = {101, 102};
    reset(0); invalid(aros_rs_socketpair(AF_INET, 1, 0, descriptors), EAFNOSUPPORT);
    assert(descriptors[0] == 101 && descriptors[1] == 102);
    reset(0); invalid(aros_rs_socketpair(AF_INET6, 2, 17, NULL), EAFNOSUPPORT);
    char control[8] = "control";
    unsigned char address_before[sizeof(address)]; memcpy(address_before, &address, sizeof(address));
    struct msghdr message = {&address, sizeof(address), (void *)1, 9, control, sizeof(control), 0x400};
    unsigned char message_before[sizeof(message)]; memcpy(message_before, &message, sizeof(message));
    reset(0); invalid(aros_rs_sendmsg(41, &message, 7), EOPNOTSUPP);
    assert(!memcmp(message_before, &message, sizeof(message)) && !memcmp(address_before, &address, sizeof(address)));
    assert(!memcmp(control, "control", sizeof(control)));
    reset(0); invalid(aros_rs_recvmsg(41, &message, 8), EOPNOTSUPP);
    assert(!memcmp(message_before, &message, sizeof(message)) && !memcmp(address_before, &address, sizeof(address)));
    assert(!memcmp(control, "control", sizeof(control)));
    reset(0); invalid(aros_rs_sendmsg(41, NULL, 0), EOPNOTSUPP);
    reset(0); invalid(aros_rs_recvmsg(41, NULL, 0), EOPNOTSUPP);
    struct aros_socket_pollfd pollfds[2]={{41,AROS_POLL_IN,0},{42,AROS_POLL_OUT,0}};
    reset(AROS_SOCKET_POLL_BATCH); expected.fd=0; expected.buffer=pollfds; expected.length=2;
    expected.arg1=23; return_value=2; result(aros_rs_poll(pollfds,2,23));
    reset(AROS_SOCKET_POLL_BATCH); expected.fd=0; expected.arg1=0; return_value=0;
    result(aros_rs_poll(NULL,0,0));
    reset(AROS_SOCKET_POLL_BATCH); expected.fd=0; expected.buffer=pollfds; expected.length=2;
    expected.arg1=-1; error_response=EINTR; result(aros_rs_poll(pollfds,2,-1));
    reset(0); invalid(aros_rs_poll(NULL,1,0),EFAULT);
    reset(0); invalid(aros_rs_poll(NULL,0,-2),EINVAL);
    struct aros_socket_pollfd local={0,AROS_POLL_IN,AROS_POLL_NVAL};
    reset(AROS_SOCKET_POLL_BATCH); expected.fd=0; expected.buffer=&local; expected.length=1;return_value=1;
    fcntl_response=0;assert(aros_rs_poll(&local,1,0)==-1 && errno==EOPNOTSUPP && calls==1);cases++;
    reset(AROS_SOCKET_POLL_BATCH); expected.fd=0; expected.buffer=&local; expected.length=1;return_value=1;
    fcntl_response=-1;fcntl_error=EBADF;result(aros_rs_poll(&local,1,0));
    reset(AROS_SOCKET_POLL_BATCH); expected.fd=0; expected.buffer=&local; expected.length=1;return_value=1;
    fcntl_error=ENOMEM;assert(aros_rs_poll(&local,1,0)==-1 && errno==ENOMEM && calls==1);cases++;
    printf("PASS %d socket bridge cases, exact dispatch records, AROS IPv4/IPv6, synchronous address copies\n", cases);
    return 0;
}
