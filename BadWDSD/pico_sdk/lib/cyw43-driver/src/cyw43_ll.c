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
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include "cyw43_config.h"
#include "cyw43_country.h"
#include "cyw43_ll.h"
#include "cyw43_internal.h"
#include "cyw43_stats.h"

#include CYW43_WIFI_NVRAM_INCLUDE_FILE

#if CYW43_USE_SPI
#include "cyw43_spi.h"
#include "cyw43_debug_pins.h"
#else
#include "cyw43_sdio.h"
#endif

struct pbuf;
uint16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr, uint16_t len, uint16_t offset);

#ifndef NDEBUG
extern bool enable_spi_packet_dumping;
#endif

#define USE_KSO (1)

#define CYW43_RAM_SIZE (512 * 1024)

// Include the file containing the WiFi+CLM firmware blob as a C array.
#include CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE

#define CYW43_CLM_ADDR (fw_data + ALIGN_UINT(CYW43_WIFI_FW_LEN, 512))

#define ALIGN_UINT(val, align) (((val) + (align) - 1) & ~((align) - 1))

// Configure the padding needed for data sent to cyw43_write_bytes().
// cyw43_read_bytes() also needs padding, but that's handled separately.
#if CYW43_USE_SPI
#define CYW43_WRITE_BYTES_PAD(len) ALIGN_UINT((len), 4)
#else
#define CYW43_WRITE_BYTES_PAD(len) ALIGN_UINT((len), 64)
#endif

// Configure the active level of the host interrupt pin.
#if CYW43_USE_SPI
#if defined(CYW43_PIN_WL_HOST_WAKE)
static const int host_interrupt_pin_active = 1;
#elif defined(CYW43_PIN_WL_IRQ)
static const int host_interrupt_pin_active = 0;
#endif
#else
static const int host_interrupt_pin_active = 0;
#endif

#if CYW43_USE_STATS
// Storage for some debug stats
uint32_t cyw43_stats[CYW43_STAT_LAST];
#endif

static inline uint32_t cyw43_get_le32(const uint8_t *buf) {
    return buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
}

static inline void cyw43_put_le16(uint8_t *buf, uint32_t x) {
    buf[0] = x;
    buf[1] = x >> 8;
}

static inline void cyw43_put_le32(uint8_t *buf, uint32_t x) {
    buf[0] = x;
    buf[1] = x >> 8;
    buf[2] = x >> 16;
    buf[3] = x >> 24;
}

#if CYW43_RESOURCE_VERIFY_DOWNLOAD
static void cyw43_xxd(size_t len, const uint8_t *buf) {
    for (size_t i = 0; i < len; ++i) {
        CYW43_PRINTF(" %02x", buf[i]);
        if (i % 32 == 31) {
            CYW43_PRINTF("\n");
        }
    }
    CYW43_PRINTF("\n");
}
#endif

/*******************************************************************************/
// CYW43 constants and types

#define SDPCM_HEADER_LEN (sizeof(struct sdpcm_header_t))
#define IOCTL_HEADER_LEN (sizeof(struct ioctl_header_t))
#define BDC_HEADER_LEN (sizeof(struct sdpcm_bdc_header_t))

#define SDIO_FUNCTION2_WATERMARK    (0x10008)
#define SDIO_BACKPLANE_ADDRESS_LOW  (0x1000a)
#define SDIO_BACKPLANE_ADDRESS_MID  (0x1000b)
#define SDIO_BACKPLANE_ADDRESS_HIGH (0x1000c)
#define SDIO_CHIP_CLOCK_CSR         (0x1000e)
#define SDIO_WAKEUP_CTRL            (0x1001e)
#define SDIO_SLEEP_CSR              (0x1001f)

#define I_HMB_SW_MASK            (0x000000f0)
#define I_HMB_FC_CHANGE               (1 << 5)

#define CHIPCOMMON_BASE_ADDRESS  (0x18000000)
#define SDIO_BASE_ADDRESS        (0x18002000)
#define WLAN_ARMCM3_BASE_ADDRESS (0x18003000)
#define SOCSRAM_BASE_ADDRESS     (0x18004000)
#define BACKPLANE_ADDR_MASK      (0x7fff)
#define WRAPPER_REGISTER_OFFSET  (0x100000)

#define SBSDIO_SB_ACCESS_2_4B_FLAG  0x08000

#define CHIPCOMMON_SR_CONTROL1   (CHIPCOMMON_BASE_ADDRESS + 0x508)
#define SDIO_INT_STATUS          (SDIO_BASE_ADDRESS + 0x20)
#define SDIO_INT_HOST_MASK       (SDIO_BASE_ADDRESS + 0x24)
#define SDIO_FUNCTION_INT_MASK   (SDIO_BASE_ADDRESS + 0x34)
#define SDIO_TO_SB_MAILBOX       (SDIO_BASE_ADDRESS + 0x40)
#define SOCSRAM_BANKX_INDEX      (SOCSRAM_BASE_ADDRESS + 0x10)
#define SOCSRAM_BANKX_PDA        (SOCSRAM_BASE_ADDRESS + 0x44)

#define SBSDIO_ALP_AVAIL_REQ     (0x08)
#define SBSDIO_HT_AVAIL_REQ      (0x10)
#define SBSDIO_ALP_AVAIL         (0x40)
#define SBSDIO_HT_AVAIL          (0x80)

#define AI_IOCTRL_OFFSET         (0x408)
#define SICF_CPUHALT             (0x0020)
#define SICF_FGC                 (0x0002)
#define SICF_CLOCK_EN            (0x0001)
#define AI_RESETCTRL_OFFSET      (0x800)
#define AIRC_RESET               (1)

#define SPI_F2_WATERMARK (32)
#define SDIO_F2_WATERMARK (8)

#define WWD_STA_INTERFACE (0)
#define WWD_AP_INTERFACE (1)
#define WWD_P2P_INTERFACE (2)

// for cyw43_sdpcm_send_common
#define CONTROL_HEADER (0)
#define ASYNCEVENT_HEADER (1)
#define DATA_HEADER (2)

#define CDCF_IOC_ID_SHIFT (16)
#define CDCF_IOC_ID_MASK (0xffff0000)
#define CDCF_IOC_IF_SHIFT (12)

#define SDPCM_GET (0)
#define SDPCM_SET (2)

#define WLC_UP (2)
#define WLC_SET_INFRA (20)
#define WLC_SET_AUTH (22)
#define WLC_GET_BSSID (23)
#define WLC_GET_SSID (25)
#define WLC_SET_SSID (26)
#define WLC_SET_CHANNEL (30)
#define WLC_DISASSOC (52)
#define WLC_GET_ANTDIV (63)
#define WLC_SET_ANTDIV (64)
#define WLC_SET_DTIMPRD (78)
#define WLC_GET_PM (85)
#define WLC_SET_PM (86)
#define WLC_SET_GMODE (110)
#define WLC_SET_WSEC (134)
#define WLC_SET_BAND (142)
#define WLC_GET_ASSOCLIST (159)
#define WLC_SET_WPA_AUTH (165)
#define WLC_SET_VAR (263)
#define WLC_GET_VAR (262)
#define WLC_SET_WSEC_PMK (268)

// SDIO bus specifics
#define SDIOD_CCCR_IOEN (0x02)
#define SDIOD_CCCR_IORDY (0x03)
#define SDIOD_CCCR_INTEN (0x04)
#define SDIOD_CCCR_BICTRL (0x07)
#define SDIOD_CCCR_BLKSIZE_0 (0x10)
#define SDIOD_CCCR_SPEED_CONTROL (0x13)
#define SDIOD_CCCR_BRCM_CARDCAP (0xf0)
#define SDIOD_SEP_INT_CTL (0xf2)
#define SDIOD_CCCR_F1BLKSIZE_0 (0x110)
#define SDIOD_CCCR_F2BLKSIZE_0 (0x210)
#define SDIOD_CCCR_F2BLKSIZE_1 (0x211)
#define INTR_CTL_MASTER_EN (0x01)
#define INTR_CTL_FUNC1_EN (0x02)
#define INTR_CTL_FUNC2_EN (0x04)
#define SDIO_FUNC_ENABLE_1 (0x02)
#define SDIO_FUNC_ENABLE_2 (0x04)
#define SDIO_FUNC_READY_1 (0x02)
#define SDIO_FUNC_READY_2 (0x04)
#define SDIO_64B_BLOCK (64)
#define SDIO_CHIP_CLOCK_CSR (0x1000e)
#define SDIO_PULL_UP (0x1000f)

// SDIO_CHIP_CLOCK_CSR bits
#define SBSDIO_ALP_AVAIL (0x40)
#define SBSDIO_FORCE_HW_CLKREQ_OFF (0x20)
#define SBSDIO_ALP_AVAIL_REQ (0x08)
#define SBSDIO_FORCE_ALP (0x01)
#define SBSDIO_FORCE_HT            ((uint32_t)0x02)

// SDIOD_CCCR_BRCM_CARDCAP bits
#define SDIOD_CCCR_BRCM_CARDCAP_CMD14_SUPPORT ((uint32_t)0x02) // Supports CMD14
#define SDIOD_CCCR_BRCM_CARDCAP_CMD14_EXT   ((uint32_t)0x04) // CMD14 is allowed in FSM command state
#define SDIOD_CCCR_BRCM_CARDCAP_CMD_NODEC   ((uint32_t)0x08) // sdiod_aos does not decode any command

// SDIOD_SEP_INT_CTL bits
#define SEP_INTR_CTL_MASK                   ((uint32_t)0x01) // out-of-band interrupt mask
#define SEP_INTR_CTL_EN                     ((uint32_t)0x02) // out-of-band interrupt output enable
#define SEP_INTR_CTL_POL                    ((uint32_t)0x04) // out-of-band interrupt polarity

// SDIO_WAKEUP_CTRL bits
#define SBSDIO_WCTRL_WAKE_TILL_ALP_AVAIL    ((uint32_t)(1 << 0)) // WakeTillAlpAvail bit
#define SBSDIO_WCTRL_WAKE_TILL_HT_AVAIL     ((uint32_t)(1 << 1)) // WakeTillHTAvail bit

// SDIO_SLEEP_CSR bits
#define SBSDIO_SLPCSR_KEEP_SDIO_ON          ((uint32_t)(1 << 0)) // KeepSdioOn bit
#define SBSDIO_SLPCSR_DEVICE_ON             ((uint32_t)(1 << 1)) // DeviceOn bit

// For determining security type from a scan
#define DOT11_CAP_PRIVACY             (0x0010)
#define DOT11_IE_ID_RSN               (48)
#define DOT11_IE_ID_VENDOR_SPECIFIC   (221)
#define WPA_OUI_TYPE1                 "\x00\x50\xF2\x01"

#define SLEEP_MAX (50)

// Multicast registered group addresses
#define MAX_MULTICAST_REGISTERED_ADDRESS (10)

#define CYW_INT_FROM_LL(ll) ((cyw43_int_t *)(ll))
#define CYW_INT_TO_LL(in) ((cyw43_ll_t *)(in))

#define CYW_EAPOL_KEY_TIMEOUT (5000)

// Errors generated by sdpcm_process_rx_packet.
#define CYW43_ERROR_WRONG_PAYLOAD_TYPE (-9)

#define AUTH_TYPE_OPEN 0
#define AUTH_TYPE_SAE 3

// management frame protection
#define MFP_NONE 0
#define MFP_CAPABLE 1
#define MFP_REQUIRED 2

// Values used for STA and AP auth settings
#define CYW43_WPA_AUTH_PSK (0x0004)
#define CYW43_WPA2_AUTH_PSK (0x0080)
#define CYW43_WPA3_AUTH_SAE_PSK (0x40000)

// Max password length
#define CYW43_WPA_MAX_PASSWORD_LEN 64
#define CYW43_WPA_SAE_MAX_PASSWORD_LEN 128

static int cyw43_ll_sdpcm_poll_device(cyw43_int_t *self, size_t *len, uint8_t **buf);
static void cyw43_write_iovar_n(cyw43_int_t *self, const char *var, size_t len, const void *buf, uint32_t iface);

void cyw43_ll_init(cyw43_ll_t *self_in, void *cb_data) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    self->cb_data = cb_data;
    self->cur_backplane_window = 0;
    self->wwd_sdpcm_packet_transmit_sequence_number = 0;
    self->wwd_sdpcm_last_bus_data_credit = 1; // we get an immediate stall if this isn't done?
    self->wlan_flow_control = 0;
    self->wwd_sdpcm_requested_ioctl_id = 0;
    self->bus_is_up = false;
    self->had_successful_packet = false;
    self->bus_data = 0;
}

void cyw43_ll_deinit(cyw43_ll_t *self_in) {
    #if CYW43_USE_SPI
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    cyw43_spi_deinit(self);
    #else
    (void)self_in;
    #endif
}

/*******************************************************************************/
// low level read/write

static uint32_t cyw43_read_reg(cyw43_int_t *self, uint32_t fn, uint32_t reg, size_t size) {
    assert(fn == BACKPLANE_FUNCTION);
    if (size == 1) {
        return cyw43_read_reg_u8(self, fn, reg);
    } else {
        assert(size == 4);
        return cyw43_read_reg_u32(self, fn, reg);
    }
}

