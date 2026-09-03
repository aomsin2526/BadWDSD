//

FUNC_DEF uint8_t FetchIsEmmc()
{
    //return (sc_read_flash_type() == 0x67) ? 1 : 0;

    const uint32_t sbVersion = volatile_read_uint32(0x24000087000);

    if ((sbVersion & 0xFF000000) != 0x04000000)
        return 0;

    const uint32_t a = volatile_read_uint32(0x24000FFF020);
    const uint32_t b = volatile_read_uint32(0x24000FFF028);
    const uint32_t c = volatile_read_uint32(0x24000FFF02C);

    if (a != 0x1fc0000c)
        return 0;

    if (b != 0x203)
        return 0;

    if (c != 0x1fc00000)
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

FUNC_DEF uint8_t emmc_is_sector_idx_valid_for_write(uint32_t sector_idx)
{
    if (sector_idx >= emmc_sector_count)
        return 0;

    // don't write to lv0ldr!!!
    if (sector_idx < (0x40000 / emmc_sector_size))
        return 0;

#if 1
    // only allow write to ros header...
    if (sector_idx != (0xC0000 / emmc_sector_size))
        return 0;
#endif

    return 1;
}

FUNC_DEF uint8_t emmc_is_interrupt_asserted()
{
    if (!is_emmc)
        dead_beep();

    const uint32_t v = volatile_read_uint32(0x24000FFF504);

    if ((v & 1) == 0)
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

    static const uint64_t a_data = 0x2401FC44000;

    static const uint64_t a_count = 0x2401FC44006;

    static const uint64_t a_high = 0x2401FC44008;
    static const uint64_t a_low = 0x2401FC4400A;

    static const uint64_t a_cmd = 0x2401FC4400C;

    static const uint64_t a_intreg = 0x2401FC44010;
    static const uint64_t a_intenreg = 0x2401FC44012;

    // start_read_sector

    {
        {
            volatile_write_uint16(a_intenreg, 9);
            volatile_write_uint16(a_count, 0);

            volatile_write_uint16(a_high, (uint16_t)(sector_idx / 0x10000));
            volatile_write_uint16(a_low, (uint16_t)sector_idx);
        }

        {
            volatile_write_uint16(a_cmd, 0x18); // !!!!!! READ !!!!!!
        }
    }

    // read_sector

    uint64_t curOutBufOffset = 0; // in bytes

    for (uint32_t idx = 0; idx < sector_count; ++idx)
    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        uint16_t v_intreg = volatile_read_uint16(a_intreg);

        if (!((v_intreg & 9) == 1))
        {
            puts("read_sector failed\n");
            dead_beep();
        }

        v_intreg &= 9;
        volatile_write_uint16(a_intreg, v_intreg);

        //

        for (uint32_t i2 = 0; i2 < (emmc_sector_size / 2); ++i2)
        {
            const uint16_t v_data = volatile_read_uint16(a_data);

            *((uint16_t*)(&outBuf_u8[curOutBufOffset])) = v_data;
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
            volatile_write_uint16(a_intreg, 0xf);
            volatile_write_uint16(a_intenreg, 4);
            volatile_write_uint16(a_count, 0);

            volatile_write_uint16(a_high, 0);
            volatile_write_uint16(a_low, 0);
        }

        {
            volatile_write_uint16(a_cmd, 0xc); // !!!!!! IDLE !!!!!!
        }

        //
    }

    // idle

    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        uint16_t v_intreg = volatile_read_uint16(a_intreg);

        if (!((v_intreg & 0xc) == 4))
        {
            puts("read_sector idle failed\n");
            dead_beep();
        }

        v_intreg &= 0xc;
        volatile_write_uint16(a_intreg, v_intreg);

        //
    }

    //
}

