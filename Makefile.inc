ARCH    = i586-elf
CC      = ${ARCH}-gcc
CXX     = ${ARCH}-g++
AS      = ${ARCH}-as
LD      = ${ARCH}-ld
AR      = ${ARCH}-ar
OBJCOPY = ${ARCH}-objcopy
OBJDUMP = ${ARCH}-objdump

SRC_DIR = src
LDS_DIR = ldscript
INC_DIR = include
BIN_DIR = bin

CFLAGS  = -Wall -nostdlib -I$(INC_DIR) -g -std=gnu99 -ffreestanding
ASFLAGS = -Wall -nostdlib
LDFLAGS = -nostdlib -g

