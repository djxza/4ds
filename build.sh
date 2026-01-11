# 1. Rebuild everything cleanly
sudo make clean
sudo make all-hdd

# 2. Check the disk image exists
ls -lh *.hdd

#-drive file=template-x86_64.hdd,format=raw,if=ide"
sudo qemu-system-x86_64 \
		-drive if=ide,unit=0,format=raw,file=template-x86_64.hdd \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		-display sdl \
		-serial stdio