static int cyw43_write_reg(cyw43_int_t *self, uint32_t fn, uint32_t reg, size_t size, uint32_t val) {
    assert(fn == BACKPLANE_FUNCTION);
    if (size == 1) {
        return cyw43_write_reg_u8(self, fn, reg, val);
    } else {
        assert(size == 4);
        return cyw43_write_reg_u32(self, fn, reg, val);
    }
}

/*******************************************************************************/
// backplane stuff

static void cyw43_set_backplane_window(cyw43_int_t *self, uint32_t addr) {
    addr = addr & ~BACKPLANE_ADDR_MASK;
    if (addr == self->cur_backplane_window) {
        return;
    }
    if ((addr & 0xff000000) != (self->cur_backplane_window & 0xff000000)) {
        cyw43_write_reg(self, BACKPLANE_FUNCTION, SDIO_BACKPLANE_ADDRESS_HIGH, 1, addr >> 24);
    }
    if ((addr & 0x00ff0000) != (self->cur_backplane_window & 0x00ff0000)) {
        cyw43_write_reg(self, BACKPLANE_FUNCTION, SDIO_BACKPLANE_ADDRESS_MID, 1, addr >> 16);
    }
    if ((addr & 0x0000ff00) != (self->cur_backplane_window & 0x0000ff00)) {
        cyw43_write_reg(self, BACKPLANE_FUNCTION, SDIO_BACKPLANE_ADDRESS_LOW, 1, addr >> 8);
    }
    self->cur_backplane_window = addr;
}

static uint32_t cyw43_read_backplane(cyw43_int_t *self, uint32_t addr, size_t size) {
    cyw43_set_backplane_window(self, addr);
    addr &= BACKPLANE_ADDR_MASK;
    #if CYW43_USE_SPI
    addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;
    #else
    if (size == 4) {
        addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;
    }
    #endif
    uint32_t reg = cyw43_read_reg(self, BACKPLANE_FUNCTION, addr, size);
    cyw43_set_backplane_window(self, CHIPCOMMON_BASE_ADDRESS);
    return reg;
}

static void cyw43_write_backplane(cyw43_int_t *self, uint32_t addr, size_t size, uint32_t val) {
    cyw43_set_backplane_window(self, addr);
    addr &= BACKPLANE_ADDR_MASK;
    #if CYW43_USE_SPI
    addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;
    #else
    if (size == 4) {
        addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;
    }
    #endif
    cyw43_write_reg(self, BACKPLANE_FUNCTION, addr, size, val);
    cyw43_set_backplane_window(self, CHIPCOMMON_BASE_ADDRESS);
}

static int cyw43_check_valid_chipset_firmware(cyw43_int_t *self, size_t len, uintptr_t source) {
    (void)self;
    // get the last bit of the firmware, the last 800 bytes
    uint32_t fw_end = 800;
    const uint8_t *b = (const uint8_t *)source + len - fw_end;

    // get length of trailer
    fw_end -= 16; // skip DVID trailer
    uint32_t trail_len = b[fw_end - 2] | b[fw_end - 1] << 8;

    if (trail_len < 500 && b[fw_end - 3] == '\0') {
        for (int i = 80; i < (int)trail_len; ++i) {
            if (strncmp((const char *)&b[fw_end - 3 - i], "Version: ", 9) == 0) {
                // valid chipset firmware found
                // print wifi firmware version info
                CYW43_DEBUG("%s\n", &b[fw_end - 3 - i]);
                return 0;
            }
        }
    }

    CYW43_WARN("could not find valid firmware\n");
    return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
}

static int cyw43_download_resource(cyw43_int_t *self, uint32_t addr, size_t len, uintptr_t source) {
    // The calls to cyw43_write_bytes() (and cyw43_read_bytes()) require data sizes that
    // are aligned to a certain amount.
    assert(CYW43_WRITE_BYTES_PAD(len) == len);

    CYW43_VDEBUG("writing %u bytes to 0x%x\n", (uint32_t)len, (uint32_t)addr);

    uint32_t block_size = CYW43_BUS_MAX_BLOCK_SIZE;

    #if CYW43_VERBOSE_DEBUG
    uint32_t t_start = cyw43_hal_ticks_us();
    #endif

    for (size_t offset = 0; offset < len; offset += block_size) {
        CYW43_EVENT_POLL_HOOK;

        size_t sz = block_size;
        if (offset + sz > len) {
            sz = len - offset;
        }
        uint32_t dest_addr = addr + offset;
        assert(((dest_addr & BACKPLANE_ADDR_MASK) + sz) <= (BACKPLANE_ADDR_MASK + 1));
        cyw43_set_backplane_window(self, dest_addr);
        const uint8_t *src = (const uint8_t *)source + offset;
        dest_addr &= BACKPLANE_ADDR_MASK;
        #if CYW43_USE_SPI
        dest_addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;
        #endif
        int ret = cyw43_write_bytes(self, BACKPLANE_FUNCTION, dest_addr, sz, src);
        if (ret != 0) {

            return CYW43_FAIL_FAST_CHECK(ret);
        }
    }

    #if CYW43_VERBOSE_DEBUG
    uint32_t t_end = cyw43_hal_ticks_us();
    uint32_t dt = t_end - t_start;
    CYW43_VDEBUG("done dnload; dt = %u us; speed = %u kbytes/sec\n", (unsigned int)dt, (unsigned int)(len * 1000 / dt));
    #endif

    #if CYW43_RESOURCE_VERIFY_DOWNLOAD

    // Verification of 380k takes about 40ms using a 512-byte transfer size
    const size_t verify_block_size = CYW43_BUS_MAX_BLOCK_SIZE;
    uint8_t buf[verify_block_size];

    #if CYW43_VERBOSE_DEBUG
    t_start = cyw43_hal_ticks_us();
    #endif

    for (size_t offset = 0; offset < len; offset += verify_block_size) {
        size_t sz = verify_block_size;
        if (offset + sz > len) {
            sz = len - offset;
        }
        uint32_t dest_addr = addr + offset;
        assert(((dest_addr & BACKPLANE_ADDR_MASK) + sz) <= (BACKPLANE_ADDR_MASK + 1));
        cyw43_set_backplane_window(self, dest_addr);
        cyw43_read_bytes(self, BACKPLANE_FUNCTION, dest_addr & BACKPLANE_ADDR_MASK, sz, buf);
        const uint8_t *src = (const uint8_t *)source + offset;
        if (memcmp(buf, src, sz) != 0) {
            CYW43_WARN("fail verify at address 0x%08x:\n", (unsigned int)dest_addr);
            cyw43_xxd(sz, src);
            cyw43_xxd(sz, buf);
            return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
        }
    }

    #if CYW43_VERBOSE_DEBUG
    t_end = cyw43_hal_ticks_us();
    dt = t_end - t_start;
    CYW43_VDEBUG("done verify; dt = %u us; speed = %u kbytes/sec\n", (unsigned int)dt, (unsigned int)(len * 1000 / dt));
    #endif

    #endif // CYW43_RESOURCE_VERIFY_DOWNLOAD

    return 0;
}

/*******************************************************************************/
// Async event parsing

typedef struct _cyw43_scan_result_internal_t {
    uint32_t version;
    uint32_t length;
    uint8_t bssid[6];
    uint16_t beacon_period;
    uint16_t capability;
    uint8_t ssid_len;
    uint8_t ssid[32];
    uint32_t rateset_count;
    uint8_t rateset_rates[16];
    uint16_t chanspec;
    uint16_t atim_window;
    uint8_t dtim_period;
    int16_t rssi;
    int8_t phy_noise;
    uint8_t n_cap;
    uint32_t nbss_cap;
    uint8_t ctl_ch;
    uint32_t reserved32[1];
    uint8_t flags;
    uint8_t reserved[3];
    uint8_t basic_mcs[16];
    uint16_t ie_offset;
    uint32_t ie_length;
    int16_t SNR;
} cyw43_scan_result_internal_t;

static inline uint32_t cyw43_be32toh(uint32_t x) {
    return (x >> 24) | (x >> 8 & 0xff00) | (x << 8 & 0xff0000) | x << 24;
}

