include ../../Makefile.inc

LIB_DIR = ../../api/haribote/$(BIN_DIR)

LDS     = lds/app.lds
HRB_LIB = ../../api/haribote/bin/libhrb.a
HRB_H   = ../../api/haribote/include

SRCS := $(wildcard *.c)
HRBS := $(foreach hrb, $(SRCS:.c=.hrb), $(BIN_DIR)/$(hrb))


all :
	$(MAKE) $(HRBS)


$(BIN_DIR)/%.hrb : %.c
	$(CC) $(CFLAGS) -Wl,-Map=$(<:.c=.map) $< -T $(LDS) -o $@ -I$(HRB_H) -L$(LIB_DIR) -lhrb


clean :
	rm -f $(HRBS)
