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

    DebugUart_RxFn();

    // doesn't work in putty, because it send every input without buffering
    //if ((get_time_in_ms() - debugUartContext.lastRxTimeInMs) > 100)
    //    DebugUart_Flush();
}

void DebugUart_Init()
{
    recursive_mutex_init(&debugUartMutex);

    debugUartContext.uartId = uart1;

    debugUartContext.txBufCurLen = 0;
    debugUartContext.txBuf[0] = 0;

    debugUartContext.lastRxTimeInMs = 0;

    Uart_Init(debugUartContext.uartId, DEBUG_UART_BAUD, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID, NULL);

    debugUartIsInited = true;
    sync();

    PrintLog("Debug Uart ready.\n");

    PrintLog("BadWDSD Pico By Kafuu(aomsin2526) (Build date: %s %s)\n", __DATE__, __TIME__);

#if PICO_TYPE == PICO_TYPE_E_PICO
    PrintLog("Pico\n");
#elif PICO_TYPE == PICO_TYPE_E_PICO_W
    PrintLog("Pico W\n");
#elif PICO_TYPE == PICO_TYPE_E_RP2040_ZERO
    PrintLog("RP2040-Zero\n");
#else
#error bad!!!
#endif
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