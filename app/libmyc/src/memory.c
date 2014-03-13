/**
 * メモリ管理
 */


#include "myc.h"
#include "onsen.h"

#define BYTE_MEM_MNG_MAX  256
#define MEM_INFO_B        8                    // メモリの先頭に置く管理情報のサイズ
#define MM_SIG            (0xDEADBEEF)         // メモリの先頭に置くシグネチャ
#define BYTES_TO_8BYTES(b)   (((b) + 7) >> 3)  // バイトを8バイト単位に変換する

void *sbrk(int sz);
void *malloc(unsigned int size_B);
int free(void *p);


extern char end;
static unsigned long l_brk = (unsigned long) &end;


struct MEMORY {
    void *addr;
    unsigned int size;
};


struct MEM_MNG {
    // 記憶領域の確保用。
    // free.addrの昇順でなければならない
    struct MEMORY *free;

    int max_free;    // freeの最大数
    int num_free;    // 空き情報の数
    int total_free;  // free.sizeの合計
    int unit;        // sizeの単位
    int info_size;   // 管理情報のサイズ
};


struct INFO_8BYTES {
    unsigned long signature;
    unsigned long size;
};

static struct MEM_MNG *l_mng_b;


static int mem_set_free(struct MEM_MNG *mng, void *vp_addr, unsigned int size);

//=============================================================================
// 関数


void *sbrk(int sz)
{
    unsigned long old_brk = l_brk;

    if (old_brk & 0x7) {
        old_brk = (old_brk + 0x7) & ~0x7;  // 8バイトアラインにする
    }

    if (sz & 0x7) {
        sz = (sz + 0x7) & ~0x7;  // 8バイトアラインにする
    }

    int ret = brk((void *) (l_brk + sz));
    if (ret == -1)
        return (void *) -1;

    l_brk += sz;
    //printf("sbrk: old_brk = %p, new_brk = %p\n", old_brk, l_brk);

    return (void *) old_brk;
}


/**
 * メモリ管理の初期化
 */
static int mem_init(void)
{
    void *p = sbrk(0x4000);

    if (p == 0)
        return -1;

    l_mng_b = p;

    l_mng_b->free       = (struct MEMORY *) (l_mng_b + 1);
    l_mng_b->max_free   = BYTE_MEM_MNG_MAX;
    l_mng_b->num_free   = 0;
    l_mng_b->total_free = 0;
    l_mng_b->unit       = 8;
    l_mng_b->info_size  = BYTES_TO_8BYTES(MEM_INFO_B);

    void *free_p = (void *) ((unsigned long) p + 0x1000);
    mem_set_free(l_mng_b, free_p, BYTES_TO_8BYTES(0x3000));

    return 0;
}


static void *malloc_sub(struct MEM_MNG *mng, unsigned int size);

/**
 * @note 8バイト単位メモリ管理の場合は、割り当てられたメモリの前に割り当てサイズが置かれる。
 *
 * @note 4KB以上確保するならページ単位メモリ管理のほうで管理させる。
 */
void *malloc(unsigned int size_B)
{
    ASSERT_RET((void *) -1, size_B, "attempted to allocate 0 bytes memory");

    if (l_mng_b == 0) {
        int ret = mem_init();

        if (ret < 0) {
            return 0;
        }
    }

    int size = BYTES_TO_8BYTES(size_B);

    void *p = malloc_sub(l_mng_b, size);

    if (p == 0) {
        unsigned long old_brk = l_brk;

        unsigned long new_brk = l_brk + size_B + 0x1000;
        int new_area_size_B = new_brk - old_brk;
        sbrk(new_area_size_B);

        mem_set_free(l_mng_b, (void *) old_brk, BYTES_TO_8BYTES(new_area_size_B));

        p = malloc_sub(l_mng_b, size);
    }

    return p;
}


