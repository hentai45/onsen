/**
 * ブートローダ
 */

#include "hrbbin.h"


/* IPLが読み込んだシリンダ数 */
#define CYLS (40)

/* IPLが読み込まれている場所 */
#define ADDR_IPL (0x7C00)

/* onsen.sysのロード先 */
#define ADDR_ONSENSYS (0x00280000)

/* ディスクキャッシュの場所 */
#define ADDR_DISK_CACHE (0x00100000)

/* ディスクキャッシュの場所（リアルモード） */
#define ADDR_DISK_CACHE_REAL (0x00008000)


static void move_disk_data(void);
static void move_onsensys(void);
static void run_onsensys(void);

/**
 * 32ビット処理のメイン関数
 */
void head32_main(void)
{
    move_disk_data();  /* ディスクデータをキャッシュへ転送 */
    move_onsensys();   /* onsen.sysを転送 */
    run_onsensys();    /* onsen.sysを実行 */
}


static void memcopy(char *dst, char *src, int num_bytes);

#define BYTES_PER_CYL (512 * 18 * 2)

/**
 * ディスクデータをキャッシュへ転送する
 */
static void move_disk_data(void)
{
    /* ブートセクタ */
    char *src = (char *) ADDR_IPL;
    char *dst = (char *) ADDR_DISK_CACHE;
    memcopy(dst, src, 512);

    /* 残り */
    src = (char *) (ADDR_DISK_CACHE_REAL + 512);
    dst = (char *) (ADDR_DISK_CACHE      + 512);
    int num_bytes = BYTES_PER_CYL * (CYLS - 1);  // IPLの分1引く
    memcopy(dst, src, num_bytes);
}


extern int OnSenMain;

/**
 * onsen.sysを転送する
 */
static void move_onsensys(void)
{
    /* onsen.sysを実行可能セグメントへ転送 */
    char *src = (char *) &OnSenMain;
    char *dst = (char *) ADDR_ONSENSYS;
    int num_bytes = 512 * 1024;
    memcopy(dst, src, num_bytes);

    /* .dataを読み書き可能セグメントへ転送 */
    HRB_HEADER *hdr = (HRB_HEADER *) &OnSenMain;
    num_bytes = hdr->data_size;
    if (num_bytes != 0) {
        src = (char *) hdr->src_data;
        dst = (char *) hdr->dst_data;
        memcopy(dst, src, num_bytes);
    }
}

/**
 * メモリコピー
 */
static void memcopy(char *dst, char *src, int num_bytes)
{
    int i;
    for (i = 0; i < num_bytes; i++) {
        *dst++ = *src++;
    }
}


/**
 * onsen.sysを実行する
 */
static void run_onsensys(void)
{
    HRB_HEADER *hdr = (HRB_HEADER *) &OnSenMain;

    __asm__ __volatile__ (
        "movl %0, %%esp\n"
        "ljmpl $2*8, $0x28001B\n"  /* 2つめの実行可能なセグメントへジャンプ */
 
        :
        : "r"(hdr->dst_data)
    );
}