static inline uint16_t cyw43_be16toh(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static void cyw43_ll_wifi_parse_scan_result(cyw43_async_event_t *ev) {
    struct _scan_result_t {
        uint32_t buflen;
        uint32_t version;
        uint16_t sync_id;
        uint16_t bss_count;
        cyw43_scan_result_internal_t bss;
    } *scan_res = (void *)((uint8_t *)ev + 48);

    if (scan_res->bss.ie_offset + scan_res->bss.ie_length > scan_res->bss.length) {
        // invalid length
        ev->status = (uint32_t)-1; // set invalid
        return;
    }

    // parse IE elements
    uint8_t *ie_ptr = (uint8_t *)&scan_res->bss + scan_res->bss.ie_offset;
    uint8_t *ie_top = ie_ptr + scan_res->bss.ie_length;
    uint8_t *ie_rsn = NULL;
    uint8_t *ie_wpa = NULL;
    while (ie_ptr < ie_top) {
        uint8_t ie_type = ie_ptr[0];
        uint8_t ie_len = ie_ptr[1];
        if (ie_ptr + 2 + ie_len <= ie_top) {
            // valid IE element
            if (ie_type == DOT11_IE_ID_RSN) {
                ie_rsn = ie_ptr;
            } else if (ie_type == DOT11_IE_ID_VENDOR_SPECIFIC) {
                if (memcmp(ie_ptr + 2, WPA_OUI_TYPE1, 4) == 0) {
                    ie_wpa = ie_ptr;
                }
            }
        }
        ie_ptr += 2 + ie_len;
    }
    int security = 0;// OPEN
    if (ie_rsn != NULL) {
        // TODO need to parse the IE to check for TKIP, AES and enterprise modes
        security |= 4;// WPA2;
    }
    if (ie_wpa != NULL) {
        // TODO need to parse the IE to check for TKIP, AES and enterprise modes
        security |= 2;// WPA;
    }
    if (scan_res->bss.capability & DOT11_CAP_PRIVACY) {
        security |= 1;// WEP_PSK;
    }

    ev->u.scan_result.channel &= 0xff;
    ev->u.scan_result.auth_mode = security;

    return;
}

static cyw43_async_event_t *cyw43_ll_parse_async_event(size_t len, uint8_t *buf) {
    // buf = &spid_buf[46], so the event data structure is only aligned to 2 bytes.
    // We do get a hard fault with unaligned data (not sure exactly why) so relocate
    // the data to an aligned address, using custom code for efficiency.
    // TODO: improve by only copying the data we need to for any given event.
    {
        uint32_t *d = (void *)&buf[-2];
        uint16_t *s = (void *)&buf[0];
        for (size_t i = (len + 3) >> 2; i; --i) {
            *d++ = s[0] | s[1] << 16;
            s += 2;
        }
    }

    // Cast data to the async event struct
    cyw43_async_event_t *ev = (void *)&buf[-2];

    // Convert endianness of fields
    ev->flags = cyw43_be16toh(ev->flags);
    ev->event_type = cyw43_be32toh(ev->event_type);
    ev->status = cyw43_be32toh(ev->status);
    ev->reason = cyw43_be32toh(ev->reason);

    // Parse additional type-specific event data
    if (ev->event_type == CYW43_EV_ESCAN_RESULT && ev->status == CYW43_STATUS_PARTIAL) {
        cyw43_ll_wifi_parse_scan_result(ev);
    }

    return ev;
}

/*******************************************************************************/
// SDPCM stuff

struct sdpcm_header_t {
    uint16_t size;
    uint16_t size_com;
    uint8_t sequence;
    uint8_t channel_and_flags;
    uint8_t next_length;
    uint8_t header_length;
    uint8_t wireless_flow_control;
    uint8_t bus_data_credit;
    uint8_t reserved[2];
};

// buf must be writable and have:
//  - SDPCM_HEADER_LEN bytes at the start for writing the headers
//  - readable data at the end for padding to get to 64 byte alignment
static int cyw43_sdpcm_send_common(cyw43_int_t *self, uint32_t kind, size_t len, uint8_t *buf) {
    // validate args
    if (kind != CONTROL_HEADER && kind != DATA_HEADER) {
        return CYW43_FAIL_FAST_CHECK(-CYW43_EINVAL);
    }

    cyw43_ll_bus_sleep((void *)self, false);

    // Wait until we are allowed to send
    // Credits are 8-bit unsigned integers that roll over, so we are stalled while they are equal
    if (self->wlan_flow_control || self->wwd_sdpcm_last_bus_data_credit == self->wwd_sdpcm_packet_transmit_sequence_number) {
        CYW43_VDEBUG("[CYW43;%u] STALL(%u;%u-%u)\n", (int)cyw43_hal_ticks_ms(), self->wlan_flow_control, self->wwd_sdpcm_packet_transmit_sequence_number, self->wwd_sdpcm_last_bus_data_credit);

        uint32_t start_us = cyw43_hal_ticks_us();
        uint32_t last_poke = start_us - 100000;
        for (;;) {
            uint32_t cur_us = cyw43_hal_ticks_us();
            if (cur_us - last_poke >= 100000) {
                CYW43_VDEBUG("STALL(%u;%u-%u): do poke at %u us\n", self->wlan_flow_control, self->wwd_sdpcm_packet_transmit_sequence_number, self->wwd_sdpcm_last_bus_data_credit, (unsigned int)(cur_us - start_us));
                last_poke = cur_us;
                #if !CYW43_USE_SPI
                cyw43_write_backplane(self, SDIO_TO_SB_MAILBOX, 4, 1 << 3);
                #endif
            }
            size_t res_len;
            uint8_t *res_buf;
            int ret = cyw43_ll_sdpcm_poll_device(self, &res_len, &res_buf);
            if (ret != -1) {
                // CYW43_WARN("STALL(%u;%u-%u): got %d, %u\n", self->wlan_flow_control, self->wwd_sdpcm_packet_transmit_sequence_number, self->wwd_sdpcm_last_bus_data_credit, ret, res_len);
            }
            /*
            if (ret == CONTROL_HEADER) {
                // it seems that res_len is always the length of the argument in buf
                memmove(buf, res_buf, len < res_len ? len : res_len);
                return;
            } else*/
            if (ret == ASYNCEVENT_HEADER) {
                cyw43_cb_process_async_event(self, cyw43_ll_parse_async_event(res_len, res_buf));
            } else if (ret == DATA_HEADER) {
                // Don't process it due to possible reentrancy issues (eg sending another ETH as part of the reception)
                // printf("STALL: not processing ethernet packet\n");
                // cyw43_tcpip_process_ethernet(self, res_len, res_buf);
            } else if (ret >= 0) {
                // printf("cyw43_do_ioctl: got unexpected packet %d\n", ret);
            }
            if (!self->wlan_flow_control && self->wwd_sdpcm_last_bus_data_credit != self->wwd_sdpcm_packet_transmit_sequence_number) {
                // CYW43_WARN("STALL(%u;%u-%u): done in %u us\n", self->wlan_flow_control, self->wwd_sdpcm_packet_transmit_sequence_number, self->wwd_sdpcm_last_bus_data_credit, (unsigned int)(cur_us - start_us));
                break;
            }
            if (cur_us - start_us > 1000000) {
                CYW43_WARN("STALL(%u;%u-%u): timeout\n", self->wlan_flow_control, self->wwd_sdpcm_packet_transmit_sequence_number, self->wwd_sdpcm_last_bus_data_credit);
                return CYW43_FAIL_FAST_CHECK(-CYW43_ETIMEDOUT);
            }
            CYW43_SDPCM_SEND_COMMON_WAIT;
        }
    }

    size_t size = SDPCM_HEADER_LEN + len;

    // create header
    struct sdpcm_header_t *header = (void *)&buf[0];
    header->size = size;
    header->size_com = ~size & 0xffff;
    header->sequence = self->wwd_sdpcm_packet_transmit_sequence_number;
    header->channel_and_flags = kind;
    header->next_length = 0;
    header->header_length = SDPCM_HEADER_LEN + (kind == DATA_HEADER ? 2 : 0);
    header->wireless_flow_control = 0;
    header->bus_data_credit = 0;
    header->reserved[0] = 0;
    header->reserved[1] = 0;

    self->wwd_sdpcm_packet_transmit_sequence_number += 1;

    // padding is taken from junk at end of buffer
    return cyw43_write_bytes(self, WLAN_FUNCTION, 0, CYW43_WRITE_BYTES_PAD(size), buf);
}

struct ioctl_header_t {
    uint32_t cmd;
    uint32_t len; // lower 16 is output len; upper 16 is input len
    uint32_t flags;
    uint32_t status;
};

#define CASE_RETURN_STRING(value) case value: \
        return # value;

#if CYW43_VERBOSE_DEBUG
static const char *ioctl_cmd_name(int id) {
    switch (id)
    {
        CASE_RETURN_STRING(WLC_UP)
        CASE_RETURN_STRING(WLC_SET_INFRA)
        CASE_RETURN_STRING(WLC_SET_AUTH)
        CASE_RETURN_STRING(WLC_GET_BSSID)
        CASE_RETURN_STRING(WLC_GET_SSID)
        CASE_RETURN_STRING(WLC_SET_SSID)
        CASE_RETURN_STRING(WLC_SET_CHANNEL)
        CASE_RETURN_STRING(WLC_DISASSOC)
        CASE_RETURN_STRING(WLC_GET_ANTDIV)
        CASE_RETURN_STRING(WLC_SET_ANTDIV)
        CASE_RETURN_STRING(WLC_SET_DTIMPRD)
        CASE_RETURN_STRING(WLC_GET_PM)
        CASE_RETURN_STRING(WLC_SET_PM)
        CASE_RETURN_STRING(WLC_SET_GMODE)
        CASE_RETURN_STRING(WLC_SET_WSEC)
        CASE_RETURN_STRING(WLC_SET_BAND)
        CASE_RETURN_STRING(WLC_GET_ASSOCLIST)
        CASE_RETURN_STRING(WLC_SET_WPA_AUTH)
        CASE_RETURN_STRING(WLC_SET_VAR)
        CASE_RETURN_STRING(WLC_GET_VAR)
        CASE_RETURN_STRING(WLC_SET_WSEC_PMK)
        default:
            assert(false);
            return "unknown";
    }
}
#endif

static int cyw43_send_ioctl(cyw43_int_t *self, uint32_t kind, uint32_t cmd, size_t len, const uint8_t *buf, uint32_t iface) {
    if (SDPCM_HEADER_LEN + 16 + len > sizeof(self->spid_buf)) {
        return CYW43_FAIL_FAST_CHECK(-CYW43_EINVAL);
    }

    self->wwd_sdpcm_requested_ioctl_id += 1;
    uint32_t flags = ((((uint32_t)self->wwd_sdpcm_requested_ioctl_id) << CDCF_IOC_ID_SHIFT) & CDCF_IOC_ID_MASK)
        | kind | (iface << CDCF_IOC_IF_SHIFT);

    // create header
    struct ioctl_header_t *header = (void *)&self->spid_buf[SDPCM_HEADER_LEN];
    header->cmd = cmd;
    header->len = len & 0xffff;
    header->flags = flags;
    header->status = 0;

    // copy in payload
    memmove(self->spid_buf + SDPCM_HEADER_LEN + 16, buf, len);

    // do transfer
    CYW43_VDEBUG("Sending cmd %s (%u) len %u flags %u status %u\n", ioctl_cmd_name(header->cmd), header->cmd, header->len, header->flags, header->status);
    if (header->cmd == WLC_SET_VAR || header->cmd == WLC_GET_VAR) {
        CYW43_VDEBUG("%s %s\n", ioctl_cmd_name(header->cmd), (const char *)buf);
    }
    return cyw43_sdpcm_send_common(self, CONTROL_HEADER, 16 + len, self->spid_buf);
}

struct sdpcm_bdc_header_t {
    uint8_t flags;
    uint8_t priority;
    uint8_t flags2;
    uint8_t data_offset;
};

int cyw43_ll_send_ethernet(cyw43_ll_t *self_in, int itf, size_t len, const void *buf, bool is_pbuf) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    if (SDPCM_HEADER_LEN + 2 + sizeof(struct sdpcm_bdc_header_t) + len > sizeof(self->spid_buf)) {
        return CYW43_FAIL_FAST_CHECK(-CYW43_EINVAL);
    }

    // create header
    // there are 2 bytes of padding after the sdpcm header,
    // corresponding to the +2 in cyw43_sdpcm_send_common for DATA_HEADER
    struct sdpcm_bdc_header_t *header = (void *)&self->spid_buf[SDPCM_HEADER_LEN + 2];
    header->flags = 0x20;
    header->priority = 0;
    header->flags2 = itf;
    header->data_offset = 0;

    // copy in payload
    if (is_pbuf) {
        pbuf_copy_partial((const struct pbuf *)buf, self->spid_buf + SDPCM_HEADER_LEN + 6, len, 0);
    } else {
        memmove(self->spid_buf + SDPCM_HEADER_LEN + 6, buf, len);
    }

    // do transfer
    return cyw43_sdpcm_send_common(self, DATA_HEADER, 6 + len, self->spid_buf);
}

static int sdpcm_process_rx_packet(cyw43_int_t *self, uint8_t *buf, size_t *out_len, uint8_t **out_buf) {
    const struct sdpcm_header_t *header = (const void *)buf;
    if (header->size != (~header->size_com & 0xffff)) {
        // invalid packet, just ignore it
        CYW43_DEBUG("Ignoring invalid packet\n");
        return -2;
    }
    if (header->size < SDPCM_HEADER_LEN) {
        // packet too small, just ignore it
        CYW43_DEBUG("Ignoring too small packet\n");
        return -3;
    }

    #ifndef NDEBUG
    // Update flow control
    if (self->wlan_flow_control != header->wireless_flow_control) {
        CYW43_DEBUG("WLAN: changed flow control: %d -> %d\n", self->wlan_flow_control, header->wireless_flow_control);
    }
    #endif
    self->wlan_flow_control = header->wireless_flow_control;

    if ((header->channel_and_flags & 0x0f) < 3) {
        // a valid header, check the bus data credit
        uint8_t credit = header->bus_data_credit - self->wwd_sdpcm_last_bus_data_credit;
        if (credit <= 20) {
            self->wwd_sdpcm_last_bus_data_credit = header->bus_data_credit;
        }
    }

    if (header->size == SDPCM_HEADER_LEN) {
        // flow control packet with no data
        CYW43_DEBUG("Ignoring flow control packet\n");
        return -4;
    }

    switch (header->channel_and_flags & 0x0f) {
        case CONTROL_HEADER: {
            if (header->size < SDPCM_HEADER_LEN + IOCTL_HEADER_LEN) {
                // packet too small, just ignore it
                CYW43_DEBUG("Ignoring too small control packet\n");
                return -5;
            }
            const struct ioctl_header_t *ioctl_header = (const void *)&buf[header->header_length];

            // TODO need to handle errors and pass them up
            // if (ioctl_header->status != 0 || (ioctl_header->flags & 0xffff) != 0)
            // printf("CTRL HDR %lx %lx %lx %d\n", ioctl_header->cmd, ioctl_header->len, ioctl_header->flags, 2000 - ioctl_header->status);

            uint16_t id = (ioctl_header->flags & CDCF_IOC_ID_MASK) >> CDCF_IOC_ID_SHIFT;
            if (id != self->wwd_sdpcm_requested_ioctl_id) {
                // id doesn't match the last one sent, just ignore it
                CYW43_DEBUG("Ignoring packet with wrong id %d != %d\n", id, self->wwd_sdpcm_requested_ioctl_id);
                return -6;
            }
            // TODO extract and check/use the interface number from ioctl_header->flags
            // at this point the packet matches the last request sent and can be processed
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wcast-qual"
            uint8_t *payload = (uint8_t *)ioctl_header + IOCTL_HEADER_LEN;
            size_t len = header->size - ((const uint8_t *)payload - (const uint8_t *)header);
            #pragma GCC diagnostic pop
            *out_len = len;
            *out_buf = payload;
            CYW43_VDEBUG("got ioctl response id=0x%x len=%d\n", id, len);
            return CONTROL_HEADER;
        }
        case DATA_HEADER: {
            if (header->size <= SDPCM_HEADER_LEN + BDC_HEADER_LEN) {
                // packet too small, just ignore it
                CYW43_DEBUG("Ignoring too small data packet\n");
                return -7;
            }
            // get bdc header
            const struct sdpcm_bdc_header_t *bdc_header = (const void *)&buf[header->header_length];
            // get the interface number
            int itf = bdc_header->flags2;
            // get payload (skip variable length header)
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wcast-qual"
            uint8_t *payload = (uint8_t *)bdc_header + BDC_HEADER_LEN + (bdc_header->data_offset << 2);
            size_t len = header->size - ((const uint8_t *)payload - (const uint8_t *)header);
            #pragma GCC diagnostic pop

            // at this point we have just the payload, ready to process
            *out_len = len | (uint32_t)itf << 31;
            *out_buf = payload;
            return DATA_HEADER;
        }
        case ASYNCEVENT_HEADER: {
            if (header->size <= SDPCM_HEADER_LEN + BDC_HEADER_LEN) {
                // packet too small, just ignore it
                CYW43_DEBUG("Ignoring too small async packet\n");
                return -8;
            }
            // get bdc header
            const struct sdpcm_bdc_header_t *bdc_header = (const void *)&buf[header->header_length];
            // get payload (skip variable length header)
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wcast-qual"
            uint8_t *payload = (uint8_t *)bdc_header + BDC_HEADER_LEN + (bdc_header->data_offset << 2);
            size_t len = header->size - ((const uint8_t *)payload - (const uint8_t *)header);
            #pragma GCC diagnostic pop

            // payload is actually an ethernet packet with type 0x886c
            if (!(payload[12] == 0x88 && payload[13] == 0x6c)) {
                // ethernet packet doesn't have the correct type
                // Note - this happens during startup but appears to be expected
                CYW43_VDEBUG("wrong payload type 0x%02x 0x%02x\n", payload[12], payload[13]);
                return CYW43_ERROR_WRONG_PAYLOAD_TYPE;
            }

            // check the Broadcom OUI
            if (!(payload[19] == 0x00 && payload[20] == 0x10 && payload[21] == 0x18)) {
                // incorrect OUI
                CYW43_DEBUG("incorrect oui\n");
                return -10;
            }

            // const struct wwd_event_header_t *event = (const void*)&payload[24];
            *out_len = len - 24;
            *out_buf = payload + 24;

            CYW43_VDEBUG("async header header size=%d header length=%d out_len=%d\n", header->size, header->header_length, *out_len);
            return ASYNCEVENT_HEADER;
        }
        default: {
            // unknown header, just ignore it
            CYW43_DEBUG("unknown header\n");
            return -11;
        }
    }
}

