#ifndef CAMD_APP_ENDPOINT_H
#define CAMD_APP_ENDPOINT_H

/*
 * Private backend of an endpoint a program publishes.
 *
 * Clients' output sessions each own a bounded native MIDI 1.0 queue; the
 * publisher takes messages from them in turn. What the publisher emits goes
 * straight to the sinks of the clients' receiving input sessions. No MIDI 1.0
 * event is converted to UMP. The object outlives its publisher until the last
 * client session has closed.
 */

#include "provider_contract.h"

#include <stddef.h>
#include <stdint.h>

struct CAMDAppEndpointConfigV1 {
    uint32_t Size;
    uint32_t Version;
    uint32_t Directions;        /* as clients see them */
    uint32_t MaxQueueCapacity;  /* per output session */
    uint32_t MaxSysExBytes;
    uint32_t MaxSessions;
    /* Called, outside the endpoint's lock, when a client queued something.
     * May be NULL. */
    void (*Notify)(void *context);
    void *NotifyContext;
};

struct CAMDAppEndpoint;

enum CAMDProviderResult camd_app_endpoint_create(
    const struct CAMDAppEndpointConfigV1 *config,
    struct CAMDAppEndpoint **endpoint);

/* The returned table and provider context remain owned by the endpoint. */
const struct CAMDProviderOpsV1 *camd_app_endpoint_ops(
    struct CAMDAppEndpoint *endpoint);
void *camd_app_endpoint_context(struct CAMDAppEndpoint *endpoint);

/* The publisher's side. emit gives a message to every receiving input
 * session; *dropped, when given, counts the sinks that had no room. take
 * removes the oldest message of the next output session that has one:
 * *sysex_count is 0 for a short message. TOO_LARGE leaves a SysEx message
 * queued and reports its size; QUEUE_FULL means nothing is waiting. */
enum CAMDProviderResult camd_app_endpoint_emit_midi1(
    struct CAMDAppEndpoint *endpoint, const struct CAMDMIDI1EventV1 *events,
    size_t event_count, uint32_t *dropped);
enum CAMDProviderResult camd_app_endpoint_emit_sysex(
    struct CAMDAppEndpoint *endpoint, const uint8_t *bytes, size_t byte_count,
    uint32_t *dropped);
enum CAMDProviderResult camd_app_endpoint_take(
    struct CAMDAppEndpoint *endpoint, struct CAMDMIDI1EventV1 *event,
    uint8_t *sysex, size_t sysex_capacity, size_t *sysex_count);

/* The publisher is gone: Notify is not called any more. */
void camd_app_endpoint_detach(struct CAMDAppEndpoint *endpoint);

/* Fails with STATE while a session is still open. */
enum CAMDProviderResult camd_app_endpoint_destroy(
    struct CAMDAppEndpoint *endpoint);

#endif
