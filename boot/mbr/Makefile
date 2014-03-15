include ../../Makefile.inc

CFLAGS  += -fno-toplevel-reorder -Os

MBR      = $(BIN_DIR)/mbr
MBR_ELF  = $(BIN_DIR)/mbr.elf
MBR_OBJS = $(SRC_DIR)/mbr0.o $(SRC_DIR)/mbr1.o
MBR_LDS  = $(LDS_DIR)/mbr.lds
MBR_ALS  = $(BIN_DIR)/mbr.lst


all : $(MBR)


$(MBR) : $(MBR_OBJS) $(MBR_LDS) Makefile
	$(LD) $(LDFLAGS) -T $(MBR_LDS) -o $(MBR_ELF) $(MBR_OBJS)
	$(OBJCOPY) -O binary $(MBR_ELF) $@
	$(OBJDUMP) -d $(MBR_OBJS) >$(MBR_ALS)


clean :
	rm -f $(BIN_DIR)/* $(MBR_OBJS)