FUNC_DEF void emmc_erase_sector(uint32_t sector_idx)
{
    if (!is_emmc)
        dead_beep();

    //

    if (!emmc_is_sector_idx_valid_for_write(sector_idx))
    {
        puts("bad sector_idx!\n");
        dead_beep();
    }

    //

    static const uint64_t a_count = 0x2401FC44006;

    static const uint64_t a_high = 0x2401FC44008;
    static const uint64_t a_low = 0x2401FC4400A;

    static const uint64_t a_cmd = 0x2401FC4400C;

    static const uint64_t a_intreg = 0x2401FC44010;
    static const uint64_t a_intenreg = 0x2401FC44012;

    // start_erase_sector

    {
        {
            volatile_write_uint16(a_intenreg, 0xc);
            volatile_write_uint16(a_count, 1);

            volatile_write_uint16(a_high, (uint16_t)(sector_idx / 0x10000));
            volatile_write_uint16(a_low, (uint16_t)sector_idx);
        }

        {
            volatile_write_uint16(a_cmd, 0x40); // !!!!!! ERASE !!!!!!
        }
    }

    // erase_sector

    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        uint16_t v_intreg = volatile_read_uint16(a_intreg);

        if (!((v_intreg & 0xc) == 4))
        {
            puts("erase_sector erase failed\n");
            dead_beep();
        }

        v_intreg &= 0xc;
        volatile_write_uint16(a_intreg, v_intreg);

        //
    }
}

// inBuf[emmc_sector_size]
FUNC_DEF void emmc_write_sector(uint32_t sector_idx, const void* inBuf)
{
    if (!is_emmc)
        dead_beep();

    const uint8_t* inBuf_u8 = (const uint8_t*)inBuf;

    //

    emmc_erase_sector(sector_idx);

    //

    static const uint64_t a_data = 0x2401FC44000;

    static const uint64_t a_count = 0x2401FC44006;

    static const uint64_t a_high = 0x2401FC44008;
    static const uint64_t a_low = 0x2401FC4400A;

    static const uint64_t a_cmd = 0x2401FC4400C;

    static const uint64_t a_intreg = 0x2401FC44010;
    static const uint64_t a_intenreg = 0x2401FC44012;

    // start_write_sector

    {
        {
            volatile_write_uint16(a_intenreg, 0xe);
            volatile_write_uint16(a_count, 1);

            volatile_write_uint16(a_high, (uint16_t)(sector_idx / 0x10000));
            volatile_write_uint16(a_low, (uint16_t)sector_idx);
        }

        {
            volatile_write_uint16(a_cmd, 0x38); // !!!!!! WRITE !!!!!!
        }
    }

    // write_sector

    uint64_t curInBufOffset = 0; // in bytes

    {
        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        uint16_t v_intreg = volatile_read_uint16(a_intreg);

        if (!((v_intreg & 2) != 0))
        {
            puts("write_sector failed 1\n");
            dead_beep();
        }

        v_intreg = 2;
        volatile_write_uint16(a_intreg, v_intreg);

        //

        for (uint32_t i2 = 0; i2 < (emmc_sector_size / 2); ++i2)
        {
            volatile_write_uint16(a_data, *((const uint16_t*)(&inBuf_u8[curInBufOffset])));
            curInBufOffset += 2;
        }

        //

        while (!emmc_is_interrupt_asserted()) {}

        //

        v_intreg = volatile_read_uint16(a_intreg);

        if (!((v_intreg & 4) != 0))
        {
            puts("write_sector failed 2\n");
            dead_beep();
        }

        v_intreg = 4;
        volatile_write_uint16(a_intreg, v_intreg);

        //
    }
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

FUNC_DEF void emmc_write(uint64_t offset, const void* data, uint64_t size)
{
    const uint8_t* dataa = (const uint8_t*)data;

    if (size == 0)
        return;

    if ((offset + size) > emmc_max_size)
    {
        puts("emmc_write overflow!!!\n");
        dead_beep();
    }

    static const uint32_t sector_size = emmc_sector_size;

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

        if ((zzz != 0) || (processSize != sector_size))
        {
            emmc_read_sectors(sector_idx, 1, buf);

            memcpy(&buf[zzz], &dataa[curDataOffset], xxx);

            emmc_write_sector(sector_idx, buf);

            curOffset += xxx;
            curDataOffset += xxx;

            left -= xxx;
        }
        else
        {
            emmc_write_sector(sector_idx, &dataa[curDataOffset]);

            curOffset += processSize;
            curDataOffset += processSize;

            left -= processSize;
        }
    }
}

//

