/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* netmidid's core: the configuration, the sockets, the ports of the CAMD
 * drivers, the counts it publishes, the system's mDNS service and the
 * loop that runs the transports (netmidid.h). */


#include "netmidid.h"

#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <dos/dos.h>
#include <proto/bsdsocket.h>

/* The transports, in the order they are asked. */
static const struct netmidid_transport *const transports[] = {
    &netmidid_applemidi,
    &netmidid_midi2,
    NULL
};
#define TRANSPORT_COUNT \
    ((int)(sizeof(transports) / sizeof(transports[0])) - 1)

uint64_t netmidid_now(void)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint64_t)time.tv_sec * 10000 + (uint64_t)time.tv_usec / 100;
}

uint64_t netmidid_elapsed(uint64_t now, uint64_t then)
{
    return now >= then ? now - then : 0;
}

uint32_t netmidid_random(uint64_t now)
{
    uint32_t value;
    if (netmidi_network_random(&value, sizeof(value)) != 0) {
        static int told;
        if (!told) {
            puts(TAG "no random source; identifiers are weak");
            srand((unsigned int)(now ^ (now >> 32)));
            told = 1;
        }
        value = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
    }
    return value;
}

int netmidid_ipv4(const char *text, uint32_t *address)
{
    struct in_addr bytes;

    bytes.s_addr = inet_addr(text);
    if (bytes.s_addr == INADDR_NONE)
        return -1;
    *address = ntohl(bytes.s_addr);
    return 0;
}

int netmidid_socket(struct netmidid *d, const struct netmidid_transport *t,
                    uint16_t port)
{
    int number = netmidi_network_open(&d->network, port);

    if (number >= 0)
        d->socket_owner[number] = t;
    return number;
}

int netmidid_port_keep(struct netmidid *d, int driver, int index,
                       const char *name)
{
    return applemidi_ports_keep(&d->ports[driver], index, name);
}

int netmidid_port_take(struct netmidid *d, int driver, const uint8_t *name,
                       size_t name_length, int owner)
{
    int port;

    if (d->shared_port)
        return 0;
    port = applemidi_ports_take(&d->ports[driver], name, name_length, owner);
    return port < 0 ? -1 : driver * NETMIDI_CAMD_PORTS + port;
}

void netmidid_port_release(struct netmidid *d, int driver, int owner)
{
    applemidi_ports_release(&d->ports[driver], owner);
}


/* Which transports run, of those in transports[], and the state each is
   given for a run. */
static int running[NETMIDID_TRANSPORTS + 1];
static void *state[NETMIDID_TRANSPORTS + 1];

/* What a CAMD program sends goes to every transport: each takes what was
 * sent to a port of its own. */
static int send_midi(void *context, const uint8_t *message, size_t length)
{
    struct netmidid *d = context;
    int result = -1;
    int i;

    if (!message || !length)
        return -1;
    if (message[0] == 0xf0) {
        size_t j;

        if (length < 2 || length > NETMIDID_SYSEX_MAX ||
            message[length - 1] != 0xf7)
            return -1;
        for (j = 1; j + 1 < length; ++j)
            if (message[j] & 0x80)
                return -1;
    }
    for (i = 0; i < TRANSPORT_COUNT; ++i)
        if (running[i] && transports[i]->send_midi &&
            transports[i]->send_midi(d, state[i], d->camd.port_now, message,
                                    length) == 0)
            result = 0;
    return result;
}

static void flush_midi(struct netmidid *d)
{
    int i;

    for (i = 0; i < TRANSPORT_COUNT; ++i)
        if (running[i] && transports[i]->flush)
            transports[i]->flush(d, state[i]);
}

/* Tells whoever asks what has passed on each port: about once a second. */
static void publish_counts(struct netmidid *d, uint64_t now)
{
    static struct netmidi_port_counts counts[NETMIDI_CAMD_PORTS *
                                             NETMIDI_CAMD_DRIVERS];
    unsigned long refused = 0;
    int i;

    if (d->shared_port || netmidid_elapsed(now, d->published) < 10000)
        return;
    d->published = now;
    memset(counts, 0, sizeof(counts));
    for (i = 0; i < TRANSPORT_COUNT; ++i)
        if (running[i] && transports[i]->counts)
            refused += transports[i]->counts(d, state[i], counts);
    netmidi_camd_publish(&d->camd, d->session_name, refused,
                         counts, NETMIDI_CAMD_PORTS * NETMIDI_CAMD_DRIVERS);
}

