#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Create a small ISO with uniquely identifiable sectors for cdreadahead."""
import argparse
from pathlib import Path
import struct


def both(value, size):
    return value.to_bytes(size, "little") + value.to_bytes(size, "big")


def directory_record(name):
    record = bytearray(33 + len(name))
    record[0] = len(record)
    record[2:10] = both(18, 4)
    record[10:18] = both(2048, 4)
    record[18:25] = bytes([126, 10, 18, 0, 0, 0, 0])
    record[25] = 2
    record[28:32] = both(1, 2)
    record[32] = len(name)
    record[33:] = name
    return record


def make_disc(path):
    sectors = 1024
    image = bytearray()
    for sector in range(sectors):
        block = bytearray((sector * 17 + offset) & 255 for offset in range(2048))
        block[:8] = struct.pack(">II", sector, sector ^ 0xffffffff)
        image.extend(block)
    image[:16 * 2048] = bytes(16 * 2048)
    pvd = bytearray(2048)
    pvd[:7] = b"\x01CD001\x01"
    pvd[8:40] = b"AROS".ljust(32)
    pvd[40:72] = b"AROS_CD_READAHEAD".ljust(32)
    pvd[80:88] = both(sectors, 4)
    pvd[120:124] = both(1, 2)
    pvd[124:128] = both(1, 2)
    pvd[128:132] = both(2048, 2)
    pvd[156:190] = directory_record(b"\x00")
    pvd[881] = 1
    image[16 * 2048:17 * 2048] = pvd
    image[17 * 2048:18 * 2048] = b"\xffCD001\x01" + bytes(2041)
    directory = directory_record(b"\x00") + directory_record(b"\x01")
    image[18 * 2048:19 * 2048] = directory + bytes(2048 - len(directory))
    path.write_bytes(image)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    make_disc(parser.parse_args().output)
