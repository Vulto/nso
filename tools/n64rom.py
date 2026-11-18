#!/usr/bin/env python3
"""n64rom.py IN OUT [SHA256]
Writes the N64 ROM IN to OUT in big-endian (.z64) byte order, padded to 4
bytes, as N64Recomp ports store it. With SHA256, OUT is only written when the
converted ROM has that hash (exit 3 = another dump or version). IN is only read."""
import hashlib
import os
import sys


def main():
    if len(sys.argv) not in (3, 4):
        print(__doc__.strip(), file=sys.stderr)
        return 1
    src, dst = sys.argv[1], sys.argv[2]
    data = bytearray(open(src, "rb").read())
    data += b"\0" * (-len(data) % 4)
    magic = bytes(data[:4])
    if magic == b"\x37\x80\x40\x12":    # .v64: 16-bit byteswapped
        data[0::2], data[1::2] = data[1::2], data[0::2]
    elif magic == b"\x40\x12\x37\x80":  # .n64: 32-bit little endian
        data[0::4], data[1::4], data[2::4], data[3::4] = data[3::4], data[2::4], data[1::4], data[0::4]
    elif magic != b"\x80\x37\x12\x40":
        print("%s: not an N64 ROM" % src, file=sys.stderr)
        return 2
    if len(sys.argv) == 4 and hashlib.sha256(data).hexdigest() != sys.argv[3].lower():
        print("%s: not the dump this port needs (sha256 %s)" % (src, sys.argv[3]), file=sys.stderr)
        return 3
    tmp = dst + ".part"
    with open(tmp, "wb") as f:
        f.write(data)
    os.replace(tmp, dst)
    return 0


if __name__ == "__main__":
    sys.exit(main())
