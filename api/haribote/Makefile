include ../../Makefile.inc

MKHDR   = ../../tools/mkhdr

KERNEL_INC_DIR = ../../kernel/$(INC_DIR)

CFLAGS += -I$(KERNEL_INC_DIR)
ARFLAGS = rcsv


LIB = $(BIN_DIR)/libhrb.a

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all :
	$(MAKE) mkhdr
	cp $(INC_DIR)/apino.h $(KERNEL_INC_DIR)/haribote
	$(MAKE) $(LIB)


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


$(LIB) : $(AS_OBJS) $(C_OBJS)
	$(AR) $(ARFLAGS) $@ $(AS_OBJS) $(C_OBJS)


clean :
	rm -f $(LIB) $(AS_OBJS) $(C_OBJS)