/* Outlives a run: a change of the configuration that leaves a service as
   it was leaves its advertisement alone. */
static struct netmidi_discovery discovery;
static int discovered;              /* services advertised, as last seen */

/* A turn of the loop for everything but the sockets. Returns 1 when a
 * transport says the program is done. */
static int periodic(struct netmidid *d, uint64_t now)
{
    int done = 0;
    int i;

    publish_counts(d, now);
    /* The system's mDNS service may come up after us, or again. */
    {
        int registered = netmidi_discovery_poll(d->discovery, now);

        if (registered > discovered)
            puts(TAG "advertised by the system mDNS service");
        if (registered >= 0)
            discovered = registered;
    }
    for (i = 0; i < TRANSPORT_COUNT; ++i)
        if (running[i] && transports[i]->periodic &&
            transports[i]->periodic(d, state[i], now))
            done = 1;
    return done;
}

/* What a configuration file looked like when it was read, to know when it
   has been changed. */
struct config_watch {
    const char *path;           /* NULL: none to watch */
    int exists;
    long modified;
    long size;
};

static void config_look(struct config_watch *watch)
{
    struct stat status;

    watch->exists = 0;
    watch->modified = watch->size = 0;
    if (watch->path && stat(watch->path, &status) == 0) {
        watch->exists = 1;
        watch->modified = (long)status.st_mtime;
        watch->size = (long)status.st_size;
    }
}

static int config_changed(const struct config_watch *watch)
{
    struct config_watch now = *watch;

    if (!watch->path)
        return 0;
    config_look(&now);
    return now.exists != watch->exists || now.modified != watch->modified ||
           now.size != watch->size;
}

#define RUN_AGAIN 100       /* the configuration changed: begin anew */


