#!/usr/bin/env python3
"""Draw a flipped 3.5\" panel and follow the ADS7846 finger."""

import glob
import os
import select
import struct

W, H = 480, 320
FONT = {
    "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    " ": ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
}


def find_touch():
    for path in sorted(glob.glob("/sys/class/input/event*/device/name")):
        name = open(path).read().strip()
        if "ADS7846" in name or "Touchscreen" in name:
            node = path.split("/")[4]
            return f"/dev/input/{node}"
    raise SystemExit("ADS7846 event device not found")


def abs_range(fd, axis):
    buf = bytearray(24)
    import fcntl
    fcntl.ioctl(fd, 0x80184540 + axis, buf)
    _val, lo, hi, _fuzz, _flat, _res = struct.unpack("iiiiii", buf)
    if hi <= lo:
        lo, hi = 0, 4095
    return lo, hi


class Screen:
    def __init__(self):
        self.fd = os.open("/dev/fb0", os.O_RDWR)
        fix = bytearray(256)
        import fcntl
        fcntl.ioctl(self.fd, 0x4602, fix)
        self.stride = struct.unpack_from("<I", fix, 48)[0]
        self.base = bytearray(self.stride * H)
        self.held = bytearray(self.stride * H)

    def pix(self, buf, x, y, r, g, b):
        if 0 <= x < W and 0 <= y < H:
            off = y * self.stride + x * 4
            buf[off:off + 4] = bytes((b, g, r, 0))

    def text(self, buf, s, x, y, scale, color):
        gw = 6 * scale
        for i, ch in enumerate(s):
            glyph = FONT.get(ch, FONT[" "])
            ox = x + i * gw
            for row, bits in enumerate(glyph):
                for col, bit in enumerate(bits):
                    if bit != "1":
                        continue
                    for dy in range(scale):
                        for dx in range(scale):
                            self.pix(buf, ox + col * scale + dx, y + row * scale + dy, *color)

    def disk(self, buf, cx, cy, rad, color):
        r2 = rad * rad
        for y in range(cy - rad, cy + rad + 1):
            for x in range(cx - rad, cx + rad + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r2:
                    self.pix(buf, x, y, *color)

    def paint_base(self):
        buf = self.base
        for y in range(H):
            for x in range(W):
                if y < 36 or y >= H - 28:
                    col = (20, 40, 70)
                else:
                    col = (8, 8, 12)
                self.pix(buf, x, y, *col)
        label = "CARPLAY"
        scale = 6
        tw = len(label) * 6 * scale - scale
        self.text(buf, label, (W - tw) // 2, 100, scale, (255, 210, 40))
        sub = "TAP"
        sw = len(sub) * 6 * scale - scale
        self.text(buf, sub, (W - sw) // 2, 170, scale, (255, 255, 255))
        for y in range(H):
            for x in range(W):
                self.pix(self.held, x, y, 20, 170, 60)
        self.present(buf)

    def present(self, buf):
        # mmap writes stay in RAM. write() marks the DRM framebuffer dirty
        # and the ILI9486 actually scans the new frame out over SPI.
        os.lseek(self.fd, 0, os.SEEK_SET)
        os.write(self.fd, buf)

    def show_touch(self, down, x, y):
        if down:
            buf = bytearray(self.held)
            self.disk(buf, x, y, 22, (255, 255, 255))
            self.disk(buf, x, y, 10, (255, 40, 40))
        else:
            buf = self.base
        self.present(buf)

    def close(self):
        os.close(self.fd)


def scale(v, lo, hi, n):
    v = lo if v < lo else hi if v > hi else v
    return (v - lo) * (n - 1) // (hi - lo)


def main():
    ev_path = find_touch()
    ev = os.open(ev_path, os.O_RDONLY | os.O_NONBLOCK)
    xlo, xhi = abs_range(ev, 0)
    ylo, yhi = abs_range(ev, 1)
    scr = Screen()
    scr.paint_base()
    log = open("/tmp/lcd_touch.log", "a", buffering=1)
    log.write(f"ready {ev_path} x={xlo}..{xhi} y={ylo}..{yhi}\n")
    down = False
    tx = ty = 0
    seen = 0
    while True:
        r, _, _ = select.select([ev], [], [], 0.2)
        if not r:
            continue
        data = os.read(ev, 24 * 64)
        dirty = False
        for i in range(0, len(data) - 23, 24):
            _sec, _usec, typ, code, val = struct.unpack("qqHHi", data[i:i + 24])
            if typ == 3 and code == 0:
                tx = val
                dirty = True
            elif typ == 3 and code == 1:
                ty = val
                dirty = True
            elif typ == 1 and code in (330, 272):
                down = val != 0
                dirty = True
            elif typ == 0 and dirty:
                sx = scale(tx, xlo, xhi, W)
                sy = scale(ty, ylo, yhi, H)
                scr.show_touch(down, sx, sy)
                seen += 1
                if seen <= 8 or seen % 20 == 0:
                    log.write(f"touch#{seen} down={int(down)} raw={tx},{ty} px={sx},{sy}\n")
                dirty = False


if __name__ == "__main__":
    main()
