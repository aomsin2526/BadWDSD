/*
 * This file is part of the cyw43-driver
 *
 * Copyright (C) 2025 George Robotics Pty Ltd
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

#include "cyw43.h"
#include "cyw43_internal.h"
#include "cyw43_spi.h"

#if CYW43_USE_SPI

static inline uint32_t cyw43_get_swap32(const uint8_t *buf) {
    return buf[1] | buf[0] << 8 | buf[3] << 16 | buf[2] << 24;
}

static inline void cyw43_put_swap32(uint8_t *buf, uint32_t x) {
    buf[1] = x;
    buf[0] = x >> 8;
    buf[3] = x >> 16;
    buf[2] = x >> 24;
}

static inline void cyw43_put_le32(uint8_t *buf, uint32_t x) {
    buf[0] = x;
    buf[1] = x >> 8;
    buf[2] = x >> 16;
    buf[3] = x >> 24;
}

static inline uint32_t pack_cmd(bool write, bool inc, uint32_t fn, uint32_t addr, uint32_t sz) {
    return write << 31 | inc << 30 | fn << 28 | (addr & 0x1ffff) << 11 | sz;
}

uint32_t read_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    // Build and send the command.
    uint8_t buf[8];
    cyw43_put_swap32(&buf[0], pack_cmd(false, true, fn, reg, 4));
    int ret = cyw43_spi_transfer(self, buf, 8, buf, 8);
    if (ret != 0) {
        return ret;
    }

    // Extract the result.
    return cyw43_get_swap32(&buf[4]);
}

// This is only used to switch the SPI bus configuration at start up.
int write_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val) {
    // Build and send the command.
    uint8_t buf[8];
    cyw43_put_swap32(&buf[0], pack_cmd(true, true, fn, reg, 4));
    cyw43_put_swap32(&buf[4], val);
    int ret = cyw43_spi_transfer(self, buf, 8, NULL, 0);

    // The above command switched SPI configuration, so change SPI bus mode to polarity=0.
    cyw43_spi_set_polarity(self, 0);

    return ret;
}

int cyw43_read_bytes(cyw43_int_t *self, uint32_t fn, uint32_t addr, size_t len, uint8_t *dest) {
    const size_t aligned_len = (len + 3U) & ~3U;
    size_t xfer_len = 4U + aligned_len; // 4-byte command then payload

    // For reads to BACKPLANE_FUNCTION, some dummy bytes must be inserted between the command from
    // the host and the response from the device, so the device has time to prepare the response.
    uint8_t *spi_buf = (uint8_t *)&self->spi_header[0];
    if (fn == BACKPLANE_FUNCTION) {
        xfer_len += CYW43_BACKPLANE_READ_PAD_LEN_BYTES;
    } else {
        spi_buf += CYW43_BACKPLANE_READ_PAD_LEN_BYTES;
    }

    // Build and send the command.
    cyw43_put_le32(spi_buf, pack_cmd(false, true, fn, addr, len));
    int ret = cyw43_spi_transfer(self, spi_buf, xfer_len, spi_buf, xfer_len);

    // Copy out the result if the buffer is different to that used by the SPI transfer.
    if (dest != self->spid_buf) {
        memmove(dest, self->spid_buf, len);
    }

    return ret;
}

int cyw43_write_bytes(cyw43_int_t *self, uint32_t fn, uint32_t addr, size_t len, const uint8_t *src) {
    const size_t aligned_len = (len + 3U) & ~3U;
    size_t xfer_len = 4U + aligned_len; // 4-byte command then payload
    uint8_t *spi_buf = (uint8_t *)&self->spi_header[CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4];

    // Build and send the command.
    cyw43_put_le32(spi_buf, pack_cmd(true, true, fn, addr, len));
    if (src != self->spid_buf) {
        memmove(self->spid_buf, src, len);
    }
    return cyw43_spi_transfer(self, spi_buf, xfer_len, NULL, 0);
}

int cyw43_read_reg_u8(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    uint8_t buf;
    cyw43_read_bytes(self, fn, reg, 1, (uint8_t *)&buf);
    return buf;
}

int cyw43_read_reg_u16(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    uint16_t buf;
    cyw43_read_bytes(self, fn, reg, 2, (uint8_t *)&buf);
    return buf;
}

uint32_t cyw43_read_reg_u32(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    uint32_t buf;
    cyw43_read_bytes(self, fn, reg, 4, (uint8_t *)&buf);
    return buf;
}

int cyw43_write_reg_u8(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val) {
    return cyw43_write_bytes(self, fn, reg, 1, (const uint8_t *)&val);
}

int cyw43_write_reg_u16(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint16_t val) {
    return cyw43_write_bytes(self, fn, reg, 2, (const uint8_t *)&val);
}

int cyw43_write_reg_u32(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val) {
    return cyw43_write_bytes(self, fn, reg, 4, (const uint8_t *)&val);
}

#endif // CYW43_USE_SPI