#if !CYW43_USE_SPI // SDIO version follows

static int cyw43_ll_sdpcm_poll_device(cyw43_int_t *self, size_t *len, uint8_t **buf) {
    // First check the SDIO interrupt line to see if the WLAN notified us
    if (!self->had_successful_packet && cyw43_cb_read_host_interrupt_pin(self->cb_data) != host_interrupt_pin_active) {
        return -1;
    }

    cyw43_ll_bus_sleep((void *)self, false);

    #if CYW43_CLEAR_SDIO_INT
    if (!self->had_successful_packet) {
        // Clear interrupt status so that HOST_WAKE/SDIO line is cleared
        CYW43_VDEBUG("Reading SDIO_INT_STATUS\n");
        uint32_t int_status = cyw43_read_backplane(self, SDIO_INT_STATUS, 4);
        if (int_status & I_HMB_SW_MASK) {
            CYW43_STAT_INC(SDIO_INT_CLEAR);
            CYW43_VDEBUG("Clearing SDIO_INT_STATUS 0x%x\n", (int)(int_status & 0xf0));
            cyw43_write_backplane(self, SDIO_INT_STATUS, 4, int_status & 0xf0);
        }
    }
    #endif

    uint16_t hdr[2];
    cyw43_read_bytes(self, WLAN_FUNCTION, 0, 4, (void *)hdr);
    if (hdr[0] == 0 && hdr[1] == 0) {
        // no packets
        self->had_successful_packet = false;
        return -1;
    }
    self->had_successful_packet = true;
    if ((hdr[0] ^ hdr[1]) != 0xffff) {
        CYW43_WARN("error: hdr mismatch %04x ^ %04x\n", hdr[0], hdr[1]);
        return -1;
    }
    size_t sz_align = hdr[0] - 4;
    if (sz_align <= 8) {
        sz_align = (sz_align + 7) & ~7;
    } else if (sz_align <= 16) {
        sz_align = (sz_align + 15) & ~15;
    } else {
        sz_align = (sz_align + 63) & ~63;
    }
    cyw43_read_bytes(self, WLAN_FUNCTION, 0, sz_align, self->spid_buf + 4);
    memcpy(self->spid_buf, hdr, 4);

    return sdpcm_process_rx_packet(self, self->spid_buf, len, buf);
}

#else // SPI version follows

static int cyw43_ll_sdpcm_poll_device(cyw43_int_t *self, size_t *len, uint8_t **buf) {
    // First check the SDIO interrupt line to see if the WLAN notified us
    if (!self->had_successful_packet && cyw43_cb_read_host_interrupt_pin(self->cb_data) != host_interrupt_pin_active) {
        return -1;
    }

    cyw43_ll_bus_sleep((void *)self, false);

    if (!self->had_successful_packet) {
        // Clear interrupt status so that HOST_WAKE/SDIO line is cleared
        static uint16_t last_spi_int;

        uint16_t spi_int = cyw43_read_reg_u16(self, BUS_FUNCTION, SPI_INTERRUPT_REGISTER);
        if (last_spi_int != spi_int) {
            if (spi_int & BUS_OVERFLOW_UNDERFLOW) {
                CYW43_WARN("Bus error condition detected 0x%x\n", spi_int);
                CYW43_STAT_INC(BUS_ERROR);
                #if CYW43_USE_STATS
                assert(CYW43_STAT_GET(BUS_ERROR) < 100); // stop eventually
                #endif
            }
        }

        #if CYW43_CLEAR_SDIO_INT
        uint32_t sdio_int = cyw43_read_backplane(self, SDIO_INT_STATUS, 4);
        if (sdio_int & I_HMB_SW_MASK) {

            uint16_t f1_info_reg = cyw43_read_reg_u16(self, BUS_FUNCTION, SPI_FUNCTION1_INFO);
            CYW43_STAT_INC(SDIO_INT_CLEAR);

            if ((f1_info_reg & SPI_FUNCTIONX_READY) == 0) {
                logic_debug_set(pin_F1_NOT_READY, 1);
                logic_debug_set(pin_F1_NOT_READY, 0);
            }

            cyw43_write_backplane(self, SDIO_INT_STATUS, 4, sdio_int & I_HMB_SW_MASK);

            // Check for bus error
            uint16_t check_spi_int = cyw43_read_reg_u16(self, BUS_FUNCTION, SPI_INTERRUPT_REGISTER);

            // F1 overflow might start here, read back and check the status
            uint32_t check_sdio_int = cyw43_read_backplane(self, SDIO_INT_STATUS, 4);
            if (!(spi_int & BUS_OVERFLOW_UNDERFLOW) && (check_spi_int & BUS_OVERFLOW_UNDERFLOW)) {
                CYW43_WARN("Bus error condition detected from 0x%x to 0x%x with sdio from 0x%x to 0x%x\n",
                    spi_int, check_spi_int, sdio_int, check_sdio_int);
            }

            spi_int = check_spi_int;
        }
        #endif
        if (spi_int) {
            CYW43_STAT_INC(SPI_INT_CLEAR);
            cyw43_write_reg_u16(self, BUS_FUNCTION, SPI_INTERRUPT_REGISTER, spi_int);
        }
        last_spi_int = spi_int;
        if (!(spi_int & F2_PACKET_AVAILABLE)) {
            return -1;
        }
    }
    CYW43_STAT_INC(SPI_PACKET_AVAILABLE);

    // See whd_bus_spi_read_frame
    #define PAYLOAD_MTU 1500
    #define LINK_HEADER 30
    #define ETHERNET_SIZE 14
    #define LINK_MTU PAYLOAD_MTU + LINK_HEADER + ETHERNET_SIZE
    #define GSPI_PACKET_OVERHEAD 8
    uint32_t bus_gspi_status = 0;

    for (int i = 0; i < 1000; ++i) {
        bus_gspi_status = cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_STATUS_REGISTER);
        if (bus_gspi_status != 0xFFFFFFFF) {
            break;
        }
    }
    CYW43_VDEBUG("bus_gspi_status 0x%x 0x%x\n", bus_gspi_status, bus_gspi_status >> 9);
    if (bus_gspi_status == 0xFFFFFFFF) {
        return -1;
    }

    uint32_t bytes_pending = 0;
    if (bus_gspi_status & GSPI_PACKET_AVAILABLE) {
        bytes_pending = (bus_gspi_status >> 9) & 0x7FF;
        if (bytes_pending == 0 || bytes_pending > (LINK_MTU - GSPI_PACKET_OVERHEAD) ||
            bus_gspi_status & F2_F3_FIFO_RD_UNDERFLOW) {
            CYW43_DEBUG("SPI invalid bytes pending %" PRIu32 "\n", bytes_pending);
            cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SPI_FRAME_CONTROL, (1 << 0));
            self->had_successful_packet = false;
            return -1;
        }
    } else {
        // No packet
        CYW43_VDEBUG("No packet\n");
        self->had_successful_packet = false;
        return -1;
    }
    int ret = cyw43_read_bytes(self, WLAN_FUNCTION, 0, bytes_pending, self->spid_buf);
    if (ret != 0) {
        return ret;
    }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-align"
    uint16_t *hdr = (uint16_t *)self->spid_buf;
    #pragma GCC diagnostic pop
    if (hdr[0] == 0 && hdr[1] == 0) {
        // no packets
        CYW43_DEBUG("No packet zero size header\n");
        self->had_successful_packet = false;
        return -1;
    }
    self->had_successful_packet = true;
    if ((hdr[0] ^ hdr[1]) != 0xffff) {
        CYW43_WARN("error: hdr mismatch %04x ^ %04x\n", hdr[0], hdr[1]);
        return -1;
    }
    return sdpcm_process_rx_packet(self, self->spid_buf, len, buf);
}

#endif

void cyw43_ll_process_packets(cyw43_ll_t *self_in) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    for (;;) {
        size_t len;
        uint8_t *buf;
        int ret = cyw43_ll_sdpcm_poll_device(self, &len, &buf);
        if (ret == -1) {
            // no packet
            break;
        } else if (ret == -4) {
            // flow control
        } else if (ret == ASYNCEVENT_HEADER) {
            cyw43_cb_process_async_event(self, cyw43_ll_parse_async_event(len, buf));
        } else if (ret == DATA_HEADER) {
            cyw43_cb_process_ethernet(self->cb_data, len >> 31, len & 0x7fffffff, buf);
        } else if (CYW43_USE_SPI && ret == CYW43_ERROR_WRONG_PAYLOAD_TYPE) {
            // Ignore this error when using the SPI interface.  It can occur when there
            // is a lot of traffic over the SPI (eg sending UDP packets continuously)
            // and seems to be harmless.
            CYW43_VDEBUG("got wrong payload type for packet\n");
        } else {
            CYW43_DEBUG("got unexpected packet %d\n", ret);
        }
    }
}

// will read the ioctl from buf
// will then write the result (max len bytes) into buf
static int cyw43_do_ioctl(cyw43_int_t *self, uint32_t kind, uint32_t cmd, size_t len, uint8_t *buf, uint32_t iface) {
    int ret = cyw43_send_ioctl(self, kind, cmd, len, buf, iface);
    if (ret != 0) {
        return ret;
    }
    uint32_t start = cyw43_hal_ticks_us();
    while (cyw43_hal_ticks_us() - start < CYW43_IOCTL_TIMEOUT_US) {
        size_t res_len;
        uint8_t *res_buf;
        ret = cyw43_ll_sdpcm_poll_device(self, &res_len, &res_buf);
        if (ret == CONTROL_HEADER) {
            #if CYW43_USE_STATS
            uint32_t time_us = cyw43_hal_ticks_us() - start;
            if (time_us > CYW43_STAT_GET(LONGEST_IOCTL_TIME)) {
                CYW43_STAT_SET(LONGEST_IOCTL_TIME, time_us);
            }
            #endif
            // it seems that res_len is always the length of the argument in buf
            memmove(buf, res_buf, len < res_len ? len : res_len);
            return 0;
        } else if (ret == ASYNCEVENT_HEADER) {
            cyw43_cb_process_async_event(self, cyw43_ll_parse_async_event(res_len, res_buf));
        } else if (ret == DATA_HEADER) {
            cyw43_cb_process_ethernet(self->cb_data, res_len >> 31, res_len & 0x7fffffff, res_buf);
        } else if (ret >= 0) {
            CYW43_WARN("do_ioctl: got unexpected packet %d\n", ret);
        }
        CYW43_DO_IOCTL_WAIT;
    }
    CYW43_WARN("do_ioctl(%u, %u, %u): timeout\n", (unsigned int)kind, (unsigned int)cmd, (unsigned int)len);
    return CYW43_FAIL_FAST_CHECK(-CYW43_ETIMEDOUT);
}

int cyw43_ll_ioctl(cyw43_ll_t *self_in, uint32_t cmd, size_t len, uint8_t *buf, uint32_t iface) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    return cyw43_do_ioctl(self, cmd & 1 ? SDPCM_SET : SDPCM_GET, cmd >> 1, len, buf, iface);
}

/*******************************************************************************/
// high-level stuff

#define CORE_WLAN_ARM (1)
#define CORE_SOCRAM (2)

static uint32_t get_core_address(int core_id) {
    if (core_id == CORE_WLAN_ARM) {
        return WLAN_ARMCM3_BASE_ADDRESS + WRAPPER_REGISTER_OFFSET;
    } else if (core_id == CORE_SOCRAM) {
        return SOCSRAM_BASE_ADDRESS + WRAPPER_REGISTER_OFFSET;
    } else {
        return 0;
    }
}

static int disable_device_core(cyw43_int_t *self, int core_id, bool core_halt) {
    uint32_t base = get_core_address(core_id);
    cyw43_read_backplane(self, base + AI_RESETCTRL_OFFSET, 1);
    uint32_t reg = cyw43_read_backplane(self, base + AI_RESETCTRL_OFFSET, 1);
    if (reg & AIRC_RESET) {
        // core already in reset
        return 0;
    }
    // TODO
    (void)core_halt;
    CYW43_WARN("core not in reset\n");
    return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
}

static void reset_device_core(cyw43_int_t *self, int core_id, bool core_halt) {
    disable_device_core(self, core_id, core_halt);
    uint32_t base = get_core_address(core_id);
    cyw43_write_backplane(self, base + AI_IOCTRL_OFFSET, 1, SICF_FGC | SICF_CLOCK_EN | (core_halt ? SICF_CPUHALT : 0));
    cyw43_read_backplane(self, base + AI_IOCTRL_OFFSET, 1);
    cyw43_write_backplane(self, base + AI_RESETCTRL_OFFSET, 1, 0);
    cyw43_delay_ms(1);
    cyw43_write_backplane(self, base + AI_IOCTRL_OFFSET, 1, SICF_CLOCK_EN | (core_halt ? SICF_CPUHALT : 0));
    cyw43_read_backplane(self, base + AI_IOCTRL_OFFSET, 1);
    cyw43_delay_ms(1);
}

