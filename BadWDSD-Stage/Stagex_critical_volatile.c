FUNC_DEF uint8_t volatile_read_uint8(uint64_t addr)
{
    eieio();
    uint8_t v = *((const volatile uint8_t*)addr);
    eieio();

    return v;
}

FUNC_DEF void volatile_write_uint8(uint64_t addr, uint8_t v)
{
    eieio();
    *((volatile uint8_t*)addr) = v;
    eieio();
}

FUNC_DEF uint16_t volatile_read_uint16(uint64_t addr)
{
    eieio();
    uint16_t v = *((const volatile uint16_t*)addr);
    eieio();

    return v;
}

FUNC_DEF void volatile_write_uint16(uint64_t addr, uint16_t v)
{
    eieio();
    *((volatile uint16_t*)addr) = v;
    eieio();
}

FUNC_DEF uint32_t volatile_read_uint32(uint64_t addr)
{
    eieio();
    uint32_t v = *((const volatile uint32_t*)addr);
    eieio();

    return v;
}

FUNC_DEF void volatile_write_uint32(uint64_t addr, uint32_t v)
{
    eieio();
    *((volatile uint32_t*)addr) = v;
    eieio();
}

FUNC_DEF uint64_t volatile_read_uint64(uint64_t addr)
{
    eieio();
    uint64_t v = *((const volatile uint64_t*)addr);
    eieio();

    return v;
}

FUNC_DEF void volatile_write_uint64(uint64_t addr, uint64_t v)
{
    eieio();
    *((volatile uint64_t*)addr) = v;
    eieio();
}

FUNC_DEF void volatile_memcpy(volatile void* dest, const volatile void* src, uint64_t count)
{
    if ((((uint64_t)dest % 8) == 0) && (((uint64_t)src % 8) == 0) && ((count % 8) == 0))
    {
        volatile uint64_t* destt = (volatile uint64_t*)dest;
        const volatile uint64_t* srcc = (const volatile uint64_t*)src;

        for (uint64_t i = 0; i < (count / 8); ++i)
            volatile_write_uint64((uint64_t)(&destt[i]), volatile_read_uint64((uint64_t)(&srcc[i])));
    }
    else if ((((uint64_t)dest % 4) == 0) && (((uint64_t)src % 4) == 0) && ((count % 4) == 0))
    {
        volatile uint32_t* destt = (volatile uint32_t*)dest;
        const volatile uint32_t* srcc = (const volatile uint32_t*)src;

        for (uint64_t i = 0; i < (count / 4); ++i)
            volatile_write_uint32((uint64_t)(&destt[i]), volatile_read_uint32((uint64_t)(&srcc[i])));
    }
    else if ((((uint64_t)dest % 2) == 0) && (((uint64_t)src % 2) == 0) && ((count % 2) == 0))
    {
        volatile uint16_t* destt = (volatile uint16_t*)dest;
        const volatile uint16_t* srcc = (const volatile uint16_t*)src;

        for (uint64_t i = 0; i < (count / 2); ++i)
            volatile_write_uint16((uint64_t)(&destt[i]), volatile_read_uint16((uint64_t)(&srcc[i])));
    }
    else
    {
        volatile uint8_t* destt = (volatile uint8_t*)dest;
        const volatile uint8_t* srcc = (const volatile uint8_t*)src;

        for (uint64_t i = 0; i < count; ++i)
            volatile_write_uint8((uint64_t)(&destt[i]), volatile_read_uint8((uint64_t)(&srcc[i])));
    }
}