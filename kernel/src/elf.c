/**
 * ELF ローダ
 *
 * ELF 形式のファイルを読み込んでタスクとして実行する
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ELF
#define HEADER_ELF

#include <stdbool.h>
#include <stdint.h>
#include "api.h"

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


#define SHT_SYMTAB  2

inline __attribute__ ((always_inline))
bool has_section(struct Elf_Phdr *phdr, struct Elf_Shdr *shdr)
{
    return (phdr->p_vaddr <= shdr->sh_addr &&
            shdr->sh_addr + shdr->sh_size <= phdr->p_vaddr + phdr->p_memsz);
}


//-----------------------------------------------------------------------------

// symbol
struct Elf_Sym {
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
};


//-----------------------------------------------------------------------------


int elf_load(void *p, unsigned int size, const char *name);
int elf_load2(struct API_REGISTERS *regs, void *p, unsigned int size);


#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "gdt.h"
#include "memory.h"
#include "paging.h"
#include "str.h"
#include "task.h"


static struct Elf_Sym *search_symbol(struct Elf_Ehdr *ehdr, const char *name);
static struct Elf_Shdr *search_shdr(struct Elf_Ehdr *ehdr, const char *name);
static struct Elf_Shdr *search_shdr_t(struct Elf_Ehdr *ehdr, int type);


//=============================================================================
// 公開関数

int elf_load(void *p, unsigned int size, const char *name)
{
    char *head = (char *) p;
    struct Elf_Ehdr *ehdr = (struct Elf_Ehdr *) head;

    // ---- 形式チェック

    /*
    if ( ! is_elf(ehdr)) {
        dbgf("This is not ELF file.\n");
        return -1;
    }
    */

    if (ehdr->e_type != ET_EXEC) {
        dbgf("This is not executable file.\n");
        return -1;
    }

    // ---- ロード

    cli();

    struct Elf_Phdr *phdr = (struct Elf_Phdr *) (head + ehdr->e_phoff);

    struct USER_PAGE *code = 0, *data = 0;

    for (int i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        switch (phdr->p_flags) {
        // ---- .text
        case PF_R | PF_X:
            if (code) {
                dbgf("file has two text sections.\n");
                return -1;
            }

            dbgf(".text: offset=%#X, vaddr=%#X, size=%Z, msize=%Z\n",
                    phdr->p_offset, phdr->p_vaddr,
                    phdr->p_filesz, phdr->p_memsz);

            code = mem_alloc_user_page(phdr->p_vaddr, phdr->p_memsz, 0);
            memcpy((void *) phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);
            break;

        // ---- .data
        case PF_R | PF_W:
            if (data) {
                dbgf("file has two data sections.\n");
                return -1;
            }

            dbgf(".data: offset=%#X, vaddr=%#X, size=%Z, msize=%Z\n",
                    phdr->p_offset, phdr->p_vaddr,
                    phdr->p_filesz, phdr->p_memsz);

            data = mem_alloc_user_page(phdr->p_vaddr, phdr->p_memsz, PTE_RW);
            memcpy((void *) phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);

            // ---- .bss 領域を0クリア

            struct Elf_Shdr *bss_shdr = search_shdr(ehdr, ".bss");

            if (bss_shdr) {
                dbgf(".bss: addr=%#X, size=%Z\n", bss_shdr->sh_addr, bss_shdr->sh_size);
                memset((void *) bss_shdr->sh_addr, 0, bss_shdr->sh_size);
            }

            break;
        }
    }

    if (code == 0) {
        dbgf("file doesn't have code section\n");

        if (data) {
            mem_free_user(data);
        }

        return -1;
    }

    // スタックの準備

    struct USER_PAGE *stack = mem_alloc_user_page(VADDR_USER_ESP - 0x8000, 0x8000, PTE_RW);
    unsigned long esp = stack->vaddr + 0x8000;

    unsigned long stack0 = (unsigned long) mem_alloc(8 * 1024);
    unsigned long esp0 = stack0 + (8 * 1024);
 

    char app_name[9];
    memcpy(app_name, name, 8);
    app_name[8] = 0;

    int pid = task_new(app_name);

    PDE *pd = create_user_pd();
    set_app_tss(pid, (PDE) pd, (void (*)(void)) ehdr->e_entry, esp, esp0);

    sti();

    struct TSS *t = pid2tss(pid);
    t->code   = code;
    t->data   = data;
    t->stack  = stack;
    t->stack0 = stack0;

    struct Elf_Sym *end_sym = search_symbol(ehdr, "_end");
    t->brk = (end_sym) ? end_sym->st_value : 0;
    dbgf("brk = %p\n", t->brk);

    task_run(pid);

    return pid;
}


