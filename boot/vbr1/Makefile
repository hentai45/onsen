include ../../Makefile.inc

CFLAGS  += -fno-toplevel-reorder -Os

VBR      = $(BIN_DIR)/vbr
VBR_ELF  = $(BIN_DIR)/vbr.elf
VBR_OBJS = $(SRC_DIR)/vbr0.o $(SRC_DIR)/vbr1.o
VBR_LDS  = $(LDS_DIR)/vbr.lds
VBR_ALS  = $(BIN_DIR)/vbr.lst


all : $(VBR)


$(VBR) : $(VBR_OBJS) $(VBR_LDS) Makefile
	$(LD) $(LDFLAGS) -T $(VBR_LDS) -o $(VBR_ELF) $(VBR_OBJS)
	$(OBJCOPY) -O binary $(VBR_ELF) $@
	$(OBJDUMP) -d $(VBR_OBJS) >$(VBR_ALS)


clean :
	rm -f $(BIN_DIR)/* $(VBR_OBJS)
