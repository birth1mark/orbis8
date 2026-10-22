#!/usr/bin/env python3
"""
Simple PS4 param.sfo generator for homebrew packages.
Generates compliant Sony SFO format without external dependencies.
"""
import struct
import sys
import os

def create_sfo(title_id="ORBS00008", title="Orbis8 - PICO-8 Player by birth1mark", version="01.00"):
    entries = {
        "APP_TYPE": (2, 4, 1), # uint32
        "APP_VER": (0, 8, version.ljust(8, '\x00')), # utf8
        "ATTRIBUTE": (2, 4, 0),
        "CATEGORY": (0, 4, "gd\x00\x00"),
        "CONTENT_ID": (0, 36, f"IV0000-{title_id}_00-0000000000000000\x00"),
        "SYSTEM_VER": (2, 4, 0),
        "TITLE": (0, 128, title.ljust(128, '\x00')),
        "TITLE_ID": (0, 12, title_id.ljust(12, '\x00')),
        "VERSION": (0, 8, version.ljust(8, '\x00')),
    }

    # Sort keys alphabetically as required by SFO specification
    sorted_keys = sorted(entries.keys())
    count = len(sorted_keys)

    key_table = bytearray()
    key_offsets = {}
    for k in sorted_keys:
        key_offsets[k] = len(key_table)
        key_table.extend(k.encode('ascii') + b'\x00')

    val_table = bytearray()
    val_entries = []

    for k in sorted_keys:
        fmt, max_len, val = entries[k]
        val_offset = len(val_table)
        if fmt == 2: # uint32
            data = struct.pack('<I', val)
            val_len = 4
        else: # string
            if isinstance(val, str):
                data = val.encode('utf-8')[:max_len]
            else:
                data = val[:max_len]
            val_len = len(data)
        val_table.extend(data)
        val_entries.append((key_offsets[k], fmt, val_len, max_len, val_offset))

    # Header: Magic (\x00PSF), Version (1.1), Key Table Offset, Data Table Offset, Entry Count
    header_size = 20
    entries_size = count * 16
    key_table_offset = header_size + entries_size
    data_table_offset = key_table_offset + len(key_table)

    # Pad data_table_offset to 4-byte boundary
    pad_len = (4 - (data_table_offset % 4)) % 4
    key_table.extend(b'\x00' * pad_len)
    data_table_offset += pad_len

    header = struct.pack(
        '<4sIIII',
        b'\x00PSF',
        0x00000101,
        key_table_offset,
        data_table_offset,
        count
    )

    entry_bytes = bytearray()
    for key_off, fmt, val_len, max_len, val_off in val_entries:
        entry_bytes.extend(struct.pack('<HHIII', key_off, (fmt << 8) | 0x04, val_len, max_len, val_off))

    return header + entry_bytes + key_table + val_table

if __name__ == "__main__":
    out_path = sys.argv[1] if len(sys.argv) > 1 else "param.sfo"
    sfo_data = create_sfo()
    with open(out_path, "wb") as f:
        f.write(sfo_data)
    print(f"Generated valid param.sfo at: {out_path} ({len(sfo_data)} bytes)")
