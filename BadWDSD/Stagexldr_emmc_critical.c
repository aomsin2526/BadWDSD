FUNC_DEF uint8_t FetchIsEmmc()
{
    return (sc_read_flash_type() == 0x67) ? 1 : 0;
}

// do not change!!!
static const uint32_t emmc_sector_size = 512;
static const uint32_t emmc_sector_count = ((256 * 1024 * 1024) / emmc_sector_size);

FUNC_DEF uint8_t emmc_is_sector_idx_valid_for_read(uint32_t sector_idx)
{
    if (sector_idx >= emmc_sector_count)
        return 0;

    return 1;
}

FUNC_DEF uint8_t emmc_is_sector_idx_valid_for_write(uint32_t sector_idx)
{
    if (sector_idx >= emmc_sector_count)
        return 0;

    // don't write to lv0ldr!!!
    if (sector_idx < (0x40000 / emmc_sector_size))
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

    // start_read_sector

    {
        {
            volatile uint16_t* p_intenreg = (volatile uint16_t*)0x2401F044012;
            volatile uint16_t* p_count = (volatile uint16_t*)0x2401F044006;

            volatile uint16_t* p_high = (volatile uint16_t*)0x2401F044008;
            volatile uint16_t* p_low = (volatile uint16_t*)0x2401F04400A;

            *p_intenreg = 9;
            *p_count = 0;

            *p_high = (uint16_t)(sector_idx / 0x10000);
            *p_low = (uint16_t)sector_idx;

            eieio();
        }

        {
            volatile uint16_t* p_cmd = (volatile uint16_t*)0x2401F04400C;
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

        volatile uint16_t* p_intreg = (volatile uint16_t*)0x2401F044010;

        if (!((*p_intreg & 9) == 1))
        {
            puts("read_sector failed\n");
            dead_beep();
        }

        *p_intreg &= 9;

        //

        for (uint32_t i2 = 0; i2 < (emmc_sector_size / 2); ++i2)
        {
            const volatile uint16_t* p_data = (const volatile uint16_t*)0x2401F044000;

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
            volatile uint16_t* p_intreg = (volatile uint16_t*)0x2401F044010;
            volatile uint16_t* p_intenreg = (volatile uint16_t*)0x2401F044012;
            volatile uint16_t* p_count = (volatile uint16_t*)0x2401F044006;

            volatile uint16_t* p_high = (volatile uint16_t*)0x2401F044008;
            volatile uint16_t* p_low = (volatile uint16_t*)0x2401F04400A;

            *p_intreg = 0xf;
            *p_intenreg = 4;
            *p_count = 0;

            *p_high = 0;
            *p_low = 0;

            eieio();
        }

        {
            volatile uint16_t* p_cmd = (volatile uint16_t*)0x2401F04400C;
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

        volatile uint16_t* p_intreg = (volatile uint16_t*)0x2401F044010;

        if (!((*p_intreg & 0xc) == 4))
        {
            puts("read_sector idle failed\n");
            dead_beep();
        }

        *p_intreg &= 0xc;

        //
    }
}