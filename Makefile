# ソフトウェア
QEMU = qemu-system-i386
QEMU_FLAGS = -m 32 -localtime -vga std


# ディレクトリ
BIN_DIR = bin
APP_BIN = app/bin


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


$(IMG) : $(IPL) $(ONSEN_SYS) $(APP_BIN)/hello test.bmp
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	mcopy $(ONSEN_SYS) -i $@ ::
	mcopy $(APP_BIN)/hello -i $@ ::
	mcopy test.bmp -i $@ ::


mount : $(IMG)
	sudo mount -o loop $< floppy

umount:
	sudo umount floppy


run :
	make img
	$(QEMU) $(QEMU_FLAGS) -fda $(IMG) &


debug_ipl :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -tui -x debug/gdbinit_ipl


debug_head :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x debug/gdbinit_head


debug :
	make img
	$(QEMU) -S -s $(QEMU_FLAGS) -fda $(IMG) &
	sleep 1
	gdb -x debug/gdbinit_main


clean :
	make clean -C boot
	make clean -C main
	make clean -C api
	make clean -C app
	rm -f $(IMG)
