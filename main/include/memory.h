// Generated by mkhdr

#ifndef HEADER_MEMORY
#define HEADER_MEMORY

//-----------------------------------------------------------------------------
// メモリマップ


#define ADDR_BASE        (0xC0000000)  /* ページングにより論理アドレスが
                                          こいつベースになる */

#define ADDR_SYS_INFO    (0x00000FF0)  /* システム情報が格納されているアドレス */
#define ADDR_FREE1_START (0x00001000)
#define ADDR_FREE1_SIZE  (0x0009E000)
#define ADDR_DISK_IMG    (0x00100000)
#define ADDR_IDT         (0x0026F800)
#define LIMIT_IDT        (0x000007FF)
#define ADDR_GDT         (0x00270000)
#define LIMIT_GDT        (0x0000FFFF)
#define ADDR_OS          (0x00280000)
#define ADDR_OS_PDT      (0x00400000)
#define MADDR_PAGE_MEM_MNG  (ADDR_OS_PDT + 0x1000)
#define ADDR_FREE2_START (0x00402000)


//-----------------------------------------------------------------------------
// バイト単位メモリ管理

void  mem_init(void);
unsigned int mem_total_free(void);
void *mem_alloc(unsigned int size_B);
int   mem_free(void *vp_vaddr);

void mem_dbg(void);

//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_B(void);

#endif