/* All the service is told comes from its configuration. */
static int run(void)
{
    static struct netmidid rt;
    struct netmidid *d = &rt;
    struct config_watch watch = {NULL, 0, 0, 0};
    uint64_t watched = 0;
    uint64_t now;
    uint64_t next_due;
    uint64_t wait_ticks;
    unsigned int network_events;
    long wait_us;
    int opened[NETMIDID_TRANSPORTS + 1];
    int any = 0;
    int result = 20;
    int done;
    int i;

    memset(d, 0, sizeof(*d));
    memset(running, 0, sizeof(running));
    memset(opened, 0, sizeof(opened));
    netmidi_config_defaults(&d->config);
    {
        /* ENV: is where a change is made to be seen at once; the copy in
           ENVARC: is read when there is none, and not watched */
        int config_result;

        watch.path = "ENV:NetMIDI";
        config_result = netmidi_config_load("ENV:NetMIDI", &d->config);
        if (config_result == 1)
            config_result = netmidi_config_load("ENVARC:NetMIDI", &d->config);
        if (config_result < 0) {
            fputs(TAG "invalid network preferences\n", stderr);
            return 20;
        }
    }
    netmidi_network_init(&d->network);
    d->latency = (uint32_t)d->config.latency_ms * 10;
    d->discovery = &discovery;
    netmidi_discovery_begin(d->discovery);

    if (netmidi_network_platform_open(&d->network) != 0) {
        fputs(TAG "bsdsocket.library unavailable\n", stderr);
        return 20;
    }
    /* A session nobody named is called as the machine is, so that two
       machines on a network are told apart. */
    if (!d->config.session_name_given) {
        char host[sizeof(d->config.session_name)];

        if (netmidi_network_hostname(host, sizeof(host)) == 0)
            snprintf(d->config.session_name, sizeof(d->config.session_name),
                     "%s", host);
    }
    /* the two are of one size */
    memcpy(d->session_name, d->config.session_name, sizeof(d->session_name));
    d->session_name[sizeof(d->session_name) - 1] = 0;
    config_look(&watch);
    if (netmidi_camd_open(&d->camd) != 0) {
        fputs(TAG "cannot initialize CAMD adapters\n", stderr);
        goto cleanup;
    }
    d->shared_port = netmidi_camd_ports(&d->camd) == 1;
    for (i = 0; i < NETMIDI_CAMD_DRIVERS; ++i)
        applemidi_ports_init(&d->ports[i], d->shared_port ? 0 :
                                           netmidi_camd_ports(&d->camd));
    for (i = 0; i < TRANSPORT_COUNT; ++i) {
        int status;

        state[i] = calloc(1, transports[i]->state_size);
        if (!state[i]) {
            fputs(TAG "not enough memory\n", stderr);
            goto cleanup;
        }
        status = transports[i]->open(d, state[i]);

        opened[i] = 1;
        if (status < 0)
            goto cleanup;
        running[i] = status == 0;
        any |= running[i];
    }
    if (!any) {
        fputs(TAG "the configuration asks for no transport\n", stderr);
        goto cleanup;
    }
    now = netmidid_now();
    if (netmidi_discovery_poll(d->discovery, now) < 0)
        puts(TAG "no system mDNS service: not advertised");
    for (i = 0; i < TRANSPORT_COUNT; ++i)
        if (running[i] && transports[i]->start &&
            transports[i]->start(d, state[i], now) != 0)
            goto cleanup;

    for (;;) {
        wait_us = 100000;
        for (i = 0; i < TRANSPORT_COUNT; ++i) {
            if (!running[i] || !transports[i]->next_due ||
                !transports[i]->next_due(d, state[i], &next_due))
                continue;
            now = netmidid_now();
            wait_ticks = next_due > now ? next_due - now : 0;
            if (wait_ticks < 1000 && (long)(wait_ticks * 100) < wait_us)
                wait_us = (long)(wait_ticks * 100);
        }
        netmidi_network_wait(&d->network, wait_us,
                             netmidi_camd_signal_bit(&d->camd),
                             &network_events);
        if (network_events & NETMIDI_NETWORK_STOP)
            break;
        for (i = 0; i < NETMIDI_NETWORK_SOCKETS; ++i) {
            int t;

            if (!(network_events & (1u << i)) || !d->socket_owner[i])
                continue;
            for (t = 0; t < TRANSPORT_COUNT; ++t)
                if (running[t] && transports[t] == d->socket_owner[i] &&
                    transports[t]->receive)
                    transports[t]->receive(d, state[t], i);
        }
        done = periodic(d, netmidid_now());
        /* A changed configuration is taken by beginning anew: sessions
           end with a goodbye and are made again under what it says now.
           Looked at every two seconds. */
        if (watch.path && netmidid_elapsed(netmidid_now(), watched) >= 20000) {
            watched = netmidid_now();
            if (config_changed(&watch)) {
                puts(TAG "the configuration changed: beginning anew");
                result = RUN_AGAIN;
                goto cleanup;
            }
        }
        netmidi_camd_poll(&d->camd, send_midi, d);
        flush_midi(d);
        if (done)
            break;
    }
    result = 0;

cleanup:
    for (i = 0; i < TRANSPORT_COUNT; ++i)
        if (running[i] && transports[i]->stop)
            transports[i]->stop(d, state[i]);
    netmidi_camd_close(&d->camd);
    netmidi_network_close(&d->network);
    for (i = 0; i < TRANSPORT_COUNT; ++i) {
        if (opened[i] && transports[i]->release)
            transports[i]->release(d, state[i]);
        free(state[i]);
        state[i] = NULL;
    }
    memset(running, 0, sizeof(running));
    return result;
}

int main(void)
{
    int result;

    /* Started by the stack's service manager: no console, no arguments.
       What it would print goes nowhere, or to the file ENV:netmidid.log
       names, for whoever wants to see it. */
    {
        const char *log = getenv("netmidid.log");
        FILE *file = NULL;

        /* with nowhere to print to, what is printed is lost: no reason
           not to run. Errors go to the log as well, after what was
           printed before them: the file is emptied once, then both
           streams append to it. */
        if (log && *log)
            file = fopen(log, "w");
        if (!file) {
            (void)freopen("NIL:", "w", stdout);
            (void)freopen("NIL:", "w", stderr);
        } else {
            fclose(file);
            if (!freopen(log, "a", stdout))
                (void)freopen("NIL:", "w", stdout);
            if (!freopen(log, "a", stderr))
                (void)freopen("NIL:", "w", stderr);
        }
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    netmidi_discovery_init(&discovery);
    do
        result = run();
    while (result == RUN_AGAIN);
    netmidi_discovery_close(&discovery);
    return result;
}
