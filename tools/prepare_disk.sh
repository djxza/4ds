#!/bin/bash
set -e

echo "=== Preparing HDD image ==="

# Create fresh 64MB disk image
echo "Creating disk image..."
rm -f template-x86_64.hdd
dd if=/dev/zero of=template-x86_64.hdd bs=1M count=64 status=none

# Create partition table
echo "Creating partition table..."
parted -s template-x86_64.hdd mklabel msdos
parted -s template-x86_64.hdd mkpart primary fat32 1MiB 100%

# Setup loop device
echo "Setting up loop device..."
LOOP_DEV=$(sudo losetup -f --show -P template-x86_64.hdd)
echo "Using loop device: $LOOP_DEV"

# Create FAT32 filesystem
echo "Creating FAT32 filesystem..."
sudo mkfs.fat -F32 ${LOOP_DEV}p1

# Mount the partition
echo "Mounting partition..."
mkdir -p mnt
sudo mount ${LOOP_DEV}p1 mnt

# Create directory structure
echo "Creating directory structure..."
sudo mkdir -p mnt/EFI/BOOT
sudo mkdir -p mnt/boot/limine

# Copy kernel
echo "Copying kernel..."
sudo cp kernel/bin-x86_64/kernel mnt/boot/

# Copy limine config
echo "Copying bootloader config..."
sudo cp limine.conf mnt/boot/limine/

# Copy bootloader files
echo "Copying bootloader files..."
sudo cp limine/limine-bios.sys mnt/boot/limine/
sudo cp limine/BOOTX64.EFI mnt/EFI/BOOT/
sudo cp limine/BOOTIA32.EFI mnt/EFI/BOOT/

# Create test files
echo "Creating test files..."
echo "Hello from 4DS Launcher!" | sudo tee mnt/TEST.TXT > /dev/null
echo "This is a test of the FAT32 filesystem." | sudo tee -a mnt/TEST.TXT > /dev/null
echo "If you can read this, filesystem is working!" | sudo tee -a mnt/TEST.TXT > /dev/null

# List files
echo "Files on disk:"
sudo ls -la mnt/
sudo ls -la mnt/boot/
sudo ls -la mnt/EFI/BOOT/

# Cleanup
echo "Cleaning up..."
sudo umount mnt
sudo losetup -d $LOOP_DEV
rmdir mnt

echo "=== HDD image ready: template-x86_64.hdd ==="
