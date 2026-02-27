#include "Include.h"

#if LED_IS_WS2812
#include "hardware/pio.h"
#include "build/generated/ws2812.pio.h"

void ws2812_put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(LED_WS2812_PIO, 0, pixel_grb << 8u);
}

void ws2812_put_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t mask = (green << 16) | (red << 8) | (blue << 0);
    ws2812_put_pixel(mask);
}

void ws2812_init()
{
    uint32_t offset = pio_add_program(LED_WS2812_PIO, &ws2812_program);
    ws2812_program_init(LED_WS2812_PIO, 0, offset, LED_PIN_ID, 800000, true);

    ws2812_put_rgb(0, 0, 0);
}
#endif

volatile bool ledIsInited = false;
volatile struct LedContext_s ledContext;

void Led_SetStatus(uint32_t status)
{
    ledContext.status = status;
}

void Led_SetBlinkIntervalInMs(uint32_t value)
{
    ledContext.blinkIntervalInMs = value;
}

bool Led_IsInited()
{
    return ledIsInited;
}

void Led_Init()
{
    ledContext.status = LED_STATUS_OFF;
    ledContext.blinkIntervalInMs = 500;

    ledContext.prevStatus = ledContext.status;

    ledContext.curLedStatus = false;

    ledContext.blink_t1 = 0;

#if LED_IS_WS2812
    ws2812_init();
#elif !LED_IS_NOT_GPIO
    gpio_deinit(LED_PIN_ID);

    gpio_init(LED_PIN_ID);
    gpio_set_dir(LED_PIN_ID, GPIO_OUT);

    gpio_put(LED_PIN_ID, false);
#endif

    ledIsInited = true;
    sync();
}

#if LED_IS_WS2812
#define LED_ON() ws2812_put_rgb(LED_RGB[0], LED_RGB[1], LED_RGB[2])
#define LED_OFF() ws2812_put_rgb(0, 0, 0)
#elif PICO_TYPE == PICO_TYPE_E_PICO_W
#define LED_ON() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1)
#define LED_OFF() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0)
#elif !LED_IS_NOT_GPIO
#define LED_ON() gpio_put(LED_PIN_ID, true)
#define LED_OFF() gpio_put(LED_PIN_ID, false)
#else
#error bad!!!
#endif

void Led_Thread()
{
    if (!Led_IsInited())
        return;

    if (ledContext.status != ledContext.prevStatus)
    {
        volatile uint32_t newStatus = ledContext.status;

        if (newStatus == LED_STATUS_ON)
        {
            ledContext.curLedStatus = true;

            LED_ON();
        }
        else if (newStatus == LED_STATUS_OFF)
        {
            ledContext.curLedStatus = false;

            LED_OFF();
        }

        ledContext.prevStatus = newStatus;
    }

    if (ledContext.status == LED_STATUS_BLINK)
    {
        uint64_t t2 = get_time_in_ms();

        if ((t2 - ledContext.blink_t1) >= ledContext.blinkIntervalInMs)
        {
            ledContext.blink_t1 = t2;

            ledContext.curLedStatus = !ledContext.curLedStatus;

            if (ledContext.curLedStatus)
                LED_ON();
            else
                LED_OFF();
        }
    }
}

#undef LED_OFF
#undef LED_ON