int elf_load2(struct API_REGISTERS *regs, void *p, unsigned int size)
{
    char *head = (char *) p;
    struct Elf_Ehdr *ehdr = (struct Elf_Ehdr *) head;

    // ---- 形式チェック

    if ( ! is_elf(ehdr)) {
        dbgf("This is not ELF file.\n");
        return -1;
    }

    if (ehdr->e_type != ET_EXEC) {
        dbgf("This is not executable file.\n");
        return -1;
    }

    // ---- ロード

    struct Elf_Phdr *phdr = (struct Elf_Phdr *) (head + ehdr->e_phoff);

    struct USER_PAGE *code = 0, *data = 0;

    for (int i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        switch (phdr->p_flags) {
        // ---- .text
        case PF_R | PF_X:
            if (code) {
                dbgf("file has two text sections.\n");
                return -1;
            }

            dbgf(".text: offset=%#X, vaddr=%#X, size=%Z, msize=%Z\n",
                    phdr->p_offset, phdr->p_vaddr,
                    phdr->p_filesz, phdr->p_memsz);

            code = mem_alloc_user_page(phdr->p_vaddr, phdr->p_memsz, 0);
            memcpy((void *) phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);
            break;

        // ---- .data
        case PF_R | PF_W:
            if (data) {
                dbgf("file has two data sections.\n");
                return -1;
            }

            dbgf(".data: offset=%#X, vaddr=%#X, size=%Z, msize=%Z\n",
                    phdr->p_offset, phdr->p_vaddr,
                    phdr->p_filesz, phdr->p_memsz);

            data = mem_alloc_user_page(phdr->p_vaddr, phdr->p_memsz, PTE_RW);
            memcpy((void *) phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);

            // ---- .bss 領域を0クリア

            struct Elf_Shdr *bss_shdr = search_shdr(ehdr, ".bss");

            if (bss_shdr) {
                dbgf(".bss: addr=%#X, size=%Z\n", bss_shdr->sh_addr, bss_shdr->sh_size);
                memset((void *) bss_shdr->sh_addr, 0, bss_shdr->sh_size);
            }

            break;
        }
    }

    if (code == 0) {
        dbgf("file doesn't have code section\n");

        if (data) {
            mem_free_user(data);
        }

        return -1;
    }

    struct Elf_Sym *end_sym = search_symbol(ehdr, "_end");
    g_cur->brk = (end_sym) ? end_sym->st_value : 0;

    g_cur->code = code;
    g_cur->data = data;

    regs->eip = ehdr->e_entry;
    regs->esp = VADDR_USER_ESP;

    return 0;
}


//=============================================================================
// 非公開関数


// シンボル名に対応するシンボルの検索
static struct Elf_Sym *search_symbol(struct Elf_Ehdr *ehdr, const char *name)
{
    char *head = (char *) ehdr;

    // 名前格納用セクション
    struct Elf_Shdr *str_shdr = search_shdr(ehdr, ".strtab");
    if (str_shdr == 0) return 0;
    char *str_sect = head + str_shdr->sh_offset;

    // シンボルテーブルセクション
    struct Elf_Shdr *sym_shdr = search_shdr_t(ehdr, SHT_SYMTAB);
    if (sym_shdr == 0) return 0;

    int num_entries = sym_shdr->sh_size / sym_shdr->sh_entsize;
    for (int k = 0; k < num_entries; k++) {
        struct Elf_Sym *symp = (struct Elf_Sym *) (head + sym_shdr->sh_offset + sym_shdr->sh_entsize * k);

        if (symp->st_name == 0)
            continue;

        char *sym_name = str_sect + symp->st_name;

        if (STRCMP(sym_name, ==, name))
            return symp;
    }

    return 0;
}


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


// タイプによるセクションヘッダの検索
static struct Elf_Shdr *search_shdr_t(struct Elf_Ehdr *ehdr, int type)
{
    char *head = (char *) ehdr;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        struct Elf_Shdr *shdr = (struct Elf_Shdr *) (head + ehdr->e_shoff + ehdr->e_shentsize * i);

        if (shdr->sh_type == type)
            return shdr;

    }

    return 0;
}