static void device_core_is_up(cyw43_int_t *self, int core_id) {
    uint32_t base = get_core_address(core_id);
    uint32_t reg = cyw43_read_backplane(self, base + AI_IOCTRL_OFFSET, 1);
    if ((reg & (SICF_FGC | SICF_CLOCK_EN)) != SICF_CLOCK_EN) {
        CYW43_WARN("core not up\n");
    }
    reg = cyw43_read_backplane(self, base + AI_RESETCTRL_OFFSET, 1);
    if (reg & AIRC_RESET) {
        CYW43_WARN("core not up\n");
    }
    // if we get here then the core is up
    CYW43_VDEBUG("core %d IS up\n", core_id);
}

#if USE_KSO
// KSO mode (keep SDIO on)
static void cyw43_kso_set(cyw43_int_t *self, int value) {
    uint8_t write_value = 0;
    if (value) {
        write_value = SBSDIO_SLPCSR_KEEP_SDIO_ON;
    }
    // these can fail and it's still ok
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_SLEEP_CSR, write_value);
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_SLEEP_CSR, write_value);

    uint8_t compare_value, bmask;
    if (value) {
        // device WAKEUP through KSO:
        // write bit 0 & read back until
        // both bits 0(kso bit) & 1 (dev on status) are set
        compare_value = SBSDIO_SLPCSR_KEEP_SDIO_ON | SBSDIO_SLPCSR_DEVICE_ON;
        bmask = compare_value;
    } else {
        // Put device to sleep, turn off KSO
        compare_value = 0;
        // Check for bit0 only, bit1(devon status) may not get cleared right away
        bmask = SBSDIO_SLPCSR_KEEP_SDIO_ON;
    }

    for (int i = 0; i < 64; ++i) {
        // Reliable KSO bit set/clr:
        // Sdiod sleep write access appears to be in sync with PMU 32khz clk
        // just one write attempt may fail, (same is with read ?)
        // in any case, read it back until it matches written value
        // this can fail and it's still ok
        int read_value = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_SLEEP_CSR);
        if (read_value >= 0 && ((read_value & bmask) == compare_value) && read_value != 0xff) {
            return; // success
        }

        cyw43_delay_ms(1);

        cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_SLEEP_CSR, write_value);
    }

    CYW43_WARN("cyw43_kso_set(%d): failed\n", value);
}
#endif

static void cyw43_ll_bus_sleep_helper(cyw43_int_t *self, bool can_sleep) {
    #if USE_KSO

    cyw43_kso_set(self, !can_sleep);

    #else

    if (can_sleep) {
        // clear request for HT
        cyw43_write_reg(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, 1, 0);
        return;
    }

    // make sure HT is available
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, SBSDIO_HT_AVAIL_REQ);
    // cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, SBSDIO_HT_AVAIL_REQ);
    for (int i = 0; i < 1000; ++i) {
        int reg = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR);
        if (reg >= 0 && (reg & SBSDIO_HT_AVAIL)) {
            return;
        }
        cyw43_delay_ms(1);
        // cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, SBSDIO_HT_AVAIL_REQ);
    }

    CYW43_WARN("could not bring bus up\n");

    #endif
}

void cyw43_ll_bus_sleep(cyw43_ll_t *self_in, bool can_sleep) {
    cyw43_int_t *self = (void *)self_in;
    if (can_sleep) {
        if (!self->bus_is_up) {
            return;
        }
        CYW43_STAT_INC(SLEEP_COUNT);
        CYW43_VDEBUG("sleep bus\n");
        self->bus_is_up = false;
        cyw43_ll_bus_sleep_helper(self, true);
    } else {
        cyw43_cb_ensure_awake(self);
        if (self->bus_is_up) {
            return;
        }
        CYW43_STAT_INC(WAKE_COUNT);
        CYW43_VDEBUG("wake bus\n");
        cyw43_ll_bus_sleep_helper(self, false);
        self->bus_is_up = true;
    }
}

#if CYW43_USE_SPI
#define CLM_CHUNK_LEN 1024
#else
#define CLM_CHUNK_LEN 1024 + 512
#endif

static void cyw43_clm_load(cyw43_int_t *self, const uint8_t *clm_ptr, size_t clm_len) {
    // Reuse spid_buf but be careful to start at the right offset in it
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    const size_t clm_dload_chunk_len = CLM_CHUNK_LEN;
    for (size_t off = 0; off < clm_len; off += clm_dload_chunk_len) {
        CYW43_EVENT_POLL_HOOK;

        uint32_t len = clm_dload_chunk_len;
        uint16_t flag = 1 << 12; // DLOAD_HANDLER_VER
        if (off == 0) {
            flag |= 2; // DL_BEGIN
        }
        if (off + len >= clm_len) {
            flag |= 4; // DL_END
            len = clm_len - off;
        }
        memcpy(buf, "clmload\x00", 8);
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-align"
        static_assert(!(offsetof(cyw43_int_t, spid_buf) & 3), "");
        *(uint16_t *)(buf + 8) = flag;
        *(uint16_t *)(buf + 10) = 2;
        *(uint32_t *)(buf + 12) = len;
        *(uint32_t *)(buf + 16) = 0;
        #pragma GCC diagnostic pop
        memcpy(buf + 20, clm_ptr + off, len);

        CYW43_VDEBUG("clm data send %u/%u\n", off + len, clm_len);

        // Send data aligned to 8 bytes; padding comes from junk at end of buf
        cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_VAR, ALIGN_UINT(20 + len, 8), buf, WWD_STA_INTERFACE);
    }

    CYW43_VDEBUG("clm data send done\n");
    // Check the status of the download
    memcpy(buf, "clmload_status\x00\x00\x00\x00\x00", 19);
    cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 19, buf, WWD_STA_INTERFACE);
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-align"
    if (*(uint32_t *)buf != 0) {
        CYW43_WARN("CLM load failed\n");
    }
    #pragma GCC diagnostic pop
    CYW43_VDEBUG("clm data load ok\n");
}

static void cyw43_write_iovar_u32(cyw43_int_t *self, const char *var, uint32_t val, uint32_t iface) {
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    size_t len = strlen(var) + 1;
    memcpy(buf, var, len);
    cyw43_put_le32(buf + len, val);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_VAR, len + 4, buf, iface);
}

static void cyw43_write_iovar_u32_u32(cyw43_int_t *self, const char *var, uint32_t val0, uint32_t val1, uint32_t iface) {
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    size_t len = strlen(var) + 1;
    memcpy(buf, var, len);
    cyw43_put_le32(buf + len, val0);
    cyw43_put_le32(buf + len + 4, val1);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_VAR, len + 8, buf, iface);
}

// buf may point anywhere in self->spid_buf (or elsewhere)
static void cyw43_write_iovar_n(cyw43_int_t *self, const char *var, size_t len, const void *buf, uint32_t iface) {
    uint8_t *iobuf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    size_t varlen = strlen(var) + 1;
    memmove(iobuf + varlen, buf, len);
    memcpy(iobuf, var, varlen);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_VAR, varlen + len, iobuf, iface);
}

int cyw43_ll_bus_init(cyw43_ll_t *self_in, const uint8_t *mac) {
    cyw43_int_t *self = (void *)self_in;

    self->startup_t0 = cyw43_hal_ticks_us();

    #if CYW43_USE_SPI

    bool success = false;
    do {
        uint32_t val = WORD_LENGTH_32 | ENDIAN_BIG | HIGH_SPEED_MODE | WAKE_UP |
            0x4 << (8 * SPI_RESPONSE_DELAY) | INTR_WITH_STATUS << (8 * SPI_STATUS_ENABLE);
        #if defined(CYW43_PIN_WL_HOST_WAKE)
        val |= INTERRUPT_POLARITY_HIGH;
        #endif

        // Initialise
        if (cyw43_spi_init(self) != 0) {
            CYW43_DEBUG("Failed to initialise cyw43\n");
            break;
        }

        cyw43_spi_gpio_setup();
        CYW43_EVENT_POLL_HOOK;
        cyw43_spi_reset();
        CYW43_EVENT_POLL_HOOK;

        // Check test register can be read
        for (int i = 0; i < 10; ++i) {
            uint32_t reg = read_reg_u32_swap(self, BUS_FUNCTION, SPI_READ_TEST_REGISTER);
            if (reg == TEST_PATTERN) {
                goto chip_up;
            }
            cyw43_delay_ms(1);
        }
        CYW43_DEBUG("Failed to read test pattern\n");
        break; // failed
    chip_up:
        // Switch to 32bit mode
        CYW43_VDEBUG("setting SPI_BUS_CONTROL 0x%x\n", val);

        if (write_reg_u32_swap(self, BUS_FUNCTION, SPI_BUS_CONTROL, val) != 0) {
            break;
        }

        val = cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_BUS_CONTROL);
        CYW43_VDEBUG("read SPI_BUS_CONTROL 0x%x\n", val);

        if (cyw43_write_reg_u8(self, BUS_FUNCTION, SPI_RESP_DELAY_F1, CYW43_BACKPLANE_READ_PAD_LEN_BYTES) != 0) {
            break;
        }

        // Make sure error interrupt bits are clear
        if (cyw43_write_reg_u8(self, BUS_FUNCTION, SPI_INTERRUPT_REGISTER,
            DATA_UNAVAILABLE | COMMAND_ERROR | DATA_ERROR | F1_OVERFLOW) != 0) {
            break;
        }

        // Enable a selection of interrupts
        uint16_t cyw43_interrupts = F2_F3_FIFO_RD_UNDERFLOW | F2_F3_FIFO_WR_OVERFLOW |
            COMMAND_ERROR | DATA_ERROR | F2_PACKET_AVAILABLE | F1_OVERFLOW;
        #if CYW43_ENABLE_BLUETOOTH
        cyw43_interrupts |= F1_INTR;
        #endif
        if (cyw43_write_reg_u16(self, BUS_FUNCTION, SPI_INTERRUPT_ENABLE_REGISTER, cyw43_interrupts) != 0) {
            break;
        }

        success = true;
    } while (false);
    if (!success) {
        CYW43_WARN("Failed to start CYW43\n");
        return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
    }

    #else

    // enumerate SDIO bus
    cyw43_sdio_transfer(0, 0, NULL); // ignore any errors
    cyw43_sdio_transfer(5, 0, NULL); // ignore any errors

    // get RCA
    uint32_t rca;
    if (cyw43_sdio_transfer(3, 0, &rca) != 0) {
        CYW43_WARN("SDIO enumerate error\n");
        return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
    }

    // select the card with RCA
    if (cyw43_sdio_transfer(7, rca, NULL) != 0) {
        CYW43_WARN("SDIO select error\n");
        return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
    }

    // set up backplane
    for (int i = 0; i < 100; ++i) {
        cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_IOEN, SDIO_FUNC_ENABLE_1);
        if (i != 0) {
            cyw43_delay_ms(1);
        }
        uint32_t reg = cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_IOEN);
        if (reg == SDIO_FUNC_ENABLE_1) {
            goto backplane_up;
        }
    }
    CYW43_WARN("no response from CYW43\n");
    return -CYW43_EIO;

backplane_up:
    CYW43_VDEBUG("backplane is up\n");

    // set the bus to 4-bits
    // (we don't need to change our local SDIO config until we need cmd53)
    uint32_t reg = cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_BICTRL);
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_BICTRL, (reg & ~3) | 2);

    // set the block size
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_BLKSIZE_0, SDIO_64B_BLOCK);
    reg = cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_BLKSIZE_0);
    if (reg != SDIO_64B_BLOCK) {
        CYW43_WARN("can't set block size\n");
        return -CYW43_EIO;
    }

    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_BLKSIZE_0, SDIO_64B_BLOCK);
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_F1BLKSIZE_0, SDIO_64B_BLOCK);
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_F2BLKSIZE_0, SDIO_64B_BLOCK);
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_F2BLKSIZE_1, 0);

    // Enable/Disable Client interrupts
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_INTEN, INTR_CTL_MASTER_EN | INTR_CTL_FUNC1_EN | INTR_CTL_FUNC2_EN);

    // enable more than 25MHz bus
    reg = cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_SPEED_CONTROL);
    assert(reg & 1); // device must support high-speed mode
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_SPEED_CONTROL, reg | 2);

    // enable high speed, 4-bit bus mode for local SDIO controller
    cyw43_sdio_enable_high_speed_4bit();

    // wait for backplane to be ready
    for (int i = 0; i < 10; ++i) {
        reg = cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_IORDY);
        if ((reg & SDIO_FUNC_READY_1) != 0) {
            goto backplane_ready;
        }
        cyw43_delay_ms(1);
    }
    CYW43_WARN("timeout waiting for backplane\n");
    return -CYW43_EIO;
backplane_ready:

    #endif

    CYW43_VDEBUG("backplane is ready\n");

    // set the ALP
    #if !CYW43_USE_SPI
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_ALP_AVAIL_REQ | SBSDIO_FORCE_ALP);
    #else
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, SBSDIO_ALP_AVAIL_REQ);
    #endif

    #if CYW43_ENABLE_BLUETOOTH
    // check we can set the watermark
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_FUNCTION2_WATERMARK, 0x10);
    uint8_t reg_8 = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_FUNCTION2_WATERMARK);
    if (reg_8 != 0x10) {
        return -CYW43_EIO;
    }
    #endif

    for (int i = 0; i < 10; ++i) {
        uint8_t reg = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR);
        if (reg & SBSDIO_ALP_AVAIL) {
            goto alp_set;
        }
        cyw43_delay_ms(1);
    }
    CYW43_WARN("timeout waiting for ALP to be set\n");
    return -CYW43_EIO;

