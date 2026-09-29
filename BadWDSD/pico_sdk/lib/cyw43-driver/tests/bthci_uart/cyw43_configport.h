#ifndef CYW43_INCLUDED_CONFIGPORT_H
#define CYW43_INCLUDED_CONFIGPORT_H

#include <stdint.h>

#define MIN(a, b)                           ((a) <= (b) ? (a) : (b))
#define static_assert(expr, msg)            typedef int static_assert_##__LINE__[(expr) ? 1 : -1]

#define CYW43_ENABLE_BLUETOOTH_OVER_UART    (1)
#define CYW43_LWIP                          (0)
#define CYW43_BT_FIRMWARE_INCLUDE_FILE      "firmware/cyw43_btfw_4343A1.h"

#define CYW43_PIN_BT_REG_ON                 (0)
#define CYW43_PIN_BT_CTS                    (1)

#define CYW43_EPERM                         (1)
#define CYW43_EIO                           (5)
#define CYW43_EINVAL                        (22)
#define CYW43_ETIMEDOUT                     (110)

#define CYW43_HAL_PIN_MODE_INPUT            (0)
#define CYW43_HAL_PIN_MODE_OUTPUT           (1)
#define CYW43_HAL_PIN_PULL_NONE             (0)
#define CYW43_HAL_MAC_WLAN0                 (0)
#define CYW43_HAL_MAC_BDADDR                (1)

static inline unsigned int cyw43_hal_ticks_us(void) {
    static unsigned int t = 0;
    return t++;
}

static inline unsigned int cyw43_hal_ticks_ms(void) {
    static unsigned int t = 0;
    return t++;
}

static inline void cyw43_delay_us(unsigned int us) {
    (void)us;
}

static inline void cyw43_delay_ms(unsigned int ms) {
    (void)ms;
}

void cyw43_hal_get_mac(int interface, uint8_t mac[6]);

void cyw43_hal_pin_config(int pin, int mode, int pull, int alt);
int cyw43_hal_pin_read(int pin);
void cyw43_hal_pin_low(int pin);
void cyw43_hal_pin_high(int pin);

void cyw43_hal_uart_set_baudrate(int baudrate);
int cyw43_hal_uart_write(const uint8_t *buf, int len);
int cyw43_hal_uart_readchar(void);

#endif // CYW43_INCLUDED_CONFIGPORT_H
