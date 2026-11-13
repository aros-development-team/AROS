#include <btcore/endian.h>

uint16_t bt_read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint32_t bt_read_le24(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

uint32_t bt_read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint64_t bt_read_le64(const uint8_t *p)
{
    return (uint64_t)bt_read_le32(p) | ((uint64_t)bt_read_le32(p + 4) << 32);
}

uint16_t bt_read_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

uint32_t bt_read_be24(const uint8_t *p)
{
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}

uint32_t bt_read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

uint64_t bt_read_be64(const uint8_t *p)
{
    return ((uint64_t)bt_read_be32(p) << 32) | (uint64_t)bt_read_be32(p + 4);
}

void bt_write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
}

void bt_write_le24(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
}

void bt_write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
    p[3] = (uint8_t)((value >> 24) & 0xffu);
}

void bt_write_le64(uint8_t *p, uint64_t value)
{
    bt_write_le32(p, (uint32_t)(value & 0xffffffffu));
    bt_write_le32(p + 4, (uint32_t)(value >> 32));
}

void bt_write_be16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)((value >> 8) & 0xffu);
    p[1] = (uint8_t)(value & 0xffu);
}

void bt_write_be24(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)((value >> 16) & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)(value & 0xffu);
}

void bt_write_be32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)((value >> 24) & 0xffu);
    p[1] = (uint8_t)((value >> 16) & 0xffu);
    p[2] = (uint8_t)((value >> 8) & 0xffu);
    p[3] = (uint8_t)(value & 0xffu);
}

void bt_write_be64(uint8_t *p, uint64_t value)
{
    bt_write_be32(p, (uint32_t)(value >> 32));
    bt_write_be32(p + 4, (uint32_t)(value & 0xffffffffu));
}