alp_set:
    CYW43_VDEBUG("ALP is set\n");

    // clear request for ALP
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, 0);

    #if !CYW43_USE_SPI
    // Enable F1 and F2
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_IOEN, SDIO_FUNC_ENABLE_1 | SDIO_FUNC_ENABLE_2);

    // here we can configure power saving and OOB interrupt

    // Enable active-low OOB interrupt (or value with SEP_INTR_CTL_POL to make it active high)
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_SEP_INT_CTL, SEP_INTR_CTL_MASK | SEP_INTR_CTL_EN);

    // Enable F2 interrupt only
    cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_INTEN, INTR_CTL_MASTER_EN | INTR_CTL_FUNC2_EN);

    cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_IORDY);
    #endif

    // START OF DOWNLOAD_FIRMWARE

    disable_device_core(self, CORE_WLAN_ARM, false);
    disable_device_core(self, CORE_SOCRAM, false);
    reset_device_core(self, CORE_SOCRAM, false);

    // this is 4343x specific stuff: Disable remap for SRAM_3
    cyw43_write_backplane(self, SOCSRAM_BANKX_INDEX, 4, 0x3);
    cyw43_write_backplane(self, SOCSRAM_BANKX_PDA, 4, 0);

    // Check that valid chipset firmware exists at the given source address.
    int ret = cyw43_check_valid_chipset_firmware(self, CYW43_WIFI_FW_LEN, fw_data);
    if (ret != 0) {
        return ret;
    }

    // Download the main WiFi firmware blob to the 43xx device.
    ret = cyw43_download_resource(self, 0x00000000, CYW43_WRITE_BYTES_PAD(CYW43_WIFI_FW_LEN), fw_data);
    if (ret != 0) {
        return ret;
    }

    // Download the NVRAM to the 43xx device.
    size_t wifi_nvram_len = CYW43_WRITE_BYTES_PAD(sizeof(wifi_nvram_4343));
    const uint8_t *wifi_nvram_data = wifi_nvram_4343;
    cyw43_download_resource(self, CYW43_RAM_SIZE - 4 - wifi_nvram_len, wifi_nvram_len, (uintptr_t)wifi_nvram_data);
    uint32_t sz = ((~(wifi_nvram_len / 4) & 0xffff) << 16) | (wifi_nvram_len / 4);
    cyw43_write_backplane(self, CYW43_RAM_SIZE - 4, 4, sz);

    reset_device_core(self, CORE_WLAN_ARM, false);
    device_core_is_up(self, CORE_WLAN_ARM);

    // wait until HT clock is available; takes about 29ms
    for (int i = 0; i < 1000; ++i) {
        uint32_t reg = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR);
        if (reg & SBSDIO_HT_AVAIL) {
            goto ht_ready;
        }
        cyw43_delay_ms(1);
    }
    CYW43_WARN("HT not ready\n");
    return -CYW43_EIO;

ht_ready:

    // Set up the interrupt mask and enable interrupts
    cyw43_write_backplane(self, SDIO_INT_HOST_MASK, 4, I_HMB_SW_MASK);

    #if !CYW43_USE_SPI
    // Enable F1 and F2 interrupts. This wasn't required for 4319 but is for the 43362
    cyw43_write_backplane(self, SDIO_FUNCTION_INT_MASK, 1, 2 | 1);

    // Lower F2 Watermark to avoid DMA Hang in F2 when SD Clock is stopped
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_FUNCTION2_WATERMARK, SDIO_F2_WATERMARK);
    #else

    #if CYW43_ENABLE_BLUETOOTH
    // Set up the interrupt mask and enable interrupts
    cyw43_write_backplane(self, SDIO_INT_HOST_MASK, 4, I_HMB_FC_CHANGE);
    #endif

    /* Lower F2 Watermark to avoid DMA Hang in F2 when SD Clock is stopped. */
    cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_FUNCTION2_WATERMARK, SPI_F2_WATERMARK);
    #endif

    // END OF DOWNLOAD_FIRMWARE

    for (int i = 0; i < 1000; ++i) {
        #if CYW43_USE_SPI
        // Wait for F2 to be ready
        uint32_t reg = cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_STATUS_REGISTER);
        if (reg & STATUS_F2_RX_READY) {
            goto f2_ready;
        }
        #else
        uint32_t reg = cyw43_read_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_IORDY);
        if (reg & SDIO_FUNC_READY_2) {
            goto f2_ready;
        }
        #endif
        cyw43_delay_ms(1);
    }
    CYW43_WARN("F2 not ready\n");
    return -CYW43_EIO;

f2_ready:

    #if USE_KSO
    {
        uint8_t reg8 = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_WAKEUP_CTRL);
        reg8 |= SBSDIO_WCTRL_WAKE_TILL_HT_AVAIL;
        cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_WAKEUP_CTRL, reg8);
        cyw43_write_reg_u8(self, BUS_FUNCTION, SDIOD_CCCR_BRCM_CARDCAP, SDIOD_CCCR_BRCM_CARDCAP_CMD_NODEC);
        cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_CHIP_CLOCK_CSR, SBSDIO_FORCE_HT);
        reg8 = cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_SLEEP_CSR);
        if (!(reg8 & SBSDIO_SLPCSR_KEEP_SDIO_ON)) {
            reg8 |= SBSDIO_SLPCSR_KEEP_SDIO_ON;
            cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_SLEEP_CSR, reg8);
        }

        // TODO: This causes the device to fail after sleep. Check if whd_bus_spi_sleep is ever called
        #if 0 && CYW43_USE_SPI
        // whd_bus_spi_sleep
        uint32_t spi_bus_reg_value = cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_BUS_CONTROL);
        spi_bus_reg_value &= ~(uint32_t)(WAKE_UP);
        cyw43_write_reg_u32(self, BUS_FUNCTION, SPI_BUS_CONTROL, spi_bus_reg_value);
        #endif
        // Put SPI interface block to sleep
        cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_PULL_UP, 0xf);
    }
    #endif

    // CLEAR PAD PULLS
    {
        cyw43_write_reg_u8(self, BACKPLANE_FUNCTION, SDIO_PULL_UP, 0);
        cyw43_read_reg_u8(self, BACKPLANE_FUNCTION, SDIO_PULL_UP);
    }

    #if CYW43_USE_SPI
    {
        // We always seem to start with a data unavailable error - so clear it now
        uint16_t spi_int_status = cyw43_read_reg_u16(self, BUS_FUNCTION, SPI_INTERRUPT_REGISTER);
        if (spi_int_status & DATA_UNAVAILABLE) {
            cyw43_write_reg_u16(self, BUS_FUNCTION, SPI_INTERRUPT_REGISTER, spi_int_status);
        }

    }
    #endif

    #ifdef NDEBUG
    // This will be a non-zero value if save/restore is enabled
    cyw43_read_backplane(self, CHIPCOMMON_SR_CONTROL1, 4);
    #endif

    cyw43_ll_bus_sleep(self_in, false);

    // Load the CLM data; it sits just after main firmware
    CYW43_VDEBUG("cyw43_clm_load start\n");
    cyw43_clm_load(self, (const uint8_t *)CYW43_CLM_ADDR, CYW43_CLM_LEN);
    CYW43_VDEBUG("cyw43_clm_load done\n");

    cyw43_write_iovar_u32(self, "bus:txglom", 0, WWD_STA_INTERFACE); // tx glomming off
    cyw43_write_iovar_u32(self, "apsta", 1, WWD_STA_INTERFACE); // apsta on

    // Set the MAC (same one used for STA and AP interfaces)
    // If CYW43_USE_OTP_MAC is set then cyw43 otp should be used to store the mac and no mac is passed in.
    // When the chip hs been initialised we can read the otp
    // If it looks like otp is unset the call cyw43_hal_generate_laa_mac to get the "port" to make a mac address for us
    #if CYW43_USE_OTP_MAC
    uint8_t mac_buf[6];
    if (!mac) {
        const uint8_t default_mac[] = { 0x00, 0xA0, 0x50, 0xb5, 0x59, 0x5e };

        // Check the if the mac is set in otp. The default mac from nvram will be used if not
        int err = cyw43_ll_wifi_get_mac(self_in, mac_buf);
        if (err != 0 || memcmp(mac_buf, default_mac, sizeof(mac_buf)) == 0) {
            // mac not set in otp, so make one up
            cyw43_hal_generate_laa_mac(WWD_STA_INTERFACE, mac_buf);
            mac = mac_buf;
        }
    }
    #endif

    if (mac) {
        cyw43_write_iovar_n(self, "cur_etheraddr", 6, mac, WWD_STA_INTERFACE);
    }

    return 0;
}

/*******************************************************************************/
// WiFi stuff

static void cyw43_set_ioctl_u32(cyw43_int_t *self, uint32_t cmd, uint32_t val, uint32_t iface) {
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    cyw43_put_le32(buf, val);
    cyw43_do_ioctl(self, SDPCM_SET, cmd, 4, buf, iface);
}

static uint32_t cyw43_get_ioctl_u32(cyw43_int_t *self, uint32_t cmd, uint32_t iface) {
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    cyw43_put_le32(buf, 0);
    cyw43_do_ioctl(self, SDPCM_GET, cmd, 4, buf, iface);
    return cyw43_get_le32(buf);
}

static uint32_t cyw43_read_iovar_u32(cyw43_int_t *self, const char *var, uint32_t iface) {
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    size_t len = strlen(var) + 1;
    memcpy(buf, var, len);
    cyw43_put_le32(buf + len, 0);
    cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, len + 4, buf, iface);
    return cyw43_get_le32(buf);
}

#if 0
#define WLC_SET_MONITOR (108)
int cyw43_set_monitor_mode(cyw43_ll_t *self, int value) {
    CYW_THREAD_ENTER;
    int ret = cyw43_ensure_up(self);
    if (ret) {
        CYW_THREAD_EXIT;
        return ret;
    }

    CYW_ENTER;
    self->is_monitor_mode = value;
    cyw43_write_iovar_u32(self, "allmulti", value, WWD_STA_INTERFACE);
    cyw43_set_ioctl_u32(self, WLC_SET_MONITOR, value, WWD_STA_INTERFACE);
    CYW_EXIT;
    CYW_THREAD_EXIT;

    return 0;
}
#endif

// Requires cyw43_ll_bus_init to have been called first
int cyw43_ll_wifi_on(cyw43_ll_t *self_in, uint32_t country) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    #if !CYW43_USE_SPI
    if (country == CYW43_COUNTRY_WORLDWIDE) {
        // For the 1DX CLM, we need to use revision 17 for the worldwide
        // country. This increases throughput by about 25% on PYBD.
        country |= (17 << 16);
    }
    #endif

    // Set country; takes about 32ms
    memcpy(buf, "country\x00", 8);
    cyw43_put_le32(buf + 8, country & 0xffff);
    if ((country >> 16) == 0) {
        cyw43_put_le32(buf + 12, (uint32_t)-1);
    } else {
        cyw43_put_le32(buf + 12, country >> 16);
    }
    cyw43_put_le32(buf + 16, country & 0xffff);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_VAR, 20, buf, WWD_STA_INTERFACE);

    cyw43_delay_ms(50);

    #ifndef NDEBUG
    // Get and print CLM version
    memcpy(buf, "clmver\x00", 7);
    cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 128, buf, WWD_STA_INTERFACE);
    CYW43_DEBUG("%s\n", buf);
    #endif

    // Set antenna to chip antenna
    cyw43_set_ioctl_u32(self, WLC_SET_ANTDIV, 0, WWD_STA_INTERFACE);

    // Set some WiFi config
    cyw43_write_iovar_u32(self, "bus:txglom", 0, WWD_STA_INTERFACE); // tx glomming off
    cyw43_write_iovar_u32(self, "apsta", 1, WWD_STA_INTERFACE); // apsta on
    cyw43_write_iovar_u32(self, "ampdu_ba_wsize", 8, WWD_STA_INTERFACE);
    cyw43_write_iovar_u32(self, "ampdu_mpdu", 4, WWD_STA_INTERFACE);
    cyw43_write_iovar_u32(self, "ampdu_rx_factor", 0, WWD_STA_INTERFACE);

    // This delay is needed for the WLAN chip to do some processing, otherwise
    // SDIOIT/OOB WL_HOST_WAKE IRQs in bus-sleep mode do no work correctly.
    uint32_t dt = cyw43_hal_ticks_us() - self->startup_t0;
    if (dt < 150000) {
        cyw43_delay_us(150000 - dt);
    }

    // Clear all async events
    memset(buf + 18 + 4, 0xff, 19); // enable them all
    #define CLR_EV(b, i) b[18 + 4 + i / 8] &= ~(1 << (i % 8))
    CLR_EV(buf, 19); // roam attempt occurred
    CLR_EV(buf, 20); // tx fail
    CLR_EV(buf, 40); // radio
    CLR_EV(buf, 44); // probe request
    CLR_EV(buf, 54); // interface change
    CLR_EV(buf, 71); // probe response
    #undef CLR_EV
    memcpy(buf, "bsscfg:event_msgs", 18);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_VAR, 18 + 4 + 19, buf, WWD_STA_INTERFACE);

    cyw43_delay_ms(50);

    // Set the interface as "up"
    cyw43_do_ioctl(self, SDPCM_SET, WLC_UP, 0, NULL, WWD_STA_INTERFACE);

    cyw43_delay_ms(50);

    return 0;
}

