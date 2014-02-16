# ソフトウェア
MKHDR=../tools/mkhdr
CC=gcc
ASMFLAGS=-Wall -nostdlib -c
CFLAGS=-Wall -std=gnu99 -nostdlib -c -I../main/include
AR=ar
ARFLAGS=rcsv


# ディレクトリ
SRC=src
INCLUDE=include
BIN=bin


# ファイル
ONSEN_LIB=$(BIN)/libonsen.a


all :
	make mkhdr
	make $(ONSEN_LIB)


$(BIN)/%.o : $(SRC)/%.s Makefile
	$(CC) $(ASMFLAGS) $(SRC)/$*.s -o $@


$(BIN)/%.o : $(SRC)/%.c $(INCLUDE)/*.h Makefile
	$(CC) $(CFLAGS) -I$(INCLUDE) $(SRC)/$*.c -o $@


asm_obj : $(SRC)/*.s
	for fname in $? ; do              \
		fname=$${fname##*/} ;         \
		fname=$(BIN)/$${fname%.*}.o ; \
		make $$fname ;                \
	done


c_obj : $(INCLUDE)/*.h $(SRC)/*.c
	for fname in $? ; do                     \
		if [ $${fname##*.} != "h" ] ; then   \
			fname=$${fname##*/} ;            \
			fname=$(BIN)/$${fname%.*}.o ;    \
			make $$fname ;                   \
		fi                                   \
	done


$(ONSEN_LIB) : $(INCLUDE)/*.h $(SRC)/*.c
	make c_obj
	$(AR) $(ARFLAGS) $@ $(BIN)/*.o

mkhdr :
	$(MKHDR) $(SRC) $(INCLUDE)

