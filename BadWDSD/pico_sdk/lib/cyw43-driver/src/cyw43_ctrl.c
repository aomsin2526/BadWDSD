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

#include <assert.h>
#include <string.h>

#include "cyw43.h"
#include "cyw43_stats.h"

#if !CYW43_USE_SPI
#include "cyw43_sdio.h"
#endif

#if CYW43_ENABLE_BLUETOOTH
#include "cyw43_btbus.h"
#endif

#if !CYW43_USE_SPI
#ifdef CYW43_PIN_WL_HOST_WAKE
#define USE_SDIOIT (0)
#else
#define USE_SDIOIT (1)
#endif
#endif

// Bits 0-3 are an enumeration, while bits 8-11 are flags.
#define WIFI_JOIN_STATE_KIND_MASK (0x000f)
#define WIFI_JOIN_STATE_ACTIVE  (0x0001)
#define WIFI_JOIN_STATE_FAIL    (0x0002)
#define WIFI_JOIN_STATE_NONET   (0x0003)
#define WIFI_JOIN_STATE_BADAUTH (0x0004)
#define WIFI_JOIN_STATE_AUTH    (0x0200)
#define WIFI_JOIN_STATE_LINK    (0x0400)
#define WIFI_JOIN_STATE_KEYED   (0x0800)
#define WIFI_JOIN_STATE_ALL     (0x0e01)

#define CYW43_STA_IS_ACTIVE(self) (((self)->itf_state >> CYW43_ITF_STA) & 1)
#define CYW43_AP_IS_ACTIVE(self) (((self)->itf_state >> CYW43_ITF_AP) & 1)

cyw43_t cyw43_state;
void (*cyw43_poll)(void);
uint32_t cyw43_sleep;

#ifndef CYW43_POST_POLL_HOOK
#define CYW43_POST_POLL_HOOK
#endif

static void cyw43_poll_func(void);
static void cyw43_wifi_ap_init(cyw43_t *self);
static void cyw43_wifi_ap_set_up(cyw43_t *self, bool up);

/*******************************************************************************/
// Initialisation and polling

void cyw43_init(cyw43_t *self) {
    #if defined(CYW43_PIN_WL_HOST_WAKE)
    cyw43_hal_pin_config(CYW43_PIN_WL_HOST_WAKE, CYW43_HAL_PIN_MODE_INPUT, CYW43_HAL_PIN_PULL_NONE, 0);
    #elif defined(CYW43_PIN_WL_IRQ)
    cyw43_hal_pin_config(CYW43_PIN_WL_IRQ, CYW43_HAL_PIN_MODE_INPUT, CYW43_HAL_PIN_PULL_NONE, 0);
    #endif
    cyw43_hal_pin_config(CYW43_PIN_WL_REG_ON, CYW43_HAL_PIN_MODE_OUTPUT, CYW43_HAL_PIN_PULL_NONE, 0);
    cyw43_hal_pin_low(CYW43_PIN_WL_REG_ON);
    #ifdef CYW43_PIN_RFSW_VDD
    cyw43_hal_pin_config(CYW43_PIN_RFSW_VDD, CYW43_HAL_PIN_MODE_OUTPUT, CYW43_HAL_PIN_PULL_NONE, 0); // RF-switch power
    cyw43_hal_pin_low(CYW43_PIN_RFSW_VDD);
    #endif

    cyw43_ll_init(&self->cyw43_ll, self);

    self->itf_state = 0;
    self->wifi_scan_state = 0;
    self->wifi_join_state = 0;
    self->pend_disassoc = false;
    self->pend_rejoin = false;
    self->pend_rejoin_wpa = false;
    self->ap_channel = 3;
    self->ap_ssid_len = 0;
    self->ap_key_len = 0;

    cyw43_poll = NULL;
    self->initted = true;

    #if CYW43_ENABLE_BLUETOOTH
    self->bt_loaded = false;
    #endif
}

