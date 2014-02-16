# ソフトウェア
ASFLAGS = -Wall -nostdlib -c
CFLAGS  = -Wall -std=gnu99 -nostdlib -c -Iinclude -I../main/include
ARFLAGS = rcsv
MKHDR = ../tools/mkhdr


# ディレクトリ
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin


# ファイル
LIB = $(BIN_DIR)/libonsen.a

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all :
	make mkhdr
	make $(LIB)


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


$(LIB) : $(AS_OBJS) $(C_OBJS)
	$(AR) $(ARFLAGS) $@ $(AS_OBJS) $(C_OBJS)


clean :
	rm -f $(AS_OBJS) $(C_OBJS)
