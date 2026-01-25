#include "Include.h"

void Hold_Init()
{
    if (!Gpio_GetOnce(HOLD_PIN_ID))
    {
        Led_SetStatus(LED_STATUS_OFF);
        dead();
    }
}