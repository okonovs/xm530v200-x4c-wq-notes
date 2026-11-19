#!/bin/bash
# XM530V200 OpenIPC firmware boot test in QEMU.
#
#   bash qemu-boot/test-xm530v200.sh [--kernel PATH] [--initrd PATH]
#
# Boots the OpenIPC uImage + initrd in QEMU's custom xm530v200 machine and
# checks for a login prompt.  Exit 0 = booted to userspace.

set -u
cd "$(dirname "$0")/.."

QEMU=${QEMU:-./qemu-src/build/qemu-system-arm}
KERNEL=${KERNEL:-/Users/oko/Projects/openipc-build/images/uImage.xm530v200}
INITRD=${INITRD:-/tmp/initrd-full.cpio}
OUT=${OUT:-/tmp/qemu-boot-test}
TIMEOUT=${TIMEOUT:-30}

mkdir -p "$OUT"

echo "== XM530V200 OpenIPC boot test =="
echo "QEMU:   $QEMU"
echo "Kernel: $KERNEL"
echo "Initrd: $INITRD"
echo "Output: $OUT"

# Check prerequisites
[ -x "$QEMU" ] || { echo "FAIL: QEMU not found at $QEMU"; exit 2; }
[ -f "$KERNEL" ] || { echo "FAIL: kernel not found at $KERNEL"; exit 2; }
[ -f "$INITRD" ] || { echo "FAIL: initrd not found at $INITRD"; exit 2; }

# Boot
timeout "$TIMEOUT" "$QEMU" \
    -M xm530v200 \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    -display none \
    -monitor none \
    -nographic \
    -serial "file:$OUT/serial.log" \
    2>"$OUT/qemu.stderr.log"
EXIT=$?

echo ""
echo "== Results =="

# Check serial log
if [ -f "$OUT/serial.log" ]; then
    LINES=$(wc -l < "$OUT/serial.log")
    echo "Serial log: $LINES lines"
    
    # Check for login prompt
    if grep -q "login:" "$OUT/serial.log" 2>/dev/null; then
        echo "PASS: login prompt reached"
    elif grep -q "Welcome to OpenIPC" "$OUT/serial.log" 2>/dev/null; then
        echo "PASS: OpenIPC welcome message reached"
    else
        echo "FAIL: no login prompt or welcome message"
        tail -10 "$OUT/serial.log" 2>/dev/null
        exit 1
    fi
    
    # Check for kernel panic
    if grep -qi "kernel panic" "$OUT/serial.log" 2>/dev/null; then
        echo "FAIL: kernel panic detected"
        grep -i "kernel panic" "$OUT/serial.log" 2>/dev/null
        exit 1
    fi
    
    # Check for DHCP lease
    if grep -q "lease.*obtained" "$OUT/serial.log" 2>/dev/null; then
        echo "PASS: DHCP lease obtained"
    else
        echo "WARN: no DHCP lease"
    fi
    
    # Check for dropbear/SSH
    if grep -qi "Starting dropbear\|dropbear.*OK" "$OUT/serial.log" 2>/dev/null; then
        echo "PASS: dropbear (SSH) started"
    else
        echo "WARN: dropbear not started"
    fi
    
    # Kernel version
    KVER=$(grep "Linux version" "$OUT/serial.log" 2>/dev/null | head -1)
    if [ -n "$KVER" ]; then
        echo "Kernel: $KVER"
    fi
else
    echo "FAIL: no serial log produced"
    exit 1
fi

# Check for timeout
if [ "$EXIT" -eq 124 ]; then
    echo "PASS: boot completed (QEMU ran full $TIMEOUT seconds, no panic)"
else
    echo "INFO: QEMU exited with code $EXIT"
fi

echo ""
echo "Full serial log: $OUT/serial.log"
echo "QEMU stderr:     $OUT/qemu.stderr.log"
