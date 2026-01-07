#!/bin/bash

OUT=build_dump.txt
echo "Dumping project into $OUT ..."
echo "" > "$OUT"

log() {
    echo -e "\n===== $1 =====\n" | tee -a "$OUT"
}

log "TREE"
tree -a | tee -a "$OUT"

log "MAKEFILE"
sed -n '1,300p' makefile | tee -a "$OUT"

log "LINKER SCRIPTS"
find . -name "*.ld" -exec sh -c 'echo "--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

log "GRUB CONFIG"
find . -name "grub.cfg" -exec sh -c 'echo "--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

log "SOURCE FILES"
find src -type f \( -name "*.c" -o -name "*.S" -o -name "*.asm" -o -name "*.h" \) \
-exec sh -c 'echo "\n--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

log "OBJECT SYMBOLS"
find obj -name "*.o" -exec sh -c 'echo "\n--- {} ---"; nm {} | head -n 50' \; | tee -a "$OUT"

echo
echo "Done. Upload or paste build_dump.txt"

