#!/usr/bin/env python3
"""Pack NSO UI assets (fonts, UI art, system cards, data) into one blob.

Scrapable game box arts/covers are never packed. Only paths listed on the
command line (or stdin) are included.
"""
import struct
import sys
from pathlib import Path

MAGIC = b"NSOAS01\0"


def main():
    if len(sys.argv) < 3:
        print("usage: pack-assets.py <out.pack> <file> [file...]", file=sys.stderr)
        return 2
    out = Path(sys.argv[1])
    files = []
    for rel in sys.argv[2:]:
        p = Path(rel)
        if not p.is_file():
            print(f"pack-assets: missing {rel}", file=sys.stderr)
            return 1
        files.append((rel.replace("\\", "/"), p.read_bytes()))

    # header: magic + count + strtab_off + data_off, then count * (rel_off, data_off, size)
    n = len(files)
    header_len = 8 + 4 + 4 + 4 + n * 12
    strtab = b""
    rel_offs = []
    for rel, _ in files:
        rel_offs.append(len(strtab))
        strtab += rel.encode("utf-8") + b"\0"
    # align data to 4
    pad = (4 - (header_len + len(strtab)) % 4) % 4
    strtab += b"\0" * pad
    data_off = header_len + len(strtab)
    payload = b""
    data_offs = []
    sizes = []
    for _, blob in files:
        # 4-align each file
        if len(payload) % 4:
            payload += b"\0" * (4 - len(payload) % 4)
        data_offs.append(data_off + len(payload))
        sizes.append(len(blob))
        payload += blob

    buf = bytearray()
    buf += MAGIC
    buf += struct.pack("<III", n, header_len, data_off)
    for i in range(n):
        buf += struct.pack("<III", rel_offs[i], data_offs[i], sizes[i])
    buf += strtab
    # data starts at data_off == len(buf) if we padded strtab correctly
    if len(buf) != data_off:
        print(f"pack-assets: layout bug {len(buf)} != {data_off}", file=sys.stderr)
        return 1
    buf += payload
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(buf)
    print(f"packed {n} files, {len(buf)} bytes -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
