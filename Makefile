include Makefile.inc

# ソフトウェア
QEMU = qemu-system-i386
QEMU_FLAGS = -m 32 -localtime -vga std
BOCHS = bochs


# ディレクトリ
GDBINIT_DIR = etc/gdbinit


# ファイル
IMG    = $(BIN_DIR)/onsen.img
MBR    = boot/mbr/$(BIN_DIR)/mbr
VBR    = boot/vbr1/$(BIN_DIR)/vbr
HEAD   = boot/head/$(BIN_DIR)/head
ONSEN  = kernel/$(BIN_DIR)/onsen
FNAMES = kernel/$(BIN_DIR)/fnames.dat
APPS   = $(wildcard app/$(BIN_DIR)/*)


all : img


img :
	$(MAKE) -C boot
	$(MAKE) -C kernel
	$(MAKE) -C app
	$(MAKE) $(IMG)


$(ONSEN_SYS) : $(HEAD) $(ONSEN)
	cat $(HEAD) $(ONSEN) > $@


$(IMG) : $(MBR) $(VBR) $(ONSEN) $(FNAMES) $(APPS)
	cat $(MBR) $(VBR) $(HEAD) $(ONSEN) >$@
	truncate -s 504K $@


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
