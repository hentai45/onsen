include ../Makefile.inc

LIB_DIR = ../api/$(BIN_DIR)

LDS       = lds/app.lds
ONSEN_LIB = ../api/bin/libonsen.a
ONSEN_H   = ../api/include/onsen.h
MSG_H     = ../main/include/msg.h
KEY_H     = ../main/include/keycode.h


all :
	cp $(ONSEN_H) $(INC_DIR)
	cp $(MSG_H) $(INC_DIR)
	cp $(KEY_H) $(INC_DIR)
	make $(BIN_DIR)/hello


$(BIN_DIR)/hello : $(ONSEN_LIB) $(LDS) hello/hello.c Makefile
	$(CC) $(CFLAGS) -Wl,-Map=hello.map hello/hello.c -T $(LDS) -o $@ -L$(LIB_DIR) -lonsen


clean :
	rm -f $(BIN_DIR)/hello $(BIN_DIR)/hello