int cyw43_ll_wifi_get_mac(cyw43_ll_t *self_in, uint8_t *addr) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    // Get mac address
    memcpy(buf, "cur_etheraddr\x00\x00\x00\x00\x00\x00\x00", 14 + 6);
    int err = cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 14 + 6, buf, WWD_STA_INTERFACE);
    if (err == 0) {
        memcpy(addr, buf, 6);
    }
    return err;
}

int cyw43_ll_wifi_update_multicast_filter(cyw43_ll_t *self_in, uint8_t *addr, bool add) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    // query the current list
    memcpy(buf, "mcast_list", 11);
    memset(buf + 11, 0, 4 + MAX_MULTICAST_REGISTERED_ADDRESS * 6);
    int err = cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 11 + 4 + MAX_MULTICAST_REGISTERED_ADDRESS * 6, buf, WWD_STA_INTERFACE);
    if (err != 0) {
        return err;
    }

    // current number of addresses
    uint32_t n = cyw43_get_le32(buf);
    buf += 4;

    for (uint32_t i = 0; i < n; ++i) {
        if (memcmp(buf + i * 6, addr, 6) == 0) {
            if (add) {
                // addr already in the list
                return 0;
            } else {
                // remove this address
                if (i < n - 1) {
                    // replace with the end of the list
                    memcpy(buf + i * 6, buf + (n - 1) * 6, 6);
                }
                --n;
            }
        }
    }
    if (add) {
        if (n == MAX_MULTICAST_REGISTERED_ADDRESS) {
            return CYW43_FAIL_FAST_CHECK(-CYW43_EPERM);
        }
        memcpy(buf + n * 6, addr, 6);
        ++n;
    }

    // update number of addresses
    buf -= 4;
    cyw43_put_le32(buf, n);

    // write back address list
    cyw43_write_iovar_n(self, "mcast_list", 4 + MAX_MULTICAST_REGISTERED_ADDRESS * 6, buf, WWD_STA_INTERFACE);
    cyw43_delay_ms(50);

    return 0;
}

int cyw43_ll_wifi_pm(cyw43_ll_t *self_in, uint32_t pm, uint32_t pm_sleep_ret, uint32_t li_bcn, uint32_t li_dtim, uint32_t li_assoc) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    // set some power saving parameters
    // PM1 is very aggressive in power saving and reduces wifi throughput
    // PM2 only saves power when there is no wifi activity for some time

    // Value passed to pm2_sleep_ret measured in ms, must be multiple of 10, between 10 and 2000
    if (pm_sleep_ret < 1) {
        pm_sleep_ret = 1;
    } else if (pm_sleep_ret > 200) {
        pm_sleep_ret = 200;
    }
    cyw43_write_iovar_u32(self, "pm2_sleep_ret", pm_sleep_ret * 10, WWD_STA_INTERFACE);

    // these parameters set beacon intervals and are used to reduce power consumption
    // while associated to an AP but not doing tx/rx
    // bcn_li_xxx is what the CYW43x will do; assoc_listen is what is sent to the AP
    // bcn_li_dtim==0 means use bcn_li_bcn
    cyw43_write_iovar_u32(self, "bcn_li_bcn", li_bcn, WWD_STA_INTERFACE);
    cyw43_write_iovar_u32(self, "bcn_li_dtim", li_dtim, WWD_STA_INTERFACE);
    cyw43_write_iovar_u32(self, "assoc_listen", li_assoc, WWD_STA_INTERFACE);

    #if 0
    CYW43_PRINTF("pm2_sleep_ret: %lu\n", cyw43_read_iovar_u32(self, "pm2_sleep_ret", WWD_STA_INTERFACE));
    CYW43_PRINTF("bcn_li_bcn: %lu\n", cyw43_read_iovar_u32(self, "bcn_li_bcn", WWD_STA_INTERFACE));
    CYW43_PRINTF("bcn_li_dtim: %lu\n", cyw43_read_iovar_u32(self, "bcn_li_dtim", WWD_STA_INTERFACE));
    CYW43_PRINTF("assoc_listen: %lu\n", cyw43_read_iovar_u32(self, "assoc_listen", WWD_STA_INTERFACE));
    #endif

    cyw43_set_ioctl_u32(self, WLC_SET_PM, pm, WWD_STA_INTERFACE);

    // Set GMODE_AUTO
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];
    cyw43_put_le32(buf, 1);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_GMODE, 4, buf, WWD_STA_INTERFACE);

    cyw43_put_le32(buf, 0); // any
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_BAND, 4, buf, WWD_STA_INTERFACE);

    return 0;
}

int cyw43_ll_wifi_get_pm(cyw43_ll_t *self_in, uint32_t *pm, uint32_t *pm_sleep_ret, uint32_t *li_bcn, uint32_t *li_dtim, uint32_t *li_assoc) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    *pm_sleep_ret = cyw43_read_iovar_u32(self, "pm2_sleep_ret", WWD_STA_INTERFACE);
    *li_bcn = cyw43_read_iovar_u32(self, "bcn_li_bcn", WWD_STA_INTERFACE);
    *li_dtim = cyw43_read_iovar_u32(self, "bcn_li_dtim", WWD_STA_INTERFACE);
    *li_assoc = cyw43_read_iovar_u32(self, "assoc_listen", WWD_STA_INTERFACE);
    *pm = cyw43_get_ioctl_u32(self, WLC_GET_PM, WWD_STA_INTERFACE);
    return 0;
}

int cyw43_ll_wifi_scan(cyw43_ll_t *self_in, cyw43_wifi_scan_options_t *opts) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    opts->version = 1; // ESCAN_REQ_VERSION
    opts->action = 1; // WL_SCAN_ACTION_START
    opts->_ = 0;
    memset(opts->bssid, 0xff, sizeof(opts->bssid));
    opts->bss_type = 2; // WICED_BSS_TYPE_ANY
    opts->nprobes = -1;
    opts->active_time = -1;
    opts->passive_time = -1;
    opts->home_time = -1;
    opts->channel_num = 0;
    opts->channel_list[0] = 0;
    cyw43_write_iovar_n(self, "escan", sizeof(cyw43_wifi_scan_options_t), opts, WWD_STA_INTERFACE);

    return 0;
}

int cyw43_ll_wifi_join(cyw43_ll_t *self_in, size_t ssid_len, const uint8_t *ssid, size_t key_len, const uint8_t *key, uint32_t auth_type, const uint8_t *bssid, uint32_t channel) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    uint8_t buf[2 + CYW43_WPA_SAE_MAX_PASSWORD_LEN];

    cyw43_write_iovar_u32(self, "ampdu_ba_wsize", 8, WWD_STA_INTERFACE);

    uint32_t wpa_auth = 0;
    if (auth_type == CYW43_AUTH_OPEN) {
        wpa_auth = 0;
    } else if (auth_type == CYW43_AUTH_WPA2_AES_PSK || auth_type == CYW43_AUTH_WPA2_MIXED_PSK) {
        wpa_auth = CYW43_WPA2_AUTH_PSK;
    } else if (auth_type == CYW43_AUTH_WPA_TKIP_PSK) {
        wpa_auth = CYW43_WPA_AUTH_PSK;
    } else if (auth_type == CYW43_AUTH_WPA3_SAE_AES_PSK || auth_type == CYW43_AUTH_WPA3_WPA2_AES_PSK) {
        wpa_auth = CYW43_WPA3_AUTH_SAE_PSK;
    } else {
        // Unsupported auth_type (security) value.
        return -CYW43_EINVAL;
    }

    // Check key length
    if (auth_type != CYW43_AUTH_OPEN && auth_type != CYW43_AUTH_WPA3_SAE_AES_PSK && key_len > CYW43_WPA_MAX_PASSWORD_LEN) {
        return -CYW43_EINVAL;
    } else if (auth_type == CYW43_AUTH_WPA3_SAE_AES_PSK && key_len > CYW43_WPA_SAE_MAX_PASSWORD_LEN) {
        return -CYW43_EINVAL;
    }

    CYW43_VDEBUG("Setting wsec=0x%x\n", auth_type & 0xff);
    cyw43_set_ioctl_u32(self, WLC_SET_WSEC, auth_type & 0xff, WWD_STA_INTERFACE);

    // supplicant variable
    CYW43_VDEBUG("Setting sup_wpa=%d\n", auth_type == 0 ? 0 : 1);
    cyw43_write_iovar_u32_u32(self, "bsscfg:sup_wpa", 0, auth_type == 0 ? 0 : 1, WWD_STA_INTERFACE);

    // set the EAPOL version to whatever the AP is using (-1)
    CYW43_VDEBUG("Setting sup_wpa2_eapver\n");
    cyw43_write_iovar_u32_u32(self, "bsscfg:sup_wpa2_eapver", 0, -1, WWD_STA_INTERFACE);

    // wwd_wifi_set_supplicant_eapol_key_timeout
    CYW43_VDEBUG("Setting sup_wpa_tmo %d\n", CYW_EAPOL_KEY_TIMEOUT);
    cyw43_write_iovar_u32_u32(self, "bsscfg:sup_wpa_tmo", 0, CYW_EAPOL_KEY_TIMEOUT, WWD_STA_INTERFACE);

    if (auth_type != CYW43_AUTH_OPEN && auth_type != CYW43_AUTH_WPA3_SAE_AES_PSK) {
        // wwd_wifi_set_passphrase
        cyw43_put_le16(buf, key_len);
        cyw43_put_le16(buf + 2, 1);
        memcpy(buf + 4, key, key_len);
        cyw43_delay_ms(2); // Delay required to allow radio firmware to be ready to receive PMK and avoid intermittent failure

        CYW43_VDEBUG("Setting wsec_pmk %d\n", key_len);
        cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_WSEC_PMK, 4 + CYW43_WPA_MAX_PASSWORD_LEN, buf, WWD_STA_INTERFACE); // 68, see wsec_pmk_t
    }

    if (wpa_auth == CYW43_WPA3_AUTH_SAE_PSK) {
        memset(buf, 0, 2 + CYW43_WPA_SAE_MAX_PASSWORD_LEN);
        cyw43_put_le16(buf, key_len);
        memcpy(buf + 2, key, key_len);
        cyw43_delay_ms(2); // Delay required to allow radio firmware to be ready to receive PMK and avoid intermittent failure
        cyw43_write_iovar_n(self, "sae_password", 2 + CYW43_WPA_SAE_MAX_PASSWORD_LEN, buf, WWD_STA_INTERFACE);
    }

    // set infrastructure mode
    CYW43_VDEBUG("Setting infra\n");
    cyw43_set_ioctl_u32(self, WLC_SET_INFRA, 1, WWD_STA_INTERFACE);

    // set auth type
    CYW43_VDEBUG("Setting auth\n");
    cyw43_set_ioctl_u32(self, WLC_SET_AUTH, (wpa_auth == CYW43_WPA3_AUTH_SAE_PSK) ? AUTH_TYPE_SAE : AUTH_TYPE_OPEN, WWD_STA_INTERFACE);
    cyw43_write_iovar_u32(self, "mfp", (wpa_auth == CYW43_WPA2_AUTH_PSK || wpa_auth == CYW43_WPA3_AUTH_SAE_PSK) ? MFP_CAPABLE : MFP_NONE, WWD_STA_INTERFACE);

    // set WPA auth mode
    CYW43_VDEBUG("Setting wpa auth 0x%x\n", wpa_auth);
    cyw43_set_ioctl_u32(self, WLC_SET_WPA_AUTH, wpa_auth, WWD_STA_INTERFACE);

    // allow relevant events through:
    //  EV_SET_SSID=0
    //  EV_AUTH=3
    //  EV_DEAUTH_IND=6
    //  EV_DISASSOC_IND=12
    //  EV_LINK=16
    //  EV_PSK_SUP=46
    //  EV_ESCAN_RESULT=69
    //  EV_CSA_COMPLETE_IND=80
    /*
    memcpy(buf, "\x00\x00\x00\x00" "\x49\x10\x01\x00\x00\x40\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00", 4 + 18);
    cyw43_write_iovar_n(self, "bsscfg:event_msgs", 4 + 18, buf, WWD_STA_INTERFACE);
    */

    cyw43_put_le32(self->last_ssid_joined, ssid_len);
    memcpy(self->last_ssid_joined + 4, ssid, ssid_len);

    if (bssid) {
        memset(buf, 0, 4 + 32 + 20 + 14);
        cyw43_put_le32(buf, ssid_len);
        memcpy(buf + 4, ssid, ssid_len);

        // scan params
        buf[4 + 32] = 0; // scan_type
        cyw43_put_le32(buf + 4 + 32 + 4, -1); // nprobes
        cyw43_put_le32(buf + 4 + 32 + 8, -1); // active_time
        cyw43_put_le32(buf + 4 + 32 + 12, -1); // passive_time
        cyw43_put_le32(buf + 4 + 32 + 16, -1); // home_time

        // assoc params
        #define WL_CHANSPEC_BW_20        0x1000
        #define WL_CHANSPEC_CTL_SB_LLL      0x0000
        #define WL_CHANSPEC_CTL_SB_NONE     WL_CHANSPEC_CTL_SB_LLL
        #define WL_CHANSPEC_BAND_2G        0x0000
        memcpy(buf + 4 + 32 + 20, bssid, 6);
        if (channel != CYW43_CHANNEL_NONE) {
            cyw43_put_le32(buf + 4 + 32 + 20 + 8, 1); // chanspec_num
            uint16_t chspec = channel | WL_CHANSPEC_BW_20 | WL_CHANSPEC_CTL_SB_NONE | WL_CHANSPEC_BAND_2G;
            cyw43_put_le16(buf + 4 + 32 + 20 + 12, chspec); // chanspec_list
        }

        // join the AP
        CYW43_VDEBUG("Join AP\n");
        cyw43_write_iovar_n(self, "join", 4 + 32 + 20 + 14, buf, WWD_STA_INTERFACE);
    } else {
        // join SSID
        CYW43_VDEBUG("Set ssid\n");
        cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_SSID, 36, self->last_ssid_joined, WWD_STA_INTERFACE);
    }

    return 0;
}

