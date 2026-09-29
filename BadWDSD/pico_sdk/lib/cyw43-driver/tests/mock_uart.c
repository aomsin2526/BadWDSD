#include "src/cyw43.h"

static const uint8_t mock_response[] = {
    0x04, 0x0e, 0x04, 0x00, 0x00, 0x00, 0x00,
};

void cyw43_hal_uart_set_baudrate(int baudrate) {
    printf("cyw43_hal_uart_set_baudrate(%d)\n", baudrate);
}

int cyw43_hal_uart_write(const uint8_t *buf, int len) {
    printf("cyw43_hal_uart_write(%02x:%02x:%02x:%02x, %d)\n", buf[0], buf[1], buf[2], buf[3], len);
    return 0;
}

int cyw43_hal_uart_readchar(void) {
    static unsigned i = 0;
    printf("cyw43_hal_uart_readchar()\n");
    int c = mock_response[i];
    i = (i + 1) % sizeof(mock_response);
    return c;
}
