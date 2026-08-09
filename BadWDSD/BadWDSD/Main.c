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

        if (((t2 - t1) > 3000) || Sc_GetNeedReboot())
        {
            Sc_ClearShutdownSuccess();
            Sc_Puts("shutdown");

            {
                uint64_t st1 = get_time_in_ms();

                while (!Sc_GetShutdownSuccess())
                {
                    uint64_t st2 = get_time_in_ms();

                    if ((st2 - st1) > 20000)
                        return;
                }
            }

            {
                Sc_ClearBringupSuccess();
                busy_wait_ms(500);

                uint64_t bt1 = get_time_in_ms();
                uint64_t lastPowerswTime = 0;

                while (!Sc_GetBringupSuccess())
                {
                    if ((get_time_in_ms() - lastPowerswTime) > 500)
                    {
                        Sc_Puts("powersw");
                        lastPowerswTime = get_time_in_ms();
                    }

                    uint64_t bt2 = get_time_in_ms();

                    if ((bt2 - bt1) > 10000)
                        return;
                }
            }

            break;
        }
    }
}

void XdrInitFail_Watchdog()
{
    if (Sc_GetXdrInitFail())
    {
        Led_SetBlinkIntervalInMs(100);
        Led_SetStatus(LED_STATUS_BLINK);

        busy_wait_ms(5000);

        Led_SetStatus(LED_STATUS_ON);
        Sc_Puts("shutdown");

        //

        Sc_ClearXdrInitFail();
        Sc_ClearTrigger();
    }
}

#if IS_EMMC

#include "../Stage0_emmc.bin.c"
#include "../Stage0b_emmc.bin.c"

#include "../Stagexldr_emmc.bin.c"

void Sc_Thread_x32_Stage0_emmc()
{
    PrintLog("Sc_Thread_x32_Stage0_emmc()\n");

    Led_SetBlinkIntervalInMs(1000);
    Led_SetStatus(LED_STATUS_BLINK);

    Sc_Init();
    Hold_Init();

    uint8_t wdslData0[64];
    uint8_t data[64];

    if (sizeof(bin2c_Stage0_emmc_bin) > 64)
    {
        PrintLog("bin2c_Stage0_emmc_bin size is bad!!!\n");
        dead();
    }

    if (sizeof(bin2c_Stage0b_emmc_bin) > 128)
    {
        PrintLog("bin2c_Stage0b_emmc_bin size is bad!!!\n");
        dead();
    }

    memcpy(data, bin2c_Stage0_emmc_bin, sizeof(bin2c_Stage0_emmc_bin));
    Xdr_GenerateReadyWDSLData_x32(data, wdslData0);

    Led_SetStatus(LED_STATUS_ON);

    while (1)
    {
        if (Sc_GetTrigger())
        {
            //

            Sc_ClearSuccess();
            Sc_ClearNeedReboot();

            //

            Xdr_SendEnableSLE_x32_PerDevice(0);
            Xdr_SendWDSD_x32_PerDevice(0, wdslData0);

            //

            Led_SetBlinkIntervalInMs(100);
            Led_SetStatus(LED_STATUS_BLINK);

            //

            busy_wait_ms(600);

            //

            DebugUart_Uninit();
            Sb_Init();

            for (size_t i = 0; i < sizeof(bin2c_Stage0b_emmc_bin); ++i)
                Sb_Putc(bin2c_Stage0b_emmc_bin[i]);

            busy_wait_ms(1);

            for (size_t i = 0; i < sizeof(bin2c_Stagexldr_emmc_bin); ++i)
            {
                Sb_Putc(bin2c_Stagexldr_emmc_bin[i]);
                busy_wait_us(100);
            }

            Sb_Uninit();
            DebugUart_Init();

            //

            Led_SetStatus(LED_STATUS_ON);

            //

            //Watchdog();

            //

            Sc_ClearTrigger();

            //
        }

        XdrInitFail_Watchdog();
    }
}

#elif XDR_IS_X32

void Sc_Thread_x32_Stage0_nor()
{
    PrintLog("Sc_Thread_x32_Stage0_nor()\n");

    Led_SetBlinkIntervalInMs(1000);
    Led_SetStatus(LED_STATUS_BLINK);

    Sc_Init();
    Hold_Init();

    uint8_t wdslData0[64];

    uint8_t data[64];
    memset(data, 0x0, 64);

    {
        // stage0

        uint64_t *d = (uint64_t *)data;

        d[0] = swap_uint64(0x480000057C6802A6);
        d[1] = swap_uint64(0x3863FFFCE8830018);
        d[2] = swap_uint64(0x7C8903A64E800420);
        d[3] = swap_uint64(0x000002401FF21000);
    }

    Xdr_GenerateReadyWDSLData_x32(data, wdslData0);

    Led_SetStatus(LED_STATUS_ON);

    while (1)
    {
        if (Sc_GetTrigger())
        {
            //

            Sc_ClearSuccess();
            Sc_ClearNeedReboot();

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

            //
        }

        XdrInitFail_Watchdog();
    }
}

#else

void Sc_Thread_x16_Stage0_nor()
{
    PrintLog("Sc_Thread_x16_Stage0_nor()\n");

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
        d[3] = swap_uint64(0x000002401FF21000);
    }

    Xdr_GenerateReadyWDSLData_x16(data, wdslData0, wdslData1);

    Led_SetStatus(LED_STATUS_ON);

    while (1)
    {
        if (Sc_GetTrigger())
        {
            //

            Sc_ClearSuccess();
            Sc_ClearNeedReboot();

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

            //
        }

        XdrInitFail_Watchdog();
    }
}

#endif

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

#if PICO_TYPE == PICO_TYPE_E_PICO_W
    if (cyw43_arch_init())
        dead();
#endif

#if PICO_TYPE == PICO_TYPE_E_PICO
    GPIO_FLOATTOHIGH(TRISTATE_PIN_ID);
#endif

#if DEBUG_UART_ENABLED
    DebugUart_Init();
#endif

    //

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

#if IS_EMMC
    PrintLog("Flash is eMMC\n");
#else
    PrintLog("Flash is NOR\n");
#endif

    //

    Led_Init();
    multicore_launch_core1(Core1_Thread);

#if IS_EMMC
    Sc_Thread_x32_Stage0_emmc();
#elif XDR_IS_X32
    Sc_Thread_x32_Stage0_nor();
#else
    Sc_Thread_x16_Stage0_nor();
#endif

    dead();
}
