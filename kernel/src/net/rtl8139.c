/******************************************************************************
 *
 * RTL8139
 *
 *
 * ＜概要＞
 * ネットワークカード RTL8139 のドライバ
 *
 *****************************************************************************/


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_NET_RTL8139
#define HEADER_NET_RTL8139


    // 関数宣言 ---------------------------------------------------------------

void rtl8139_init ( void );
void rtl8139_handler ( int *esp );
void rtl8139_receive ( void );
void rtl8139_send ( void *p  ,  int len );
void rtl8139_dbg ( void );


#endif




//=============================================================================
// 非公開ヘッダ


    // システム ヘッダ --------------------------------------------------------

#include <stdbool.h>



    // ローカル ヘッダ --------------------------------------------------------

#include "asm_inthandler.h"
#include "asmfunc.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "intr.h"
#include "msg.h"
#include "msg_q.h"
#include "net/ether.h"
#include "net/ip.h"
#include "pci.h"
#include "str.h"
#include "task.h"



    // 定数 -------------------------------------------------------------------

#define MAX_RX_LEN 8192
#define MAX_RX_EXT_LEN 4096
#define MAX_TX_LEN 2048

#define VENDOR_ID  0x10EC   // Realtek
#define DEVICE_ID  0x8139   // RTL8139



    // 構造体 -----------------------------------------------------------------

struct PACKET_HEADER {

    // flags
    uint16_t ROK      :1;    // Receive OK
    uint16_t FAE      :1;    // Frame Alignment Error
    uint16_t CRC      :1;    // CRC Error
    uint16_t LONG     :1;    // Long Packet
    uint16_t RUNT     :1;    // Runt Packet Received
    uint16_t ISE      :1;    // Invalid Symbol Error
    uint16_t RESERVED :7;
    uint16_t BAR      :1;    // Broadcast Address Received
    uint16_t PAM      :1;    // Physical Address Matched
    uint16_t MAR      :1;    // Multicast Address Received

    uint16_t length;
};



    // 変数宣言 ---------------------------------------------------------------

static struct PCI_CONF *l_pci;
static int l_bar;

static uint8_t l_rx[MAX_RX_LEN + MAX_RX_EXT_LEN];  // 受信バッファ
static int l_rx_i;

// 実装を簡単にするため、とりあえず l_tx[0] しか使わん
static uint8_t l_tx[4][MAX_TX_LEN];
static bool l_tx0_empty  =  true;

static uint8_t l_mac[6];  // MACアドレス




//=============================================================================
// 公開関数

void
rtl8139_init ( void )
{
    auto void get_rtl8139_pci_conf ( void );
    auto void set_intr_handler ( void );
    auto void enable_pci_bus_mastering ( void );
    auto void wake_up ( void );
    auto void software_reset ( void );
    auto void init_rx_buf ( void );
    auto void init_tx_buf ( void );
    auto void set_TOK_ROK ( void );
    auto void conf_rx_buf ( void );
    auto void enable_rx_tx ( void );
    auto void get_mac ( void );


    get_rtl8139_pci_conf ( );
    set_intr_handler ( );
    enable_pci_bus_mastering ( );
    wake_up ( );
    software_reset ( );
    init_rx_buf  ( );
    init_tx_buf ( );
    set_TOK_ROK ( );
    conf_rx_buf ( );
    enable_rx_tx ( );
    get_mac ( );



    void get_rtl8139_pci_conf ( void )
    {
        l_pci  =  pci_get_conf ( VENDOR_ID  ,  DEVICE_ID );
        ASSERT ( l_pci != 0  ,  "" );

        l_bar  =  l_pci->io_addr;
    }



    void set_intr_handler ( void )
    {
        // 割り込みハンドラの登録
        set_gate_desc ( 0x20 + l_pci->irq_line  ,  KERNEL_CS  ,  asm_rtl8139_handler  ,  0  ,  SEG_TYPE_INTR_GATE  ,  DPL0 );

        // 割り込みを有効にする
        intr_enable ( l_pci->irq_line );
    }



    void enable_pci_bus_mastering ( void )
    {
        l_pci->cmd  |=  0x04;
        pci_write_cmd ( l_pci );
    }



    void wake_up ( void )
    {
        out8 ( l_bar + 0x52  ,  0x00 );
    }



    void software_reset ( void )
    {
        out8 ( l_bar + 0x37  ,  0x10 );
        while ( in8 ( l_bar + 0x37 )  &  0x10 )
            ;
    }



    void init_rx_buf ( void )
    {
        out32 ( l_bar + 0x30  ,  (uint32_t) _PA ( l_rx ) );
    }



    void init_tx_buf ( void )
    {
        for ( int i = 0;  i < 4;  i++ )
        {
            out32 ( l_bar + 0x20 + (i*4)  ,  (uint32_t) _PA ( l_tx[i] ) );
        }
    }



    void set_TOK_ROK ( void )
    {
        // TOK(Transmit OK) と ROK(Recieve OK) ビットをオン
        out16 ( l_bar + 0x3C  ,  0x0005 );
    }



    void conf_rx_buf ( void )
    {
        // (1 << 7) で WRAP ビットオン, 0xf で AB+AM+APM+AAP
        out32 ( l_bar + 0x44  ,  0xf | (1<<7) );
    }



    void enable_rx_tx ( void )
    {
        out8 ( l_bar + 0x37  ,  0x0C );
    }



    void get_mac ( void )
    {
        dbgf ( "MAC Address" );

        for ( int i = 0;  i < 6;  i++ )
        {
            static char sep  =  ' ';
            l_mac[i]  =  in8 ( l_bar + i );
            dbgf ( "%c%02X"  ,  sep  ,  l_mac[i] );
            sep  =  ':';
        }

        dbgf ( "\n" );
    }
}



