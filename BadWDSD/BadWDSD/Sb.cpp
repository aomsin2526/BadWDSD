#include "Include.hpp"

bool sbIsInited = false;
struct SbContext_s sbContext;

bool Sb_IsInited()
{
    return sbIsInited;
}

void Sb_RxFn()
{
    if (!Sb_IsInited())
        return;

    while (uart_is_readable(sbContext.uartId))
    {
        char ch = uart_getc(sbContext.uartId);
    }
}

void Sb_Thread()
{
    if (get_core_num() != 1)
        dead();

    if (!Sb_IsInited())
        return;

    Sb_RxFn();
}

void Sb_Init()
{
    if (Sb_IsInited())
        dead();

    sbContext.uartId = uart1;

    Uart_Init(sbContext.uartId, SB_UART_BAUD, true, SB_UART_RX_PIN_ID, true, SB_UART_TX_PIN_ID);
    sbIsInited = true;
}

void Sb_Uninit()
{
    if (!Sb_IsInited())
        dead();

    sbIsInited = false;
    Uart_Uninit(sbContext.uartId, true, SB_UART_RX_PIN_ID, true, SB_UART_TX_PIN_ID);
}

void Sb_Putc(char c)
{
    if (!Sb_IsInited())
        dead();

    Uart_Putc(sbContext.uartId, c);
}