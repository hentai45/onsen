PRJ_DIR = ..

include $(PRJ_DIR)/Makefile.inc

API_LIB_DIR = api/$(BIN_DIR)
MYC_LIB_DIR = libmyc/$(BIN_DIR)
KERNEL_INC_DIR = $(PRJ_DIR)/kernel/$(INC_DIR)
MYC_INC_DIR = libmyc/$(INC_DIR)

CFLAGS += -Iapi/$(INC_DIR) -I$(MYC_INC_DIR)

ONSEN_LIB = $(LIB_DIR)/libonsen.a

SRCS  := $(wildcard *.c)
OBJS  := $(SRCS:.c=.o)
BINS  := $(foreach fname, $(SRCS:.c=), $(BIN_DIR)/$(fname))


all :
	$(MAKE) -C api
	$(MAKE) -C libmyc
	cp $(KERNEL_INC_DIR)/msg.h $(INC_DIR)
	cp $(KERNEL_INC_DIR)/keycode.h $(INC_DIR)
	$(MAKE) $(BINS)


$(BIN_DIR)/% : %.c
	$(CC) $(CFLAGS) -Wl,-Map=$(<:.c=.map),-emain $< -o $@ -L$(MYC_LIB_DIR) -lmyc -L$(API_LIB_DIR) -lonsen


clean :
	$(MAKE) clean -C api
	$(MAKE) clean -C libmyc
	rm -f $(BINS)
