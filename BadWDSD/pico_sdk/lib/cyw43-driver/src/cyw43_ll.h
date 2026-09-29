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

#ifndef CYW43_INCLUDED_CYW43_LL_H
#define CYW43_INCLUDED_CYW43_LL_H

#include <stdbool.h>
#include <stdint.h>
#include "cyw43_config.h"

// External interface

/**
 * \addtogroup cyw43_ll
 */
//!\{

/**
 *  \internal
 *  \file cyw43_ll.h
 *  \brief Low Level CYW43 driver interface
 */

// IOCTL commands
// Bottom bit is used to indicate "set", so divide by 2 to get real ioctl value
#define CYW43_IOCTL_GET_SSID            (0x32)
#define CYW43_IOCTL_GET_CHANNEL         (0x3a)
#define CYW43_IOCTL_SET_DISASSOC        (0x69)
#define CYW43_IOCTL_GET_ANTDIV          (0x7e)
#define CYW43_IOCTL_SET_ANTDIV          (0x81)
#define CYW43_IOCTL_SET_MONITOR         (0xd9)
#define CYW43_IOCTL_GET_RSSI            (0xfe)
#define CYW43_IOCTL_GET_VAR             (0x20c)
#define CYW43_IOCTL_SET_VAR             (0x20f)

// Async events, event_type field
#define CYW43_EV_SET_SSID               (0)
#define CYW43_EV_JOIN                   (1)
#define CYW43_EV_AUTH                   (3)
#define CYW43_EV_DEAUTH                 (5)
#define CYW43_EV_DEAUTH_IND             (6)
#define CYW43_EV_ASSOC                  (7)
#define CYW43_EV_DISASSOC               (11)
#define CYW43_EV_DISASSOC_IND           (12)
#define CYW43_EV_LINK                   (16)
#define CYW43_EV_PRUNE                  (23)
#define CYW43_EV_PSK_SUP                (46)
#define CYW43_EV_ICV_ERROR              (49)
#define CYW43_EV_ESCAN_RESULT           (69)
#define CYW43_EV_CSA_COMPLETE_IND       (80)
#define CYW43_EV_ASSOC_REQ_IE           (87)
#define CYW43_EV_ASSOC_RESP_IE          (88)

// Event status values
#define CYW43_STATUS_SUCCESS            (0)
#define CYW43_STATUS_FAIL               (1)
#define CYW43_STATUS_TIMEOUT            (2)
#define CYW43_STATUS_NO_NETWORKS        (3)
#define CYW43_STATUS_ABORT              (4)
#define CYW43_STATUS_NO_ACK             (5)
#define CYW43_STATUS_UNSOLICITED        (6)
#define CYW43_STATUS_ATTEMPT            (7)
#define CYW43_STATUS_PARTIAL            (8)
#define CYW43_STATUS_NEWSCAN            (9)
#define CYW43_STATUS_NEWASSOC           (10)

// Values used for STA and AP auth settings
#define CYW43_SUP_DISCONNECTED          (0)         // Disconnected
#define CYW43_SUP_CONNECTING            (1)         // Connecting
#define CYW43_SUP_IDREQUIRED            (2)         // ID Required
#define CYW43_SUP_AUTHENTICATING        (3)         // Authenticating
#define CYW43_SUP_AUTHENTICATED         (4)         // Authenticated
#define CYW43_SUP_KEYXCHANGE            (5)         // Key Exchange
#define CYW43_SUP_KEYED                 (6)         // Key Exchanged
#define CYW43_SUP_TIMEOUT               (7)         // Timeout
#define CYW43_SUP_LAST_BASIC_STATE      (8)         // Last Basic State
#define CYW43_SUP_KEYXCHANGE_WAIT_M1    CYW43_SUP_AUTHENTICATED     // Waiting to receive handshake msg M1
#define CYW43_SUP_KEYXCHANGE_PREP_M2    CYW43_SUP_KEYXCHANGE        // Preparing to send handshake msg M2
#define CYW43_SUP_KEYXCHANGE_WAIT_M3    CYW43_SUP_LAST_BASIC_STATE  // Waiting to receive handshake msg M3
#define CYW43_SUP_KEYXCHANGE_PREP_M4    (9)         // Preparing to send handshake msg M4
#define CYW43_SUP_KEYXCHANGE_WAIT_G1    (10)        // Waiting to receive handshake msg G1
#define CYW43_SUP_KEYXCHANGE_PREP_G2    (11)        // Preparing to send handshake msg G2

