ARCH    = i586-elf
CC      = ${ARCH}-gcc
CPP     = ${ARCH}-g++
AS      = ${ARCH}-as
LD      = ${ARCH}-ld
AR      = ${ARCH}-ar
OBJCOPY = ${ARCH}-objcopy
OBJDUMP = ${ARCH}-objdump

SRC_DIR = src
LDS_DIR = lds
INC_DIR = include
BIN_DIR = bin

CFLAGS  = -Wall -nostdlib -I$(INC_DIR)
ASFLAGS = -Wall -nostdlib
LDFLAGS = -nostdlib

