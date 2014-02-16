CC=gcc
CFLAGS=-Wall -nostdlib -Iinclude -Wl,-entry=OnSenMain

BIN=bin
LIB=../api/bin
INCLUDE=include

ONSEN_LIB=../api/bin/libonsen.a
ONSEN_H=../api/include/onsen.h
MSG_H=../main/include/msg.h
KEY_H=../main/include/keycode.h


all :
	cp $(ONSEN_H) $(INCLUDE)
	cp $(MSG_H) $(INCLUDE)
	cp $(KEY_H) $(INCLUDE)
	make $(BIN)/hello
	make $(BIN)/crack


$(BIN)/hello : $(ONSEN_LIB) hello/*.c Makefile
	$(CC) $(CFLAGS) hello/*.c -o $@ -L$(LIB) -lonsen


$(BIN)/crack : $(ONSEN_LIB) crack/*.s Makefile
	$(CC) $(CFLAGS) crack/*.s -o $@ -L$(LIB) -lonsen