void cyw43_ll_wifi_set_wpa_auth(cyw43_ll_t *self_in) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    cyw43_set_ioctl_u32(self, WLC_SET_WPA_AUTH, CYW43_WPA_AUTH_PSK, WWD_STA_INTERFACE);
}

void cyw43_ll_wifi_rejoin(cyw43_ll_t *self_in) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_SSID, 36, self->last_ssid_joined, WWD_STA_INTERFACE);
}

int cyw43_ll_wifi_get_bssid(cyw43_ll_t *self_in, uint8_t *bssid) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    return cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_BSSID, 6, bssid, WWD_STA_INTERFACE);
}

/*******************************************************************************/
// WiFi AP

int cyw43_ll_wifi_ap_init(cyw43_ll_t *self_in, size_t ssid_len, const uint8_t *ssid, uint32_t auth, size_t key_len, const uint8_t *key, uint32_t channel) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    // Get state of AP
    // TODO: this can fail with sdpcm status = 0xffffffe2 (NOTASSOCIATED)
    // in such a case the AP is not up and we should not check the result
    memcpy(buf, "bss\x00", 4);
    cyw43_put_le32(buf + 4, WWD_AP_INTERFACE);
    cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 8, buf, WWD_STA_INTERFACE);
    uint32_t res = cyw43_get_le32(buf); // le or be?

    if (res) {
        // already up ...
        return 0;
    }

    // Check key length
    if (auth != CYW43_AUTH_OPEN && auth != CYW43_AUTH_WPA3_SAE_AES_PSK && key_len > CYW43_WPA_MAX_PASSWORD_LEN) {
        return -CYW43_EINVAL;
    } else if (auth == CYW43_AUTH_WPA3_SAE_AES_PSK && key_len > CYW43_WPA_SAE_MAX_PASSWORD_LEN) {
        return -CYW43_EINVAL;
    }

    // set the AMPDU parameter for AP (window size = 2)
    cyw43_write_iovar_u32(self, "ampdu_ba_wsize", 2, WWD_STA_INTERFACE);

    // set SSID
    cyw43_put_le32(buf, WWD_AP_INTERFACE);
    cyw43_put_le32(buf + 4, ssid_len);
    memset(buf + 8, 0, 32);
    memcpy(buf + 8, ssid, ssid_len);
    cyw43_write_iovar_n(self, "bsscfg:ssid", 4 + 4 + 32, buf, WWD_STA_INTERFACE);

    // set channel
    cyw43_set_ioctl_u32(self, WLC_SET_CHANNEL, channel, WWD_STA_INTERFACE);

    // set security type
    cyw43_write_iovar_u32_u32(self, "bsscfg:wsec", WWD_AP_INTERFACE, auth, WWD_STA_INTERFACE);

    // set management frame protection
    uint32_t auth_mfp = MFP_NONE;
    if (auth == CYW43_AUTH_WPA3_SAE_AES_PSK) {
        auth_mfp = MFP_REQUIRED;
    } else if (auth == CYW43_AUTH_WPA3_WPA2_AES_PSK) {
        auth_mfp = MFP_CAPABLE;
    }
    cyw43_write_iovar_u32(self, "mfp", auth_mfp, WWD_AP_INTERFACE);

    if (auth == CYW43_AUTH_OPEN) {
        // nothing to do
    } else {
        // set WPA/WPA2 auth params
        uint32_t val = 0;
        if (auth == CYW43_AUTH_WPA_TKIP_PSK) {
            val = CYW43_WPA_AUTH_PSK;
        } else if (auth == CYW43_AUTH_WPA2_AES_PSK || auth == CYW43_AUTH_WPA2_MIXED_PSK) {
            val = CYW43_WPA_AUTH_PSK | CYW43_WPA2_AUTH_PSK;
        } else if (auth == CYW43_AUTH_WPA3_SAE_AES_PSK) {
            val = CYW43_WPA3_AUTH_SAE_PSK;
        } else if (auth == CYW43_AUTH_WPA3_WPA2_AES_PSK) {
            val = CYW43_WPA3_AUTH_SAE_PSK | CYW43_WPA2_AUTH_PSK;
        }
        assert(val);
        cyw43_write_iovar_u32_u32(self, "bsscfg:wpa_auth", WWD_AP_INTERFACE, val, WWD_STA_INTERFACE);

        // set password
        if (val & CYW43_WPA3_AUTH_SAE_PSK) {
            cyw43_put_le16(buf, key_len);
            memset(buf + 2, 0, CYW43_WPA_SAE_MAX_PASSWORD_LEN);
            memcpy(buf + 2, key, key_len);
            cyw43_delay_ms(2); // Delay required to allow radio firmware to be ready to receive PMK and avoid intermittent failure
            cyw43_write_iovar_n(self, "sae_password", 2 + CYW43_WPA_SAE_MAX_PASSWORD_LEN, buf, WWD_AP_INTERFACE);
        }
        if (val & (CYW43_WPA_AUTH_PSK | CYW43_WPA2_AUTH_PSK)) {
            cyw43_put_le16(buf, key_len);
            cyw43_put_le16(buf + 2, 1);
            memset(buf + 4, 0, CYW43_WPA_MAX_PASSWORD_LEN);
            memcpy(buf + 4, key, key_len);
            cyw43_delay_ms(2); // WICED has this
            cyw43_do_ioctl(self, SDPCM_SET, WLC_SET_WSEC_PMK, 2 + 2 + CYW43_WPA_MAX_PASSWORD_LEN, buf, WWD_AP_INTERFACE);
        }
    }

    if (auth == CYW43_AUTH_WPA3_SAE_AES_PSK || auth == CYW43_AUTH_WPA3_WPA2_AES_PSK) {
        cyw43_write_iovar_u32(self, "sae_max_pwe_loop", WWD_AP_INTERFACE, 5);
    }

    // set GMode to auto (value of 1)
    cyw43_set_ioctl_u32(self, WLC_SET_GMODE, 1, WWD_AP_INTERFACE);

    // set multicast tx rate to 11Mbps
    cyw43_write_iovar_u32(self, "2g_mrate", 11000000 / 500000, WWD_AP_INTERFACE);

    // set DTIM period
    cyw43_set_ioctl_u32(self, WLC_SET_DTIMPRD, 1, WWD_AP_INTERFACE);

    return 0;
}

int cyw43_ll_wifi_ap_set_up(cyw43_ll_t *self_in, bool up) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    cyw43_write_iovar_u32_u32(self, "bss", WWD_AP_INTERFACE, up, WWD_STA_INTERFACE);

    return 0;
}

int cyw43_ll_wifi_ap_get_stas(cyw43_ll_t *self_in, int *num_stas, uint8_t *macs) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);

    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    // Get max number of associated STAs
    memcpy(buf, "maxassoc\x00", 9);
    cyw43_put_le32(buf + 9, WWD_AP_INTERFACE);
    cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 9 + 4, buf, WWD_STA_INTERFACE);
    int err = cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 9 + 4, buf, WWD_STA_INTERFACE);
    if (err != 0) {
        return err;
    }
    uint32_t maxassoc = cyw43_get_le32(buf);
    uint32_t max_macs_buf = (sizeof(self->spid_buf) - SDPCM_HEADER_LEN - 16 - 4) / 6;
    maxassoc = MIN(maxassoc, max_macs_buf);

    if (macs == NULL) {
        // Return just the maximum number of STAs
        *num_stas = maxassoc;
        return 0;
    }

    // Make sure all MACs will fit in buffer; size specified in num_stas
    maxassoc = MIN(maxassoc, (uint32_t)*num_stas);

    // Get associated STAs
    cyw43_put_le32(buf, maxassoc);
    err = cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_ASSOCLIST, 4 + maxassoc * 6, buf, WWD_AP_INTERFACE);
    if (err != 0) {
        return err;
    }
    uint32_t stas_connected = cyw43_get_le32(buf);
    *num_stas = MIN(stas_connected, maxassoc);
    memcpy(macs, buf + 4, *num_stas * 6);
    return 0;
}

#if CYW43_GPIO

int cyw43_ll_gpio_set(cyw43_ll_t *self_in, int gpio_n, bool gpio_en) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    if (!(gpio_n >= 0 && gpio_n < CYW43_NUM_GPIOS)) {
        return -1;
    }
    CYW43_VDEBUG("cyw43_set_gpio %d=%d\n", gpio_n, gpio_en);
    cyw43_write_iovar_u32_u32(self, "gpioout", 1 << gpio_n, gpio_en ? (1 << gpio_n) : 0, WWD_STA_INTERFACE);
    return 0;
}

int cyw43_ll_gpio_get(cyw43_ll_t *self_in, int gpio_n, bool *gpio_en) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    uint8_t *buf = &self->spid_buf[SDPCM_HEADER_LEN + 16];

    if (!(gpio_n >= 0 && gpio_n < CYW43_NUM_GPIOS)) {
        return -1;
    }

    memcpy(buf, "ccgpioin\x00", 9);
    int err = cyw43_do_ioctl(self, SDPCM_GET, WLC_GET_VAR, 9, buf, WWD_STA_INTERFACE);
    if (err != 0) {
        return err;
    }

    *gpio_en = (cyw43_get_le32(buf) & (1 << gpio_n)) ? true : false;
    CYW43_VDEBUG("cyw43_get_gpio %d=%s\n", gpio_n, (gpio_en ? "true" : "false"));
    return 0;
}

#endif

// Is there work to do?
bool cyw43_ll_has_work(cyw43_ll_t *self_in) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    int int_pin = cyw43_cb_read_host_interrupt_pin(self->cb_data);
    return int_pin == host_interrupt_pin_active;
}

#if CYW43_ENABLE_BLUETOOTH

bool cyw43_ll_bt_has_work(cyw43_ll_t *self_in) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    uint32_t int_status = cyw43_read_backplane(self, SDIO_INT_STATUS, 4);
    if (int_status & I_HMB_FC_CHANGE) {
        cyw43_write_backplane(self, SDIO_INT_STATUS, 4, int_status & I_HMB_FC_CHANGE);
        return true;
    }
    return false;
}

void cyw43_ll_write_backplane_reg(cyw43_ll_t *self_in, uint32_t addr, uint32_t val) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    cyw43_write_backplane(self, addr, sizeof(uint32_t), val);
}

uint32_t cyw43_ll_read_backplane_reg(cyw43_ll_t *self_in, uint32_t addr) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    return cyw43_read_backplane(self, addr, sizeof(uint32_t));
}

int cyw43_ll_write_backplane_mem(cyw43_ll_t *self_in, uint32_t addr, uint32_t len, const uint8_t *buf) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    while (len > 0) {
        const uint32_t backplane_addr_start = addr & BACKPLANE_ADDR_MASK;
        const uint32_t backplane_addr_end = MIN(backplane_addr_start + len, BACKPLANE_ADDR_MASK + 1);
        const uint32_t backplane_len = backplane_addr_end - backplane_addr_start;
        cyw43_set_backplane_window(self, addr);
        int ret = cyw43_write_bytes(self, BACKPLANE_FUNCTION, backplane_addr_start | SBSDIO_SB_ACCESS_2_4B_FLAG, backplane_len, buf);
        if (ret != 0) {
            CYW43_PRINTF("backplane write 0x%lx,0x%lx failed", addr, backplane_len);
        }
        addr += backplane_len;
        buf += backplane_len;
        len -= backplane_len;
    }
    cyw43_set_backplane_window(self, CHIPCOMMON_BASE_ADDRESS);
    return 0;
}

int cyw43_ll_read_backplane_mem(cyw43_ll_t *self_in, uint32_t addr, uint32_t len, uint8_t *buf) {
    cyw43_int_t *self = CYW_INT_FROM_LL(self_in);
    assert(len <= CYW43_BUS_MAX_BLOCK_SIZE);
    assert(((addr & BACKPLANE_ADDR_MASK) + len) <= (BACKPLANE_ADDR_MASK + 1));
    cyw43_set_backplane_window(self, addr);
    int ret = cyw43_read_bytes(self, BACKPLANE_FUNCTION, (addr & BACKPLANE_ADDR_MASK) | SBSDIO_SB_ACCESS_2_4B_FLAG, len, buf);
    cyw43_set_backplane_window(self, CHIPCOMMON_BASE_ADDRESS);
    return ret;
}

#endif
