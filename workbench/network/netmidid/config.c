/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <netmidi/config.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The keys every transport shares are read here; each other key is
   offered to the transports' own readers (config_applemidi.c,
   config_midi2.c), and one that none takes makes the file invalid. */

int netmidi_config_take(char *field, size_t size, const char *text)
{
    size_t length = 0;

    while (text[length])
        if (++length >= size)
            return -1;
    memcpy(field, text, length + 1);
    return 0;
}

static char *trim(char *text)
{
    char *end;
    while (isspace((unsigned char)*text))
        ++text;
    for (end = text; *end; ++end)
        ;
    while (end > text && isspace((unsigned char)end[-1]))
        --end;
    *end = 0;
    return text;
}

int netmidi_config_port(const char *text, uint16_t *port)
{
    char *end;
    unsigned long value;
    if (!*text || *text == '-')
        return -1;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno || *end || value == 0 || value >= 65535)
        return -1;
    *port = (uint16_t)value;
    return 0;
}

int netmidi_config_port_name(const char *number, const char *value,
                             char (*names)[64])
{
    char *end;
    unsigned long port;

    errno = 0;
    port = strtoul(number, &end, 10);
    if (errno || end == number || *end || port >= 16 || !*value)
        return -1;
    return netmidi_config_take(names[port], sizeof(names[0]), value);
}

void netmidi_config_defaults(struct netmidi_network_config *config)
{
    memset(config, 0, sizeof(*config));
    /* the service's name on the network, until a user gives it one */
    netmidi_config_take(config->session_name, sizeof(config->session_name),
                        "AROS");
    netmidi_applemidi_config_defaults(&config->applemidi);
}

int netmidi_config_save(const char *path,
                        const struct netmidi_network_config *config)
{
    FILE *file;
    int failed;

    if (!path || !config)
        return -1;
    file = fopen(path, "w");
    if (!file)
        return -1;
    fprintf(file, "session_name=%s\n", config->session_name);
    if (config->latency_ms)
        fprintf(file, "latency_ms=%d\n", config->latency_ms);
    netmidi_applemidi_config_save(file, &config->applemidi);
    failed = ferror(file);
    return fclose(file) || failed ? -1 : 0;
}

/* A key every transport shares: 1 taken, 0 not one of them, -1 bad. */
static int common_key(struct netmidi_network_config *config, const char *key,
                      const char *value)
{
    if (strcmp(key, "session_name") == 0) {
        if (!*value || netmidi_config_take(config->session_name,
                                           sizeof(config->session_name),
                                           value))
            return -1;
        config->session_name_given = 1;
        return 1;
    }
    if (strcmp(key, "latency_ms") == 0) {
        char *end;
        unsigned long ms;

        errno = 0;
        ms = strtoul(value, &end, 10);
        if (errno || end == value || *end || ms > 2000)
            return -1;
        config->latency_ms = (int)ms;
        return 1;
    }
    return 0;
}

int netmidi_config_load(const char *path, struct netmidi_network_config *config)
{
    struct netmidi_network_config next;
    FILE *file;
    char line[256];
    char *key;
    const char *value;
    char *equal;
    size_t length;
    int result = -1;
    int taken;

    if (!path || !config)
        return -1;
    file = fopen(path, "r");
    if (!file)
        return errno == ENOENT ? 1 : -1;
    next = *config;
    while (fgets(line, sizeof(line), file)) {
        /* a line that did not fit is no line of ours */
        for (length = 0; line[length]; ++length)
            ;
        if (length && line[length - 1] != '\n' && !feof(file))
            goto done;
        key = trim(line);
        if (!*key || *key == '#' || *key == ';')
            continue;
        equal = strchr(key, '=');
        if (!equal)
            goto done;
        *equal = 0;
        key = trim(key);
        value = trim(equal + 1);
        taken = common_key(&next, key, value);
        if (!taken)
            taken = netmidi_applemidi_config_key(&next.applemidi, key, value);
        if (taken != 1)
            goto done;
    }
    if (ferror(file))
        goto done;
    if (netmidi_applemidi_config_check(&next.applemidi) != 0)
        goto done;
    *config = next;
    result = 0;
done:
    fclose(file);
    return result;
}
