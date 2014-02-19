#ifndef MEMORY_H
#define MEMORY_H

#define ADDR_SYS_INFO    (0x00000FF0)  /* システム情報が格納されているアドレス */
#define ADDR_IPL         (0x7C00)      /* IPLが読み込まれている場所 */
#define ADDR_DISK_SRC    (0x00008000)  /* ディスクキャッシュの場所（リアルモード） */
#define ADDR_DISK_DST    (0x00100000)  /* ディスクキャッシュの場所 */
#define ADDR_OS          (0x00280000)  /* OSのロード先 */
#define ADDR_OS_PDT      (0x00400000)  /* 必ず4KB境界であること */
#define ADDR_FREE2_START (0x00402000)
#define ADDR_MEMTEST_START ADDR_FREE2_START
#define ADDR_MEMTEST_END (0xBFFFFFFF)

#endif
