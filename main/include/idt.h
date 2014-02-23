// Generated by mkhdr

#ifndef HEADER_IDT
#define HEADER_IDT

/// はりぼて互換のAPIを呼び出す時の割り込み番号
#define IDT_HRB_API_NO  0x40

/// API(システムコール)を呼び出すときの割り込み番号。44は大分県の都道府県コード
#define IDT_API_NO  0x44


void idt_init(void);
void set_gate_desc(int no, unsigned short sel, void (*f)(void),
        int count, int type, int dpl);

#endif
