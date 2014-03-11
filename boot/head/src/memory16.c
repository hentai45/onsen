// http://wiki.osdev.org/Detecting_Memory_(x86)

__asm__ (".code16gcc\n");
 
#include <stdint.h>
#include "memory.h"
#include "sysinfo.h"

#define E820_SIGNATURE  (0x534D4150)

#define MAX_ENTRIES  128

struct MEMORY_MAP_ENTRY {
    uint32_t base_low;     // base address QWORD
    uint32_t base_high;
    uint32_t length_low;   // length QWORD
    uint32_t length_high;
    uint16_t type;         // entry Ttpe
    uint16_t acpi;         // exteded
} __attribute__ ((packed));


static int set_memory_map_table(struct MEMORY_MAP_ENTRY *ent);
 
void detect_memory(void)
{
    struct MEMORY_MAP_ENTRY *ent = (struct MEMORY_MAP_ENTRY *) ADDR_MMAP_TBL;

    int num_entries = set_memory_map_table(ent);

    if (num_entries == -1) {
        // TODO: 何かメッセージを出そうよ
        __asm__ __volatile__ ("hlt");
    }

    // TODO
    // 本当は、アドレス順でソート・同じタイプで隣接しているアドレスの結合・
    // 重なりの除去もしたほうがいいけど、重なりはない前提でプログラムする。

    g_sys_info->mmap_entries = num_entries;
}


static int set_memory_map_table(struct MEMORY_MAP_ENTRY *ent)
{
    int i_ent = 0;
    int contID = 0;

    // INT0x15(EAX = 0xE820) は ES:DIの位置に情報を置く。
    // IPLでESを変更しているので0に戻しておく。
    __asm__ __volatile__ ("movw %0, %%es" : : "r" (0));

    while (i_ent < MAX_ENTRIES) {
        int signature, bytes;

        __asm__ __volatile__ (
            "int  $0x15"

            : "=a" (signature), "=b" (contID), "=c" (bytes)
            : "0" (0xE820), "1" (contID), "2" (24), "d" (E820_SIGNATURE), "D" (ent)
        );

        if (signature != E820_SIGNATURE)
            return -1;

        // 未知のタイプは「予約済み」タイプとする
        if (ent->type < 1 || 5 < ent->type)
            ent->type = 2;  // 予約済み

        if (bytes > 20 && (ent->acpi & 0x0001) == 0) {
            // ignore this entry
        } else {
            ent++;
            i_ent++;
        }

        if (contID == 0)
            break;
    }

    return i_ent;
}
