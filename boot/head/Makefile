include ../../Makefile.inc

CFLAGS  += -fno-toplevel-reorder -Os

HEAD     = $(BIN_DIR)/head.bin
HEAD_LDS = $(LDS_DIR)/head.lds

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all : $(HEAD)


$(HEAD) : $(AS_OBJS) $(C_OBJS) $(HEAD_LDS) Makefile
	$(LD) $(LDFLAGS) -T $(HEAD_LDS) -o $@ $(AS_OBJS) $(C_OBJS)
	$(OBJDUMP) -d $(AS_OBJS) $(C_OBJS) > bin/head.ls


clean :
	rm -f $(HEAD) $(AS_OBJS) $(C_OBJS)
