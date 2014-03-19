include Makefile.inc

# ソフトウェア
QEMU = qemu-system-i386
QEMU_FLAGS = -m 32 -localtime -vga std
BOCHS = bochs


# ディレクトリ
GDBINIT_DIR = etc/gdbinit

MOUNT_DIR = partition2


# ファイル
IMG    = $(BIN_DIR)/onsen.img
MBR    = boot/mbr/$(BIN_DIR)/mbr
VBR    = boot/vbr1/$(BIN_DIR)/vbr
HEAD   = boot/head/$(BIN_DIR)/head
ONSEN  = kernel/$(BIN_DIR)/onsen
FNAMES = kernel/$(BIN_DIR)/fnames.dat
APPS   = $(wildcard app/$(BIN_DIR)/*)
VOL2   = vol2.img


all : img


img :
	$(MAKE) -C boot
	$(MAKE) -C kernel
	$(MAKE) -C app
	$(MAKE) $(IMG)


$(IMG) : $(MBR) $(VBR) $(HEAD) $(ONSEN)
	cat $(MBR) $(VBR) $(HEAD) $(ONSEN) >$@
	truncate -s 516096 $@
	dd bs=512 skip=1008 if=$(VOL2) count=2016 >>$@


vol2 : $(VOL2)


$(VOL2) : $(MBR) $(FNAMES) $(APPS)
	cp $(MBR) $@
	truncate -s 1548288 $@
	sudo losetup -o516096 /dev/loop0 $@
	sudo mke2fs /dev/loop0
	sudo mount -text2 /dev/loop0 $(MOUNT_DIR)
	sudo cp Makefile.inc $(MOUNT_DIR)
	sudo cp $(FNAMES) $(MOUNT_DIR)
	sudo cp $(APPS) $(MOUNT_DIR)
	sync
	sudo umount /dev/loop0
	sudo losetup -d /dev/loop0


fdisk :
	@fdisk -u -C3 -S63 -H16 $(IMG)


run :
	$(MAKE) runq


runb :
	$(MAKE) img
	$(BOCHS) -f etc/bochsrc


runq :
	$(MAKE) img
	$(QEMU) $(QEMU_FLAGS) -hda $(IMG) &


# Usage: make dbg f=gdbinit_file
dbg :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -hda $(IMG) &
	sleep 1
	gdb -x $(GDBINIT_DIR)/$(f)


clean :
	$(MAKE) clean -C boot
	$(MAKE) clean -C kernel
	$(MAKE) clean -C app
	rm -f $(BIN_DIR)/*
