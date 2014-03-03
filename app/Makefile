include ../Makefile.inc

LIB_DIR = ../api/$(BIN_DIR)

LDS       = lds/app.lds
ONSEN_LIB = ../api/bin/libonsen.a
ONSEN_H   = ../api/include/onsen.h
MSG_H     = ../kernel/include/msg.h
KEY_H     = ../kernel/include/keycode.h


all :
	cp $(ONSEN_H) $(INC_DIR)
	cp $(MSG_H) $(INC_DIR)
	cp $(KEY_H) $(INC_DIR)
	$(MAKE) $(BIN_DIR)/hello
	$(MAKE) $(BIN_DIR)/test
	$(MAKE) -C haribote


$(BIN_DIR)/hello : $(ONSEN_LIB) $(LDS) hello.c Makefile
	$(CC) $(CFLAGS) -Wl,-Map=hello.map hello.c -T $(LDS) -o $@ -L$(LIB_DIR) -lonsen


$(BIN_DIR)/test : $(ONSEN_LIB) $(LDS) test.c Makefile
	$(CC) $(CFLAGS) test.c -T $(LDS) -o $@ -L$(LIB_DIR) -lonsen


clean :
	rm -f $(BIN_DIR)/hello $(BIN_DIR)/test
	$(MAKE) clean -C haribote