// Values for AP auth setting
#define CYW43_REASON_INITIAL_ASSOC      (0)         // initial assoc
#define CYW43_REASON_LOW_RSSI           (1)         // roamed due to low RSSI
#define CYW43_REASON_DEAUTH             (2)         // roamed due to DEAUTH indication
#define CYW43_REASON_DISASSOC           (3)         // roamed due to DISASSOC indication
#define CYW43_REASON_BCNS_LOST          (4)         // roamed due to lost beacons
#define CYW43_REASON_FAST_ROAM_FAILED   (5)         // roamed due to fast roam failure
#define CYW43_REASON_DIRECTED_ROAM      (6)         // roamed due to request by AP
#define CYW43_REASON_TSPEC_REJECTED     (7)         // roamed due to TSPEC rejection
#define CYW43_REASON_BETTER_AP          (8)         // roamed due to finding better AP

// prune reason codes
#define CYW43_REASON_PRUNE_ENCR_MISMATCH    (1)     // encryption mismatch
#define CYW43_REASON_PRUNE_BCAST_BSSID      (2)     // AP uses a broadcast BSSID
#define CYW43_REASON_PRUNE_MAC_DENY         (3)     // STA's MAC addr is in AP's MAC deny list
#define CYW43_REASON_PRUNE_MAC_NA           (4)     // STA's MAC addr is not in AP's MAC allow list
#define CYW43_REASON_PRUNE_REG_PASSV        (5)     // AP not allowed due to regulatory restriction
#define CYW43_REASON_PRUNE_SPCT_MGMT        (6)     // AP does not support STA locale spectrum mgmt
#define CYW43_REASON_PRUNE_RADAR            (7)     // AP is on a radar channel of STA locale
#define CYW43_REASON_RSN_MISMATCH           (8)     // STA does not support AP's RSN
#define CYW43_REASON_PRUNE_NO_COMMON_RATES  (9)     // No rates in common with AP
#define CYW43_REASON_PRUNE_BASIC_RATES      (10)    // STA does not support all basic rates of BSS
#define CYW43_REASON_PRUNE_CCXFAST_PREVAP   (11)    // CCX FAST ROAM: prune previous AP
#define CYW43_REASON_PRUNE_CIPHER_NA        (12)    // BSS's cipher not supported
#define CYW43_REASON_PRUNE_KNOWN_STA        (13)    // AP is already known to us as a STA
#define CYW43_REASON_PRUNE_CCXFAST_DROAM    (14)    // CCX FAST ROAM: prune unqualified AP
#define CYW43_REASON_PRUNE_WDS_PEER         (15)    // AP is already known to us as a WDS peer
#define CYW43_REASON_PRUNE_QBSS_LOAD        (16)    // QBSS LOAD - AAC is too low
#define CYW43_REASON_PRUNE_HOME_AP          (17)    // prune home AP
#define CYW43_REASON_PRUNE_AP_BLOCKED       (18)    // prune blocked AP
#define CYW43_REASON_PRUNE_NO_DIAG_SUPPORT  (19)    // prune due to diagnostic mode not supported

// WPA failure reason codes carried in the WLC_E_PSK_SUP event
#define CYW43_REASON_SUP_OTHER              (0)     // Other reason
#define CYW43_REASON_SUP_DECRYPT_KEY_DATA   (1)     // Decryption of key data failed
#define CYW43_REASON_SUP_BAD_UCAST_WEP128   (2)     // Illegal use of ucast WEP128
#define CYW43_REASON_SUP_BAD_UCAST_WEP40    (3)     // Illegal use of ucast WEP40
#define CYW43_REASON_SUP_UNSUP_KEY_LEN      (4)     // Unsupported key length
#define CYW43_REASON_SUP_PW_KEY_CIPHER      (5)     // Unicast cipher mismatch in pairwise key
#define CYW43_REASON_SUP_MSG3_TOO_MANY_IE   (6)     // WPA IE contains > 1 RSN IE in key msg 3
#define CYW43_REASON_SUP_MSG3_IE_MISMATCH   (7)     // WPA IE mismatch in key message 3
#define CYW43_REASON_SUP_NO_INSTALL_FLAG    (8)     // INSTALL flag unset in 4-way msg
#define CYW43_REASON_SUP_MSG3_NO_GTK        (9)     // encapsulated GTK missing from msg 3
#define CYW43_REASON_SUP_GRP_KEY_CIPHER     (10)    // Multicast cipher mismatch in group key
#define CYW43_REASON_SUP_GRP_MSG1_NO_GTK    (11)    // encapsulated GTK missing from group msg 1
#define CYW43_REASON_SUP_GTK_DECRYPT_FAIL   (12)    // GTK decrypt failure
#define CYW43_REASON_SUP_SEND_FAIL          (13)    // message send failure
#define CYW43_REASON_SUP_DEAUTH             (14)    // received FC_DEAUTH
#define CYW43_REASON_SUP_WPA_PSK_TMO        (15)    // WPA PSK 4-way handshake timeout

