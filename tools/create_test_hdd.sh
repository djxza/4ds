#!/bin/bash
# Create a test disk image with FAT32
dd if=/dev/zero of=test.hdd bs=1M count=32
mkfs.fat -F 32 test.hdd

# Create mount point and mount
mkdir -p mnt
sudo mount -o loop test.hdd mnt

# Copy your icon
sudo cp brew.png mnt/
sudo cp kernel/src/TEST.TXT mnt/

# Unmount
sudo umount mnt

echo "Created test.hdd with your images"