void cyw43_deinit(cyw43_t *self) {
    CYW43_THREAD_ENTER;
    if (cyw43_poll == NULL) {
        CYW43_THREAD_EXIT;
        return;
    }

    // Stop the TCP/IP network interfaces.
    cyw43_cb_tcpip_deinit(self, 0);
    cyw43_cb_tcpip_deinit(self, 1);

    // Turn off the SDIO bus.
    #if USE_SDIOIT
    cyw43_sdio_set_irq(false);
    #endif
    #if !CYW43_USE_SPI
    cyw43_sdio_deinit();
    #endif

    // Power off the WLAN chip and make sure all state is reset.
    cyw43_ll_deinit(&self->cyw43_ll);
    cyw43_init(self);

    #if CYW43_ENABLE_BLUETOOTH
    self->bt_loaded = false;
    #endif

    CYW43_THREAD_EXIT;
}

static int cyw43_ensure_up(cyw43_t *self) {
    CYW43_THREAD_LOCK_CHECK;

    #ifndef NDEBUG
    assert(cyw43_is_initialized(self)); // cyw43_init has not been called
    #endif
    if (cyw43_poll != NULL) {
        cyw43_ll_bus_sleep(&self->cyw43_ll, false);
        return 0;
    }

    // Disable the netif if it was previously up
    cyw43_cb_tcpip_deinit(self, CYW43_ITF_STA);
    cyw43_cb_tcpip_deinit(self, CYW43_ITF_AP);
    self->itf_state = 0;

    // Reset and power up the WL chip
    cyw43_hal_pin_low(CYW43_PIN_WL_REG_ON);
    cyw43_delay_ms(20);
    cyw43_hal_pin_high(CYW43_PIN_WL_REG_ON);
    cyw43_delay_ms(50);

    #if !CYW43_USE_SPI
    // Initialise SDIO bus
    // IRQ priority only needs to be higher than CYW43_THREAD_ENTER/EXIT protection (PENDSV)
    cyw43_sdio_init();
    #endif

    // Initialise the low-level driver
    #if !CYW43_USE_OTP_MAC
    cyw43_hal_get_mac(CYW43_HAL_MAC_WLAN0, self->mac);

    int ret = cyw43_ll_bus_init(&self->cyw43_ll, self->mac);
    #else
    // Not setting mac address. It should come from otp
    int ret = cyw43_ll_bus_init(&self->cyw43_ll, NULL);
    #endif

    if (ret != 0) {
        return ret;
    }

    #if CYW43_USE_OTP_MAC
    // Get our mac address cyw43_hal_get_mac can get this from cyw43_state.mac
    cyw43_ll_wifi_get_mac(&self->cyw43_ll, self->mac);
    #endif

    CYW43_DEBUG("cyw43 loaded ok, mac %02x:%02x:%02x:%02x:%02x:%02x\n",
        self->mac[0], self->mac[1], self->mac[2], self->mac[3], self->mac[4], self->mac[5]);

    // Enable async events from low-level driver
    cyw43_sleep = CYW43_SLEEP_MAX;
    cyw43_poll = cyw43_poll_func;
    #if USE_SDIOIT
    cyw43_sdio_set_irq(true);
    #elif !CYW43_USE_SPI
    // If CYW43_PIN_WL_HOST_WAKE has a falling edge, cyw43_poll (if it's not NULL) should be called.
    cyw43_hal_pin_config_irq_falling(CYW43_PIN_WL_HOST_WAKE, true);
    #endif
    #if defined(CYW43_PIN_WL_IRQ)
    cyw43_hal_pin_config_irq_falling(CYW43_PIN_WL_IRQ, true);
    #endif

    // Kick things off
    cyw43_schedule_internal_poll_dispatch(cyw43_poll_func);

    return ret;
}

