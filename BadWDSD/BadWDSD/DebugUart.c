#include "Include.h"

volatile bool debugUartIsInited = false;
volatile struct DebugUartContext_s debugUartContext;

bool DebugUart_IsInited()
{
    return debugUartIsInited;
}

void DebugUart_RxFn()
{
    while (uart_is_readable(debugUartContext.uartId))
    {
        char ch = uart_getc(debugUartContext.uartId);

        if (!Sc_IsReadyForUser())
            continue;

        if (ch == 0)
            continue;

        if (ch == '\r')
            continue;

        if (ch == '\n')
        {
            debugUartContext.txBuf[debugUartContext.txBufLen] = '\r';
            ++debugUartContext.txBufLen;
            debugUartContext.txBuf[debugUartContext.txBufLen] = 0;
        }

        debugUartContext.txBuf[debugUartContext.txBufLen] = ch;
        ++debugUartContext.txBufLen;
        debugUartContext.txBuf[debugUartContext.txBufLen] = 0;

        if ((ch == '\n') || (debugUartContext.txBufLen >= 2000))
        {
            Sc_Puts(debugUartContext.txBuf);
            debugUartContext.txBufLen = 0;
        }
    }
}

void DebugUart_Thread()
{
    if (!DebugUart_IsInited())
        return;

    DebugUart_RxFn();
}

void DebugUart_Init()
{
    debugUartContext.uartId = uart1;

    debugUartContext.txBufLen = 0;
    debugUartContext.txBuf[0] = 0;

    Uart_Init(debugUartContext.uartId, DEBUG_UART_BAUD, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID, NULL);

    debugUartIsInited = true;
    sync();

    PrintLog("Debug Uart ready.\n");

    PrintLog("BadWDSD Pico By Kafuu(aomsin2526) (Build date: %s %s)\n", __DATE__, __TIME__);
#if PICO_IS_ZERO
    PrintLog("Zero version.\n");
#endif
}

void DebugUart_Putc(char c)
{
    if (!DebugUart_IsInited())
        return;

    Uart_Putc(debugUartContext.uartId, c);
}

void DebugUart_Puts(const char* buf)
{
    if (!DebugUart_IsInited())
        return;

    Uart_Puts(debugUartContext.uartId, buf);
}