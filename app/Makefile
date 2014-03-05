PRJ_DIR = ..

include $(PRJ_DIR)/Makefile.inc

LIB_DIR = api/$(BIN_DIR)
KERNEL_INC_DIR = $(PRJ_DIR)/kernel/$(INC_DIR)

CFLAGS += -Iapi/$(INC_DIR)

ONSEN_LIB = $(LIB_DIR)/libonsen.a

SRCS  := $(wildcard *.c)
OBJS  := $(SRCS:.c=.o)
BINS  := $(foreach fname, $(SRCS:.c=), $(BIN_DIR)/$(fname))


all :
	$(MAKE) -C api
	cp $(KERNEL_INC_DIR)/msg.h $(INC_DIR)
	cp $(KERNEL_INC_DIR)/keycode.h $(INC_DIR)
	$(MAKE) $(BINS)


$(BIN_DIR)/% : %.c
	$(CC) $(CFLAGS) -Wl,-Map=$(<:.c=.map),-emain $< -o $@ -L$(LIB_DIR) -lonsen


clean :
	$(MAKE) clean -C api
	rm -f $(BINS)