// 割り込みハンドラ
void
rtl8139_handler ( int *esp )
{
    auto void clear_rx ( void );
    auto void clear_tx ( void );
    auto void notify_rtl8139_intr_end ( void );


    clear_rx ( );
    clear_tx ( );
    notify_rtl8139_intr_end ( );



    void clear_rx ( void )
    {
        uint16_t status  =  in16 ( l_bar + 0x3E );

        if ( status  ==  0x1 )
        {
            struct MSG msg  =  { .message  =  MSG_NET_RX };
            msg_q_put ( g_root_pid  ,  &msg );

            out16 ( l_bar + 0x3E  ,  status );
        }
    }



    void clear_tx ( void )
    {
        uint16_t status  =  in16 ( l_bar + 0x3E );

        if ( status  ==  0x4 )
        {
            struct MSG msg  =  { .message  =  MSG_NET_TX };
            msg_q_put ( g_root_pid  ,  &msg );

            l_tx0_empty  =  true;

            out16 ( l_bar + 0x3E  ,  status );
        }
    }



    void notify_rtl8139_intr_end ( void )
    {
        notify_intr_end ( /* IRQ = */ l_pci->irq_line );
    }
}



void
rtl8139_receive ( void )
{
    auto bool is_rx_buf_empty ( void );
    auto bool is_bad_rx_packet ( struct PACKET_HEADER *header );
    auto bool equal_mac ( uint8_t *mac1  ,  uint8_t *mac2 );
    auto void incr_rx_i ( int len );


    while ( is_rx_buf_empty ( ) )
    {
        unsigned char *p  =  &l_rx[l_rx_i];
        struct PACKET_HEADER *header  =  ( struct PACKET_HEADER * ) p;

        if ( is_bad_rx_packet ( header ) )
        {
            dbgf ( "bad packet\n" );
            return;
        }

        int len  =  header->length;

        // リングバッファの末尾と先頭に分かれて格納される場合は、
        // バッファの先頭領域を、バッファの最後に余分に確保している領域にコピーする。
        // これで、連続した領域としてアクセスできる
        if ( l_rx_i + len   >   MAX_RX_LEN )
        {
            int copy_len  =  l_rx_i + len - MAX_RX_LEN;
            memcpy ( l_rx + MAX_RX_LEN  ,  l_rx  ,  copy_len );
        }

        struct ETH_HEADER *eth  =  (struct ETH_HEADER *) ( p + 4 );

        if ( equal_mac ( eth->dst_mac  ,  l_mac ) )
        {
            eth_handler ( eth  ,  len );
        }

        incr_rx_i ( len );

        out16 ( l_bar + 0x38  ,  l_rx_i - 0x10 );
    }



    bool is_rx_buf_empty ( void )
    {
        uint8_t status  =  in8 ( l_bar + 0x37 );

        if ( status  &  0x01 )
            return false;
        else
            return true;
    }



    bool is_bad_rx_packet ( struct PACKET_HEADER *header )
    {
        if ( header->RUNT  ||  header->LONG  ||  header->CRC  ||  header->FAE )
            return true;

        if ( header->ROK )
        {
            if ( header->length  >  MAX_RX_LEN )
            {
                return true;
            }

            return false;
        }
        else
        {
            return true;
        }
    }



    bool equal_mac ( uint8_t *mac1  ,  uint8_t *mac2 )
    {
        for ( int i = 0;  i < 6;  i++ )
        {
            if ( mac1[i]  !=  mac2[i] )
                return false;
        }

        return true;
    }



    void incr_rx_i ( int len )
    {
        l_rx_i  +=  len + 4;            // 4はヘッダーの長さ
        l_rx_i   =  ( l_rx_i + 3 ) & ~3;  // 4バイトアライン
        l_rx_i  %=  MAX_RX_LEN;

        dbgf ( "next i = %d\n\n"  ,  l_rx_i );
    }
}



void
rtl8139_send ( void *p  ,  int len )
{
    ASSERT ( len < MAX_TX_LEN  ,  "len = %d"  ,  len );

    while ( ! l_tx0_empty )
        ;

    l_tx0_empty  =  false;

    memcpy ( l_tx[0]  ,  p  ,  len );
    out32 ( l_bar + 0x20  ,  (uint32_t) _PA ( l_tx[0] ) );
    out32 ( l_bar + 0x10  ,  len );
}



void
rtl8139_dbg ( void )
{
    /*
    struct IP_HEADER ip;

    ip.version  =  4;
    ip.ihl  =  5;
    ip.len  =  sizeof ( struct IP_HEADER );
    ip.ttl  =  15;

    ip.dst_ip[0]  =  10;
    ip.dst_ip[1]  =  0;
    ip.dst_ip[2]  =  0;
    ip.dst_ip[3]  =  2;

    ip.src_ip[0]  =  0;
    ip.src_ip[1]  =  0;
    ip.src_ip[2]  =  0;
    ip.src_ip[3]  =  0;
    */

    char p[32];

    for ( int i = 0;  i < 32;  i++ )
    {
        p[i]  =  i;
    }

    rtl8139_send ( p  ,  32 );
}




//=============================================================================
// 非公開関数




