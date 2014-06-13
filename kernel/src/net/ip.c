/**
 * IP (Internet Protocol)
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_NET_IP
#define HEADER_NET_IP

#include <stdint.h>


struct IP_HEADER {

    uint8_t  version : 4;
    uint8_t  ihl     : 4;
    uint8_t  dscp    : 6;
    uint8_t  ecn     : 2;
    uint16_t len;  // サイズ（ヘッダとデータを含む）
    uint16_t id;
    uint16_t flags  : 3;
    uint16_t fragment_offset  : 13;
    uint8_t  ttl;
    uint8_t  protcol;
    uint16_t checksum;
    uint8_t  dst_ip [ 4 ];
    uint8_t  src_ip [ 4 ];
    uint16_t len_type;
};


#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"



//=============================================================================
// 公開関数


void
ip_handler ( struct IP_HEADER * ip , int len )
{
}


//=============================================================================
// 非公開関数


