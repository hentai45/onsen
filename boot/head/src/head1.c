/**
 * ブートローダ
 */

__asm__ (".code16gcc\n");

#include "asmfunc.h"
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


/**
 * 一切の割り込みを受け付けないようにする
 * AT互換機の仕様では、
 * CLI前にこれをやっておかないと、たまにハングアップする
 */
void disable_interrupts(void)
{
    outb(0x21, 0xFF);  /* 全マスタの割り込みを禁止 */
    nop();             /* OUT命令を連続させるとうまくいかない機種があるらしい */
    outb(0xA1, 0xFF);  /* 全スレーブの割り込みを禁止 */
    cli();             /* CPUレベルでも割り込み禁止 */
}


static void wait_kbc_sendready(void);

/**
 * CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定する
 */
void enable_a20(void)
{
    wait_kbc_sendready();
    outb(0x64, 0xD1);
    wait_kbc_sendready();
    outb(0x60, 0xDF);  /* enable A20 */
    wait_kbc_sendready();
}

#define PORT_R_KBC_STATE            0x0064
#define KBC_STATE_SEND_NOT_READY      0x02

/**
 * キーボードコントローラがデータ送信可能になるのを待つ
 */
static void wait_kbc_sendready(void)
{
    for (;;) {
        if ((inb(PORT_R_KBC_STATE) & KBC_STATE_SEND_NOT_READY) == 0) {
            return;
        }
    }
}

static void memcopy(char *dst, char *src, int num_bytes);

#define BYTES_PER_CYL (512 * 18 * 2)

/**
 * ディスクデータをキャッシュへ転送する
 */
void move_disk_data(void)
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
void move_onsensys(void)
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
void run_onsensys(void)
{
    HRB_HEADER *hdr = (HRB_HEADER *) &OnSenMain;

    __asm__ __volatile__ (
        "movl %0, %%esp\n"
        "ljmpl $2*8, $0x28001B\n"  /* 2つめの実行可能なセグメントへジャンプ */
 
        :
        : "r"(hdr->dst_data)
    );
}