static void *malloc_sub(struct MEM_MNG *mng, unsigned int size)
{
    if (mng->total_free < size) {
        return 0;
    }

    for (int i = 0; i < mng->num_free; i++) {
        if (mng->free[i].size >= size + mng->info_size) {
            // 空きが見つかった

            struct MEMORY *mem = &mng->free[i];

            unsigned int addr = (unsigned int) mem->addr;

            if (mng->info_size > 0) {
                // 割り当てサイズの記録。
                // ここで管理していることを表す印も追加する。
                struct INFO_8BYTES *info = (struct INFO_8BYTES *) addr;
                info->signature = MM_SIG;
                info->size = size + mng->info_size;
                addr += mng->info_size * mng->unit;
            }

            // 空きアドレスをずらす
            mem->addr += ((size + mng->info_size) * mng->unit);
            mem->size -= (size + mng->info_size);
            mng->total_free -= size + mng->info_size;

            if (mem->size <= mng->info_size) {
                // 空いた隙間をずらす
                mng->num_free--;
                for ( ; i < mng->num_free; i++) {
                    mng->free[i] = mng->free[i + 1];
                }
            }

            return (void *) addr;
        }
    }

    return 0;
}



int free(void *vp_vaddr)
{
    ASSERT_RET(-1, vp_vaddr != 0, "attempted to free address 0");

    unsigned int vaddr = (unsigned int) vp_vaddr;

    ASSERT_RET(-1, (unsigned int) &end <= vaddr && vaddr <= l_brk, "%p", vp_vaddr);

    // メモリ管理が返すメモリは、下位3ビットが0であるはず
    ASSERT_RET(-1, (vaddr & 0x7) == 0, "(vaddr(%p) & 0x7) != 0", vp_vaddr);

    // 割り当てサイズの取得
    struct INFO_8BYTES *info = (struct INFO_8BYTES *) (vaddr - MEM_INFO_B);

    // 「ここで管理されてます」という印はあるか？
    ASSERT_RET(-1, info->signature == MM_SIG, "not found the memory management signature");

    return mem_set_free(l_mng_b, (void *) info, info->size);
}


static int mem_set_free(struct MEM_MNG *mng, void *vp_addr, unsigned int size)
{
    char *addr = (char *) vp_addr;

    ASSERT_RET(-1, size, "");

    // ---- 管理している空きメモリとまとめれるならまとめる

    int i;
    struct MEMORY *next = 0;

    // 挿入位置または結合位置を検索
    for (i = 0; i < mng->num_free; i++) {
        if (mng->free[i].addr > vp_addr) {
            next = &mng->free[i];
            break;
        }
    }

    // うしろに空きがなければ末尾に追加
    if (next == 0) {
        if (mng->num_free >= mng->max_free) {
            return -1;
        }

        mng->free[i].addr = vp_addr;
        mng->free[i].size = size;

        mng->num_free++;  // 空きを１つ追加
        mng->total_free += size;

        return 0;
    }

    char *end_addr = addr + (size * mng->unit);

    // 前があるか？
    if (i > 0) {
        struct MEMORY *prev = &mng->free[i - 1];
        char *prev_end_addr = (char *) prev->addr + (prev->size * mng->unit);

        // 前とまとめれるか？
        if (prev_end_addr == addr) {
            prev->size += size;
            mng->total_free += size;

            // 後ろとまとめれるか？
            if (next != 0 && end_addr == next->addr) {
                prev->size += next->size;

                mng->num_free--; // 空き１つ追加されて前後２つ削除された

                for (; i < mng->num_free; i++) {
                    mng->free[i] = mng->free[i + 1];
                }
            }

            return 0;
        }
    }

    // 前とはまとめれなかった

    // 後ろとまとめれるか？
    if (next != 0 && end_addr == next->addr) {
        next->addr = addr;
        next->size += size;
        mng->total_free += size;
        // 空きが１つ追加されて１つ削除されたのでmng->num_freeは変わらない
        return 0;
    }

    // 前にも後ろにもまとめれなかった

    if (mng->num_free < mng->max_free) {
        // free[i]より後ろを、後ろへずらして、すきまをつくる
        for (int j = mng->num_free; j > i; j--) {
            mng->free[j] = mng->free[j - 1];
        }

        next->addr = addr;
        next->size = size;

        mng->num_free++;  // 空きが１つ追加
        mng->total_free += size;
        return 0;
    }


    // 失敗

    return -1;
}

