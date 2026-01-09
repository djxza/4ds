#!/bin/bash

OUT=build_dump.txt
echo "Im trying to make an os, but when i start up only black is visible in qemu, like the framebuffer is laoding but i cant access it. project dump:"
echo "$OUT ..."
echo "" > "$OUT"

log() {
    echo -e "\n===== $1 =====\n" | tee -a "$OUT"
}

log "TREE"
tree -a | tee -a "$OUT"

log "MAKEFILE"
sed -n '1,300p' makefile | tee -a "$OUT"
log "kernel/GNUMAKEFILE"
sed -n '1,300p' kernel/GNUmakefile | tee -a "$OUT"

log "LINKER SCRIPTS"
find . -name "*.ld" -exec sh -c 'echo "--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

log "GRUB CONFIG"
find . -name "grub.cfg" -exec sh -c 'echo "--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

log "SOURCE FILES"
find ./kernel/src -type f \( -name "*.c" -o -name "*.S" -o -name "*.asm" -o -name "*.h" \) \
-exec sh -c 'echo "\n--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

log "OBJECT SYMBOLS"
find obj -name "*.o" -exec sh -c 'echo "\n--- {} ---"; nm {} | head -n 50' \; | tee -a "$OUT"

echo
echo "Done. Upload or paste build_dump.txt"

