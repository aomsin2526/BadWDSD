#include "Include.h"

volatile bool sbIsInited = false;
volatile struct SbContext_s sbContext;

bool Sb_IsInited()
{
    return sbIsInited;
}

void Sb_Init()
{
    if (Sb_IsInited())
        return;

    sbContext.uartId = uart1;

    Uart_Init(sbContext.uartId, SB_UART_BAUD, false, 0, true, SB_UART_TX_PIN_ID);
    sbIsInited = true;
}

void Sb_Uninit()
{
    if (!Sb_IsInited())
        return;

    sbIsInited = false;
    Uart_Uninit(sbContext.uartId, false, 0, true, SB_UART_TX_PIN_ID);
}

void Sb_Putc(char c)
{
    if (!Sb_IsInited())
        return;

    Uart_Putc(sbContext.uartId, c);
}