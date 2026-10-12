#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
#  Freemyipod Linux 6.14 + U-Boot para iPod Nano 7th Gen (N31 / S5L8740)
#  Arranque por USB DFU (Tethered en RAM — NO modifica tu instalación de iPodOS)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -d "${SCRIPT_DIR}/linux-nano7g" ]]; then
    BUNDLE_DIR="${SCRIPT_DIR}/linux-nano7g"
else
    BUNDLE_DIR="${SCRIPT_DIR}"
fi

WIND3X_BIN="${BUNDLE_DIR}/wInd3x"
DFU_UTIL_BIN="${BUNDLE_DIR}/dfu-util"
UBOOT_BIN="${BUNDLE_DIR}/u-boot.bin"
KERNEL_ITB="${BUNDLE_DIR}/kernel_fdt_n31.itb"

if command -v dfu-util >/dev/null 2>&1; then
    DFU_UTIL_BIN="$(command -v dfu-util)"
fi

echo "=================================================================="
echo "  Linux 6.14 (Freemyipod N31) + U-Boot — iPod Nano 7th Generation"
echo "=================================================================="
echo "  U-Boot : ${UBOOT_BIN}"
echo "  Kernel : ${KERNEL_ITB} (Linux 6.14 + DTB + Initramfs con Fastfetch)"
echo "=================================================================="

# 1. Pre-poblar caché de wInd3x con WTF.s5l8740xall.release.dfu local si existe
CACHE_DIR="${HOME}/.local/share/wInd3x"
mkdir -p "${CACHE_DIR}"

WTF_HASH_1="561313ee4e0cfdcc17caeb9b1a768789c02ea160e30accedcc14e593c063c540"
WTF_HASH_2="61b4e139ba920da4c0bfd493807b3b0735f9eeaebbca588cd37cb5584a8d2551"

if [[ ! -f "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_1}.bin" ]]; then
    for candidate in \
        "${HOME}/Descargas/WTF.s5l8740xall.release.dfu" \
        "${HOME}/Downloads/WTF.s5l8740xall.release.dfu" \
        "${SCRIPT_DIR}/WTF.s5l8740xall.release.dfu"; do
        if [[ -f "${candidate}" ]]; then
            echo "[*] Copiando WTF oficial local a la caché de wInd3x..."
            cp "${candidate}" "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_1}.bin"
            cp "${candidate}" "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_2}.bin"
            break
        fi
    done
fi

if [[ ! -f "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_1}.bin" ]]; then
    for ipsw in "${HOME}"/Descargas/*.ipsw "${HOME}"/Downloads/*.ipsw; do
        if [[ -f "${ipsw}" ]] && command -v unzip >/dev/null 2>&1; then
            echo "[*] Extrayendo WTF.s5l8740xall.release.dfu desde ${ipsw}..."
            unzip -p "${ipsw}" "Firmware/dfu/WTF.s5l8740xall.release.dfu" \
                > "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_1}.bin" || true
            if [[ -s "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_1}.bin" ]]; then
                cp "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_1}.bin" \
                   "${CACHE_DIR}/n7g-wtf-upstream-${WTF_HASH_2}.bin"
                break
            fi
        fi
    done
fi

# 2. Esperar a que el iPod Nano 7G entre en modo DFU (05ac:1234)
if ! lsusb -d 05ac:1234 >/dev/null 2>&1; then
    echo ""
    echo ">>> Pon tu iPod Nano 7th Gen en modo DFU:"
    echo "    1. Mantén el iPod conectado por USB a la PC."
    echo "    2. Mantén presionados simultáneamente [Botón Home] + [Botón Power/Arriba]"
    echo "       durante unos 8–10 segundos hasta que la pantalla quede completamente negra"
    echo "       (suelta ambos botones justo cuando se apague la pantalla, antes del logo de Apple)."
    echo ""
    echo -n "[*] Esperando dispositivo USB 05ac:1234 (DFU)... "
    while ! lsusb -d 05ac:1234 >/dev/null 2>&1; do
        sleep 0.5
    done
    echo "¡Detectado!"
else
    echo "[+] iPod Nano 7G detectado en modo DFU (05ac:1234)."
fi

# 3. Explotar BootROM con S5Late y enviar WTF parcheado + U-Boot a DRAM
echo ""
echo "[1/3] Ejecutando exploit S5Late y cargando U-Boot en RAM..."
echo "      (Nota: la primera vez tardará ~35s en desencriptar WTF; las siguientes ~3s)"
"${WIND3X_BIN}" cfw run "${UBOOT_BIN}"

# 4. Esperar a que U-Boot inicialice la pantalla y exponga DFU USB (05ac:ff00)
echo ""
echo -n "[2/3] Esperando a U-Boot en el iPod (USB 05ac:ff00)... "
for _ in $(seq 1 60); do
    if lsusb -d 05ac:ff00 >/dev/null 2>&1; then
        break
    fi
    sleep 0.5
done

if ! lsusb -d 05ac:ff00 >/dev/null 2>&1; then
    echo "ERROR: No apareció U-Boot (05ac:ff00) tras 30 segundos."
    exit 1
fi
echo "¡U-Boot listo y pantalla encendida!"

# 5. Enviar kernel_fdt_n31.itb (Alpine Linux 3.21.8 + Graphical UI) y arrancar
echo ""
echo "[3/3] Enviando kernel_fdt_n31.itb (Alpine Linux 3.21.8 + UI) por DFU..."
"${DFU_UTIL_BIN}" --device 05ac:ff00 --alt firmware --download "${KERNEL_ITB}"
"${DFU_UTIL_BIN}" --device 05ac:ff00 --alt firmware -e -R >/dev/null 2>&1 || true

echo ""
echo "=================================================================="
echo "  ¡LINUX 6.14 ESTÁ ARRANCANDO EN TU IPOD NANO 7TH GEN!"
echo "=================================================================="
echo "  En la pantalla del iPod verás:"
echo "    - El pingüino Tux + logs del kernel Linux 6.14"
echo "    - Fastfetch con los datos del SoC Apple S5L8740 (Cortex-A5)"
echo "    - Menú interactivo con los botones físicos del iPod:"
echo "        [Vol +]        -> Mostrar Fastfetch en pantalla"
echo "        [Play / Pause] -> Reloj analógico 3D (fbclock)"
echo "        [Vol -]        -> Estado de Hardware / Crypto / Batería"
echo "        [Home]         -> Volver al menú en pantalla"
echo "        [Power 2 seg]  -> Apagar y volver al iPodOS normal"
echo ""
echo "  Para entrar por terminal desde tu PC (Red USB RNDIS/CDC):"
echo "    1. Asigna IP a la interfaz USB nueva en tu PC (ej. usb0 o enp*u*):"
echo "       sudo ip addr add 192.168.7.1/24 dev usb0 && sudo ip link set usb0 up"
echo "    2. Conéctate por Telnet (root sin contraseña):"
echo "       telnet 192.168.7.2"
echo "=================================================================="
