include ../../Makefile.inc

CFLAGS  += -fno-toplevel-reorder -Os

IPL_BIN  = $(BIN_DIR)/ipl.bin
IPL_ELF  = $(BIN_DIR)/ipl.elf
IPL_OBJS = $(SRC_DIR)/ipl0.o $(SRC_DIR)/ipl1.o
IPL_LDS  = $(LDS_DIR)/ipl.lds
IPL_ALS  = $(BIN_DIR)/ipl.ls


all : $(IPL_BIN)


$(IPL_BIN) : $(IPL_OBJS) $(IPL_LDS) Makefile
	$(LD) $(LDFLAGS) -T $(IPL_LDS) -o $(IPL_ELF) $(IPL_OBJS)
	$(OBJCOPY) -O binary $(IPL_ELF) $@
	$(OBJDUMP) -d $(IPL_OBJS) >$(IPL_ALS)


clean :
	rm -f $(IPL_BIN) $(IPL_ELF) $(IPL_OBJS)
