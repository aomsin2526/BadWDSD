#ifndef CYW43_INCLUDED_CONFIGPORT_H
#define CYW43_INCLUDED_CONFIGPORT_H

#include <stdint.h>

#define MIN(a, b)                           ((a) <= (b) ? (a) : (b))
#define static_assert(expr, msg)            typedef int static_assert_##__LINE__[(expr) ? 1 : -1]
#define CYW43_ARRAY_SIZE(a)                 (sizeof(a) / sizeof((a)[0]))

#define CYW43_RESOURCE_VERIFY_DOWNLOAD      (1)

#define CYW43_USE_SPI                       (0)
#define CYW43_LWIP                          (0)

#define CYW43_VERBOSE_DEBUG                 (1)
#define CYW43_VDEBUG(...)                   printf(__VA_ARGS__)
#define CYW43_DEBUG(...)                    printf(__VA_ARGS__)
#define CYW43_DEBUG(...)                    printf(__VA_ARGS__)
#define CYW43_INFO(...)                     printf(__VA_ARGS__)
#define CYW43_WARN(...)                     printf(__VA_ARGS__)

#define CYW43_PIN_RFSW_VDD                  (1)
#define CYW43_PIN_WL_REG_ON                 (2)
#define CYW43_PIN_WL_SDIO_1                 (3)
#define CYW43_PIN_WL_HOST_WAKE              (4)

#define CYW43_EPERM                         (1)
#define CYW43_EIO                           (5)
#define CYW43_EINVAL                        (22)
#define CYW43_ETIMEDOUT                     (110)

#define CYW43_THREAD_ENTER                  do { } while (0)
#define CYW43_THREAD_EXIT                   do { } while (0)
#define CYW43_THREAD_LOCK_CHECK             do { } while (0)

#define CYW43_SDPCM_SEND_COMMON_WAIT        do { } while (0)
#define CYW43_DO_IOCTL_WAIT                 do { } while (0)

#define CYW43_HAL_PIN_MODE_INPUT            (0)
#define CYW43_HAL_PIN_MODE_OUTPUT           (1)
#define CYW43_HAL_PIN_PULL_NONE             (0)
#define CYW43_HAL_MAC_WLAN0                 (0)

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

static inline void cyw43_hal_get_mac(int interface, uint8_t mac[6]) {
    (void)interface;
    (void)mac;
}

static inline void cyw43_hal_pin_config(int pin, int mode, int pull, int alt) {
    (void)pin;
    (void)mode;
    (void)pull;
    (void)alt;
}

static inline void cyw43_hal_pin_config_irq_falling(int pin, int enable) {
    (void)pin;
    (void)enable;
}

static inline int cyw43_hal_pin_read(int pin) {
    (void)pin;
    return 0;
}

static inline void cyw43_hal_pin_low(int pin) {
    (void)pin;
}

static inline void cyw43_hal_pin_high(int pin) {
    (void)pin;
}

static inline void cyw43_schedule_internal_poll_dispatch(void (*func)(void)) {
    (void)func;
}

#endif // CYW43_INCLUDED_CONFIGPORT_H
