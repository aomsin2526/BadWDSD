#include "Include.h"

volatile bool scIsInited = false;
volatile struct ScContext_s scContext;

void Sc_RxFn()
{
    while (uart_is_readable(scContext.uartId))
    {
        char ch = uart_getc(scContext.uartId);

        if (ch == 0)
            continue;

        scContext.rxBuf[scContext.rxBufLen] = ch;
        ++scContext.rxBufLen;
        scContext.rxBuf[scContext.rxBufLen] = 0;

        bool trigger = false;

        if (strstr(scContext.rxBuf, "XDR Link"))
            trigger = true;

        if (trigger)
        {
            scContext.trigger = true;
            sync();
        }

        bool success = false;

        if (strstr(scContext.rxBuf, "BadWDSD"))
            success = true;

        if (success)
        {
            scContext.success = true;
            sync();
        }

        bool reset = false;

        if ((get_time_in_ms() - scContext.lastScTxTimeInMs) > 5000)
        {

#if SC_IS_SW
            if (strstr(scContext.rxBuf, "OK 00000000:3A"))
#else
            if (strstr(scContext.rxBuf, "[mullion]$"))
#endif
                reset = true;

        }

        if (reset)
            watchdog_reboot(0, 0, 0);

        if (strstr(scContext.rxBuf, "[mullion]$ "))
        {
            scContext.rxBufLen = 0;
            scContext.rxBuf[scContext.rxBufLen] = 0;
        }

        if (ch == '\n' || (scContext.rxBufLen >= 1023))
        {
            volatile struct Sc_SendCommandContext_s *ctx = scContext.sendCommandCtx;
            bool ctx_done = false;

            if (ctx != NULL)
            {
                // PrintLog("rxBuf = %s", scContext.rxBuf);
                // PrintLog("expectedResponse = %s", ctx->expectedResponse);

                if ((strlen(ctx->expectedResponse) == 0) || strstr(scContext.rxBuf, ctx->expectedResponse))
                {
                    // PrintLog("strstr ok!\n");

                    strcpy(ctx->response, scContext.rxBuf);
                    ctx->responseLen = scContext.rxBufLen;

                    scContext.sendCommandCtx = NULL;
                    sync();

                    ctx_done = true;
                }
            }

            if (scContext.rxBufLen >= 2 && !strstr(scContext.rxBuf, "#!:4"))
            {
                PrintLog("Sc_Rx: ");

                --scContext.rxBufLen;
                scContext.rxBuf[scContext.rxBufLen] = 0;

                --scContext.rxBufLen;
                scContext.rxBuf[scContext.rxBufLen] = '\n';

                PrintLog("%s", scContext.rxBuf);
            }

            scContext.rxBufLen = 0;
            scContext.rxBuf[scContext.rxBufLen] = 0;

            if (ctx_done)
            {
                ctx->done = true;
                sync();
            }
        }
    }
}

void Sc_Thread()
{
    if (!Sc_IsInited())
        return;

    Sc_RxFn();
}

bool Sc_IsInited()
{
    return scIsInited;
}

bool Sc_GetScLite()
{
    return !Gpio_GetOnce(SC_LITE_PIN_ID);
}

bool Sc_GetScBanksel()
{
    return !Gpio_GetOnce(SC_BANKSEL_PIN_ID);
}

volatile struct Sc_SendCommandContext_s cmdCtx;

