/**
 * メモリ
 */

#include "asmfunc.h"

unsigned int memtest(unsigned int start, unsigned int end);

static unsigned int memtest_sub(unsigned int start, unsigned int end);
static char is486(void);
static void enable_cache(void);
static void disable_cache(void);

/**
 * @return 末尾の有効なアドレス
 */
unsigned int memtest(unsigned int start, unsigned int end)
{
    char flag486 = is486();

    if (flag486 != 0) {
        disable_cache();
    }

    unsigned int ret = memtest_sub(start, end);

    if (flag486 != 0) {
        enable_cache();
    }

    return ret;
}


static unsigned int memtest_sub(unsigned int start, unsigned int end)
{
    unsigned int i, old, pattern0 = 0xAA55AA55, pattern1 = 0x55AA55AA;
    volatile unsigned int *p;

    // 4KBごとにチェック
    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *) (i + 0xFFC);
        old = *p;
        *p = pattern0;

        *p ^= 0xFFFFFFFF;  // 反転
        if (*p != pattern1) {
            // メモリでない
            *p = old;
            break;
        }

        *p ^= 0xFFFFFFFF;  // 反転して元に戻す
        if (*p != pattern0) {
            // メモリでない
            *p = old;
            break;
        }

        *p = old;
    }

    return i;
}


#define EFLAGS_AC_BIT      (0x00040000)
#define CR0_CACHE_DISABLE  (0x60000000)

static char is486(void)
{
    /**
     * 386 では AC-bit を 1 にしても自動で 0 に戻る
     * ことを利用して486 であるかどうかを確認する
     */
    unsigned int eflg = load_eflags();
    eflg |= EFLAGS_AC_BIT;  // AC-bit = 1
    store_eflags(eflg);

    char flag486 = 0;
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        flag486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;  // AC-bit = 0
    store_eflags(eflg);

    return flag486;
}


static void enable_cache(void)
{
    unsigned int cr0 = load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE;  // キャッシュ許可
    store_cr0(cr0);
}


static void disable_cache(void)
{
    unsigned int cr0 = load_cr0();
    cr0 |= CR0_CACHE_DISABLE;  // キャッシュ禁止
    store_cr0(cr0);
}

