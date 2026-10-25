#!/bin/bash
# build_kernel.sh - configure, build and install the patched kernel.
# Run apply_syscalls.sh first.  Does NOT reboot; do that yourself at the end.
#
# Usage: ./build_kernel.sh [--skip-menuconfig] [path/to/linux-6.16.x]
#
# make menuconfig opens by default (the project asks for it; just save and
# exit).  Pass --skip-menuconfig for an unattended build.

set -euo pipefail

MENU=1
ARGS=()
for a in "$@"; do
	if [ "$a" = "--skip-menuconfig" ]; then MENU=0; else ARGS+=("$a"); fi
done

KDIR="${ARGS[0]:-}"
if [ -z "$KDIR" ]; then
	KDIR="$(ls -d "$HOME"/linux-6.16* 2>/dev/null | head -n 1 || true)"
fi
if [ -z "$KDIR" ] || [ ! -f "$KDIR/Makefile" ]; then
	echo "usage: $0 [--skip-menuconfig] /path/to/linux-6.16.x" >&2
	exit 1
fi
cd "$KDIR"

if ! grep -q 'sys_start_elevator' include/linux/syscalls.h; then
	echo "Syscalls are not applied yet. Run apply_syscalls.sh first." >&2
	exit 1
fi

# Build dependencies (Ubuntu/Debian)
if command -v apt-get >/dev/null; then
	sudo apt-get update
	sudo apt-get install -y build-essential libncurses-dev bison flex \
		libssl-dev libelf-dev bc dwarves zstd cpio rsync git
fi

# Start from the running kernel's config
if [ ! -f .config ]; then
	cp "/boot/config-$(uname -r)" .config
fi

# Disable the certificate settings that stop a custom kernel from building
# and shrink the build (no debug info / BTF).  The version string is left
# alone so that uname -r reports plain 6.16.x.
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
scripts/config --set-str SYSTEM_TRUSTED_KEYS ""
scripts/config --set-str SYSTEM_REVOCATION_KEYS ""
scripts/config --disable MODULE_SIG
scripts/config --disable DEBUG_INFO_BTF
scripts/config --disable DEBUG_INFO
make olddefconfig

if [ "$MENU" -eq 1 ]; then
	make menuconfig
fi

make -j"$(nproc)"
sudo make modules_install
sudo make install -j"$(nproc)"
sudo update-grub

echo
echo "Kernel installed. Now run:  sudo reboot"
echo "After reboot, check:        uname -r     (should be 6.16.x, no -generic)"
