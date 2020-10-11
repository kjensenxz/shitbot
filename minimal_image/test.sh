#!/bin/sh
qemu-system-x86_64 \
	-no-user-config \
	-sandbox on,obsolete=deny,elevateprivileges=deny,spawn=deny,resourcecontrol=deny \
	-nodefaults -nographic -no-reboot -no-shutdown \
	-m 64M \
	-kernel /home/kbjensen/prog/sandbox/minimal_image/bzImage \
	-initrd /home/kbjensen/prog/sandbox/minimal_image/initramfs.cpio.gz \
	-watchdog i6300esb -watchdog-action poweroff \
	-serial mon:stdio -append 'console=ttyS0,115200 bin=/bin/init' \
	-cpu Haswell-v1 
