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

typedef unsigned int    Elf32_Addr;
typedef unsigned short  Elf32_Half;
typedef unsigned int    Elf32_Off;
typedef int             Elf32_Sword;
typedef unsigned int    Elf32_Word;

//-----------------------------------------------------------------------------

#define EI_NIDENT   16

// ELF ヘッダ
typedef struct Elf_Ehdr {
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
} Elf_Ehdr;


// ---- Elf_Ehdr.e_ident
inline __attribute__ ((always_inline))
bool is_elf(Elf_Ehdr *ehdr)
{
    return (ehdr->e_ident[0] == 0x7F && ehdr->e_ident[1] == 'E' &&
            ehdr->e_ident[2] == 'L'  && ehdr->e_ident[3] == 'F');
}


// ---- Elf_Ehdr.e_entry
#define ET_EXEC   2


//-----------------------------------------------------------------------------

/// プログラムヘッダ
typedef struct Elf_Phdr {
    Elf32_Word    p_type;
    Elf32_Off     p_offset;    ///< ファイル先頭からのセグメント位置
    Elf32_Addr    p_vaddr;     ///< ロード先の仮想アドレス
    Elf32_Addr    p_paddr;     ///< ロード先の物理アドレス
    Elf32_Word    p_filesz;    ///< ファイル中でのセグメントのサイズ
    Elf32_Word    p_memsz;     ///< メモリ上でのセグメントのサイズ
    Elf32_Word    p_flags;
    Elf32_Word    p_align;
} Elf_Phdr;


// ---- Elf_Phdr.p_type
#define PT_LOAD    1


// ---- Elf_Phdr.p_flags
#define PF_R        0x4
#define PF_W        0x2
#define PF_X        0x1


//-----------------------------------------------------------------------------

/// セクションヘッダ
typedef struct Elf_Shdr {
    Elf32_Word    sh_name;        ///< セクション名の格納位置
    Elf32_Word    sh_type;
    Elf32_Word    sh_flags;
    Elf32_Addr    sh_addr;        ///< ロード先仮想アドレス
    Elf32_Off     sh_offset;      ///< ファイル先頭からのセクション位置
    Elf32_Word    sh_size;        ///< セクションのサイズ（バイト）
    Elf32_Word    sh_link;
    Elf32_Word    sh_info;
    Elf32_Word    sh_addralign;
    Elf32_Word    sh_entsize;
} Elf_Shdr;


inline __attribute__ ((always_inline))
bool has_section(Elf_Phdr *phdr, Elf_Shdr *shdr)
{
    return (phdr->p_vaddr <= shdr->sh_addr &&
            shdr->sh_addr + shdr->sh_size <= phdr->p_vaddr + phdr->p_memsz);
}

typedef void (*ENTRY_FUNC)(void);

ENTRY_FUNC elf_load(void *p);

#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"
#include "str.h"

static Elf_Shdr *search_shdr(Elf_Ehdr *ehdr, const char *name);


//=============================================================================
// 公開関数

ENTRY_FUNC elf_load(void *p)
{
    char *head = (char *) p;
    Elf_Ehdr *ehdr = (Elf_Ehdr *) head;

    // ---- 形式チェック

    if (! is_elf(ehdr)) {
        DBGF("This is not ELF file.");
        return 0;
    }

    if (ehdr->e_type != ET_EXEC) {
        DBGF("This is not executable file.");
        return 0;
    }

    // ---- ロード

    Elf_Phdr *phdr = (Elf_Phdr *) (head + ehdr->e_phoff);

    Elf_Shdr *bss_shdr = search_shdr(ehdr, ".bss");

    for (int i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        switch (phdr->p_flags) {
        case PF_R | PF_X:
            DBGF(".text");
            break;

        case PF_R | PF_W:
            DBGF(".data");
            break;
        }

        if (bss_shdr && has_section(phdr, bss_shdr)) {
            // BSS 領域を0クリア
            //memset((void *) bss_shdr->sh_addr, 0, bss_shdr->sh_size);
        }
    }

    // ---- スタックの準備


    return (ENTRY_FUNC) ehdr->e_entry;
}


//=============================================================================
// 非公開関数


static Elf_Shdr *search_shdr(Elf_Ehdr *ehdr, const char *name)
{
    char *head = (char *) ehdr;
    Elf_Shdr *shdr = (Elf_Shdr *) (head + ehdr->e_shoff);

    // 名前格納用セクション
    Elf_Shdr *name_shdr = (Elf_Shdr *) (shdr + ehdr->e_shstrndx);
    char *name_sect = head + name_shdr->sh_offset;

    for (int i = 0; i < ehdr->e_shnum; i++, shdr++) {
        if (s_cmp(name_sect + shdr->sh_name, name) == 0) {
            return shdr;
        }
    }

    return 0;
}



