typedef unsigned int uint32_t;

uint32_t entry(uint32_t skip, uint32_t* buf, uint32_t len)
{
    // skip always 0
    // F5 04 FD 92 0D FD 54 B7 38 FC 88 95
    uint32_t* data = (uint32_t*)0x39000;

    for (uint32_t i = 0; i < len; ++i)
        buf[i] = data[i];

    return 0;
}