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
 * \file
 * \brief CYW43 SPI API
*/

#ifndef CYW43_INCLUDED_CYW43_SPI_H
#define CYW43_INCLUDED_CYW43_SPI_H

// Test register value
#define TEST_PATTERN 0xFEEDBEADu

// Register addresses
#define SPI_BUS_CONTROL                 ((uint32_t)0x0000)
#define SPI_RESPONSE_DELAY              ((uint32_t)0x0001)
#define SPI_STATUS_ENABLE               ((uint32_t)0x0002)
#define SPI_RESET_BP                    ((uint32_t)0x0003)  // (corerev >= 1)
#define SPI_INTERRUPT_REGISTER          ((uint32_t)0x0004)  // 16 bits - Interrupt status
#define SPI_INTERRUPT_ENABLE_REGISTER   ((uint32_t)0x0006)  // 16 bits - Interrupt mask
#define SPI_STATUS_REGISTER             ((uint32_t)0x0008)  // 32 bits
#define SPI_FUNCTION1_INFO              ((uint32_t)0x000C)  // 16 bits
#define SPI_FUNCTION2_INFO              ((uint32_t)0x000E)  // 16 bits
#define SPI_FUNCTION3_INFO              ((uint32_t)0x0010)  // 16 bits
#define SPI_READ_TEST_REGISTER          ((uint32_t)0x0014)  // 32 bits
#define SPI_RESP_DELAY_F0               ((uint32_t)0x001c)  // 8 bits (corerev >= 3)
#define SPI_RESP_DELAY_F1               ((uint32_t)0x001d)  // 8 bits (corerev >= 3)
#define SPI_RESP_DELAY_F2               ((uint32_t)0x001e)  // 8 bits (corerev >= 3)
#define SPI_RESP_DELAY_F3               ((uint32_t)0x001f)  // 8 bits (corerev >= 3)

// SPI_FUNCTIONX_BITS
#define SPI_FUNCTIONX_ENABLED           (1 << 0)
#define SPI_FUNCTIONX_READY             (1 << 1)

// SPI_BUS_CONTROL Bits
#define WORD_LENGTH_32                  ((uint32_t)0x01)    // 0/1 16/32 bit word length
#define ENDIAN_BIG                      ((uint32_t)0x02)    // 0/1 Little/Big Endian
#define CLOCK_PHASE                     ((uint32_t)0x04)    // 0/1 clock phase delay
#define CLOCK_POLARITY                  ((uint32_t)0x08)    // 0/1 Idle state clock polarity is low/high
#define HIGH_SPEED_MODE                 ((uint32_t)0x10)    // 1/0 High Speed mode / Normal mode
#define INTERRUPT_POLARITY_HIGH         ((uint32_t)0x20)    // 1/0 Interrupt active polarity is high/low
#define WAKE_UP                         ((uint32_t)0x80)    // 0/1 Wake-up command from Host to WLAN

// SPI_STATUS_ENABLE bits
#define STATUS_ENABLE                   ((uint32_t)0x01)    // 1/0 Status sent/not sent to host after read/write
#define INTR_WITH_STATUS                ((uint32_t)0x02)    // 0/1 Do-not / do-interrupt if status is sent
#define RESP_DELAY_ALL                  ((uint32_t)0x04)    // Applicability of resp delay to F1 or all func's read
#define DWORD_PKT_LEN_EN                ((uint32_t)0x08)    // Packet len denoted in dwords instead of bytes
#define CMD_ERR_CHK_EN                  ((uint32_t)0x20)    // Command error check enable
#define DATA_ERR_CHK_EN                 ((uint32_t)0x40)    // Data error check enable

// SPI_INTERRUPT_REGISTER and SPI_INTERRUPT_ENABLE_REGISTER bits
#define DATA_UNAVAILABLE                ((uint32_t)0x0001)  // Requested data not available; Clear by writing a "1"
#define F2_F3_FIFO_RD_UNDERFLOW         ((uint32_t)0x0002)
#define F2_F3_FIFO_WR_OVERFLOW          ((uint32_t)0x0004)
#define COMMAND_ERROR                   ((uint32_t)0x0008)  // Cleared by writing 1
#define DATA_ERROR                      ((uint32_t)0x0010)  // Cleared by writing 1
#define F2_PACKET_AVAILABLE             ((uint32_t)0x0020)
#define F3_PACKET_AVAILABLE             ((uint32_t)0x0040)
#define F1_OVERFLOW                     ((uint32_t)0x0080)  // Due to last write. Bkplane has pending write requests
#define GSPI_PACKET_AVAILABLE           ((uint32_t)0x0100)
#define MISC_INTR1                      ((uint32_t)0x0200)
#define MISC_INTR2                      ((uint32_t)0x0400)
#define MISC_INTR3                      ((uint32_t)0x0800)
#define MISC_INTR4                      ((uint32_t)0x1000)
#define F1_INTR                         ((uint32_t)0x2000)
#define F2_INTR                         ((uint32_t)0x4000)
#define F3_INTR                         ((uint32_t)0x8000)

#define BUS_OVERFLOW_UNDERFLOW (F1_OVERFLOW | F2_F3_FIFO_RD_UNDERFLOW | F2_F3_FIFO_WR_OVERFLOW)

// SPI_STATUS_REGISTER bits
#define STATUS_DATA_NOT_AVAILABLE       ((uint32_t)0x00000001)
#define STATUS_UNDERFLOW                ((uint32_t)0x00000002)
#define STATUS_OVERFLOW                 ((uint32_t)0x00000004)
#define STATUS_F2_INTR                  ((uint32_t)0x00000008)
#define STATUS_F3_INTR                  ((uint32_t)0x00000010)
#define STATUS_F2_RX_READY              ((uint32_t)0x00000020)
#define STATUS_F3_RX_READY              ((uint32_t)0x00000040)
#define STATUS_HOST_CMD_DATA_ERR        ((uint32_t)0x00000080)
#define STATUS_F2_PKT_AVAILABLE         ((uint32_t)0x00000100)
#define STATUS_F2_PKT_LEN_MASK          ((uint32_t)0x000FFE00)
#define STATUS_F2_PKT_LEN_SHIFT         ((uint32_t)9)
#define STATUS_F3_PKT_AVAILABLE         ((uint32_t)0x00100000)
#define STATUS_F3_PKT_LEN_MASK          ((uint32_t)0xFFE00000)
#define STATUS_F3_PKT_LEN_SHIFT         ((uint32_t)21)

#define SPI_FRAME_CONTROL               ((uint32_t)0x1000D)

// Read and write before switching to 32bit mode
uint32_t read_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg);
int write_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val);

// These must be provided by a port, if the SPI bus interface is used.
int cyw43_spi_init(cyw43_int_t *self);
void cyw43_spi_deinit(cyw43_int_t *self);
void cyw43_spi_gpio_setup(void);
void cyw43_spi_reset(void);
void cyw43_spi_set_polarity(cyw43_int_t *self, int pol);
int cyw43_spi_transfer(cyw43_int_t *self, const uint8_t *tx, size_t tx_length, uint8_t *rx, size_t rx_length);

#endif
