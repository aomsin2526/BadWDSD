#include "src/cyw43.h"

void cyw43_hal_get_mac(int interface, uint8_t mac[6]) {
    (void)mac;
    printf("cyw43_hal_get_mac(%d)\n", interface);
}

void cyw43_hal_pin_config(int pin, int mode, int pull, int alt) {
    printf("cyw43_hal_pin_config(%d, %d, %d, %d)\n", pin, mode, pull, alt);
}

int cyw43_hal_pin_read(int pin) {
    printf("cyw43_hal_pin_read(%d)\n", pin);
    return 0;
}

void cyw43_hal_pin_low(int pin) {
    printf("cyw43_hal_pin_low(%d)\n", pin);
}

void cyw43_hal_pin_high(int pin) {
    printf("cyw43_hal_pin_high(%d)\n", pin);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    cyw43_bluetooth_controller_init();
    cyw43_bluetooth_controller_sleep_maybe();
    cyw43_bluetooth_controller_woken();
    cyw43_bluetooth_controller_wakeup();
    cyw43_bluetooth_controller_deinit();
    return 0;
}