// This function must always be executed at the level where CYW43_THREAD_ENTER is effectively active
static void cyw43_poll_func(void) {
    CYW43_THREAD_LOCK_CHECK;

    if (cyw43_poll == NULL) {
        // Poll scheduled during deinit, just ignore it
        return;
    }

    CYW43_STAT_INC(CYW43_RUN_COUNT);

    cyw43_t *self = &cyw43_state;

    #if CYW43_ENABLE_BLUETOOTH
    if (self->bt_loaded && cyw43_ll_bt_has_work(&self->cyw43_ll)) {
        cyw43_bluetooth_hci_process();
    }
    #endif

    if (cyw43_ll_has_work(&self->cyw43_ll)) {
        cyw43_ll_process_packets(&self->cyw43_ll);
    }

    if (self->pend_disassoc) {
        self->pend_disassoc = false;
        cyw43_ll_ioctl(&self->cyw43_ll, CYW43_IOCTL_SET_DISASSOC, 0, NULL, CYW43_ITF_STA);
    }

    if (self->pend_rejoin_wpa) {
        self->pend_rejoin_wpa = false;
        cyw43_ll_wifi_set_wpa_auth(&self->cyw43_ll);
    }

    if (self->pend_rejoin) {
        self->pend_rejoin = false;
        cyw43_ll_wifi_rejoin(&self->cyw43_ll);
        self->wifi_join_state = WIFI_JOIN_STATE_ACTIVE;
    }

    if (cyw43_sleep == 0) {
        cyw43_ll_bus_sleep(&self->cyw43_ll, true);
        #if !USE_SDIOIT && !CYW43_USE_SPI
        cyw43_sdio_deinit(); // save power while WLAN bus sleeps
        #endif
    }

    #if USE_SDIOIT
    cyw43_sdio_set_irq(true);
    #endif

    #ifdef CYW43_POST_POLL_HOOK
    CYW43_POST_POLL_HOOK
    #endif

}

/*******************************************************************************/
// Callback interface to low-level driver

int cyw43_cb_read_host_interrupt_pin(void *cb_data) {
    (void)cb_data;
    #if defined(CYW43_PIN_WL_HOST_WAKE)
    // SDIO or SPI mode with WL_HOST_WAKE pin.
    return cyw43_hal_pin_read(CYW43_PIN_WL_HOST_WAKE);
    #elif defined(CYW43_PIN_WL_IRQ)
    // SPI mode with WL_IRQ pin (aliased to WL_SDIO_1).
    return cyw43_hal_pin_read(CYW43_PIN_WL_IRQ);
    #else
    // SDIO mode with WL_SDIO_1 pin in interrupt mode.
    return cyw43_hal_pin_read(CYW43_PIN_WL_SDIO_1);
    #endif
}

void cyw43_cb_ensure_awake(void *cb_data) {
    (void)cb_data;
    cyw43_sleep = CYW43_SLEEP_MAX;
    #if !USE_SDIOIT && !CYW43_USE_SPI
    cyw43_sdio_reinit();
    #endif
}

static const char *const cyw43_async_event_name_table[89] = {
    //[0 ... 88] = NULL,
    [CYW43_EV_SET_SSID] = "SET_SSID",
    [CYW43_EV_JOIN] = "JOIN",
    [CYW43_EV_AUTH] = "AUTH",
    [CYW43_EV_DEAUTH_IND] = "DEAUTH_IND",
    [CYW43_EV_ASSOC] = "ASSOC",
    [CYW43_EV_DISASSOC] = "DISASSOC",
    [CYW43_EV_DISASSOC_IND] = "DISASSOC_IND",
    [CYW43_EV_LINK] = "LINK",
    [CYW43_EV_PSK_SUP] = "PSK_SUP",
    [CYW43_EV_ESCAN_RESULT] = "ESCAN_RESULT",
    [CYW43_EV_CSA_COMPLETE_IND] = "CSA_COMPLETE_IND",
    [CYW43_EV_ASSOC_REQ_IE] = "ASSOC_REQ_IE",
    [CYW43_EV_ASSOC_RESP_IE] = "ASSOC_RESP_IE",
};

static void cyw43_dump_async_event(const cyw43_async_event_t *ev) {
    CYW43_PRINTF("[% 8d] ASYNC(%04x,",
        (int)cyw43_hal_ticks_ms(),
        (unsigned int)ev->flags
        );
    if (ev->event_type < CYW43_ARRAY_SIZE(cyw43_async_event_name_table)
        && cyw43_async_event_name_table[ev->event_type] != NULL) {
        CYW43_PRINTF("%s", cyw43_async_event_name_table[ev->event_type]);
    } else {
        CYW43_PRINTF("%u", (unsigned int)ev->event_type);
    }
    CYW43_PRINTF(",%u,%u,%u)\n",
        (unsigned int)ev->status,
        (unsigned int)ev->reason,
        (unsigned int)ev->interface
        );
}

