#include "Include.h"

volatile bool debugUartIsInited = false;
volatile struct DebugUartContext_s debugUartContext;

recursive_mutex_t debugUartMutex;

bool DebugUart_IsInited()
{
    return debugUartIsInited;
}

void DebugUart_Flush()
{
    if (debugUartContext.txBufCurLen == 0)
        return;

    Sc_Puts(debugUartContext.txBuf);

    debugUartContext.txBufCurLen = 0;
    debugUartContext.txBuf[debugUartContext.txBufCurLen] = 0;
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

        debugUartContext.txBuf[debugUartContext.txBufCurLen] = ch;
        ++debugUartContext.txBufCurLen;
        debugUartContext.txBuf[debugUartContext.txBufCurLen] = 0;

        if ((ch == '\r') || (ch == '\n') || (debugUartContext.txBufCurLen >= (DEBUG_UART_TXBUF_SIZE - 1)))
            DebugUart_Flush();

        debugUartContext.lastRxTimeInMs = get_time_in_ms();
    }
}

void DebugUart_Thread()
{
    if (!DebugUart_IsInited())
        return;

    recursive_mutex_enter_blocking(&debugUartMutex);
    DebugUart_RxFn();
    recursive_mutex_exit(&debugUartMutex);
}

void DebugUart_Init()
{
    if (DebugUart_IsInited())
        return;

    recursive_mutex_init(&debugUartMutex);

    debugUartContext.uartId = uart1;

    debugUartContext.txBufCurLen = 0;
    debugUartContext.txBuf[0] = 0;

    debugUartContext.lastRxTimeInMs = 0;

    Uart_Init(debugUartContext.uartId, DEBUG_UART_BAUD, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID);

    debugUartIsInited = true;
    sync();
}

void DebugUart_Uninit()
{
    if (!DebugUart_IsInited())
        return;

    recursive_mutex_enter_blocking(&debugUartMutex);
    debugUartIsInited = false;
    sync();
    recursive_mutex_exit(&debugUartMutex);

    Uart_Uninit(debugUartContext.uartId, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID);
}

void DebugUart_Putc(char c)
{
    if (!DebugUart_IsInited())
        return;

    recursive_mutex_enter_blocking(&debugUartMutex);
    Uart_Putc(debugUartContext.uartId, c);
    recursive_mutex_exit(&debugUartMutex);
}

void DebugUart_Puts(const char* buf)
{
    if (!DebugUart_IsInited())
        return;

    recursive_mutex_enter_blocking(&debugUartMutex);
    Uart_Puts(debugUartContext.uartId, buf);
    recursive_mutex_exit(&debugUartMutex);
}