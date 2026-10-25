#!/bin/bash
# apply_syscalls.sh - add start_elevator/issue_request/stop_elevator
# (548/549/550) to a Linux kernel source tree.  Safe to run more than once.
#
# Usage: ./apply_syscalls.sh [path/to/linux-6.16.x]   (default: ~/linux-6.16*)

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_C="$HERE/../syscalls.c"
SRC_MK="$HERE/syscalls.Makefile"

KDIR="${1:-}"
if [ -z "$KDIR" ]; then
	KDIR="$(ls -d "$HOME"/linux-6.16* 2>/dev/null | head -n 1 || true)"
fi
if [ -z "$KDIR" ] || [ ! -f "$KDIR/Makefile" ]; then
	echo "usage: $0 /path/to/linux-6.16.x" >&2
	exit 1
fi
KDIR="$(cd "$KDIR" && pwd)"
if [ ! -w "$KDIR" ]; then
	echo "$KDIR is not writable. Run: sudo chown -R \$USER $(readlink -f "$KDIR")" >&2
	exit 1
fi

TBL="$KDIR/arch/x86/entry/syscalls/syscall_64.tbl"
HDR="$KDIR/include/linux/syscalls.h"
TOP="$KDIR/Makefile"
for f in "$TBL" "$HDR" "$TOP"; do
	[ -f "$f" ] || { echo "missing $f" >&2; exit 1; }
done

# 1. <kernel>/syscalls/{syscalls.c,Makefile}
mkdir -p "$KDIR/syscalls"
cp "$SRC_C" "$KDIR/syscalls/syscalls.c"
cp "$SRC_MK" "$KDIR/syscalls/Makefile"
echo "[ok] copied syscalls.c and Makefile to $KDIR/syscalls/"

# 2. syscall_64.tbl: numbers 548-550 are unused on x86_64 (x32 stops at 547)
if grep -qE '^548[[:space:]]' "$TBL"; then
	echo "[skip] syscall_64.tbl already has an entry for 548"
else
	[ -n "$(tail -c1 "$TBL")" ] && echo >> "$TBL"
	printf '548\tcommon\tstart_elevator\t\tsys_start_elevator\n' >> "$TBL"
	printf '549\tcommon\tissue_request\t\tsys_issue_request\n'  >> "$TBL"
	printf '550\tcommon\tstop_elevator\t\tsys_stop_elevator\n'  >> "$TBL"
	echo "[ok] added 548/549/550 to syscall_64.tbl"
fi

# 3. include/linux/syscalls.h: prototypes just before the final #endif
if grep -q 'sys_start_elevator' "$HDR"; then
	echo "[skip] syscalls.h already declares the elevator syscalls"
else
	TMP="$(mktemp)"
	cat > "$TMP" << 'PROTO'

/* elevator system calls (548-550), see syscalls/syscalls.c */
asmlinkage long sys_start_elevator(void);
asmlinkage long sys_issue_request(int start_floor, int destination_floor,
				  int type);
asmlinkage long sys_stop_elevator(void);

PROTO
	LAST="$(grep -n '^#endif' "$HDR" | tail -n 1 | cut -d: -f1)"
	sed -i "$((LAST - 1))r $TMP" "$HDR"
	rm -f "$TMP"
	echo "[ok] added prototypes to syscalls.h"
fi

# 4. top-level Makefile: build syscalls/ into the kernel (core-y)
if grep -qE '^core-y.*[[:space:]]syscalls/' "$TOP"; then
	echo "[skip] top-level Makefile already builds syscalls/"
else
	if ! grep -qE '^core-y[[:space:]]*\+=[[:space:]]*kernel/' "$TOP"; then
		echo "Could not find the 'core-y += kernel/ ...' line in $TOP." >&2
		echo "Add 'syscalls/' to the core-y list by hand." >&2
		exit 1
	fi
	sed -i -E '/^core-y[[:space:]]*\+=[[:space:]]*kernel\//s|$| syscalls/|' "$TOP"
	echo "[ok] added syscalls/ to core-y in top-level Makefile"
fi

echo "Done. Next: ./build_kernel.sh $KDIR"
