include ../Makefile.inc

MKHDR    = ../tools/mkhdr
MKFNAMES = ../tools/mkfuncnames


ONSEN     = $(BIN_DIR)/onsen
ONSEN_ELF = $(BIN_DIR)/onsen.elf
LDS       = $(LDS_DIR)/os.lds
SYMBOLS   = $(BIN_DIR)/symbols.txt
MAP       = $(BIN_DIR)/onsen.map
FNAMES    = $(BIN_DIR)/fnames.dat
ASMLIST   = $(BIN_DIR)/onsen.lst

AS_SRCS := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.S))
AS_SRCS += $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/mem/*.S))
AS_OBJS := $(AS_SRCS:.S=.o)

C_SRCS  := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
C_OBJS  := $(C_SRCS:.c=.o)


all :
	$(MAKE) mkhdr
	$(MAKE) $(ONSEN)
	$(MAKE) mkfnames


mkhdr :
	$(MKHDR) $(SRC_DIR) $(INC_DIR)


mkfnames : $(MAP)
	$(MKFNAMES) $(SYMBOLS) $(FNAMES)


# objcopyで巨大なファイルができたなら.note.gnu.build-idなどの余計なセクションが
# 残っている可能性がある。その場合はstripで余計なセクションを捨てる。
# strip -R .note.gnu.build-id -o $(BIN_DIR)/temp.elf $(ONSEN_ELF)
# $(OBJCOPY) -S -O binary $(BIN_DIR)/temp.elf $@
$(ONSEN) : $(AS_OBJS) $(C_OBJS) $(LDS) Makefile
	$(CC) $(CFLAGS) -Wl,-Map=$(MAP) $(AS_OBJS) $(C_OBJS) -T $(LDS) -o $(ONSEN_ELF)
	strip -o $@ $(ONSEN_ELF)
	nm $(ONSEN_ELF) > $(SYMBOLS)
	$(OBJDUMP) -d $(AS_OBJS) $(C_OBJS) >$(ASMLIST)


clean :
	rm -f $(BIN_DIR)/* $(AS_OBJS) $(C_OBJS)