/**
 * \name Authorization types
 * \brief Used when setting up an access point, or connecting to an access point
 * \anchor CYW43_AUTH_
 */
//!\{
#define CYW43_AUTH_OPEN (0)                     ///< No authorisation required (open)
#define CYW43_AUTH_WPA_TKIP_PSK   (0x00200002)  ///< WPA authorisation
#define CYW43_AUTH_WPA2_AES_PSK   (0x00400004)  ///< WPA2 authorisation (preferred)
#define CYW43_AUTH_WPA2_MIXED_PSK (0x00400006)  ///< WPA2/WPA mixed authorisation
#define CYW43_AUTH_WPA3_SAE_AES_PSK  (0x01000004)   ///< WPA3 AES authorisation
#define CYW43_AUTH_WPA3_WPA2_AES_PSK (0x01400004)   ///< WPA2/WPA3 authorisation
//!\}

/*!
 * \brief Power save mode parameter passed to cyw43_ll_wifi_pm
 */
#define CYW43_NO_POWERSAVE_MODE           (0) ///< No Powersave mode
#define CYW43_PM1_POWERSAVE_MODE          (1) ///< Powersave mode on specified interface without regard for throughput reduction
#define CYW43_PM2_POWERSAVE_MODE          (2) ///< Powersave mode on specified interface with High throughput

// The maximum block size for transfers on the bus.
#if CYW43_USE_SPI
#define CYW43_BUS_MAX_BLOCK_SIZE 64
#define CYW43_BACKPLANE_READ_PAD_LEN_BYTES 16
#define CYW43_LL_STATE_SIZE_WORDS (526 + 1 + ((CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4) + 1) + CYW43_INCLUDE_LEGACY_F1_OVERFLOW_WORKAROUND_VARIABLES * 4)
#else // SDIO
#define CYW43_BUS_MAX_BLOCK_SIZE 16384
#define CYW43_BACKPLANE_READ_PAD_LEN_BYTES 0
#define CYW43_LL_STATE_SIZE_WORDS (526 + 1)
#endif

/*!
 * \brief To indicate no specific channel when calling cyw43_ll_wifi_join with bssid specified
 */
#define CYW43_CHANNEL_NONE (0xffffffff) ///< No Channel specified (use the AP's channel)

/*!
 * \brief Network interface types
 * \anchor CYW43_ITF_
 */
//!\{
enum {
    CYW43_ITF_STA,  ///< Client interface STA mode
    CYW43_ITF_AP,   ///< Access point (AP) interface mode
};
//!\}

/*!
 * \brief Structure to return wifi scan results
 */
//!\{
typedef struct _cyw43_ev_scan_result_t {
    uint32_t _0[5];
    uint8_t bssid[6];   ///< access point mac address
    uint16_t _1[2];
    uint8_t ssid_len;   ///< length of wlan access point name
    uint8_t ssid[32];   ///< wlan access point name
    uint32_t _2[5];
    uint16_t channel;   ///< wifi channel
    uint16_t _3;
    uint8_t auth_mode;  ///< wifi auth mode \ref CYW43_AUTH_
    int16_t rssi;       ///< signal strength
} cyw43_ev_scan_result_t;
//!\}

typedef struct _cyw43_async_event_t {
    uint16_t _0;
    uint16_t flags;
    uint32_t event_type;
    uint32_t status;
    uint32_t reason;
    uint8_t _1[30];
    uint8_t interface;
    uint8_t _2;
    union {
        cyw43_ev_scan_result_t scan_result;
    } u;
} cyw43_async_event_t;

/*!
 * \brief wifi scan options passed to cyw43_wifi_scan
 */
//!\{
typedef struct _cyw43_wifi_scan_options_t {
    uint32_t version;           ///< version (not used)
    uint16_t action;            ///< action (not used)
    uint16_t _;                 ///< not used
    uint32_t ssid_len;          ///< ssid length, 0=all
    uint8_t ssid[32];           ///< ssid name
    uint8_t bssid[6];           ///< bssid (not used)
    int8_t bss_type;            ///< bssid type (not used)
    int8_t scan_type;           ///< scan type 0=active, 1=passive
    int32_t nprobes;            ///< number of probes (not used)
    int32_t active_time;        ///< active time (not used)
    int32_t passive_time;       ///< passive time (not used)
    int32_t home_time;          ///< home time (not used)
    int32_t channel_num;        ///< number of channels (not used)
    uint16_t channel_list[1];   ///< channel list (not used)
} cyw43_wifi_scan_options_t;
//!\}

