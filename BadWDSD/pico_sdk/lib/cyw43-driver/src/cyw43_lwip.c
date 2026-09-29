/*
 * This file is part of the cyw43-driver
 *
 * Copyright (C) 2019-2022 George Robotics Pty Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Any redistribution, use, or modification in source or binary form is done
 *    solely for personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE LICENSOR AND COPYRIGHT OWNER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE LICENSOR OR COPYRIGHT OWNER BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This software is also available for use with certain devices under different
 * terms, as set out in the top level LICENSE file.  For commercial licensing
 * options please email contact@georgerobotics.com.au.
 */

#include <string.h>

#include "cyw43.h"
#include "cyw43_stats.h"
#if CYW43_LWIP
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/dns.h"
#include "lwip/igmp.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#endif

#if CYW43_NETUTILS
#include "shared/netutils/netutils.h"
#endif

#if CYW43_LWIP

#if CYW43_NETUTILS
static void cyw43_ethernet_trace(cyw43_t *self, struct netif *netif, size_t len, const void *data, unsigned int flags) {
    bool is_tx = flags & NETUTILS_TRACE_IS_TX;
    if ((is_tx && (self->trace_flags & CYW43_TRACE_ETH_TX))
        || (!is_tx && (self->trace_flags & CYW43_TRACE_ETH_RX))) {
        const uint8_t *buf;
        if (len == (size_t)-1) {
            // data is a pbuf
            const struct pbuf *pbuf = data;
            buf = pbuf->payload;
            len = pbuf->len; // restricted to print only the first chunk of the pbuf
        } else {
            // data is actual data buffer
            buf = data;
        }

        if (self->trace_flags & CYW43_TRACE_MAC) {
            CYW43_PRINTF("[% 8d] ETH%cX itf=%c%c len=%u", (int)cyw43_hal_ticks_ms(), is_tx ? 'T' : 'R', netif->name[0], netif->name[1], len);
            CYW43_PRINTF(" MAC type=%d subtype=%d data=", buf[0] >> 2 & 3, buf[0] >> 4);
            for (size_t i = 0; i < len; ++i) {
                CYW43_PRINTF(" %02x", buf[i]);
            }
            CYW43_PRINTF("\n");
            return;
        }

        if (self->trace_flags & CYW43_TRACE_ETH_FULL) {
            flags |= NETUTILS_TRACE_PAYLOAD;
        }
        netutils_ethernet_trace(MP_PYTHON_PRINTER, len, buf, flags);
    }
}
#endif

static err_t cyw43_netif_output(struct netif *netif, struct pbuf *p) {
    cyw43_t *self = netif->state;
    #if CYW43_NETUTILS
    if (self->trace_flags != 0) {
        cyw43_ethernet_trace(self, netif, (size_t)-1, p, NETUTILS_TRACE_IS_TX | NETUTILS_TRACE_NEWLINE);
    }
    #endif
    int itf = netif->name[1] - '0';
    int ret = cyw43_send_ethernet(self, itf, p->tot_len, (void *)p, true);
    if (ret) {
        CYW43_WARN("send_ethernet failed: %d\n", ret);
        return ERR_IF;
    }
    CYW43_STAT_INC(PACKET_OUT_COUNT);
    return ERR_OK;
}

#if LWIP_IGMP
static err_t cyw43_netif_update_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, enum netif_mac_filter_action action) {
    cyw43_t *self = netif->state;
    uint8_t mac[] = { 0x01, 0x00, 0x5e, ip4_addr2(group) & 0x7F, ip4_addr3(group), ip4_addr4(group) };

    if (action != IGMP_ADD_MAC_FILTER && action != IGMP_DEL_MAC_FILTER) {
        return ERR_VAL;
    }

    if (cyw43_wifi_update_multicast_filter(self, mac, action == IGMP_ADD_MAC_FILTER)) {
        return ERR_IF;
    }

    return ERR_OK;
}
#endif

#if LWIP_IPV6
static err_t cyw43_macfilter(struct netif *netif, const ip6_addr_t *group, enum netif_mac_filter_action action) {
    uint8_t address[6] = { 0x33, 0x33 };
    memcpy(address + 2, group->addr + 3, 4);
    if (action != NETIF_ADD_MAC_FILTER && action != NETIF_DEL_MAC_FILTER) {
        return ERR_VAL;
    }
    if (cyw43_wifi_update_multicast_filter(netif->state, address, action == NETIF_ADD_MAC_FILTER)) {
        return ERR_IF;
    }
    return ERR_OK;
}
#endif

static err_t cyw43_netif_init(struct netif *netif) {
    netif->linkoutput = cyw43_netif_output;
    #if LWIP_IPV4
    netif->output = etharp_output;
    #endif
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;
    #if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
    netif->mld_mac_filter = cyw43_macfilter;
    netif->flags |= NETIF_FLAG_MLD6;
    #endif
    cyw43_wifi_get_mac(netif->state, netif->name[1] - '0', netif->hwaddr);
    netif->hwaddr_len = sizeof(netif->hwaddr);
    #if LWIP_IGMP
    netif_set_igmp_mac_filter(netif, cyw43_netif_update_igmp_mac_filter);
    #endif
    return ERR_OK;
}

//#ifndef NDEBUG
//static void netif_status_callback(struct netif *netif) {
//    const ip_addr_t *ip = netif_ip_addr4(netif);
//    if (ip) {
//        CYW43_INFO("Got ip %s\n", ip4addr_ntoa(ip));
//    }
//}
//#endif

#ifndef CYW43_HOST_NAME
#define CYW43_HOST_NAME "PYBD"
#endif

