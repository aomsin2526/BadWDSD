FUNC_DEF uint8_t FetchIsEmmc()
{
    if (IsLv1())
        return GetStagexContext()->cached_is_emmc;

    //return (sc_read_flash_type() == 0x67) ? 1 : 0;

    const volatile uint32_t* sbVersion = (const volatile uint32_t*)0x24000087000;

    if ((*sbVersion & 0xFF000000) != 0x04000000)
        return 0;

    const volatile uint32_t* a = (const volatile uint32_t*)0x24000FFF020;
    const volatile uint32_t* b = (const volatile uint32_t*)0x24000FFF028;
    const volatile uint32_t* c = (const volatile uint32_t*)0x24000FFF02C;

    if (*a != 0x1fc0000c)
        return 0;

    if (*b != 0x203)
        return 0;

    if (*c != 0x1fc00000)
        return 0;

    return 1;
}

// do not change!!!
static const uint32_t emmc_sector_size = 512;
static const uint32_t emmc_sector_count = 0x1893000;

static const uint64_t emmc_max_size = (((uint64_t)emmc_sector_count) * emmc_sector_size);

FUNC_DEF uint8_t emmc_is_sector_idx_valid_for_read(uint32_t sector_idx)
{
    if (sector_idx >= emmc_sector_count)
        return 0;

    return 1;
}

FUNC_DEF uint8_t emmc_is_interrupt_asserted()
{
    if (!is_emmc)
        dead_beep();

    const volatile uint32_t* v = (const volatile uint32_t*)0x24000FFF504;

    if ((*v & 1) == 0)
        return 1;

    return 0;
}

// outBuf[sector_count * emmc_sector_size]
FUNC_DEF void emmc_read_sectors(uint32_t sector_idx, uint32_t sector_count, void* outBuf)
{
    if (!is_emmc)
        dead_beep();

    uint8_t* outBuf_u8 = (uint8_t*)outBuf;

    //

    if (sector_count == 0)
    {
        puts("bad sector_count!\n");
        dead_beep();
    }

    for (uint32_t idx = 0; idx < sector_count; ++idx)
    {
        if (!emmc_is_sector_idx_valid_for_read(sector_idx + idx))
        {
            puts("bad sector_idx!\n");
            dead_beep();
        }
    }

    //

    uint64_t msr = 0;

    if (IsLv1())
        msr = intr_disable();

    // start_read_sector

    {
        {
            volatile uint16_t* p_intenreg = (volatile uint16_t*)0x2401FC44012;
            volatile uint16_t* p_count = (volatile uint16_t*)0x2401FC44006;

            volatile uint16_t* p_high = (volatile uint16_t*)0x2401FC44008;
            volatile uint16_t* p_low = (volatile uint16_t*)0x2401FC4400A;

            *p_intenreg = 9;
            *p_count = 0;

            *p_high = (uint16_t)(sector_idx / 0x10000);
            *p_low = (uint16_t)sector_idx;

            eieio();
        }

        {
            volatile uint16_t* p_cmd = (volatile uint16_t*)0x2401FC4400C;
            *p_cmd = 0x18; // !!!!!! READ !!!!!!

            eieio();
        }
    }

    // read_sector

    uint64_t curOutBufOffset = 0; // in bytes

    for (uint32_t idx = 0; idx < sector_count; ++idx)
    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        volatile uint16_t* p_intreg = (volatile uint16_t*)0x2401FC44010;

        if (!((*p_intreg & 9) == 1))
        {
            puts("read_sector failed\n");
            dead_beep();
        }

        *p_intreg &= 9;

        //

        for (uint32_t i2 = 0; i2 < (emmc_sector_size / 2); ++i2)
        {
            const volatile uint16_t* p_data = (const volatile uint16_t*)0x2401FC44000;

            *((uint16_t*)(&outBuf_u8[curOutBufOffset])) = *p_data;
            curOutBufOffset += 2;
        }

        //
    }

    // start_idle

    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        {
            volatile uint16_t* p_intreg = (volatile uint16_t*)0x2401FC44010;
            volatile uint16_t* p_intenreg = (volatile uint16_t*)0x2401FC44012;
            volatile uint16_t* p_count = (volatile uint16_t*)0x2401FC44006;

            volatile uint16_t* p_high = (volatile uint16_t*)0x2401FC44008;
            volatile uint16_t* p_low = (volatile uint16_t*)0x2401FC4400A;

            *p_intreg = 0xf;
            *p_intenreg = 4;
            *p_count = 0;

            *p_high = 0;
            *p_low = 0;

            eieio();
        }

        {
            volatile uint16_t* p_cmd = (volatile uint16_t*)0x2401FC4400C;
            *p_cmd = 0xc; // !!!!!! IDLE !!!!!!

            eieio();
        }

        //
    }

    // idle

    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        volatile uint16_t* p_intreg = (volatile uint16_t*)0x2401FC44010;

        if (!((*p_intreg & 0xc) == 4))
        {
            puts("read_sector idle failed\n");
            dead_beep();
        }

        *p_intreg &= 0xc;

        //
    }

    //

    if (IsLv1())
        msr_store(msr);

    //
}

FUNC_DEF void emmc_read(uint64_t offset, void* data, uint64_t size)
{
    uint8_t* dataa = (uint8_t*)data;

    if (size == 0)
        return;

    if ((offset + size) > emmc_max_size)
    {
        puts("emmc_read overflow!!!\n");
        dead_beep();
    }

    static const uint32_t sector_size = emmc_sector_size;
    uint32_t burst_size = (1 * 1024 * 1024);

    uint8_t buf[sector_size];

    uint64_t curOffset = offset;
    uint64_t curDataOffset = 0;

    uint64_t left = size;

    while (left > 0)
    {
        uint32_t processSize = (left > sector_size) ? sector_size : left;
        uint32_t zzz = (curOffset % sector_size);
        uint32_t yyy = (sector_size - zzz);
        uint32_t xxx = (yyy > processSize) ? processSize : yyy;

        uint32_t sector_idx = (curOffset / sector_size);

        while (burst_size > left)
            burst_size -= sector_size;

        if ((zzz != 0) || (processSize != sector_size))
        {
            emmc_read_sectors(sector_idx, 1, buf);

            memcpy(&dataa[curDataOffset], &buf[zzz], xxx);

            curOffset += xxx;
            curDataOffset += xxx;

            left -= xxx;
        }
        else if ((burst_size > 0) && (left >= burst_size) && ((burst_size % sector_size) == 0))
        {
            emmc_read_sectors(sector_idx, (burst_size / sector_size), &dataa[curDataOffset]);

            curOffset += burst_size;
            curDataOffset += burst_size;

            left -= burst_size;
        }
        else
        {
            emmc_read_sectors(sector_idx, 1, &dataa[curDataOffset]);

            curOffset += processSize;
            curDataOffset += processSize;

            left -= processSize;
        }
    }
}

FUNC_DEF void NorRead(uint32_t offset, void* outBuf, uint32_t readSize)
{
    if (is_emmc)
        dead_beep();

    if (readSize == 0)
        return;

    if ((offset + readSize) >= (16 * 1024 * 1024))
        dead_beep();

    memcpy(outBuf, (const void*)(0x2401F000000 + offset), readSize);
}

FUNC_DEF void FlashRead(uint32_t offset, void* outBuf, uint32_t readSize)
{
    if (readSize == 0)
        return;

    if (is_emmc)
        emmc_read(offset, outBuf, readSize);
    else
        NorRead(offset, outBuf, readSize);
}