#ifndef MEMORY_H
#define MEMORY_H

#define VADDR_BASE          (0xC0000000)

#define MADDR_OS_PDT        (0x00001000)  // 必ず4KB境界であること
#define ADDR_SYS_INFO       (0x00002000)  // システム情報が格納されているアドレス
#define ADDR_MMAP_TBL       (0x00003000)  // 使用可能メモリ情報のテーブル
#define VADDR_OS_STACK      (0xC0200000)

#define VADDR_VRAM          (0xE0000000)
#define VADDR_PD_SELF       (0xFFFFF000)

void memory_copy32(char *dst, char *src, int num_bytes);
void memory_set32(char *p, char val, int num_bytes);

#endif
