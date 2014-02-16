ASMFLAGS=-Wall -nostdlib

all : ipl.bin asmhead.bin

ipl.bin : ipl.s ipl.lds Makefile
	$(CC) $(ASMFLAGS) ipl.s -T ipl.lds -o $@

asmhead.bin : asmhead.s asmhead.lds Makefile
	$(CC) $(ASMFLAGS) asmhead.s -T asmhead.lds -o $@

clean :
	rm -f ipl.bin asmhead.bin