void cyw43_cb_tcpip_init(cyw43_t *self, int itf) {
    ip_addr_t ipconfig[4];
    #if LWIP_IPV6
    #define IP(x) ((x).u_addr.ip4)
    #else
    #define IP(x) (x)
    #endif
    #if LWIP_IPV4
    if (itf == 0) {
        #if LWIP_DHCP
        // need to zero out to get isconnected() working
        IP4_ADDR(&IP(ipconfig[0]), 0, 0, 0, 0);
        #else
        // using static IP address
        IP(ipconfig[0]).addr = PP_HTONL(CYW43_DEFAULT_IP_STA_ADDRESS);
        #endif
        IP(ipconfig[2]).addr = PP_HTONL(CYW43_DEFAULT_IP_STA_GATEWAY);
    } else {
        IP(ipconfig[0]).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
        IP(ipconfig[2]).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_GATEWAY);
    }
    IP(ipconfig[1]).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
    IP(ipconfig[3]).addr = PP_HTONL(CYW43_DEFAULT_IP_DNS);
    #endif
    #undef IP

    struct netif *n = &self->netif[itf];
    n->name[0] = 'w';
    n->name[1] = '0' + itf;
    #if NO_SYS
    netif_input_fn input_func = ethernet_input;
    #else
    netif_input_fn input_func = tcpip_input;
    #endif
    #if LWIP_IPV4
    netif_add(n, ip_2_ip4(&ipconfig[0]), ip_2_ip4(&ipconfig[1]), ip_2_ip4(&ipconfig[2]), self, cyw43_netif_init, input_func);
    #elif LWIP_IPV6
    netif_add(n, self, cyw43_netif_init, input_func);
    #else
    #error Unsupported
    #endif
    #if LWIP_NETIF_HOSTNAME
    netif_set_hostname(n, CYW43_HOST_NAME);
    #endif
    netif_set_default(n);
    netif_set_up(n);

//    #ifndef NDEBUG
//    netif_set_status_callback(n, netif_status_callback);
//    #endif

    if (itf == CYW43_ITF_STA) {
        #if LWIP_IPV4
        #if LWIP_DNS
        dns_setserver(0, &ipconfig[3]);
        #endif
        #if LWIP_DHCP
        dhcp_set_struct(n, &self->dhcp_client);
        dhcp_start(n);
        #endif
        #endif
        #if LWIP_IPV6
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        n->mld_mac_filter(n, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
        netif_create_ip6_linklocal_address(n, 1);
        netif_set_ip6_autoconfig_enabled(n, 1);
        #endif
    } else {
        #if CYW43_NETUTILS
        dhcp_server_init(&self->dhcp_server, &ipconfig[0], &ipconfig[1]);
        #endif
    }

    // Hook for any additional initialization than needs to be done.
    CYW43_CB_TCPIP_INIT_EXTRA(self, itf);
}

void cyw43_cb_tcpip_deinit(cyw43_t *self, int itf) {
    // Hook for any additional deinitialization than needs to be done.
    CYW43_CB_TCPIP_DEINIT_EXTRA(self, itf);

    struct netif *n = &self->netif[itf];
    if (itf == CYW43_ITF_STA) {
        #if LWIP_IPV4 && LWIP_DHCP
        dhcp_stop(n);
        #endif
    } else {
        #if CYW43_NETUTILS
        dhcp_server_deinit(&self->dhcp_server);
        #endif
    }
    struct netif *netif;
    NETIF_FOREACH(netif) {
        if (netif == n) {
            netif_remove(netif);
            #if LWIP_IPV4
            ip_2_ip4(&netif->ip_addr)->addr = 0;
            #endif
            netif->flags = 0;
        }
    }
}

void cyw43_cb_process_ethernet(void *cb_data, int itf, size_t len, const uint8_t *buf) {
    cyw43_t *self = cb_data;
    struct netif *netif = &self->netif[itf];
    #if CYW43_NETUTILS
    if (self->trace_flags) {
        cyw43_ethernet_trace(self, netif, len, buf, NETUTILS_TRACE_NEWLINE);
    }
    #endif
    if (netif->flags & NETIF_FLAG_LINK_UP) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p != NULL) {
            pbuf_take(p, buf, len);
            if (netif->input(p, netif) != ERR_OK) {
                pbuf_free(p);
            }
            CYW43_STAT_INC(PACKET_IN_COUNT);
        }
    }
}

void cyw43_cb_tcpip_set_link_up(cyw43_t *self, int itf) {
    netif_set_link_up(&self->netif[itf]);
}

void cyw43_cb_tcpip_set_link_down(cyw43_t *self, int itf) {
    netif_set_link_down(&self->netif[itf]);
}

int cyw43_tcpip_link_status(cyw43_t *self, int itf) {
    struct netif *netif = &self->netif[itf];
    if ((netif->flags & (NETIF_FLAG_UP | NETIF_FLAG_LINK_UP)) == (NETIF_FLAG_UP | NETIF_FLAG_LINK_UP)) {
        bool have_address = false;
        #if LWIP_IPV4
        have_address = (ip_2_ip4(&netif->ip_addr)->addr != 0);
        #else
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            int state = netif_ip6_addr_state(netif, i);
            const ip6_addr_t *addr = netif_ip6_addr(netif, i);
            if (ip6_addr_ispreferred(state) && ip6_addr_isglobal(addr)) {
                have_address = true;
                break;
            }
        }
        #endif
        if (have_address) {
            return CYW43_LINK_UP;
        } else {
            return CYW43_LINK_NOIP;
        }
    } else {
        return cyw43_wifi_link_status(self, itf);
    }
}

#endif //  CYW43_LWIP
