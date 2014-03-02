include ../Makefile.inc

MKHDR    = ../tools/mkhdr
MKFNAMES = ../tools/mkfuncnames

CFLAGS += -I$(API_INC_DIR)

API_INC_DIR = ../api/$(INC_DIR)


ONSEN_BIN = $(BIN_DIR)/onsen.sys
ONSEN_ELF = $(BIN_DIR)/onsen.elf
LDS = $(LDS_DIR)/os.lds
MAP = onsen.map
FNAMES = $(BIN_DIR)/fnames.bin

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_SRCS += $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/mem/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)

CPP_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
CPP_OBJS  := $(CPP_SRCS:.cpp=.o)

all :
	$(MAKE) mkhdr
	$(MAKE) $(ONSEN_BIN)
	$(MAKE) mkfnames


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


mkfnames : $(MAP)
	$(MKFNAMES) $(MAP) $(FNAMES)


# objcopyで巨大なファイルができたなら.note.gnu.build-idなどの余計なセクションが
# 残っている可能性がある。その場合はstripで余計なセクションを捨てる。
# strip -R .note.gnu.build-id -o $(BIN_DIR)/temp.elf $(ONSEN_ELF)
# $(OBJCOPY) -S -O binary $(BIN_DIR)/temp.elf $@
$(ONSEN_BIN) : $(AS_OBJS) $(C_OBJS) $(CPP_OBJS) $(LDS) Makefile
	$(CC) $(CFLAGS) -Wl,-Map=$(MAP) $(AS_OBJS) $(C_OBJS) $(CPP_OBJS) -T $(LDS) -o $(ONSEN_ELF)
	$(OBJCOPY) -S -O binary $(ONSEN_ELF) $@
	$(OBJDUMP) -d $(AS_OBJS) $(C_OBJS) >$(BIN_DIR)/onsen.ls


clean :
	rm -f $(ONSEN_BIN) $(ONSEN_ELF) $(AS_OBJS) $(C_OBJS) $(CPP_OBJS)