void cyw43_cb_process_async_event(void *cb_data, const cyw43_async_event_t *ev) {
    cyw43_t *self = cb_data;

    if (self->trace_flags & CYW43_TRACE_ASYNC_EV) {
        cyw43_dump_async_event(ev);
    }

    if (ev->event_type == CYW43_EV_ESCAN_RESULT && self->wifi_scan_state == 1) {
        // Escan result event
        if (ev->status == 8) {
            // Partial result
            int ret = self->wifi_scan_cb(self->wifi_scan_env, &ev->u.scan_result);
            if (ret != 0) {
                // TODO need to abort scan, or just ignore any more results
            }
        } else if (ev->status == 0) {
            // Scan complete
            self->wifi_scan_state = 2;
        }

    } else if (ev->event_type == CYW43_EV_DISASSOC) {
        cyw43_cb_tcpip_set_link_down(self, CYW43_ITF_STA);
        self->wifi_join_state = 0x0000;

    #if 0
    } else if (ev->event_type == CYW43_EV_DISASSOC_IND) {
        if (ev->interface == CYW43_ITF_AP) {
            // Station disassociated with our AP, let DHCP server know so it can free the IP address
            dhcp_server_disassoc(&self->dhcp_server, buf + 24);
        }
    #endif

        // WiFi join events
    } else if (ev->event_type == CYW43_EV_PRUNE) {
        if (ev->status == 0 && ev->reason == 8) {
            // RSN mismatch, retry join with WPA auth
            self->pend_rejoin = true;
            self->pend_rejoin_wpa = true;
            cyw43_schedule_internal_poll_dispatch(cyw43_poll_func);
        }
    } else if (ev->event_type == CYW43_EV_SET_SSID) {
        if (ev->status == 0) {
            // Success setting SSID
        } else if (ev->status == 3 && ev->reason == 0) {
            self->wifi_join_state = WIFI_JOIN_STATE_NONET;
            // No matching SSID found (could be out of range, or down)
        } else {
            // Other failure setting SSID
            self->wifi_join_state = WIFI_JOIN_STATE_FAIL;
        }
    } else if (ev->event_type == CYW43_EV_AUTH) {
        if (ev->status == 0) {
            if ((self->wifi_join_state & WIFI_JOIN_STATE_KIND_MASK) == WIFI_JOIN_STATE_BADAUTH) {
                // A good-auth follows a bad-auth, so change the join state back to active.
                self->wifi_join_state = (self->wifi_join_state & ~WIFI_JOIN_STATE_KIND_MASK) | WIFI_JOIN_STATE_ACTIVE;
            }
            self->wifi_join_state |= WIFI_JOIN_STATE_AUTH;
        } else if (ev->status == 6) {
            // Unsolicited auth packet, ignore it
        } else {
            // Cannot authenticate
            self->wifi_join_state = WIFI_JOIN_STATE_BADAUTH;
        }
    } else if (ev->event_type == CYW43_EV_DEAUTH_IND) {
        if (ev->status == 0 && ev->reason == 2) {
            // Deauth, probably because password was wrong; disassociate
            self->pend_disassoc = true;
            cyw43_schedule_internal_poll_dispatch(cyw43_poll_func);
        }
    } else if (ev->event_type == CYW43_EV_LINK) {
        if (ev->status == 0) {
            if (ev->flags & 1) {
                // Link is up
                if (ev->interface == CYW43_ITF_STA) {
                    self->wifi_join_state |= WIFI_JOIN_STATE_LINK;
                } else {
                    cyw43_cb_tcpip_set_link_up(self, ev->interface);
                }
            } else {
                // Link is down
                cyw43_cb_tcpip_set_link_down(self, ev->interface);
            }
        }
    } else if (ev->event_type == CYW43_EV_PSK_SUP) {
        if (ev->status == 6) { // WLC_SUP_KEYED
            self->wifi_join_state |= WIFI_JOIN_STATE_KEYED;
        } else if ((ev->status == 4 || ev->status == 8 || ev->status == 10) && ev->reason == 15) {
            // Timeout waiting for key exchange M1/M3/G1
            // Probably at edge of the cell, retry
            self->pend_rejoin = true;
            cyw43_schedule_internal_poll_dispatch(cyw43_poll_func);
        } else {
            // PSK_SUP failure
            self->wifi_join_state = WIFI_JOIN_STATE_BADAUTH;
        }
    } else if (ev->event_type == CYW43_EV_ICV_ERROR) {
        self->pend_rejoin = true;
        self->pend_rejoin_wpa = false;
        cyw43_schedule_internal_poll_dispatch(cyw43_poll_func);
    }

    if (self->wifi_join_state == WIFI_JOIN_STATE_ALL) {
        // STA connected
        self->wifi_join_state = WIFI_JOIN_STATE_ACTIVE;
        cyw43_cb_tcpip_set_link_up(self, CYW43_ITF_STA);
    }
}

