# ソフトウェア
QEMU = qemu-system-i386
QEMU_FLAGS = -m 32 -localtime -vga std
BOCHS = bochs


# ディレクトリ
BIN_DIR = bin
APP_BIN = app/bin
HRB_APP_BIN = app/haribote/bin


# ファイル
IMG = $(BIN_DIR)/a.img
IPL = boot/ipl/bin/ipl.bin
HEAD = boot/head/bin/head.bin
ONSEN = kernel/bin/onsen.sys
ONSEN_SYS = $(BIN_DIR)/onsen.sys
FNAMES = kernel/$(BIN_DIR)/fnames.bin
HRBS = $(wildcard $(HRB_APP_BIN)/*.hrb)


all : img


img :
	$(MAKE) -C boot
	$(MAKE) mkhdr -C api
	$(MAKE) -C kernel
	$(MAKE) -C api
	$(MAKE) -C app
	$(MAKE) $(IMG)


$(ONSEN_SYS) : $(HEAD) $(ONSEN)
	cat $(HEAD) $(ONSEN) > $@


$(IMG) : $(IPL) $(ONSEN_SYS) $(FNAMES) test.bmp $(HRBS) $(APP_BIN)/a
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	mcopy $(ONSEN_SYS) -i $@ ::
	mcopy $(FNAMES) -i $@ ::
	mcopy $(HRBS) -i $@ ::
	mcopy $(APP_BIN)/a -i $@ ::


mount : $(IMG)
	sudo mount -o loop $< floppy

umount:
	sudo umount floppy


run :
	$(MAKE) runq


runb :
	$(MAKE) img
	$(BOCHS)


runq :
	$(MAKE) img
	$(QEMU) $(QEMU_FLAGS) -fda $(IMG) &


dipl :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/ipl


dhead16 :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/head16


dhead32 :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/head32


dos :
	$(MAKE) img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/os


clean :
	$(MAKE) clean -C boot
	$(MAKE) clean -C kernel
	$(MAKE) clean -C api
	$(MAKE) clean -C app
	rm -f $(IMG)
