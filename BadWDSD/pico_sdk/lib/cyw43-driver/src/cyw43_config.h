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

/**
 * \internal
 * \file
*/

// Import port-specific configuration file.
#ifdef CYW43_CONFIG_FILE
#include CYW43_CONFIG_FILE
#else
#include <cyw43_configport.h>
#endif

// Bus configuration.

#ifndef CYW43_USE_SPI
#define CYW43_USE_SPI (0)
#endif

#ifndef CYW43_CLEAR_SDIO_INT
#define CYW43_CLEAR_SDIO_INT (0)
#endif

#ifndef CYW43_INCLUDE_LEGACY_F1_OVERFLOW_WORKAROUND_VARIABLES
#define CYW43_INCLUDE_LEGACY_F1_OVERFLOW_WORKAROUND_VARIABLES (0)
#endif

// The following pins can be defined, depending on the configuration:
// - CYW43_PIN_RFSW_VDD         - RF-switch power (active high)
// - CYW43_PIN_RFSW_SELECT      - RF-switch select
// - CYW43_PIN_WL_REG_ON        - WL power on (active high)
// - CYW43_PIN_WL_HOST_WAKE     - for SDIO or SPI transport
// - CYW43_PIN_WL_IRQ           - the gSPI IRQ line
// - CYW43_PIN_WL_SDIO_1        - for SDIO transport
// - CYW43_PIN_BT_REG_ON        - BT power on (active high)
// - CYW43_PIN_BT_HOST_WAKE     - for BT HCI UART transport
// - CYW43_PIN_BT_DEV_WAKE      - for BT HCI UART transport
// - CYW43_PIN_BT_CTS           - for BT HCI UART transport, used to wait for CTS to go low

// Legacy pin configuration CYW43_PIN_WL_RFSW_VDD is renamed to CYW43_PIN_RFSW_VDD.
#ifdef CYW43_PIN_WL_RFSW_VDD
#define CYW43_PIN_RFSW_VDD CYW43_PIN_WL_RFSW_VDD
#endif

// Firmware configuration.

// Whether Bluetooth support is enabled.
// This option uses the WiFi SPI/SDIO interface as the Bluetooth transport.
#ifndef CYW43_ENABLE_BLUETOOTH
#define CYW43_ENABLE_BLUETOOTH (0)
#endif

// Whether Bluetooth support is enabled.
// This option uses a UART as the Bluetooth transport.
#ifndef CYW43_ENABLE_BLUETOOTH_OVER_UART
#define CYW43_ENABLE_BLUETOOTH_OVER_UART (0)
#endif

// This include should define:
// - CYW43_WIFI_FW_LEN
// - CYW43_CLM_LEN
// - const uintptr_t fw_data
#ifndef CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE
#if CYW43_ENABLE_BLUETOOTH
#define CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE "firmware/wb43439A0_7_95_49_00_combined.h"
#else
#define CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE "firmware/w43439A0_7_95_49_00_combined.h"
#endif
#endif

// This include should define a wifi_nvram_4343[] variable.
#ifndef CYW43_WIFI_NVRAM_INCLUDE_FILE
#define CYW43_WIFI_NVRAM_INCLUDE_FILE "firmware/wifi_nvram_43439.h"
#endif

// This should be defined by the port if needed, to override the default
// alignment, or add more attributes, for the firmware and NVRAM resources.
#ifndef CYW43_RESOURCE_ATTRIBUTE
#define CYW43_RESOURCE_ATTRIBUTE __attribute__((aligned(4)))
#endif

// Whether the download of resources should be verified.
#ifndef CYW43_RESOURCE_VERIFY_DOWNLOAD
#define CYW43_RESOURCE_VERIFY_DOWNLOAD (0)
#endif

// Timing and timeout configuration.

#ifndef CYW43_IOCTL_TIMEOUT_US
#define CYW43_IOCTL_TIMEOUT_US (500000)
#endif

