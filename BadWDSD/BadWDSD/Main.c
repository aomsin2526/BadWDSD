#include "Include.h"

uint16_t swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}

uint64_t swap_uint64(uint64_t val)
{
    return ((val << 56) & 0xff00000000000000UL) |
           ((val << 40) & 0x00ff000000000000UL) |
           ((val << 24) & 0x0000ff0000000000UL) |
           ((val << 8) & 0x000000ff00000000UL) |
           ((val >> 8) & 0x00000000ff000000UL) |
           ((val >> 24) & 0x0000000000ff0000UL) |
           ((val >> 40) & 0x000000000000ff00UL) |
           ((val >> 56) & 0x00000000000000ffUL);
}

void Watchdog()
{
    uint64_t t1 = get_time_in_ms();

    while (!Sc_GetSuccess())
    {
        uint64_t t2 = get_time_in_ms();

        if ((t2 - t1) > 2500)
        {
            Sc_ClearShutdownSuccess();
            Sc_Puts("shutdown");

            {
                uint64_t st1 = get_time_in_ms();

                while (!Sc_GetShutdownSuccess())
                {
                    uint64_t st2 = get_time_in_ms();

                    if ((st2 - st1) > 10000)
                        break;
                }
            }

            busy_wait_ms(500);
            Sc_Puts("powersw");
            break;
        }
    }
}

void Sc_Thread_x16_Stage0()
{
    PrintLog("Sc_Thread_x16_Stage0()\n");

    Led_SetBlinkIntervalInMs(1000);
    Led_SetStatus(LED_STATUS_BLINK);

    Sc_Init();
    Hold_Init();

    uint8_t wdslData0[32];
    uint8_t wdslData1[32];

    uint8_t data[32];
    memset(data, 0x0, 32);

    {
        // stage0

        uint64_t *d = (uint64_t *)data;

        d[0] = swap_uint64(0x480000057C6802A6);
        d[1] = swap_uint64(0x3863FFFCE8830018);
        d[2] = swap_uint64(0x7C8903A64E800420);
        d[3] = swap_uint64(0x000002401F031000);
    }

    Xdr_GenerateReadyWDSLData_x16(data, wdslData0, wdslData1);

    Led_SetStatus(LED_STATUS_ON);

    while (1)
    {
        if (Sc_GetTrigger())
        {
            //

            Sc_ClearSuccess();

            //

            Xdr_SendEnableSLE_x16_PerDevice(0);
            Xdr_SendWDSD_x16_PerDevice(0, wdslData0);

            //

            Xdr_SendEnableSLE_x16_PerDevice(1);
            Xdr_SendWDSD_x16_PerDevice(1, wdslData1);

            //

            Led_SetBlinkIntervalInMs(100);
            Led_SetStatus(LED_STATUS_BLINK);

            //

            busy_wait_ms(200);

            //

            Xdr_SendDisableSLE_x16_PerDevice(1);
            Xdr_SendDisableSLE_x16_PerDevice(0);

            //

            busy_wait_ms(300);

            //

            Led_SetStatus(LED_STATUS_ON);

            //

            Watchdog();

            //

            Sc_ClearTrigger();
        }
    }
}

void Sc_Thread_x32_Stage0()
{
    PrintLog("Sc_Thread_x32_Stage0()\n");

    Led_SetBlinkIntervalInMs(1000);
    Led_SetStatus(LED_STATUS_BLINK);

    Sc_Init();
    Hold_Init();

    uint8_t wdslData0[64];

    uint8_t data[32];
    memset(data, 0x0, 32);

    {
        // stage0

        uint64_t *d = (uint64_t *)data;

        d[0] = swap_uint64(0x480000057C6802A6);
        d[1] = swap_uint64(0x3863FFFCE8830018);
        d[2] = swap_uint64(0x7C8903A64E800420);
        d[3] = swap_uint64(0x000002401F031000);
    }

    Xdr_GenerateReadyWDSLData_x32(data, wdslData0);

    Led_SetStatus(LED_STATUS_ON);

    while (1)
    {
        if (Sc_GetTrigger())
        {
            //

            Sc_ClearSuccess();

            //

            Xdr_SendEnableSLE_x32_PerDevice(0);
            Xdr_SendWDSD_x32_PerDevice(0, wdslData0);

            //

            Led_SetBlinkIntervalInMs(100);
            Led_SetStatus(LED_STATUS_BLINK);

            //

            busy_wait_ms(200);

            //

            //Xdr_SendDisableSLE_x32_PerDevice(0);

            //
            
            busy_wait_ms(300);

            //

            Led_SetStatus(LED_STATUS_ON);

            //

            Watchdog();

            //

            Sc_ClearTrigger();
        }
    }
}

void Core1_Thread()
{
    while (1)
    {
        if (Led_IsInited())
            Led_Thread();

        if (Sc_IsInited())
            Sc_Thread();

        if (DebugUart_IsInited())
            DebugUart_Thread();
    }
}

void main()
{
#if 0
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    set_sys_clock_khz(250000, true);
#endif

#if !PICO_IS_ZERO
    GPIO_FLOATTOHIGH(TRISTATE_PIN_ID);
#endif

#if DEBUG_UART_ENABLED
    DebugUart_Init();
#endif

    Led_Init();
    multicore_launch_core1(Core1_Thread);

#if XDR_IS_X32
    Sc_Thread_x32_Stage0();
#else
    Sc_Thread_x16_Stage0();
#endif

    dead();
}
