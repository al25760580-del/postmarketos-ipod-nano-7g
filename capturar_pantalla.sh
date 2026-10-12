#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-/home/mila/Descargas/linux-nano7g/captura_ipod.png}"

nmcli con up ipod-usb0 >/dev/null 2>&1 || true

python3 - "$OUT" << 'PY'
import sys, socket, struct, zlib

out_path = sys.argv[1]
s = socket.create_connection(("192.168.7.2", 2323), timeout=8)
cmd = b"echo ___START_RAW_FB___; (/usr/bin/drm-dump 2>/dev/null || cat /dev/fb0); echo ___END_RAW_FB___\nexit\n"
s.sendall(cmd)
buf = bytearray()
while True:
    chunk = s.recv(65536)
    if not chunk:
        break
    buf.extend(chunk)
s.close()

m_start = b"___START_RAW_FB___\n"
i = buf.find(m_start)
if i < 0:
    print("Error: no se encontro el marcador de inicio", file=sys.stderr)
    sys.exit(1)
raw_start = i + len(m_start)
raw = bytes(buf[raw_start:raw_start + 414720])
if len(raw) < 414720:
    print(f"Error: solo se recibieron {len(raw)} bytes", file=sys.stderr)
    sys.exit(2)

w, h = 240, 432
rows = []
for y in range(h):
    row = bytearray([0])
    line = raw[y*w*4:(y+1)*w*4]
    for x in range(w):
        b, g, r, _ = line[x*4:x*4+4]
        row.extend((r, g, b))
    rows.append(bytes(row))
raw_rgb = b"".join(rows)

def chunk_png(t, d):
    return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)

png = (
    b"\x89PNG\r\n\x1a\n"
    + chunk_png(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    + chunk_png(b"IDAT", zlib.compress(raw_rgb, 9))
    + chunk_png(b"IEND", b"")
)
with open(out_path, "wb") as f:
    f.write(png)
print(f"Captura guardada en: {out_path} ({len(png)} bytes)")
PY
