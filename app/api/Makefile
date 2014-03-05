PRJ_DIR = ../..

include $(PRJ_DIR)/Makefile.inc

MKHDR = $(PRJ_DIR)/tools/mkhdr

KERNEL_INC_DIR = $(PRJ_DIR)/kernel/$(INC_DIR)

CFLAGS += -I$(KERNEL_INC_DIR)
ARFLAGS = rcsv


LIB = $(BIN_DIR)/libonsen.a

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_SRCS += $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/mem/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all :
	$(MAKE) mkhdr
	$(MAKE) $(LIB)


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


$(LIB) : $(AS_OBJS) $(C_OBJS)
	$(AR) $(ARFLAGS) $@ $(AS_OBJS) $(C_OBJS)


clean :
	rm -f $(LIB) $(AS_OBJS) $(C_OBJS)
