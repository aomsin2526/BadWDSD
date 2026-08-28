#include "Include.h"

void dead()
{
    while (1) {}
}

uint64_t get_time_in_us()
{
    return time_us_64();
}

uint64_t get_time_in_ms()
{
    return time_us_64() / 1000;
}

void WaitInNs(uint64_t ns)
{
    // 1 cycle = 100 ns
    uint64_t waitCycle = 1 * (ns / 100);

    uint64_t junk = 0;

    while (1)
    {
        ++junk;

        if (junk == waitCycle)
            break;
    }
}