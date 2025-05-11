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

void Button_Thread_x16_PatternTest()
{
    PrintLog("Button_Thread_x16_PatternTest()\n");

    Led_SetStatus(LED_STATUS_ON);

    gpio_deinit(BUTTON_PIN_ID);
    gpio_init(BUTTON_PIN_ID);

    gpio_set_dir(BUTTON_PIN_ID, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_ID);

    busy_wait_ms(500);

    bool prev_button_state = true;
    bool cur_button_state = prev_button_state;

    while (1)
    {
        cur_button_state = gpio_get(BUTTON_PIN_ID);

        // button pressed
        if (!cur_button_state && prev_button_state)
        {
            // addr = 0x100, value = 0x2a242c201a1c4c26
            // addr = 0x108, value = 0x28462240164a102e
            // addr = 0x110, value = 0x443e42343830324e
            // addr = 0x118, value = 0x1e184812363c3a14

            // addr = 0x120, value = 0x2b252d211b1d4d27
            // addr = 0x128, value = 0x29472341174b112f
            // addr = 0x130, value = 0x453f43353931334f
            // addr = 0x138, value = 0x1f194913373d3b15

            uint8_t wdslData[32];

            uint8_t data[32];
            memset(data, 0x0, 32);

            for (uint32_t i = 0; i < 32; ++i)
                data[i] = (0x10 + i);

            Xdr_ConvertDataToWDSLData_x16(data, wdslData);

            Xdr_SendEnableSLE_x16_PerDevice(0);
            Xdr_SendWDSD_x16_PerDevice(0, wdslData);

            //

            for (uint32_t i = 0; i < 32; ++i)
                data[i] = (0x30 + i);

            Xdr_ConvertDataToWDSLData_x16(data, wdslData);

            Xdr_SendEnableSLE_x16_PerDevice(1);
            Xdr_SendWDSD_x16_PerDevice(1, wdslData);

            //

            Led_SetBlinkIntervalInMs(100);
            Led_SetStatus(LED_STATUS_BLINK);

            //

            busy_wait_ms(6000);

            //

            Xdr_SendDisableSLE_x16_PerDevice(1);
            Xdr_SendDisableSLE_x16_PerDevice(0);

            //
        }

        // button released
        if (cur_button_state && !prev_button_state)
        {
            Led_SetStatus(LED_STATUS_ON);
        }

        prev_button_state = cur_button_state;
    }
}

void Button_Thread_x16_Stage0()
{
    PrintLog("Button_Thread_x16_Stage0()\n");

    Led_SetStatus(LED_STATUS_ON);

    gpio_deinit(BUTTON_PIN_ID);
    gpio_init(BUTTON_PIN_ID);

    gpio_set_dir(BUTTON_PIN_ID, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_ID);

    busy_wait_ms(500);

    bool prev_button_state = true;
    bool cur_button_state = prev_button_state;

    while (1)
    {
        cur_button_state = gpio_get(BUTTON_PIN_ID);

        // button pressed
        if (!cur_button_state && prev_button_state)
        {
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

            busy_wait_ms(6000);

            //

            Xdr_SendDisableSLE_x16_PerDevice(1);
            Xdr_SendDisableSLE_x16_PerDevice(0);

            //
        }

        // button released
        if (cur_button_state && !prev_button_state)
        {
            Led_SetStatus(LED_STATUS_ON);
        }

        prev_button_state = cur_button_state;
    }
}

void Watchdog()
{
    if (Sc_GetScBypass())
        return;

    uint64_t t1 = get_time_in_ms();

    while (!Sc_GetSuccess())
    {
        uint64_t t2 = get_time_in_ms();

        if ((t2 - t1) > 2500)
        {
            Sc_Puts("shutdown\r\n");
            busy_wait_ms(2500);

            if (Sc_GetScRecovery())
                Sc_Puts("bringup\r\n");
            else
                Sc_Puts("powersw\r\n");

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

            busy_wait_ms(500);

            //

            Xdr_SendDisableSLE_x16_PerDevice(1);
            Xdr_SendDisableSLE_x16_PerDevice(0);

            //

            Led_SetStatus(LED_STATUS_ON);

            //

            Sc_ClearTrigger();

            //

            Watchdog();
        }
    }
}

void Sc_Thread_x32_Stage0()
{
    PrintLog("Sc_Thread_x32_Stage0()\n");

    Led_SetBlinkIntervalInMs(1000);
    Led_SetStatus(LED_STATUS_BLINK);

    Sc_Init();

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

            busy_wait_ms(500);

            //

            //Xdr_SendDisableSLE_x32_PerDevice(0);

            //

            Led_SetStatus(LED_STATUS_ON);

            //

            Sc_ClearTrigger();

            //

            Watchdog();
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
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    set_sys_clock_khz(250000, true);

#if DEBUG_UART_ENABLED
    DebugUart_Init();
#endif

    Hold_Init();
    Led_Init();

    multicore_launch_core1(Core1_Thread);

#if BUTTON_MODE_ENABLED
    // Button_Thread_x16_PatternTest();
    Button_Thread_x16_Stage0();
#else

#if XDR_IS_X32
    Sc_Thread_x32_Stage0();
#else
    Sc_Thread_x16_Stage0();
#endif

#endif

    dead();
}
