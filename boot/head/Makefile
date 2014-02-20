include ../../Makefile.inc

CFLAGS  += -fno-toplevel-reorder -Os -ffixed-cr2 -ffixed-cr3 -ffixed-cr4

HEAD_BIN = $(BIN_DIR)/head.bin
HEAD_ELF = $(BIN_DIR)/head.elf
HEAD_LDS = $(LDS_DIR)/head.lds

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all : $(HEAD_BIN)


$(HEAD_BIN) : $(AS_OBJS) $(C_OBJS) $(HEAD_LDS) Makefile
	$(LD) $(LDFLAGS) -T $(HEAD_LDS) -o $(HEAD_ELF) $(AS_OBJS) $(C_OBJS)
	$(OBJCOPY) -O binary $(HEAD_ELF) $@
	$(OBJDUMP) -d $(AS_OBJS) $(C_OBJS) > bin/head.ls


clean :
	rm -f $(HEAD_BIN) $(HEAD_ELF) $(AS_OBJS) $(C_OBJS)
