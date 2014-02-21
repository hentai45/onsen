#ifndef MEMORY_H
#define MEMORY_H

#define VADDR_BASE          (0xC0000000)

#define ADDR_SYS_INFO       (0x00000A00)  // システム情報が格納されているアドレス
#define MADDR_OS_PDT        (0x00001000)  // 必ず4KB境界であること
#define ADDR_IPL            (0x00007C00)  // IPLが読み込まれている場所
#define ADDR_DISK_SRC       (0x00008000)  // ディスクキャッシュの場所（リアルモード）
#define ADDR_DISK_DST       (0x00100000)  // ディスクキャッシュの場所
#define ADDR_OS             (0x00280000)  // OSのロード先
#define MADDR_MEMTEST_START (0x00400000)
#define MADDR_MEMTEST_END   (0xBFFFFFFF)

#define VADDR_VRAM          (0xE0000000)
#define VADDR_PD_SELF       (0xFFFFF000)

#endif
