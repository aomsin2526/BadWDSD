#include "Include.h"

void GPIO_FLOATTOLOW(uint32_t pinId)
{
    gpio_set_dir(pinId, GPIO_OUT);
    gpio_put(pinId, false);

    gpio_set_drive_strength(pinId, GPIO_DRIVE_STRENGTH_12MA);

    gpio_set_function(pinId, GPIO_FUNC_SIO);
}

void GPIO_FLOATTOHIGH(uint32_t pinId)
{
    gpio_set_dir(pinId, GPIO_OUT);
    gpio_put(pinId, true);

    gpio_set_drive_strength(pinId, GPIO_DRIVE_STRENGTH_12MA);

    gpio_set_function(pinId, GPIO_FUNC_SIO);
}

void GPIO_FLOAT(uint32_t pinId)
{
    io_bank0_hw->io[pinId].ctrl = GPIO_FUNC_NULL << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
}

void GPIO_FLOATTOLOW2(uint32_t pinId1, uint32_t pinId2)
{
    GPIO_FLOATTOLOW(pinId1);

    if (pinId2 != pinId1)
        GPIO_FLOATTOLOW(pinId2);
}

void GPIO_FLOAT2(uint32_t pinId1, uint32_t pinId2)
{
    if (pinId2 != pinId1)
        GPIO_FLOAT(pinId2);

    GPIO_FLOAT(pinId1);
}

bool Gpio_GetOnce(uint32_t pinId)
{
    gpio_deinit(pinId);

    gpio_init(pinId);

    gpio_set_dir(pinId, GPIO_IN);
    gpio_pull_up(pinId);

    busy_wait_ms(50);

    bool val = gpio_get(pinId);

    gpio_deinit(pinId);

    return val;
}