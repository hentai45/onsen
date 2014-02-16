# ソフトウェア
CC=gcc
ASMFLAGS=-Wall -nostdlib
CFLAGS=-Wall -std=gnu99 -nostdlib
MKHDR=../tools/mkhdr


# ディレクトリ
SRC=src
LDS=lds
INCLUDE=include
ONSEN_INC=../api/include
BIN=bin


# ファイル
ONSEN=$(BIN)/onsen.sys


all :
	make mkhdr
	make $(ONSEN)


mkhdr :
	$(MKHDR) $(SRC) $(INCLUDE)


$(ONSEN) : $(ONSEN_INC)/onsen.h $(INCLUDE)/*.h $(SRC)/boot/boot.S $(SRC)/*.S $(SRC)/mem/*.S $(SRC)/*.c $(LDS)/os.lds Makefile
	$(CC) $(ASMFLAGS) -c -I$(INCLUDE) $(SRC)/boot/boot.S -o $(BIN)/boot.o
	$(CC) $(CFLAGS) -Wl,-Map=onsen.map -I$(INCLUDE) -I$(ONSEN_INC) $(BIN)/boot.o $(SRC)/*.S $(SRC)/mem/*.S $(SRC)/*.c -T $(LDS)/os.lds -o $@

