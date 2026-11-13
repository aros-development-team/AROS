#ifndef BTCORE_ENDIAN_H
#define BTCORE_ENDIAN_H

#include <btcore/types.h>

/*
 * Byte-wise access to multi-byte fields at an explicit, known byte order.
 * These never cast a buffer pointer to a wider integer type, so they are
 * safe at any alignment and independent of host CPU endianness. This is
 * the only place protocol code should cross between wire bytes and native
 * integers (see project.md, "Compatibilidade obrigatória com big-endian
 * e little-endian").
 */

uint16_t bt_read_le16(const uint8_t *p);
uint32_t bt_read_le24(const uint8_t *p);
uint32_t bt_read_le32(const uint8_t *p);
uint64_t bt_read_le64(const uint8_t *p);

uint16_t bt_read_be16(const uint8_t *p);
uint32_t bt_read_be24(const uint8_t *p);
uint32_t bt_read_be32(const uint8_t *p);
uint64_t bt_read_be64(const uint8_t *p);

void bt_write_le16(uint8_t *p, uint16_t value);
void bt_write_le24(uint8_t *p, uint32_t value);
void bt_write_le32(uint8_t *p, uint32_t value);
void bt_write_le64(uint8_t *p, uint64_t value);

void bt_write_be16(uint8_t *p, uint16_t value);
void bt_write_be24(uint8_t *p, uint32_t value);
void bt_write_be32(uint8_t *p, uint32_t value);
void bt_write_be64(uint8_t *p, uint64_t value);

#endif /* BTCORE_ENDIAN_H */
