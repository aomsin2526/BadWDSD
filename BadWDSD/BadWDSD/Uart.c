#include "Include.h"

void Uart_Init(uart_inst_t *uartId, uint32_t baud, bool rxEnabled, uint32_t rxPinId, bool txEnabled, uint32_t txPinId, void *rxFn)
{
    uart_init(uartId, baud);

    if (txEnabled)
        gpio_set_function(txPinId, UART_FUNCSEL_NUM(uartId, txPinId));

    if (rxEnabled)
        gpio_set_function(rxPinId, UART_FUNCSEL_NUM(uartId, rxPinId));

    uart_set_hw_flow(uartId, false, false);
    uart_set_format(uartId, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uartId, true);

    if (rxEnabled && (rxFn != NULL))
    {
        uint32_t irq = (uartId == uart0) ? UART0_IRQ : UART1_IRQ;

        irq_set_exclusive_handler(irq, rxFn);

        irq_set_enabled(irq, true);
        uart_set_irq_enables(uartId, true, false);
    }
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