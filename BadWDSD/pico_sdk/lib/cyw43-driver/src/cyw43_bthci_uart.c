/*
 * This file is part of the cyw43-driver
 *
 * Copyright (C) 2019-2025 George Robotics Pty Ltd
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

// CYW43 Bluetooth HCI-over-UART low-level driver.

#include <assert.h>

#include "cyw43.h"

#if CYW43_ENABLE_BLUETOOTH_OVER_UART

// This firmware file defines btfw_data and btfw_len.
#include CYW43_BT_FIRMWARE_INCLUDE_FILE

#define HCI_CMD_HEADER_LEN (4)
#define HCI_RESP_HEADER_LEN (6)
#define HCI_PAYLOAD_LEN_MAX (256)

#ifndef CYW43_BT_UART_BAUDRATE_STARTUP
#define CYW43_BT_UART_BAUDRATE_STARTUP (115200)
#endif

#ifdef CYW43_PIN_BT_DEV_WAKE
#define CYW43_BT_SLEEP_TIMEOUT_MS (500)
static uint32_t cyw43_bluetooth_sleep_ticks;
#endif

#define CYW43_BT_WAIT_CTS_LOW_TIMEOUT_US (200000)
#define CYW43_BT_UART_READCHAR_BLOCKING_TIMEOUT_US (100000)

#if defined(CYW43_BT_UART_BAUDRATE_DOWNLOAD_FIRMWARE) || defined(CYW43_BT_UART_BAUDRATE_ACTIVE_USE)

static inline void cyw43_bluetooth_put_le16(uint8_t *buf, uint16_t val) {
    buf[0] = val;
    buf[1] = val >> 8;
}

static inline void cyw43_bluetooth_put_le32(uint8_t *buf, uint32_t val) {
    buf[0] = val;
    buf[1] = val >> 8;
    buf[2] = val >> 16;
    buf[3] = val >> 24;
}

#endif

static void cyw43_bluetooth_wait_cts_low(uint32_t timeout_us) {
    uint32_t start_us = cyw43_hal_ticks_us();
    while (cyw43_hal_pin_read(CYW43_PIN_BT_CTS) != 0) {
        if (cyw43_hal_ticks_us() - start_us > timeout_us) {
            // Timeout.
            break;
        }
        cyw43_delay_ms(1);
    }
}

#if !defined(cyw43_hal_uart_readchar_blocking)
static int cyw43_hal_uart_readchar_blocking(void) {
    uint32_t start_us = cyw43_hal_ticks_us();
    int c;
    while ((c = cyw43_hal_uart_readchar()) == -1) {
        if (cyw43_hal_ticks_us() - start_us > CYW43_BT_UART_READCHAR_BLOCKING_TIMEOUT_US) {
            // Timeout.
            return -CYW43_ETIMEDOUT;
        }
        CYW43_HAL_UART_READCHAR_BLOCKING_WAIT;
    }
    return c;
}
#endif

static int cyw43_bluetooth_hci_cmd_raw(uint8_t *cmd_buf, size_t param_len, const uint8_t *param_buf) {
    assert(param_len <= HCI_PAYLOAD_LEN_MAX);

    // Construct the full HCI packet, with command header and parameters.
    #if defined(cyw43_hal_uart_static_cmd_buf)
    uint8_t *buf = cyw43_hal_uart_static_cmd_buf;
    #else
    uint8_t buf[HCI_CMD_HEADER_LEN + HCI_PAYLOAD_LEN_MAX];
    #endif
    memcpy(buf, cmd_buf, HCI_CMD_HEADER_LEN);
    memcpy(buf + HCI_CMD_HEADER_LEN, param_buf, param_len);

    // Send the HCI command to the BT controller.
    cyw43_hal_uart_write((void *)buf, HCI_CMD_HEADER_LEN + param_len);

    // Read in the response header from the BT controller.
    for (size_t i = 0; i < HCI_RESP_HEADER_LEN; ++i) {
        int c = cyw43_hal_uart_readchar_blocking();
        if (c < 0) {
            return -CYW43_ETIMEDOUT;
        }
        buf[i] = c;
    }

    // Expect a command complete event (event 0x0e).
    if (buf[0] != 0x04 || buf[1] != 0x0e) {
        CYW43_WARN("cyw43_bluetooth_hci_cmd_raw: unknown response: %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
        return -CYW43_EIO;
    }

    // Read in the response payload from the BT controller.
    size_t sz = buf[2] - 3;
    for (size_t i = 0; i < sz; ++i) {
        int c = cyw43_hal_uart_readchar_blocking();
        if (c < 0) {
            return -CYW43_ETIMEDOUT;
        }
        // Store the first few bytes in the input command buffer, for the caller to retrieve.
        if (i < HCI_CMD_HEADER_LEN) {
            cmd_buf[i] = c;
        }
    }

    return sz;
}

static int cyw43_bluetooth_hci_cmd(int ogf, int ocf, size_t param_len, const uint8_t *param_buf) {
    uint8_t cmd_buf[4];
    cmd_buf[0] = 0x01;
    cmd_buf[1] = ocf;
    cmd_buf[2] = ogf << 2 | ocf >> 8;
    cmd_buf[3] = param_len;
    return cyw43_bluetooth_hci_cmd_raw(cmd_buf, param_len, param_buf);
}

#if defined(CYW43_BT_UART_BAUDRATE_DOWNLOAD_FIRMWARE) || defined(CYW43_BT_UART_BAUDRATE_ACTIVE_USE)
static int cyw43_bluetooth_hci_set_baudrate(uint32_t baudrate) {
    uint8_t param_buf[6];
    cyw43_bluetooth_put_le16(param_buf, 0);
    cyw43_bluetooth_put_le32(param_buf + 2, baudrate);
    return cyw43_bluetooth_hci_cmd(0x3f, 0x18, sizeof(param_buf), param_buf);
}
#endif

static int cyw43_bluetooth_download_firmware(const uint8_t *firmware, size_t firmware_len) {
    cyw43_bluetooth_hci_cmd(0x3f, 0x2e, 0, NULL);

    const uint8_t *firmware_max = firmware + firmware_len;
    bool last_packet = false;
    while (!last_packet && firmware < firmware_max) {
        // Validate firmware packet.
        if (firmware[1] != 0xfc) {
            CYW43_WARN("cyw43_bluetooth_download_firmware: unexpected packet %02x\n", firmware[1]);
            return -CYW43_EIO;
        }

        // Construct HCI header.
        uint8_t cmd_buf[4];
        memcpy(cmd_buf + 1, firmware, 3);
        firmware += 3;
        cmd_buf[0] = 1;
        last_packet = cmd_buf[1] == 0x4e;
        uint8_t len = cmd_buf[3];

        // Send out firmware packet.
        int ret = cyw43_bluetooth_hci_cmd_raw(cmd_buf, len, firmware);
        if (ret < 1 || cmd_buf[0] != 0) {
            CYW43_WARN("cyw43_bluetooth_download_firmware: unexpected response %02x\n", cmd_buf[0]);
            return -CYW43_EIO;
        }

        // Advance to the next firmware packet.
        firmware += len;
    }

    #ifdef CYW43_PIN_RFSW_SELECT
    // RF switch must select high path during BT patch boot.
    cyw43_hal_pin_config(CYW43_PIN_RFSW_SELECT, CYW43_HAL_PIN_MODE_INPUT, CYW43_HAL_PIN_PULL_UP, 0);
    #endif

    // Delay for a little to let the patch firmware reset and pull CTS high,
    // and then wait for it to finish booting, waiting for CTS to go low.
    cyw43_delay_ms(10);
    cyw43_bluetooth_wait_cts_low(400000);

    #ifdef CYW43_PIN_RFSW_SELECT
    // Select chip antenna (could also select external).
    cyw43_hal_pin_config(CYW43_PIN_RFSW_SELECT, CYW43_HAL_PIN_MODE_INPUT, CYW43_HAL_PIN_PULL_DOWN, 0);
    #endif

    return 0;
}

int cyw43_bluetooth_controller_init(void) {
    // Configure the BT control pins.
    cyw43_hal_pin_config(CYW43_PIN_BT_REG_ON, CYW43_HAL_PIN_MODE_OUTPUT, CYW43_HAL_PIN_PULL_NONE, 0);
    cyw43_hal_pin_low(CYW43_PIN_BT_REG_ON);
    #ifdef CYW43_PIN_BT_HOST_WAKE
    cyw43_hal_pin_config(CYW43_PIN_BT_HOST_WAKE, CYW43_HAL_PIN_MODE_INPUT, CYW43_HAL_PIN_PULL_NONE, 0);
    #endif
    #ifdef CYW43_PIN_BT_DEV_WAKE
    cyw43_hal_pin_config(CYW43_PIN_BT_DEV_WAKE, CYW43_HAL_PIN_MODE_OUTPUT, CYW43_HAL_PIN_PULL_NONE, 0);
    cyw43_hal_pin_low(CYW43_PIN_BT_DEV_WAKE);
    #endif

    #ifdef CYW43_PIN_RFSW_VDD
    // TODO don't select antenna if wifi is enabled
    cyw43_hal_pin_config(CYW43_PIN_RFSW_VDD, CYW43_HAL_PIN_MODE_OUTPUT, CYW43_HAL_PIN_PULL_NONE, 0); // RF-switch power
    cyw43_hal_pin_high(CYW43_PIN_RFSW_VDD); // Turn the RF-switch on
    #endif

    // Turn on the BT controller.
    cyw43_hal_pin_low(CYW43_PIN_BT_REG_ON);
    cyw43_hal_uart_set_baudrate(CYW43_BT_UART_BAUDRATE_STARTUP);
    cyw43_delay_ms(100);
    cyw43_hal_pin_high(CYW43_PIN_BT_REG_ON);
    cyw43_bluetooth_wait_cts_low(100000);

    // Reset.
    cyw43_bluetooth_hci_cmd(0x03, 0x0003, 0, NULL);

    #ifdef CYW43_BT_UART_BAUDRATE_DOWNLOAD_FIRMWARE
    // Change baudrate to download firmware.
    cyw43_bluetooth_hci_set_baudrate(CYW43_BT_UART_BAUDRATE_DOWNLOAD_FIRMWARE);
    cyw43_hal_uart_set_baudrate(CYW43_BT_UART_BAUDRATE_DOWNLOAD_FIRMWARE);
    #endif

    // Download the BT patch firmware.
    cyw43_bluetooth_download_firmware(btfw_data, btfw_len);

    #ifdef CYW43_BT_UART_BAUDRATE_DOWNLOAD_FIRMWARE
    // Change UART baudrate back to default after patch firmware boots.
    cyw43_hal_uart_set_baudrate(CYW43_BT_UART_BAUDRATE_STARTUP);
    #endif

    #ifdef CYW43_BT_UART_BAUDRATE_ACTIVE_USE
    // Change baudrate to increase speed for main use.
    cyw43_bluetooth_hci_set_baudrate(CYW43_BT_UART_BAUDRATE_ACTIVE_USE);
    cyw43_hal_uart_set_baudrate(CYW43_BT_UART_BAUDRATE_ACTIVE_USE);
    #endif

    // Reset.
    cyw43_bluetooth_hci_cmd(0x03, 0x0003, 0, NULL);

    // Set BD_ADDR (sent as little endian).
    uint8_t bdaddr[6];
    uint8_t bdaddr_le[6];
    cyw43_hal_get_mac(CYW43_HAL_MAC_BDADDR, bdaddr);
    bdaddr_le[0] = bdaddr[5];
    bdaddr_le[1] = bdaddr[4];
    bdaddr_le[2] = bdaddr[3];
    bdaddr_le[3] = bdaddr[2];
    bdaddr_le[4] = bdaddr[1];
    bdaddr_le[5] = bdaddr[0];
    cyw43_bluetooth_hci_cmd(0x3f, 0x0001, sizeof(bdaddr_le), bdaddr_le);

    // Configure sleep mode.
    cyw43_bluetooth_hci_cmd(0x3f, 0x27, 12, (const uint8_t *)"\x01\x02\x02\x00\x00\x00\x01\x00\x00\x00\x00\x00");

    // HCI_Write_LE_Host_Support.
    cyw43_bluetooth_hci_cmd(3, 109, 2, (const uint8_t *)"\x01\x00");

    #ifdef CYW43_PIN_BT_DEV_WAKE
    // Let the BT device sleep.
    cyw43_hal_pin_high(CYW43_PIN_BT_DEV_WAKE);
    #endif

    return 0;
}

int cyw43_bluetooth_controller_deinit(void) {
    cyw43_hal_pin_low(CYW43_PIN_BT_REG_ON);
    return 0;
}

int cyw43_bluetooth_controller_sleep_maybe(void) {
    #ifdef CYW43_PIN_BT_DEV_WAKE
    if (cyw43_hal_pin_read(CYW43_PIN_BT_DEV_WAKE) == 0) {
        if (cyw43_hal_ticks_ms() - cyw43_bluetooth_sleep_ticks > CYW43_BT_SLEEP_TIMEOUT_MS) {
            cyw43_hal_pin_high(CYW43_PIN_BT_DEV_WAKE); // let sleep
        }
    }
    #endif
    return 0;
}

bool cyw43_bluetooth_controller_woken(void) {
    #ifdef CYW43_PIN_BT_HOST_WAKE
    return !!cyw43_hal_pin_read(CYW43_PIN_BT_HOST_WAKE);
    #else
    return true;
    #endif
}

int cyw43_bluetooth_controller_wakeup(void) {
    #ifdef CYW43_PIN_BT_DEV_WAKE
    cyw43_bluetooth_sleep_ticks = cyw43_hal_ticks_ms();
    if (cyw43_hal_pin_read(CYW43_PIN_BT_DEV_WAKE) != 0) {
        cyw43_hal_pin_low(CYW43_PIN_BT_DEV_WAKE); // wake up
        // Use delay_us rather than delay_ms to prevent running the scheduler (which
        // might result in more BLE operations).
        cyw43_delay_us(5000); // can't go lower than this
    }
    #endif

    return 0;
}

#endif // CYW43_ENABLE_BLUETOOTH_OVER_UART
