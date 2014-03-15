/**
 * ELF ローダ
 *
 * ELF 形式のファイルを読み込んでタスクとして実行する
 */


#include <stdbool.h>
#include <stdint.h>
#include "memory.h"

typedef uint32_t  Elf32_Addr;
typedef uint16_t  Elf32_Half;
typedef uint32_t  Elf32_Off;
typedef int32_t   Elf32_Sword;
typedef uint32_t  Elf32_Word;
typedef uint32_t  Elf32_Size;

//-----------------------------------------------------------------------------

#define EI_NIDENT   16

// ELF ヘッダ
struct Elf_Ehdr {
    unsigned char e_ident[EI_NIDENT];  // マジックナンバなど
    Elf32_Half    e_type;              // ファイルタイプ
    Elf32_Half    e_machine;           // マシンアーキテクチャ
    Elf32_Word    e_version;           // ELF バージョン
    Elf32_Addr    e_entry;             // エントリポイント
    Elf32_Off     e_phoff;             // プログラムヘッダのオフセット
    Elf32_Off     e_shoff;             // セクションヘッダのオフセット
    Elf32_Word    e_flags;             // 未使用
    Elf32_Half    e_ehsize;            // ELF ヘッダのサイズ（バイト）
    Elf32_Half    e_phentsize;         // プログラムヘッダエントリのサイズ
    Elf32_Half    e_phnum;             // プログラムヘッダエントリの数
    Elf32_Half    e_shentsize;         // セクションヘッダエントリのサイズ
    Elf32_Half    e_shnum;             // セクションヘッダエントリの数
    Elf32_Half    e_shstrndx;          // セクション名格納用セクション
};


// ---- Elf_Ehdr.e_ident
inline __attribute__ ((always_inline))
bool is_elf(struct Elf_Ehdr *ehdr)
{
    return (ehdr->e_ident[0] == 0x7F && ehdr->e_ident[1] == 'E' &&
            ehdr->e_ident[2] == 'L'  && ehdr->e_ident[3] == 'F');
}


// ---- Elf_Ehdr.e_entry
#define ET_EXEC   2


//-----------------------------------------------------------------------------

// プログラムヘッダ
struct Elf_Phdr {
    Elf32_Word    p_type;
    Elf32_Off     p_offset;    // ファイル先頭からのセグメント位置
    Elf32_Addr    p_vaddr;     // ロード先の仮想アドレス
    Elf32_Addr    p_paddr;     // ロード先の物理アドレス
    Elf32_Word    p_filesz;    // ファイル中でのセグメントのサイズ
    Elf32_Word    p_memsz;     // メモリ上でのセグメントのサイズ
    Elf32_Word    p_flags;
    Elf32_Word    p_align;
};


// ---- Elf_Phdr.p_type
#define PT_LOAD    1


// ---- Elf_Phdr.p_flags
#define PF_R        0x4
#define PF_W        0x2
#define PF_X        0x1


//-----------------------------------------------------------------------------

// セクションヘッダ
struct Elf_Shdr {
    Elf32_Word    sh_name;        // セクション名の格納位置
    Elf32_Word    sh_type;
    Elf32_Word    sh_flags;
    Elf32_Addr    sh_addr;        // ロード先仮想アドレス
    Elf32_Off     sh_offset;      // ファイル先頭からのセクション位置
    Elf32_Word    sh_size;        // セクションのサイズ（バイト）
    Elf32_Word    sh_link;
    Elf32_Word    sh_info;
    Elf32_Word    sh_addralign;
    Elf32_Word    sh_entsize;
};


inline __attribute__ ((always_inline))
bool has_section(struct Elf_Phdr *phdr, struct Elf_Shdr *shdr)
{
    return (phdr->p_vaddr <= shdr->sh_addr &&
            shdr->sh_addr + shdr->sh_size <= phdr->p_vaddr + phdr->p_memsz);
}


typedef void (*MAIN_FUNC)(void);


static void forever_hlt(void);
static struct Elf_Shdr *search_shdr(struct Elf_Ehdr *ehdr, const char *name);


void run_elf(void *p)
{
    char *head = (char *) p;
    struct Elf_Ehdr *ehdr = (struct Elf_Ehdr *) head;

    // ---- 形式チェック

    if ( ! is_elf(ehdr)) {
        //print("This is not ELF file");
        forever_hlt();
    }

    if (ehdr->e_type != ET_EXEC) {
        //print("This is not executable file");
        forever_hlt();
    }

    // ---- ロード

    struct Elf_Phdr *phdr = (struct Elf_Phdr *) (head + ehdr->e_phoff);

    unsigned long code = 0, data = 0;

    for (int i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        switch (phdr->p_flags) {
        // ---- .text
        case PF_R | PF_X:
            if (code) {
                //print("file has two text sections");
                forever_hlt();
            }

            code = phdr->p_vaddr;
            memory_copy32((void *) phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);
            break;

        // ---- .data
        case PF_R | PF_W:
            if (data) {
                //print("file has two data sections");
                forever_hlt();
            }

            data = phdr->p_vaddr;
            memory_copy32((void *) phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);

            // ---- .bss 領域を0クリア

            struct Elf_Shdr *bss_shdr = search_shdr(ehdr, ".bss");

            if (bss_shdr) {
                memory_set32((void *) bss_shdr->sh_addr, 0, bss_shdr->sh_size);
            }

            break;
        }
    }

    if (code == 0) {
        //print("file doesn't have code section");
        forever_hlt();
    }

    if (data == 0) {
        //print("file doesn't have data section");
        forever_hlt();
    }


    __asm__ __volatile__ ("movl %0, %%esp" : : "r" (VADDR_OS_STACK));

    MAIN_FUNC main_func = (MAIN_FUNC) ehdr->e_entry;

    main_func();

    forever_hlt();
}


#define STRCMP(a, R, b)  (strcmp(a,b) R 0)
int  strcmp(const char *s, const char *t);

// 名前によるセクションヘッダの検索
static struct Elf_Shdr *search_shdr(struct Elf_Ehdr *ehdr, const char *name)
{
    char *head = (char *) ehdr;

    // 名前格納用セクション
    struct Elf_Shdr *name_shdr = (struct Elf_Shdr *) (head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
    char *name_sect = head + name_shdr->sh_offset;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        struct Elf_Shdr *shdr = (struct Elf_Shdr *) (head + ehdr->e_shoff + ehdr->e_shentsize * i);

        char *sect_name = name_sect + shdr->sh_name;

        if (STRCMP(sect_name, ==, name)) {
            return shdr;
        }
    }

    return 0;
}


// s<tなら<0, s==tなら0, s>tなら>0を返す
int strcmp(const char *s, const char *t)
{
    for ( ; *s == *t; s++, t++) {
        if (*s == '\0' || *t == '\0') {
            if (*s == '\0' && *t == '\0')
                return 0;
            else
                break;
        }
    }

    return *s - *t;
}


static void forever_hlt(void)
{
    for (;;) {
        __asm__ __volatile__ ("hlt");
    }
}