typedef struct _cyw43_ll_t {
    uint32_t opaque[CYW43_LL_STATE_SIZE_WORDS]; // note: array of words
} cyw43_ll_t;

void cyw43_ll_init(cyw43_ll_t *self, void *cb_data);
void cyw43_ll_deinit(cyw43_ll_t *self);

int cyw43_ll_bus_init(cyw43_ll_t *self, const uint8_t *mac);
void cyw43_ll_bus_sleep(cyw43_ll_t *self, bool can_sleep);
void cyw43_ll_process_packets(cyw43_ll_t *self);
int cyw43_ll_ioctl(cyw43_ll_t *self, uint32_t cmd, size_t len, uint8_t *buf, uint32_t iface);
int cyw43_ll_send_ethernet(cyw43_ll_t *self, int itf, size_t len, const void *buf, bool is_pbuf);

int cyw43_ll_wifi_on(cyw43_ll_t *self, uint32_t country);
int cyw43_ll_wifi_pm(cyw43_ll_t *self, uint32_t pm, uint32_t pm_sleep_ret, uint32_t li_bcn, uint32_t li_dtim, uint32_t li_assoc);
int cyw43_ll_wifi_get_pm(cyw43_ll_t *self, uint32_t *pm, uint32_t *pm_sleep_ret, uint32_t *li_bcn, uint32_t *li_dtim, uint32_t *li_assoc);
int cyw43_ll_wifi_scan(cyw43_ll_t *self, cyw43_wifi_scan_options_t *opts);

int cyw43_ll_wifi_join(cyw43_ll_t *self, size_t ssid_len, const uint8_t *ssid, size_t key_len, const uint8_t *key, uint32_t auth_type, const uint8_t *bssid, uint32_t channel);
void cyw43_ll_wifi_set_wpa_auth(cyw43_ll_t *self);
void cyw43_ll_wifi_rejoin(cyw43_ll_t *self);
int cyw43_ll_wifi_get_bssid(cyw43_ll_t *self_in, uint8_t *bssid);

int cyw43_ll_wifi_ap_init(cyw43_ll_t *self, size_t ssid_len, const uint8_t *ssid, uint32_t auth, size_t key_len, const uint8_t *key, uint32_t channel);
int cyw43_ll_wifi_ap_set_up(cyw43_ll_t *self, bool up);
int cyw43_ll_wifi_ap_get_stas(cyw43_ll_t *self, int *num_stas, uint8_t *macs);

#if CYW43_GPIO
int cyw43_ll_gpio_set(cyw43_ll_t *self, int gpio_n, bool gpio_en);
int cyw43_ll_gpio_get(cyw43_ll_t *self_in, int gpio_n, bool *gpio_en);
#endif

// Get mac address
int cyw43_ll_wifi_get_mac(cyw43_ll_t *self_in, uint8_t *addr);

// Add/remove multicast address
int cyw43_ll_wifi_update_multicast_filter(cyw43_ll_t *self_in, uint8_t *addr, bool add);

// Returns true while there's work to do
bool cyw43_ll_has_work(cyw43_ll_t *self);
bool cyw43_ll_bt_has_work(cyw43_ll_t *self);

// Callbacks to be provided by mid-level interface
int cyw43_cb_read_host_interrupt_pin(void *cb_data);
void cyw43_cb_ensure_awake(void *cb_data);
void cyw43_cb_process_async_event(void *cb_data, const cyw43_async_event_t *ev);
void cyw43_cb_process_ethernet(void *cb_data, int itf, size_t len, const uint8_t *buf);

// Low level methods used for bluetooth
void cyw43_ll_write_backplane_reg(cyw43_ll_t *self_in, uint32_t addr, uint32_t val);
uint32_t cyw43_ll_read_backplane_reg(cyw43_ll_t *self_in, uint32_t addr);
int cyw43_ll_write_backplane_mem(cyw43_ll_t *self_in, uint32_t addr, uint32_t len, const uint8_t *buf);
int cyw43_ll_read_backplane_mem(cyw43_ll_t *self_in, uint32_t addr, uint32_t len, uint8_t *buf);

//!\}

#endif // CYW43_INCLUDED_CYW43_LL_H