struct ros_s
{
    uint64_t offset1; // 0x20 or 0x700010
    uint64_t offset2; // 0x20 or 0x700010

    uint64_t region_size; // 0xE00000

    uint64_t unknown; // 0
};

_Static_assert((sizeof(struct ros_s) == 32), "ros_s too big!");

FUNC_DEF void check_ros(const struct ros_s* ros)
{
    if (sizeof(struct ros_s) != 32)
    {
        puts("bad sizeof!\n");
        dead_beep();
    }

    if (!((ros->offset1 == 0x20) || (ros->offset1 == 0x700010)))
    {
        puts("bad offset1!\n");
        dead_beep();
    }

    if (!((ros->offset2 == 0x20) || (ros->offset2 == 0x700010)))
    {
        puts("bad offset2!\n");
        dead_beep();
    }

    if (ros->offset1 != ros->offset2)
    {
        puts("ros->offset1 != ros->offset2!\n");

        puts("offset1 = ");
        print_hex(ros->offset1);
        puts("\n");

        puts("offset2 = ");
        print_hex(ros->offset2);
        puts("\n");
    }

    if (ros->region_size != 0xE00000)
    {
        puts("bad region_size!\n");
        dead_beep();
    }

    if (ros->unknown != 0)
    {
        puts("bad unknown!\n");
        dead_beep();
    }
}

FUNC_DEF uint8_t get_os_bank_indicator_from_ros()
{
    struct ros_s ros;
    emmc_read(0xC0000, &ros, sizeof(ros));
    check_ros(&ros);

    uint64_t offset = ros.offset1; // lv0ldr use this

    if (offset == 0x20) // ros0
        return 0xff;
    else if (offset == 0x700010) // ros1
        return 0x00;

    puts("bad!!!\n");
    dead_beep();
    return 0;
}

FUNC_DEF void switch_ros(uint8_t os_bank_indicator)
{
    puts("switch_ros()\n");

    puts("os_bank_indicator = ");
    print_hex(os_bank_indicator);
    puts("\n");

    uint8_t ros_os_bank_indicator = get_os_bank_indicator_from_ros();

    puts("ros_os_bank_indicator = ");
    print_hex(ros_os_bank_indicator);
    puts("\n");

    if (os_bank_indicator == ros_os_bank_indicator)
    {
        puts("skip\n");
        return;
    }

    uint32_t expected_ros_crc32 = 0;
    uint32_t ros_flash_offset = 0;

    uint64_t ros_offset = 0; // 0x20 or 0x700010

    if (os_bank_indicator == 0xff) // ros0
    {
        puts("switch to ros0...\n");

        expected_ros_crc32 = sc_read_ros0_crc32();
        ros_flash_offset = 0xC0020;

        ros_offset = 0x20;
    }
    else if (os_bank_indicator == 0x00) // ros1
    {
        puts("switch to ros1...\n");

        expected_ros_crc32 = sc_read_ros1_crc32();
        ros_flash_offset = 0x7C0010;

        ros_offset = 0x700010;
    }
    else
    {
        puts("bad!!!\n");
        dead_beep();
    }

    puts("expected_ros_crc32 = ");
    print_hex(expected_ros_crc32);
    puts("\n");

    puts("ros_flash_offset = ");
    print_hex(ros_flash_offset);
    puts("\n");

    puts("ros_offset = ");
    print_hex(ros_offset);
    puts("\n");

    uint32_t ros_crc32 = 0;

    {
        uint8_t* tmpBuf = (uint8_t*)0x3000000;
        emmc_read(ros_flash_offset, tmpBuf, 0x6FFFF0);

        ros_crc32 = crc32c(0, tmpBuf, 0x6FFFF0);

        if (ros_crc32 != expected_ros_crc32)
        {
            puts("crc32 check failed!!!\n");
            dead_beep();
        }

        puts("crc32 check ok!\n");
    }

    {
        struct ros_s ros;

        ros.offset1 = ros_offset;
        ros.offset2 = ros_offset;

        ros.region_size = 0xE00000;
        ros.unknown = 0;

        check_ros(&ros);
        emmc_write(0xC0000, &ros, sizeof(ros));
    }

    puts("switch_ros done\n");
}

//