/*******************************************************************************/
// Ioctl and Ethernet interface

int cyw43_ioctl(cyw43_t *self, uint32_t cmd, size_t len, uint8_t *buf, uint32_t iface) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    ret = cyw43_ll_ioctl(&self->cyw43_ll, cmd, len, buf, iface);
    CYW43_THREAD_EXIT;

    return ret;
}

int cyw43_send_ethernet(cyw43_t *self, int itf, size_t len, const void *buf, bool is_pbuf) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    ret = cyw43_ll_send_ethernet(&self->cyw43_ll, itf, len, buf, is_pbuf);
    CYW43_THREAD_EXIT;

    return ret;
}

/*******************************************************************************/
// WiFi control

static int cyw43_wifi_on(cyw43_t *self, uint32_t country) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    #ifdef CYW43_PIN_RFSW_VDD
    // Turn the RF-switch on
    cyw43_hal_pin_high(CYW43_PIN_RFSW_VDD);
    #endif

    ret = cyw43_ll_wifi_on(&self->cyw43_ll, country);
    CYW43_THREAD_EXIT;

    return ret;
}

int cyw43_wifi_pm(cyw43_t *self, uint32_t pm_in) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    // pm_in: 0x00adbrrm
    uint32_t pm = pm_in & 0xf;
    uint32_t pm_sleep_ret = (pm_in >> 4) & 0xff;
    uint32_t li_bcn = (pm_in >> 12) & 0xf;
    uint32_t li_dtim = (pm_in >> 16) & 0xf;
    uint32_t li_assoc = (pm_in >> 20) & 0xf;

    ret = cyw43_ll_wifi_pm(&self->cyw43_ll, pm, pm_sleep_ret, li_bcn, li_dtim, li_assoc);
    CYW43_THREAD_EXIT;

    return ret;
}

int cyw43_wifi_get_pm(cyw43_t *self, uint32_t *pm_out) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    uint32_t pm = 0;
    uint32_t pm_sleep_ret = 0;
    uint32_t li_bcn = 0;
    uint32_t li_dtim = 0;
    uint32_t li_assoc = 0;

    ret = cyw43_ll_wifi_get_pm(&self->cyw43_ll, &pm, &pm_sleep_ret, &li_bcn, &li_dtim, &li_assoc);
    if (ret == 0) {
        // pm_out: 0x00adbrrm
        *pm_out = cyw43_pm_value((uint8_t)pm, (uint16_t)pm_sleep_ret, (uint8_t)li_bcn, (uint8_t)li_dtim, (uint8_t)li_assoc);
    }
    CYW43_THREAD_EXIT;

    return ret;
}

int cyw43_wifi_get_mac(cyw43_t *self, int itf, uint8_t mac[6]) {
    (void)self;
    (void)itf;
    cyw43_hal_get_mac(CYW43_HAL_MAC_WLAN0, &mac[0]);
    return 0;
}

int cyw43_wifi_update_multicast_filter(cyw43_t *self, uint8_t *addr, bool add) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ll_wifi_update_multicast_filter(&self->cyw43_ll, addr, add);
    CYW43_THREAD_EXIT;
    return ret;
}

