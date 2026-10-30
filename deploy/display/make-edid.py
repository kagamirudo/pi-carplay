#!/usr/bin/env python3
"""128-byte EDID for the Elecrow / Waveshare 7\" 1024x600 HDMI panel.

Timing is the panel's published sequence, not CVT:

    pixel clock 49.00 MHz
    H 1024 active, 48 front, 96 sync, 144 back  (blank 288)
    V 600 active, 3 front, 10 sync, 11 back     (blank 24)
    image size 154 x 86 mm
"""

from pathlib import Path


def detailed_timing():
    pixel_clock = 4900  # 49.00 MHz in units of 10 kHz
    h_active, h_blank, h_fp, h_sync = 1024, 288, 48, 96
    v_active, v_blank, v_fp, v_sync = 600, 24, 3, 10
    h_mm, v_mm = 154, 86
    raw = bytearray(18)
    raw[0] = pixel_clock & 0xFF
    raw[1] = (pixel_clock >> 8) & 0xFF
    raw[2] = h_active & 0xFF
    raw[3] = h_blank & 0xFF
    raw[4] = ((h_active >> 8) << 4) | ((h_blank >> 8) & 0x0F)
    raw[5] = v_active & 0xFF
    raw[6] = v_blank & 0xFF
    raw[7] = ((v_active >> 8) << 4) | ((v_blank >> 8) & 0x0F)
    raw[8] = h_fp & 0xFF
    raw[9] = h_sync & 0xFF
    raw[10] = ((v_fp & 0x0F) << 4) | (v_sync & 0x0F)
    raw[11] = (
        ((v_sync >> 4) & 0x03) << 6
        | ((v_fp >> 4) & 0x03) << 4
        | ((h_sync >> 8) & 0x03) << 2
        | ((h_fp >> 8) & 0x03)
    )
    raw[12] = h_mm & 0xFF
    raw[13] = v_mm & 0xFF
    raw[14] = ((h_mm >> 8) << 4) | ((v_mm >> 8) & 0x0F)
    raw[17] = 0x1E  # non-interlaced, digital separate, positive sync
    return bytes(raw)


def descriptor_text(tag, text):
    payload = text.encode("ascii")[:13]
    payload = payload + b"\n" + b" " * 13
    return bytes([0, 0, 0, tag, 0]) + payload[:13]


def manufacturer(code):
    value = 0
    for char in code:
        value = (value << 5) | (ord(char) - ord("A") + 1)
    return bytes([(value >> 8) & 0xFF, value & 0xFF])


def build():
    edid = bytearray(128)
    edid[0:8] = bytes([0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00])
    edid[8:10] = manufacturer("ELE")
    edid[10:12] = (0x0070).to_bytes(2, "little")  # RC070-ish product code
    edid[12:16] = (1).to_bytes(4, "little")
    edid[16] = 1          # week
    edid[17] = 2026 - 1990
    edid[18:20] = bytes([1, 3])
    edid[20] = 0x80       # digital
    edid[21] = 15         # 15 cm
    edid[22] = 9          # 9 cm
    edid[23] = 120        # gamma 2.20
    edid[24] = 0x0A       # RGB, preferred timing is descriptor 1
    # sRGB chromaticity, established timings none, unused standard timings
    edid[25:35] = bytes([0xEE, 0x91, 0xA3, 0x54, 0x4C, 0x99, 0x26, 0x0F, 0x50, 0x54])
    edid[35:38] = bytes(3)
    edid[38:54] = bytes([0x01, 0x01]) * 8
    edid[54:72] = detailed_timing()
    edid[72:90] = descriptor_text(0xFC, "Elecrow 7in")
    edid[90:108] = bytes([0, 0, 0, 0xFD, 0, 50, 75, 30, 80, 6, 0, 0, 10, 10, 10, 10, 10, 10])
    edid[108:126] = descriptor_text(0x10, "")
    edid[126] = 0
    edid[127] = (256 - (sum(edid[:127]) & 0xFF)) & 0xFF
    if sum(edid) & 0xFF:
        raise SystemExit("EDID checksum failed")
    timing = edid[54:72]
    clock = timing[0] | (timing[1] << 8)
    if clock != 4900:
        raise SystemExit(f"pixel clock {clock} is not 49.00 MHz")
    return bytes(edid)


def main():
    blob = build()
    dest = Path(__file__).resolve().parent / "elecrow-1024x600.bin"
    dest.write_bytes(blob)
    print(f"wrote {dest} ({len(blob)} bytes), checksum 0x{blob[-1]:02x}")


if __name__ == "__main__":
    main()
