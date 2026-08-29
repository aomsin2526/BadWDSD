#include "Include.hpp"

bool debugUartIsInited = false;
struct DebugUartContext_s debugUartContext;

recursive_mutex_t debugUartMutex;

class DebugUartMutexHolder
{
public:
    DebugUartMutexHolder()
    {
        recursive_mutex_enter_blocking(&debugUartMutex);
    };

    ~DebugUartMutexHolder()
    {
        recursive_mutex_exit(&debugUartMutex);
    };
};

#define DEBUG_UART_MUTEX_HOLDER DebugUartMutexHolder mutexHolder

bool DebugUart_IsInited()
{
    return debugUartIsInited;
}

void DebugUart_Flush()
{
    DEBUG_UART_MUTEX_HOLDER;

    if (debugUartContext.txBufCurLen == 0)
        return;

    Sc_Puts(debugUartContext.txBuf);

    debugUartContext.txBufCurLen = 0;
    debugUartContext.txBuf[debugUartContext.txBufCurLen] = 0;
}

void DebugUart_RxFn()
{
    DEBUG_UART_MUTEX_HOLDER;

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
    if (get_core_num() != 1)
        dead();

    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        return;

    DebugUart_RxFn();
}

void DebugUart_Init()
{
    DEBUG_UART_MUTEX_HOLDER;

    if (DebugUart_IsInited())
        return;

    debugUartContext.uartId = uart1;

    debugUartContext.txBufCurLen = 0;
    debugUartContext.txBuf[0] = 0;

    debugUartContext.lastRxTimeInMs = 0;

    Uart_Init(debugUartContext.uartId, DEBUG_UART_BAUD, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID);
    debugUartIsInited = true;
}

void DebugUart_Uninit()
{
    DEBUG_UART_MUTEX_HOLDER;

    debugUartIsInited = false;
    Uart_Uninit(debugUartContext.uartId, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID);
}

void DebugUart_Putc(char c)
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        return;

    Uart_Putc(debugUartContext.uartId, c);
}

void DebugUart_Puts(const char* buf)
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        return;

    Uart_Puts(debugUartContext.uartId, buf);
}