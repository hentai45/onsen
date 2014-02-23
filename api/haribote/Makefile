include ../../Makefile.inc

MKHDR   = ../../tools/mkhdr

CFLAGS += -I$(MAIN_INC_DIR)
ARFLAGS = rcsv


MAIN_INC_DIR = ../main/$(INC_DIR)


LIB = $(BIN_DIR)/libhrb.a

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all :
	make mkhdr
	cp $(INC_DIR)/haribote.h ../../main/$(INC_DIR)
	make $(LIB)


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


$(LIB) : $(AS_OBJS) $(C_OBJS)
	$(AR) $(ARFLAGS) $@ $(AS_OBJS) $(C_OBJS)


clean :
	rm -f $(LIB) $(AS_OBJS) $(C_OBJS)
