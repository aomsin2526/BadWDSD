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

void DebugUart_ProcessChar(char ch)
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!Sc_IsReadyForUser())
        return;

    if (ch == 0)
        return;

    debugUartContext.txBuf[debugUartContext.txBufCurLen] = ch;
    ++debugUartContext.txBufCurLen;
    debugUartContext.txBuf[debugUartContext.txBufCurLen] = 0;

    if ((ch == '\r') || (ch == '\n') || (debugUartContext.txBufCurLen >= (DEBUG_UART_TXBUF_SIZE - 1)))
        DebugUart_Flush();

    debugUartContext.lastRxTimeInMs = get_time_in_ms();
}

void DebugUart_RxFn()
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        return;

    if (debugUartContext.isUartInited)
    {
        while (uart_is_readable(debugUartContext.uartId))
        {
            char ch = uart_getc(debugUartContext.uartId);
            DebugUart_ProcessChar(ch);
        }
    }

    while (1)
    {
        int32_t ch = getchar_timeout_us(0);

        if (ch == PICO_ERROR_TIMEOUT)
            break;

        DebugUart_ProcessChar((char)ch);
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

void DebugUart_Init(bool initUart)
{
    DEBUG_UART_MUTEX_HOLDER;

    if (DebugUart_IsInited())
        dead();

    debugUartContext.uartId = uart1;

    debugUartContext.txBufCurLen = 0;
    debugUartContext.txBuf[0] = 0;

    debugUartContext.lastRxTimeInMs = 0;

    if (initUart)
        Uart_Init(debugUartContext.uartId, DEBUG_UART_BAUD, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID);

    debugUartContext.isUartInited = initUart;

    debugUartIsInited = true;
}

void DebugUart_Uninit()
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        dead();

    debugUartIsInited = false;

    if (debugUartContext.isUartInited)
        Uart_Uninit(debugUartContext.uartId, true, DEBUG_UART_RX_PIN_ID, true, DEBUG_UART_TX_PIN_ID);
}

void DebugUart_Putc(char c)
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        return;

    if (debugUartContext.isUartInited)
        Uart_Putc(debugUartContext.uartId, c);

    putchar(c);
}

void DebugUart_Puts(const char* buf)
{
    DEBUG_UART_MUTEX_HOLDER;

    if (!DebugUart_IsInited())
        return;

    if (debugUartContext.isUartInited)
        Uart_Puts(debugUartContext.uartId, buf);

    printf(buf);
}