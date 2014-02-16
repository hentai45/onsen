# ソフトウェア
ASFLAGS = -Wall -nostdlib
CFLAGS  = -Wall -std=gnu99 -nostdlib -I$(INC_DIR) -I$(API_INC_DIR)
MKHDR   = ../tools/mkhdr


# ディレクトリ
SRC_DIR = src
LDS_DIR = lds
INC_DIR = include
API_INC_DIR = ../api/include
BIN_DIR = bin


# ファイル
ONSEN = $(BIN_DIR)/onsen.sys
LDS = $(LDS_DIR)/os.lds
MAP = onsen.map

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_SRCS += $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/mem/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all :
	make mkhdr
	make $(ONSEN)


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


$(ONSEN) : $(AS_OBJS) $(C_OBJS) $(LDS) Makefile
	$(CC) $(CFLAGS) -Wl,-Map=$(MAP) $(AS_OBJS) $(C_OBJS) -T $(LDS) -o $@


clean :
	rm -f $(AS_OBJS) $(C_OBJS)