#ifndef CYW43_SLEEP_MAX
#define CYW43_SLEEP_MAX (50)
#endif

// Called while waiting for an incoming character on the BT HCI UART.
#ifndef CYW43_HAL_UART_READCHAR_BLOCKING_WAIT
#define CYW43_HAL_UART_READCHAR_BLOCKING_WAIT cyw43_delay_us(10)
#endif

// Miscellaneous configuration.

// No other options right now for TCP/IP stack.
#ifndef CYW43_LWIP
#define CYW43_LWIP (1)
#endif

#ifndef CYW43_NETUTILS
#define CYW43_NETUTILS (0)
#endif

#ifndef CYW43_USE_OTP_MAC
#define CYW43_USE_OTP_MAC (0)
#endif

#ifndef CYW43_GPIO
#define CYW43_GPIO (0)
#endif

#ifndef CYW43_PRINTF
#include <stdio.h>
#define CYW43_PRINTF(...) printf(__VA_ARGS__)
#endif

#ifndef CYW43_VDEBUG
#define CYW43_VDEBUG(...) (void)0
#define CYW43_VERBOSE_DEBUG 0
#endif

#ifndef CYW43_DEBUG
#ifdef NDEBUG
#define CYW43_DEBUG(...) (void)0
#else
#define CYW43_DEBUG(...) CYW43_PRINTF(__VA_ARGS__)
#endif
#endif

#ifndef CYW43_INFO
#define CYW43_INFO(...) CYW43_PRINTF(__VA_ARGS__)
#endif

#ifndef CYW43_WARN
#define CYW43_WARN(...) CYW43_PRINTF("[CYW43] " __VA_ARGS__)
#endif

#ifndef CYW43_FAIL_FAST_CHECK
#define CYW43_FAIL_FAST_CHECK(res) (res)
#endif

// This should be defined by the port if needed, to let background processes
// run during long blocking operations such as WiFi initialisation.
#ifndef CYW43_EVENT_POLL_HOOK
#define CYW43_EVENT_POLL_HOOK
#endif

#ifndef CYW43_DEFAULT_IP_STA_ADDRESS
#define CYW43_DEFAULT_IP_STA_ADDRESS LWIP_MAKEU32(0, 0, 0, 0)
#endif

#ifndef CYW43_DEFAULT_IP_AP_ADDRESS
#define CYW43_DEFAULT_IP_AP_ADDRESS LWIP_MAKEU32(192, 168, 4, 1)
#endif

#ifndef CYW43_DEFAULT_IP_MASK
#define CYW43_DEFAULT_IP_MASK LWIP_MAKEU32(255, 255, 255, 0)
#endif

#ifndef CYW43_DEFAULT_IP_STA_GATEWAY
#define CYW43_DEFAULT_IP_STA_GATEWAY LWIP_MAKEU32(192, 168, 0, 1)
#endif

#ifndef CYW43_DEFAULT_IP_AP_GATEWAY
#define CYW43_DEFAULT_IP_AP_GATEWAY LWIP_MAKEU32(192, 168, 4, 1)
#endif

#ifndef CYW43_DEFAULT_IP_DNS
#define CYW43_DEFAULT_IP_DNS LWIP_MAKEU32(8, 8, 8, 8)
#endif

// Hook for any additional TCP/IP initialization than needs to be done.
// Called after the netif specified by `itf` has been set up.
#ifndef CYW43_CB_TCPIP_INIT_EXTRA
#define CYW43_CB_TCPIP_INIT_EXTRA(self, itf) do { } while (0)
#endif

// Hook for any additional TCP/IP deinitialization than needs to be done.
// Called before the netif specified by `itf` is removed.
#ifndef CYW43_CB_TCPIP_DEINIT_EXTRA
#define CYW43_CB_TCPIP_DEINIT_EXTRA(self, itf) do { } while (0)
#endif