void cyw43_wifi_set_up(cyw43_t *self, int itf, bool up, uint32_t country) {
    CYW43_THREAD_ENTER;
    if (up) {
        if (self->itf_state == 0) {
            if (cyw43_wifi_on(self, country) != 0) {
                CYW43_THREAD_EXIT;
                return;
            }
            cyw43_wifi_pm(self, CYW43_DEFAULT_PM);
        }
        if (itf == CYW43_ITF_AP) {
            cyw43_wifi_ap_init(self);
        }
        if ((self->itf_state & (1 << itf)) == 0) {
            cyw43_cb_tcpip_deinit(self, itf);
            cyw43_cb_tcpip_init(self, itf);
            self->itf_state |= 1 << itf;
        }
        if (itf == CYW43_ITF_AP) {
            // Avoid a race by bringing the AP link up after the interface
            cyw43_wifi_ap_set_up(self, true);
        }
    } else {
        if (itf == CYW43_ITF_AP) {
            cyw43_wifi_ap_set_up(self, false);
        }
        if (self->itf_state & (1 << itf)) {
            cyw43_cb_tcpip_deinit(self, itf);
            self->itf_state &= ~(1 << itf);
        }
    }
    CYW43_THREAD_EXIT;
}

int cyw43_wifi_scan(cyw43_t *self, cyw43_wifi_scan_options_t *opts, void *env, int (*result_cb)(void *, const cyw43_ev_scan_result_t *)) {
    CYW43_THREAD_ENTER;
    if (self->itf_state == 0) {
        CYW43_THREAD_EXIT;
        return -CYW43_EPERM;
    }

    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    // Set state and callback data
    self->wifi_scan_state = 1;
    self->wifi_scan_env = env;
    self->wifi_scan_cb = result_cb;

    // Start the scan
    ret = cyw43_ll_wifi_scan(&self->cyw43_ll, opts);

    CYW43_THREAD_EXIT;

    return ret;
}

int cyw43_wifi_link_status(cyw43_t *self, int itf) {
    if (itf == CYW43_ITF_STA) {
        int s = self->wifi_join_state & WIFI_JOIN_STATE_KIND_MASK;
        if (s == WIFI_JOIN_STATE_ACTIVE) {
            return CYW43_LINK_JOIN;
        } else if (s == WIFI_JOIN_STATE_FAIL) {
            return CYW43_LINK_FAIL;
        } else if (s == WIFI_JOIN_STATE_NONET) {
            return CYW43_LINK_NONET;
        } else if (s == WIFI_JOIN_STATE_BADAUTH) {
            return CYW43_LINK_BADAUTH;
        } else {
            return CYW43_LINK_DOWN;
        }
    } else {
        return CYW43_LINK_DOWN;
    }
}

/*******************************************************************************/
// WiFi STA

int cyw43_wifi_join(cyw43_t *self, size_t ssid_len, const uint8_t *ssid, size_t key_len, const uint8_t *key, uint32_t auth_type, const uint8_t *bssid, uint32_t channel) {
    CYW43_THREAD_ENTER;
    if (!CYW43_STA_IS_ACTIVE(self)) {
        CYW43_THREAD_EXIT;
        return -CYW43_EPERM;
    }

    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }

    ret = cyw43_ll_wifi_join(&self->cyw43_ll, ssid_len, ssid, key_len, key, auth_type, bssid, channel);

    if (ret == 0) {
        // Wait for responses: EV_AUTH, EV_LINK, EV_SET_SSID, EV_PSK_SUP
        // Will get EV_DEAUTH_IND if password is invalid
        self->wifi_join_state = WIFI_JOIN_STATE_ACTIVE;

        if (auth_type == CYW43_AUTH_OPEN) {
            // For open security we don't need EV_PSK_SUP, so set that flag indicator now
            self->wifi_join_state |= WIFI_JOIN_STATE_KEYED;
        }
    }

    CYW43_THREAD_EXIT;

    return ret;
}

