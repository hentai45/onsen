include ../../Makefile.inc

LIB_DIR = ../../api/haribote/$(BIN_DIR)

LDS     = lds/app.lds
HRB_LIB = ../../api/haribote/bin/libhrb.a
HRB_H   = ../../api/haribote/include


all :
	make $(BIN_DIR)/hello.hrb


$(BIN_DIR)/hello.hrb : $(HRB_LIB) $(LDS) hello/hello.c Makefile
	$(CC) $(CFLAGS) -Wl,-Map=hello.map hello/hello.c -T $(LDS) -o $@ -I$(HRB_H) -L$(LIB_DIR) -lhrb


clean :
	rm -f $(BIN_DIR)/hello.hrb
