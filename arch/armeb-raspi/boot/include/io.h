#ifndef _IO_H
#define _IO_H

#include <exec/types.h>
#include <aros/macros.h>
#include <stdint.h>

static inline uint32_t rd32le(uint32_t iobase) {
    return AROS_LE2LONG(*(volatile uint32_t *)(iobase));
}

static inline uint32_t rd32be(uint32_t iobase) {
    return AROS_BE2LONG(*(volatile uint32_t *)(iobase));
}

static inline uint16_t rd16le(uint32_t iobase) {
    return AROS_LE2WORD(*(volatile uint16_t *)(iobase));
}

static inline uint16_t rd16be(uint32_t iobase) {
    return AROS_BE2WORD(*(volatile uint16_t *)(iobase));
}

static inline uint8_t rd8(uint32_t iobase) {
    return *(volatile uint8_t *)(iobase);
}

static inline void wr32le(uint32_t iobase, uint32_t value) {
    *(volatile uint32_t *)(iobase) = AROS_LONG2LE(value);
}

static inline void wr32be(uint32_t iobase, uint32_t value) {
    *(volatile uint32_t *)(iobase) = AROS_LONG2LE(value);
}

static inline void wr16le(uint32_t iobase, uint16_t value) {
    *(volatile uint16_t *)(iobase) = AROS_WORD2LE(value);
}

static inline void wr16be(uint32_t iobase, uint16_t value) {
    *(volatile uint16_t *)(iobase) = AROS_WORD2BE(value);
}

static inline void wr8be(uint32_t iobase, uint8_t value) {
    *(volatile uint8_t *)(iobase) = value;
}

#endif /* _IO_H */