int cyw43_wifi_leave(cyw43_t *self, int itf) {
    // Disassociate with SSID
    return cyw43_ioctl(self, CYW43_IOCTL_SET_DISASSOC, 0, NULL, itf);
}


int cyw43_wifi_get_rssi(cyw43_t *self, int32_t *rssi) {
    if (!rssi || !CYW43_STA_IS_ACTIVE(self)) {
        return -CYW43_EPERM;
    }
    return cyw43_ioctl(self, CYW43_IOCTL_GET_RSSI, sizeof(*rssi), (uint8_t *)rssi, CYW43_ITF_STA);
}

int cyw43_wifi_get_bssid(cyw43_t *self, uint8_t bssid[6]) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ll_wifi_get_bssid(&self->cyw43_ll, bssid);
    CYW43_THREAD_EXIT;
    return ret;
}

/*******************************************************************************/
// WiFi AP

static void cyw43_wifi_ap_init(cyw43_t *self) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return;
    }

    cyw43_ll_wifi_ap_init(&self->cyw43_ll, self->ap_ssid_len, self->ap_ssid, self->ap_auth, self->ap_key_len, self->ap_key, self->ap_channel);
    CYW43_THREAD_EXIT;
}

static void cyw43_wifi_ap_set_up(cyw43_t *self, bool up) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return;
    }

    cyw43_ll_wifi_ap_set_up(&self->cyw43_ll, up);
    CYW43_THREAD_EXIT;
}

void cyw43_wifi_ap_get_max_stas(cyw43_t *self, int *max_stas) {
    cyw43_wifi_ap_get_stas(self, max_stas, NULL);
}

void cyw43_wifi_ap_get_stas(cyw43_t *self, int *num_stas, uint8_t *macs) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return;
    }

    ret = cyw43_ll_wifi_ap_get_stas(&self->cyw43_ll, num_stas, macs);
    if (ret != 0) {
        *num_stas = 0;
    }
    CYW43_THREAD_EXIT;
}

/*******************************************************************************/
// GPIO control

#if CYW43_GPIO

int cyw43_gpio_set(cyw43_t *self, int gpio, bool val) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }
    ret = cyw43_ll_gpio_set(&self->cyw43_ll, gpio, val);
    CYW43_THREAD_EXIT;
    return ret;
}

int cyw43_gpio_get(cyw43_t *self, int gpio, bool *val) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW43_THREAD_EXIT;
        return ret;
    }
    ret = cyw43_ll_gpio_get(&self->cyw43_ll, gpio, val);
    CYW43_THREAD_EXIT;
    return ret;
}

#endif

#if CYW43_ENABLE_BLUETOOTH
static int cyw43_ensure_bt_up(cyw43_t *self) {
    CYW43_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret == 0 && !self->bt_loaded) {
        ret = cyw43_btbus_init(&self->cyw43_ll); // todo: Passing cyw43_ll is a bit naff
        if (ret == 0) {
            self->bt_loaded = true;
        }
    }
    CYW43_THREAD_EXIT;
    return ret;
}

// Just load firmware
int cyw43_bluetooth_hci_init(void) {
    return cyw43_ensure_bt_up(&cyw43_state);
}

// Read data
int cyw43_bluetooth_hci_read(uint8_t *buf, uint32_t max_size, uint32_t *len) {
    cyw43_t *self = &cyw43_state;
    int ret = cyw43_ensure_bt_up(self);
    if (ret) {
        return ret;
    }
    ret = cyw43_btbus_read(buf, max_size, len);
    if (ret) {
        CYW43_PRINTF("cyw43_bluetooth_hci_read: failed to read from shared bus\n");
        return ret;
    }
    return 0;
}

// Write data, buffer needs to include space for a 4 byte header and include this in len
int cyw43_bluetooth_hci_write(uint8_t *buf, size_t len) {
    cyw43_t *self = &cyw43_state;
    int ret = cyw43_ensure_bt_up(self);
    if (ret) {
        return ret;
    }
    ret = cyw43_btbus_write(buf, len);
    if (ret) {
        CYW43_PRINTF("cyw43_bluetooth_hci_write: failed to write to shared bus\n");
        return ret;
    }
    return 0;
}
#endif
