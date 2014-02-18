include ../../Makefile.inc

CFLAGS  += -fno-toplevel-reorder -Os

IPL      = $(BIN_DIR)/ipl.bin
IPL_OBJS = $(SRC_DIR)/ipl0.o $(SRC_DIR)/ipl1.o
IPL_LDS  = $(LDS_DIR)/ipl.lds
IPL_ALS  = $(BIN_DIR)/ipl.ls


all : $(IPL)


$(IPL) : $(IPL_OBJS) $(IPL_LDS) Makefile
	$(LD) $(LDFLAGS) -T $(IPL_LDS) -o $@ $(IPL_OBJS)
	$(OBJDUMP) -d $(IPL_OBJS) >$(IPL_ALS)


clean :
	rm -f $(IPL)
