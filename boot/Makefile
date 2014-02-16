ASFLAGS=-Wall -nostdlib

all : ipl.bin asmhead.bin

ipl.bin : ipl.s ipl.lds Makefile
	$(CC) $(ASFLAGS) ipl.s -T ipl.lds -o $@

asmhead.bin : asmhead.s asmhead.lds Makefile
	$(CC) $(ASFLAGS) asmhead.s -T asmhead.lds -o $@

clean :
	rm -f ipl.bin asmhead.bin
