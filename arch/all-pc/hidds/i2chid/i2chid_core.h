/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef I2CHID_CORE_H
#define I2CHID_CORE_H
#include <stdint.h>
#include <stddef.h>
#define IH_MAX_REPORT 512
#define IH_MAX_DESCRIPTOR 8192
#define IH_MAX_FIELDS 128
struct ih_descriptor
{
    uint16_t report_length, report_register, input_register, input_length;
    uint16_t command_register, data_register, vendor, product;
};
struct ih_field
{
    uint16_t bit, usage, page, collection;
    uint8_t size, report, relative;
    int32_t minimum, maximum;
};
struct ih_layout
{
    struct ih_field fields[IH_MAX_FIELDS];
    unsigned count;
    uint16_t report_bits[256];
    uint8_t has_ids;
};
struct ih_sample
{
    int32_t x, y, id;
    uint32_t xrange, yrange;
    unsigned buttons;
    int contact, relative;
};
int ih_descriptor(const uint8_t *, size_t, struct ih_descriptor *);
int ih_parse(const uint8_t *, size_t, struct ih_layout *);
/* 1: pointer sample, 0: non-pointer report/reset completion, -1: malformed */
int ih_decode(const struct ih_layout *, const uint8_t *, size_t, struct ih_sample *);
struct ih_bus
{
    void *context;
    uint32_t (*read)(void *, unsigned);
    void (*write)(void *, unsigned, uint32_t);
    /* Bounded poll yield; return nonzero to cancel the transfer. */
    int (*pause)(void *);
    uint16_t address, rx_depth, tx_depth;
    uint32_t failure;
};
/* Caller owns controller; timing configured separately. Every wait is bounded. */
int ih_transfer(struct ih_bus *, const uint8_t *, size_t, uint8_t *, size_t);
#endif
