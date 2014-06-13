/**
 * イーサネット
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_NET_ETHER
#define HEADER_NET_ETHER

#include <stdint.h>


#define ETH_TYPE_IP   0x0080
#define ETH_TYPE_IPv6 0xDD86


struct ETH_HEADER {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t len_type;
};


void eth_handler(struct ETH_HEADER *eth, int len);


#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"

static char *get_mac_str(uint8_t *mac);


//=============================================================================
// 公開関数


void eth_handler(struct ETH_HEADER *eth, int len)
{
    dbgf("len = %d\n", len);

    dbgf("mac: %s  =>  ", get_mac_str(eth->dst_mac));
    dbgf("%s\n", get_mac_str(eth->src_mac));

    dbgf("type = %X: ", eth->len_type);
    if (eth->len_type == ETH_TYPE_IP)
    {
        dbgf("IP\n");
    }
    else if (eth->len_type == ETH_TYPE_IPv6)
    {
        dbgf("IPv6\n");
    }
    else
    {
        dbgf("Unknown\n");
    }
}



//=============================================================================
// 非公開関数


static char *get_mac_str(uint8_t *mac)
{
    static char s[32];
    snprintf(s, 32, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return s;
}
