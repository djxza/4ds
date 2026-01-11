# Nuke built-in rules and variables
.SUFFIXES:
MAKEFLAGS += --no-builtin-rules --no-builtin-variables

# ========= CONFIGURATION =========
ARCH ?= x86_64
IMAGE_NAME := template-$(ARCH)
QEMUFLAGS ?= -m 2G

# Toolchain configuration
HOST_CC := cc
HOST_CFLAGS := -g -O2 -pipe -Wall
HOST_CPPFLAGS :=
HOST_LDFLAGS :=
HOST_LIBS :=

# Architecture-specific configurations
# Format: ARCH:MACHINE:CPU:GPU
X86_64_MACHINE := q35
X86_64_CPU := 
X86_64_GPU := VGA,vgamem_mb=16

AARCH64_MACHINE := virt
AARCH64_CPU := cortex-a72
AARCH64_GPU := ramfb

RISCV64_MACHINE := virt
RISCV64_CPU := rv64
RISCV64_GPU := ramfb

LOONGARCH64_MACHINE := virt
LOONGARCH64_CPU := la464
LOONGARCH64_GPU := ramfb

# Set machine, cpu, and gpu based on ARCH
ifeq ($(ARCH),x86_64)
MACHINE := $(X86_64_MACHINE)
CPU := $(X86_64_CPU)
GPU_DEVICE := $(X86_64_GPU)
endif

ifeq ($(ARCH),aarch64)
MACHINE := $(AARCH64_MACHINE)
CPU := $(AARCH64_CPU)
GPU_DEVICE := $(AARCH64_GPU)
endif

ifeq ($(ARCH),riscv64)
MACHINE := $(RISCV64_MACHINE)
CPU := $(RISCV64_CPU)
GPU_DEVICE := $(RISCV64_GPU)
endif

ifeq ($(ARCH),loongarch64)
MACHINE := $(LOONGARCH64_MACHINE)
CPU := $(LOONGARCH64_CPU)
GPU_DEVICE := $(LOONGARCH64_GPU)
endif

# Define supported architectures
SUPPORTED_ARCHS := x86_64 aarch64 riscv64 loongarch64

# ========= VALIDATION =========
ifeq ($(filter $(ARCH),$(SUPPORTED_ARCHS)),)
$(error Unsupported architecture: $(ARCH). Supported: $(SUPPORTED_ARCHS))
endif

# ========= PHONY TARGETS =========
.PHONY: all all-hdd run run-hdd clean distclean dump help

all: $(IMAGE_NAME).iso

all-hdd: $(IMAGE_NAME).hdd

run: run-$(ARCH)

run-hdd: run-hdd-$(ARCH)

clean:
	$(MAKE) -C kernel clean
	rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd

distclean:
	$(MAKE) -C kernel distclean
	rm -rf iso_root *.iso *.hdd limine edk2-ovmf

dump:
	chmod +x ./tools/dump_proj.sh
	./tools/dump_proj.sh

help:
	@echo "Available targets:"
	@echo "  all           - Build ISO image for $(ARCH)"
	@echo "  all-hdd       - Build hard disk image for $(ARCH)"
	@echo "  run           - Run ISO in QEMU for $(ARCH)"
	@echo "  run-hdd       - Run HDD in QEMU for $(ARCH)"
	@echo "  clean         - Remove build artifacts"
	@echo "  distclean     - Remove all generated files"
	@echo "  dump          - Dump project structure"
	@echo ""
	@echo "Configuration:"
	@echo "  ARCH          - Target architecture ($(ARCH))"
	@echo "  QEMUFLAGS     - QEMU additional flags ($(QEMUFLAGS))"
	@echo ""
	@echo "Supported architectures: $(SUPPORTED_ARCHS)"

# ========= DEPENDENCIES =========
.PHONY: deps limine-deps kernel-deps ovmf-deps

deps: ovmf-deps limine-deps kernel-deps

ovmf-deps: edk2-ovmf

limine-deps: limine/limine

kernel-deps: kernel/.deps-obtained

edk2-ovmf:
	@echo "Downloading EDK2 OVMF firmware..."
	curl -Ls https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/edk2-ovmf.tar.gz | \
		tar -xzf - 2>/dev/null || \
		(echo "Failed to download OVMF firmware" && false)

limine/limine:
	@echo "Building Limine bootloader..."
	rm -rf limine
	git clone -q --branch=v10.x-binary --depth=1 \
		https://codeberg.org/Limine/Limine.git limine 2>/dev/null
	$(MAKE) -s -C limine \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"

kernel/.deps-obtained:
	./kernel/get-deps

.PHONY: kernel
kernel: kernel/.deps-obtained
	$(MAKE) -C kernel ARCH=$(ARCH)

# Update the QEMU command to use our HDD correctly
define QEMU_UEFI_RUN
qemu-system-$(ARCH) \
    -M $(MACHINE) \
    $(if $(CPU),-cpu $(CPU)) \
    $(if $(GPU_DEVICE),-device $(GPU_DEVICE)) \
    -drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -display sdl \
    -serial stdio \
    -drive file=$(1),format=raw,index=0,media=disk \
    $(QEMUFLAGS)
endef

