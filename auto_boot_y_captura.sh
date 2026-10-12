#!/usr/bin/env bash
set -euo pipefail
cd /home/mila/Descargas/linux-nano7g
echo '[*] Esperando iPod en modo DFU (05ac:1234)...'
while ! lsusb -d 05ac:1234 >/dev/null 2>&1; do
    sleep 0.5
done
echo '[+] DFU detectado! Arrancando U-Boot + Linux 6.14 con usb0 corregido...'
./wInd3x cfw run ./u-boot.bin
for _ in $(seq 1 60); do
    if lsusb -d 05ac:ff00 >/dev/null 2>&1; then
        break
    fi
    sleep 0.5
done
dfu-util --device 05ac:ff00 --alt firmware --download ./kernel_fdt_n31.itb
dfu-util --device 05ac:ff00 --alt firmware -e -R >/dev/null 2>&1 || true
echo '[*] Esperando interfaz usb0 (0525:a4a2)...'
for _ in $(seq 1 40); do
    if lsusb -d 0525:a4a2 >/dev/null 2>&1; then
        break
    fi
    sleep 0.5
done
sleep 2
nmcli con up ipod-usb0 >/dev/null 2>&1 || true
echo '[*] Esperando a 192.168.7.2:2323 y al inicio del escritorio + touch (~8s)...'
for _ in $(seq 1 30); do
    if ping -c 1 -W 1 192.168.7.2 >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
sleep 8
./capturar_pantalla.sh /home/mila/Descargas/linux-nano7g/captura_ipod.png
