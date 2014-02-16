# ソフトウェア
QEMU=qemu-system-i386
QEMU_OPT=-m 32 -localtime -vga std


# ディレクトリ
BIN=bin
APP_BIN=app/bin


# ファイル
IMG=$(BIN)/a.img
ONSEN_SYS=main/bin/onsen.sys


all : img


$(IMG) : $(ONSEN_SYS) $(APP_BIN)/hello test.bmp
	cp grub.img $@
	mcopy $(ONSEN_SYS) -i $@ ::
	mcopy $(APP_BIN)/hello -i $@ ::
	mcopy main/src/task.c -i $@ ::
	mcopy test.bmp -i $@ ::


img :
	make mkhdr -C api
	make -C main
	make -C api
	make -C app
	make $(IMG)


run :
	make img
	$(QEMU) $(QEMU_OPT) -fda $(IMG) &


gdbinit :
	echo 'target remote localhost:1234' >$@
	echo 'set architecture i8086' >>$@
	echo 'break *0x07c00' >>$@
	echo 'c' >>$@


debug : gdbinit
	make img
	$(QEMU) -S -s $(QEMU_OPT) -fda $(IMG) &
	sleep 1
	gdb -x gdbinit