.PHONY: run-x86_64 run-aarch64 run-riscv64 run-loongarch64
.PHONY: run-hdd-x86_64 run-hdd-aarch64 run-hdd-riscv64 run-hdd-loongarch64

run-x86_64: edk2-ovmf $(IMAGE_NAME).iso
	$(call QEMU_UEFI_RUN,-cdrom $(IMAGE_NAME).iso)

run-aarch64: edk2-ovmf $(IMAGE_NAME).iso
	$(call QEMU_UEFI_RUN,-cdrom $(IMAGE_NAME).iso)

run-riscv64: edk2-ovmf $(IMAGE_NAME).iso
	$(call QEMU_UEFI_RUN,-cdrom $(IMAGE_NAME).iso)

run-loongarch64: edk2-ovmf $(IMAGE_NAME).iso
	$(call QEMU_UEFI_RUN,-cdrom $(IMAGE_NAME).iso)

run-hdd-x86_64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-x86_64 \
		-M $(MACHINE) \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-x86_64.fd,readonly=on \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		-display sdl \
		-serial stdio \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

run-hdd-aarch64: edk2-ovmf $(IMAGE_NAME).hdd
	$(call QEMU_UEFI_RUN,-hda $(IMAGE_NAME).hdd)

run-hdd-riscv64: edk2-ovmf $(IMAGE_NAME).hdd
	$(call QEMU_UEFI_RUN,-hda $(IMAGE_NAME).hdd)

run-hdd-loongarch64: edk2-ovmf $(IMAGE_NAME).hdd
	$(call QEMU_UEFI_RUN,-hda $(IMAGE_NAME).hdd)

# BIOS targets (x86_64 only)
.PHONY: run-bios run-hdd-bios
run-bios: $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M q35 \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS)

run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M q35 \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

# ========= IMAGE BUILDING =========
# Common ISO creation function
define CREATE_ISO
	rm -rf iso_root
	mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	cp -v kernel/bin-$(ARCH)/kernel iso_root/boot/
	cp -v limine.conf iso_root/boot/limine/
	cp -v limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v $(1) iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	rm -rf iso_root
endef

# x86_64-specific ISO creation
ifeq ($(ARCH),x86_64)
$(IMAGE_NAME).iso: limine/limine kernel
	rm -rf iso_root
	mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	cp -v kernel/bin-$(ARCH)/kernel iso_root/boot/
	cp -v limine.conf iso_root/boot/limine/
	cp -v limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine/BOOTX64.EFI limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	./limine/limine bios-install $(IMAGE_NAME).iso
	rm -rf iso_root
else
# Non-x86_64 ISO creation
$(IMAGE_NAME).iso: limine/limine kernel
	$(call CREATE_ISO,limine/BOOT$(shell echo $(ARCH) | tr '[:lower:]' '[:upper:]').EFI)
endif

# HDD image creation - FIXED VERSION with proper UEFI boot support
# HDD image creation - MINIMAL WORKING VERSION
# HDD image creation - WORKING VERSION with mtools

$(IMAGE_NAME).hdd: limine/limine kernel
	@echo "=== Creating HDD image ==="
	
	# Clean up
	-rm -f $(IMAGE_NAME).hdd mnt -rf 2>/dev/null || true
	
	# Use mformat to create a FAT32 image
	dd if=/dev/zero of=$(IMAGE_NAME).hdd bs=512 count=131072
	mkfs.fat -F 32 $(IMAGE_NAME).hdd
	
	# Create mount point
	mkdir -p mnt
	
	# Mount using mtools
	# Create directory structure
	mmd -i $(IMAGE_NAME).hdd ::/EFI
	mmd -i $(IMAGE_NAME).hdd ::/EFI/BOOT
	mmd -i $(IMAGE_NAME).hdd ::/boot
	mmd -i $(IMAGE_NAME).hdd ::/boot/limine
	
	# Copy files
	mcopy -i $(IMAGE_NAME).hdd kernel/bin-$(ARCH)/kernel ::/boot/kernel
	mcopy -i $(IMAGE_NAME).hdd limine.conf ::/boot/limine/limine.conf
	
	# Create TEST.TXT with sample content
	echo "This is a test file from 4DS Launcher!" | mcopy -i $(IMAGE_NAME).hdd - ::/TEST.TXT
	echo "Hello from FAT32 filesystem!" | mcopy -i $(IMAGE_NAME).hdd - ::/HELLO.TXT
	
	# Architecture-specific bootloader
ifeq ($(ARCH),x86_64)
	mcopy -i $(IMAGE_NAME).hdd limine/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(IMAGE_NAME).hdd limine/limine-bios.sys ::/boot/limine/limine-bios.sys
	sudo ./limine/limine bios-install $(IMAGE_NAME).hdd 2>/dev/null || true
else
	mcopy -i $(IMAGE_NAME).hdd limine/BOOT$(shell echo $(ARCH) | tr '[:lower:]' '[:upper:]').EFI ::/EFI/BOOT/BOOT$(shell echo $(ARCH) | tr '[:lower:]' '[:upper:]').EFI
endif
	
	# Clean up
	rmdir mnt 2>/dev/null || true
	
	@echo "=== HDD image created ==="
