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
	$(MAKE) $(BIN_DIR)/a
	$(MAKE) -C haribote


#$(CC) $(CFLAGS) -Wl,-Map=hello.map hello.c -T $(LDS) -o $@ -L$(LIB_DIR) -lonsen
$(BIN_DIR)/a : $(ONSEN_LIB) $(LDS) a.c Makefile
	$(CC) $(CFLAGS) -Wl,-Map=hello.map,-eentry a.c -o $@ -L$(LIB_DIR) -lonsen


clean :
	rm -f $(BIN_DIR)/a
	$(MAKE) clean -C haribote
