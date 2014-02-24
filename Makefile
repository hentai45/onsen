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
ONSEN = main/bin/onsen.sys
ONSEN_SYS = $(BIN_DIR)/onsen.sys


all : img


img :
	make -C boot
	make mkhdr -C api
	make -C main
	make -C api
	make -C app
	make $(IMG)


$(ONSEN_SYS) : $(HEAD) $(ONSEN)
	cat $(HEAD) $(ONSEN) > $@


$(IMG) : $(IPL) $(ONSEN_SYS) $(HRB_APP_BIN)/hello.hrb $(APP_BIN)/test test.bmp
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	mcopy $(ONSEN_SYS) -i $@ ::
	mcopy $(HRB_APP_BIN)/hello.hrb -i $@ ::
	mcopy app/hello/hello.c -i $@ ::
	mcopy $(APP_BIN)/test -i $@ ::
	mcopy test.bmp -i $@ ::


mount : $(IMG)
	sudo mount -o loop $< floppy

umount:
	sudo umount floppy


run :
	make runq


runb :
	make img
	$(BOCHS)


runq :
	make img
	$(QEMU) $(QEMU_FLAGS) -fda $(IMG) &


dipl :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/ipl


dhead16 :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/head16


dhead32 :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/head32


dos :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit/os


clean :
	make clean -C boot
	make clean -C main
	make clean -C api
	make clean -C app
	rm -f $(IMG)
