include Makefile.inc

# ソフトウェア
QEMU = qemu-system-i386
QEMU_FLAGS = -m 32 -localtime -vga std
BOCHS = bochs


# ディレクトリ
GDBINIT_DIR = etc/gdbinit


# ファイル
IMG    = $(BIN_DIR)/a.img
IPL    = boot/ipl/$(BIN_DIR)/ipl.bin
HEAD   = boot/head/$(BIN_DIR)/head.bin
ONSEN  = kernel/$(BIN_DIR)/onsen.sys
ONSEN_SYS = $(BIN_DIR)/onsen.sys
FNAMES = kernel/$(BIN_DIR)/fnames.bin
APPS   = $(wildcard app/$(BIN_DIR)/*)


all : img


img :
	$(MAKE) -C boot
	$(MAKE) -C kernel
	$(MAKE) -C app
	$(MAKE) $(IMG)


$(ONSEN_SYS) : $(HEAD) $(ONSEN)
	cat $(HEAD) $(ONSEN) > $@


$(IMG) : $(IPL) $(ONSEN_SYS) $(FNAMES) $(APPS)
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	mcopy $(ONSEN_SYS) -i $@ ::
	mcopy $(FNAMES) -i $@ ::
	mcopy $(APPS) -i $@ ::


mount : $(IMG)
	sudo mount -o loop $< floppy

umount:
	sudo umount floppy


run :
	$(MAKE) runq


runb :
	$(MAKE) img
	$(BOCHS) -f etc/bochsrc


runq :
	$(MAKE) img
	$(QEMU) $(QEMU_FLAGS) -fda $(IMG) &


dipl :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x $(GDBINIT_DIR)/ipl


dhead16 :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x $(GDBINIT_DIR)/head16


dhead32 :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x $(GDBINIT_DIR)/head32


dos :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x $(GDBINIT_DIR)/os


clean :
	$(MAKE) clean -C boot
	$(MAKE) clean -C kernel
	$(MAKE) clean -C app
	rm -f $(IMG)
