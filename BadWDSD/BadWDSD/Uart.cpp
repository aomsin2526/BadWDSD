#include "Include.hpp"

void Uart_Init(uart_inst_t *uartId, uint32_t baud, bool rxEnabled, uint32_t rxPinId, bool txEnabled, uint32_t txPinId)
{
    if (txEnabled)
        gpio_set_function(txPinId, UART_FUNCSEL_NUM(uartId, txPinId));

    if (rxEnabled)
        gpio_set_function(rxPinId, UART_FUNCSEL_NUM(uartId, rxPinId));

    uart_init(uartId, baud);

    uart_set_hw_flow(uartId, false, false);
    uart_set_format(uartId, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uartId, true);
}

void Uart_Uninit(uart_inst_t* uartId, bool rxEnabled, uint32_t rxPinId, bool txEnabled, uint32_t txPinId)
{
    uart_deinit(uartId);

    if (txEnabled)
        gpio_set_function(txPinId, GPIO_FUNC_NULL);

    if (rxEnabled)
        gpio_set_function(rxPinId, GPIO_FUNC_NULL);
}

void Uart_Putc(uart_inst_t* uartId, char c)
{
    while (!uart_is_writable(uartId)) {}

    uart_putc_raw(uartId, c);
}

void Uart_Puts(uart_inst_t* uartId, const char* buf)
{
    while (*buf != 0)
    {
        char c = *buf;

        if ((c == '\r') || (c == '\n'))
        {
            Uart_Putc(uartId, '\r');
            Uart_Putc(uartId, '\n');
            break;
        }
        else
            Uart_Putc(uartId, c);

        ++buf;
    }
}