void Sc_Init()
{
    scContext.uartId = uart0;

    scContext.rxBufLen = 0;
    scContext.rxBuf[0] = 0;

    scContext.txBufLen = 0;
    scContext.txBuf[0] = 0;

    scContext.trigger = false;

    scContext.success = false;

    scContext.sendCommandCtx = NULL;

    scContext.lastScTxTimeInMs = 0;

    Uart_Init(scContext.uartId, SC_UART_BAUD, true, SC_UART_RX_PIN_ID, true, SC_UART_TX_PIN_ID, NULL);

    scIsInited = true;
    sync();

    {
        PrintLog("SC Auth...\n");

        //

        sprintf(cmdCtx.cmd, "somejunk\r\n");

#if SC_IS_SW
        sprintf(cmdCtx.expectedResponse, "NG F");
#else
        sprintf(cmdCtx.expectedResponse, "Unknown Command");
#endif

        Sc_SendCommand(&cmdCtx);

        //

        uint64_t iv[2];

        iv[0] = swap_uint64(0x0);
        iv[1] = swap_uint64(0x0);

        // 71f03f184c01c5ebc3f6a22a42ba9525

        WORD sc2tb_key_schedule[60];
        uint64_t sc2tb_key[2];

        sc2tb_key[0] = swap_uint64(0x71f03f184c01c5eb);
        sc2tb_key[1] = swap_uint64(0xc3f6a22a42ba9525);

        aes_key_setup((BYTE *)sc2tb_key, sc2tb_key_schedule, 128);

        // 907e730f4d4e0a0b7b75f030eb1d9d36

        WORD tb2sc_key_schedule[60];
        uint64_t tb2sc_key[2];

        tb2sc_key[0] = swap_uint64(0x907e730f4d4e0a0b);
        tb2sc_key[1] = swap_uint64(0x7b75f030eb1d9d36);

        aes_key_setup((BYTE *)tb2sc_key, tb2sc_key_schedule, 128);

        //

        {

#if SC_IS_SW
            PrintLog("SC is SW\n");
#else
            PrintLog("SC is CXRF\n");
#endif

#if SC_IS_SW

            {
                sprintf(cmdCtx.cmd, "SETCMDLONG FF FF\r\n");
                sprintf(cmdCtx.expectedResponse, "OK 00000000");

                Sc_SendCommand(&cmdCtx);

                // PrintLog("responseLen = %u\n", cmdCtx.responseLen);
                // PrintLog("response = %s", cmdCtx.response);
            }

#else

            {
                sprintf(cmdCtx.cmd, "scopen\r\n");
                sprintf(cmdCtx.expectedResponse, "SC_READY\r\n");

                Sc_SendCommand(&cmdCtx);

                // PrintLog("responseLen = %u\n", cmdCtx.responseLen);
                // PrintLog("response = %s", cmdCtx.response);
            }

#endif

            {

#if SC_IS_SW
                sprintf(cmdCtx.cmd, "AUTH1 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\r\n");
                sprintf(cmdCtx.expectedResponse, "OK 00000000 10100000FFFFFFFF0000000000000000");
#else
                sprintf(cmdCtx.cmd, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\r\n");
                sprintf(cmdCtx.expectedResponse, "10100000FFFFFFFF0000000000000000");
#endif

                Sc_SendCommand(&cmdCtx);

                //PrintLog("responseLen = %u\n", cmdCtx.responseLen);
                //PrintLog("response = %s", cmdCtx.response);

#if SC_IS_SW
                if (cmdCtx.responseLen != 144)
#else
                if (cmdCtx.responseLen != 130)
#endif
                {
                    PrintLog("bad responseLen!!!\n");
                    dead();
                }

                uint8_t auth1r[64];

                for (uint32_t i = 0; i < 64; ++i)
                {
                    uint32_t ii = (i * 2);

#if SC_IS_SW
                    ii += 12;
#endif

                    char str[3];

                    str[0] = cmdCtx.response[ii];
                    str[1] = cmdCtx.response[(ii + 1)];
                    str[2] = 0;

                    uint32_t val;
                    sscanf(str, "%x", &val);

                    uint8_t *valx = (uint8_t *)&val;
                    auth1r[i] = valx[0];

                    // PrintLog("%02X", (uint32_t)auth1r[i]);
                }

                // PrintLog("\n");

                uint8_t data[48];
                aes_decrypt_cbc(&auth1r[16], 48, data, sc2tb_key_schedule, 128, (BYTE *)iv);

                //PrintLog("data = ");

                //for (uint32_t i = 0; i < 48; ++i)
                //    PrintLog("%02X", (uint32_t)data[i]);

                //PrintLog("\n");

                uint8_t new_data[48];
                memset(new_data, 0, 48);

                memcpy(&new_data[0], &data[8], 8);
                memcpy(&new_data[8], &data[0], 8);

                //PrintLog("new_data = ");

                //for (uint32_t i = 0; i < 48; ++i)
                //    PrintLog("%02X", (uint32_t)new_data[i]);

                //PrintLog("\n");

                uint8_t auth2r[64];
                memset(auth2r, 0, 64);

                aes_encrypt_cbc(&new_data[0], 48, &auth2r[16], tb2sc_key_schedule, 128, (BYTE *)iv);

                auth2r[0] = 0x10;
                auth2r[1] = 0x01;

                //PrintLog("auth2r = ");

                //for (uint32_t i = 0; i < 64; ++i)
                //    PrintLog("%02X", (uint32_t)auth2r[i]);

                //PrintLog("\n");

                char auth2r_str[256];

                for (uint32_t i = 0; i < 64; ++i)
                {
                    uint32_t ii = (i * 2);

                    char str[3];
                    sprintf(str, "%02X", (uint32_t)auth2r[i]);

                    auth2r_str[ii] = str[0];
                    auth2r_str[(ii + 1)] = str[1];
                }

                auth2r_str[128] = 0;

#if SC_IS_SW

                {
                    sprintf(cmdCtx.cmd, "SETCMDLONG FF FF\r\n");
                    sprintf(cmdCtx.expectedResponse, "OK 00000000");

                    Sc_SendCommand(&cmdCtx);

                    // PrintLog("responseLen = %u\n", cmdCtx.responseLen);
                    // PrintLog("response = %s", cmdCtx.response);
                }

#endif

                {
#if SC_IS_SW
                    sprintf(cmdCtx.cmd, "AUTH2 %s\r\n", auth2r_str);
                    sprintf(cmdCtx.expectedResponse, "OK 00000000");
#else
                    sprintf(cmdCtx.cmd, "%s\r\n", auth2r_str);
                    sprintf(cmdCtx.expectedResponse, "SC_SUCCESS");
#endif

                    // PrintLog("xxx = %s\n", cmdCtx.expectedResponse);

                    Sc_SendCommand(&cmdCtx);
                }
            }
        }

        PrintLog("SC Auth success!\n");

        {
            bool banksel = Sc_GetScBanksel();
            bool lite = Sc_GetScLite();

#if SC_IS_SW

            // Sc_Tx: w 1211 03:DF
            // Sc_Rx: # addr=00001211 num=01 val=03:B1
            // Sc_Rx: OK 00000000:3A

            if (banksel)
            {
                sprintf(cmdCtx.cmd, "w 1224 %s\r\n", banksel ? "00" : "ff");
                sprintf(cmdCtx.expectedResponse, "OK 00000000");

                Sc_SendCommand(&cmdCtx);
            }

            {
                sprintf(cmdCtx.cmd, "w 1211 03\r\n");
                sprintf(cmdCtx.expectedResponse, "OK 00000000");

                Sc_SendCommand(&cmdCtx);
            }

            {
                sprintf(cmdCtx.cmd, "w f00 %s\r\n", lite ? "01" : "00");
                sprintf(cmdCtx.expectedResponse, "OK 00000000");

                Sc_SendCommand(&cmdCtx);
            }

#else

            // Sc_Tx: w 48c24 00
            // Sc_Rx: [mullion]$ w 48c24 00
            // Sc_Rx: w complete!

            if (banksel)
            {
                sprintf(cmdCtx.cmd, "w 48c24 %s\r\n", banksel ? "00" : "ff");
                sprintf(cmdCtx.expectedResponse, "w complete!");

                Sc_SendCommand(&cmdCtx);
            }

            {
                sprintf(cmdCtx.cmd, "w 48c11 03\r\n");
                sprintf(cmdCtx.expectedResponse, "w complete!");

                Sc_SendCommand(&cmdCtx);
            }

            {
                sprintf(cmdCtx.cmd, "w 3000 %s\r\n", lite ? "01" : "00");
                sprintf(cmdCtx.expectedResponse, "w complete!");

                Sc_SendCommand(&cmdCtx);
            }

#endif

            if (banksel)
            {
                Led_SetBlinkIntervalInMs(100);
                Led_SetStatus(LED_STATUS_BLINK);

                while (1)
                {
                    busy_wait_ms(1000);

                    Sc_Puts("shutdown\r\n");
                }
            }
        }
    }
}

bool Sc_GetTrigger()
{
    return scContext.trigger;
}

void Sc_ClearTrigger()
{
    scContext.trigger = false;
}

bool Sc_GetSuccess()
{
    return scContext.success;
}

void Sc_ClearSuccess()
{
    scContext.success = false;
}

void Sc_Putc(char c)
{
    if (!Sc_IsInited())
        return;

#if SC_IS_SW

    if (c == '\n')
        return;

    if (c == '\r')
    {
        uint32_t checksum = 0;

        for (uint32_t i = 0; i < scContext.txBufLen; ++i)
            checksum += scContext.txBuf[i];

        checksum %= 0x100;

        // PrintLog("checksum = 0x%X\n", (uint32_t)checksum);

        PrintLog("Sc_Tx: ");
        PrintLog((const char *)scContext.txBuf);
        PrintLog(":%02X\n", checksum);

        Uart_Putc(scContext.uartId, ':');

        {
            char str[8];
            sprintf(str, "%02X", (uint32_t)checksum);

            Uart_Puts(scContext.uartId, str);
        }

        Uart_Putc(scContext.uartId, '\r');
        Uart_Putc(scContext.uartId, '\n');

        scContext.txBufLen = 0;
        scContext.txBuf[scContext.txBufLen] = 0;

        return;
    }

#endif

    {
        scContext.txBuf[scContext.txBufLen] = c;
        ++scContext.txBufLen;
        scContext.txBuf[scContext.txBufLen] = 0;

        if ((c == '\n') || (scContext.txBufLen >= 2047))
        {
            PrintLog("Sc_Tx: ");
            PrintLog((const char *)scContext.txBuf);

            scContext.txBufLen = 0;
        }
    }

    Uart_Putc(scContext.uartId, c);

    scContext.lastScTxTimeInMs = get_time_in_ms();
}

void Sc_Puts(const char *buf)
{
    if (!Sc_IsInited())
        return;

    while (*buf != 0)
    {
        Sc_Putc(*buf);
        ++buf;
    }
}

void Sc_SendCommand(volatile struct Sc_SendCommandContext_s *ctx)
{
    if (!Sc_IsInited())
        return;

    ctx->done = false;
    sync();

    scContext.sendCommandCtx = ctx;
    sync();

    Sc_Puts(ctx->cmd);
    sync();

    uint64_t t1 = get_time_in_ms();

    while (!ctx->done)
    {
        uint64_t t2 = get_time_in_ms();

        if ((t2 - t1) > 2000)
            watchdog_reboot(0, 0, 0);
    }
}