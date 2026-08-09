#define ENTRY_WAIT_IN_MS 100 // 1000

#if STAGEX_DEBUG_ENABLED

#if 0

Sc_Rx: #!0x0 = 25 2b 29 3b 39 37 35 27:1
Sc_Rx: #!0x8 = 31 23 3d 21 2d 2f 3f 33:7
Sc_Rx: #!0x10 = 07 1f 0f 1d 13 09 11 03:6
Sc_Rx: #!0x18 = 1b 19 01 15 05 0b 0d 17:6
Sc_Rx: #!0x20 = 24 2a 28 3a 38 36 34 26:4
Sc_Rx: #!0x28 = 30 22 3c 20 2c 2e 3e 32:9
Sc_Rx: #!0x30 = 06 1e 0e 1c 12 08 10 02:5
Sc_Rx: #!0x38 = 1a 18 00 14 04 0a 0c 16:6
Sc_Rx: #!0x40 = 25 2b 29 3b 39 37 35 27:4
Sc_Rx: #!0x48 = 31 23 3d 21 2d 2f 3f 33:A
Sc_Rx: #!0x50 = 07 1f 0f 1d 13 09 11 03:6
Sc_Rx: #!0x58 = 1b 19 01 15 05 0b 0d 17:6
Sc_Rx: #!0x60 = 24 2a 28 3a 38 36 34 26:4
Sc_Rx: #!0x68 = 30 22 3c 20 2c 2e 3e 32:9
Sc_Rx: #!0x70 = 06 1e 0e 1c 12 08 10 02:6
Sc_Rx: #!0x78 = 1a 18 00 14 04 0a 0c 16:6
Sc_Rx: #!0x80 = 7f 69 65 79 7b 75 6f 67:A
Sc_Rx: #!0x88 = 71 61 7d 63 6d 77 6b 73:9
Sc_Rx: #!0x90 = 4f 5d 4d 5f 53 51 4b 43:E
Sc_Rx: #!0x98 = 5b 55 59 45 47 49 41 57:3
Sc_Rx: #!0xa0 = 7e 68 64 78 7a 74 6e 66:C
Sc_Rx: #!0xa8 = 70 60 7c 62 6c 76 6a 72:B
Sc_Rx: #!0xb0 = 4e 5c 4c 5e 52 50 4a 42:0
Sc_Rx: #!0xb8 = 5a 54 58 44 46 48 40 56:5
Sc_Rx: #!0xc0 = 7f 69 65 79 7b 75 6f 67:D
Sc_Rx: #!0xc8 = 71 61 7d 63 6d 77 6b 73:C
Sc_Rx: #!0xd0 = 4f 5d 4d 5f 53 51 4b 43:0
Sc_Rx: #!0xd8 = 5b 55 59 45 47 49 41 57:6
Sc_Rx: #!0xe0 = 7e 68 64 78 7a 74 6e 66:C
Sc_Rx: #!0xe8 = 70 60 7c 62 6c 76 6a 72:B
Sc_Rx: #!0xf0 = 4e 5c 4c 5e 52 50 4a 42:0
Sc_Rx: #!0xf8 = 5a 54 58 44 46 48 40 56:6

#endif

FUNC_DEF void WDSDTest_1()
{
    uint8_t* data0 = (uint8_t*)(0x1000 + 0); // [64]
    uint8_t* wdslData0 = (uint8_t*)(0x1000 + 64); // [64]

    uint8_t* data1 = (uint8_t*)(0x1000 + 128); // [64]
    uint8_t* wdslData1 = (uint8_t*)(0x1000 + 192); // [64]

    for (uint8_t i = 0; i < 64; ++i)
        data0[i] = (0x0 + i); // 0x0 - 0x3f

    for (uint8_t i = 0; i < 64; ++i)
        data1[i] = (0x40 + i); // 0x40 - 0x7f

    Xdr_ConvertDataToWDSLData_x32(data0, wdslData0);
    Xdr_ConvertDataToWDSLData_x32(data1, wdslData1);

    uint32_t* cmd0 = (uint32_t*)(0x2000); // [64]
    uint32_t* cmd1 = (uint32_t*)(0x3000); // [64]

    for (uint8_t i = 0; i < 64; ++i)
        cmd0[i] = (XDR_SCMD(XDR_SCMD_SDW, 0, XDR_WDSL) | wdslData0[i]);

    for (uint8_t i = 0; i < 64; ++i)
        cmd1[i] = (XDR_SCMD(XDR_SCMD_SDW, 0, XDR_WDSL) | wdslData1[i]);
}

FUNC_DEF void WDSDTest_2()
{
    uint32_t* cmd0 = (uint32_t*)(0x2000); // [64]
    uint32_t* cmd1 = (uint32_t*)(0x3000); // [64]

    Xdr_Ch0_SendScmd(XDR_SCMD(XDR_SCMD_SDW, 0, XDR_CFG) | XDR_SLE_ENABLED_X32);
    Xdr_Ch1_SendScmd(XDR_SCMD(XDR_SCMD_SDW, 0, XDR_CFG) | XDR_SLE_ENABLED_X32);

    for (uint8_t i = 0; i < 64; ++i)
        Xdr_Ch0_SendScmd(cmd0[i]);

    for (uint8_t i = 0; i < 64; ++i)
        Xdr_Ch1_SendScmd(cmd1[i]);

    {
        register uint64_t r3 asm("r3");
        register uint64_t r4 asm("r4");
        register uint64_t r5 asm("r5");

        for (r5 = 0; r5 < 256; ++r5)
        {
            r3 = (0x100 + r5);
            r4 = 0;

            asm volatile("stb %0, 0(%1)"::"r"(r4),"r"(r3):);
            asm volatile("dcbf %0, %1"::"r"(r4),"r"(r3):);
            eieio();
        }
    }

    Xdr_Ch0_SendScmd(XDR_SCMD(XDR_SCMD_SDW, 0, XDR_CFG) | XDR_SLE_DISABLED_X32);
    Xdr_Ch1_SendScmd(XDR_SCMD(XDR_SCMD_SDW, 0, XDR_CFG) | XDR_SLE_DISABLED_X32);
}

FUNC_DEF void WDSDTest_3()
{
    for (uint64_t i = 0; i < 256; i += 8)
    {
        uint64_t addr = (0x100 + i);

        real_print_hex(addr);
        real_puts(" = ");
        real_hexdump((const void*)addr, 8);
        real_puts("\n");
    }
}

FUNC_DEF void WDSDTest()
{
    WDSDTest_1();
    WDSDTest_2();
    WDSDTest_3();

    dead_beep();
}

#endif

FUNC_DEF void Stage1()
{
    //

    real_sc_puts_init();
    sc_puts("Bad_WDSD Stage1 by Kafuu(aomsin2526)" " (Build Date: " __DATE__ " " __TIME__ ")\n");

    print_pc();

#if 0

    {
        const volatile uint32_t* spu_avail = (const volatile uint32_t*)0x20000509C38;

        puts("spu_avail = ");
        print_hex(*spu_avail); // 0xef for [INFO]: SPU enable [0, 1, 2, 5, 6, 7] 11101111
        puts("\n");
    }

#endif

    uint64_t myspu_id = calc_myspu_id();

    puts("myspu_id = ");
    print_decimal(myspu_id);
    puts("\n");

    HW_Init_SPU();

    //

#if STAGEX_DEBUG_ENABLED
    uint8_t stagex_debug_flag = sc_read_stagex_debug_flag();

    real_puts("stagex_debug_flag = ");
    real_print_hex(stagex_debug_flag);
    real_puts("\n");
#endif

    //

    is_emmc = FetchIsEmmc();

    puts("is_emmc = ");
    print_decimal(is_emmc);
    puts("\n");

    //

    if (!is_emmc)
    {
        uint8_t request_os_bank_indicator = sc_read_request_os_bank_indicator();

        puts("request_os_bank_indicator = ");
        print_hex(request_os_bank_indicator);
        puts("\n");

        if (request_os_bank_indicator == 0x1)
        {
            puts("Switching to ros0...\n");

            sc_write_os_bank_indicator(0xff);
            sc_write_recovery_mode_flag(0xff);
        }
        else if (request_os_bank_indicator == 0x2)
        {
            puts("Switching to ros1...\n");
            
            sc_write_os_bank_indicator(0x00);
            sc_write_recovery_mode_flag(0xff);
        }

        sc_write_request_os_bank_indicator(0xff);

        //

        uint8_t real_os_bank_indicator = sc_read_os_bank_indicator();

        puts("real_os_bank_indicator = ");
        print_hex(real_os_bank_indicator);
        puts("\n");

        uint8_t shadow_os_bank_indicator = ((real_os_bank_indicator == 0xFF) ? 0x1 : 0x2); // ros0 or ros1
        sc_write_shadow_os_bank_indicator(shadow_os_bank_indicator);

        puts("shadow_os_bank_indicator = ");
        print_hex(shadow_os_bank_indicator);
        puts("\n");
    }

    //

    uint8_t os_bank_indicator = get_os_bank_indicator();

    puts("os_bank_indicator = ");
    print_hex(os_bank_indicator);
    puts("\n");

    uint16_t fwVersion = CoreOS2_Bank_GetFWVersion(os_bank_indicator);

    puts("fwVersion = ");
    print_decimal(fwVersion);
    puts("\n");

#if LOGGING_ENABLED
    uint8_t tid = read_targetid();

    puts("tid = ");
    print_hex(tid);
    puts("\n");
#endif

    uint8_t isqCFW = CoreOS2_Bank_IsqCFW(os_bank_indicator);

    puts("isqCFW = ");
    print_decimal(isqCFW);
    puts("\n");

    uint8_t isqCFW_jig = CoreOS2_Bank_IsqCFW_jig(os_bank_indicator);

    puts("isqCFW_jig = ");
    print_decimal(isqCFW_jig);
    puts("\n");

    //

    if (isqCFW)
    {
        uint64_t power_up_cause[2];
        sc_query_system_power_up_cause(power_up_cause);

        puts("power_up_cause[0] = ");
        print_hex(power_up_cause[0]);
        puts("\n");

        puts("power_up_cause[1] = ");
        print_hex(power_up_cause[1]);
        puts("\n");

        if ((power_up_cause[0] & 0x00000000ffffffff) == 0x200)
        {
            sc_led_blinking_green();
            sc_puts("Wake source is BT!, waiting for power cycle by modchip...\n");
            dead_beep();
        }
    }
    else
        sc_triple_beep();

    sc_led_short_green_long_yellow();

    // inform modchip that we are good...
    sc_puts("BadWDSD ok!\n");

    //

#if STAGEX_DEBUG_ENABLED
    if (stagex_debug_flag == 0x1)
        WDSDTest();
#endif

    // do not remove!
    memset((void*)0, 0, (16 * 1024 * 1024));

    // clear lv2
    memset((void*)0x1000000, 0, 0x10000);
    memset((void*)0x2000000, 0, 0x10000);
    memset((void*)0x3000000, 0, 0x10000);
    memset((void*)0x4000000, 0, 0x10000);
    memset((void*)0x5000000, 0, 0x10000);
    memset((void*)0x6000000, 0, 0x10000);
    memset((void*)0x7000000, 0, 0x10000);
    memset((void*)0x8000000, 0, 0x10000);

    //

    uint64_t new_stagex_addr = 0x1010000;

    if (!is_emmc)
        Stagex_Relocate((const void*)0x2401FF21000, 0x2401FF21000, new_stagex_addr);

    //

#if HDDKEYDUMPER_ENABLED
    uint8_t hdd_key_dumper_flag = sc_read_hdd_key_dumper_flag();

    if ((hdd_key_dumper_flag != 0x1) && (hdd_key_dumper_flag != 0xfe))
        sc_write_hdd_key_dumper_flag(0xff);
#endif

    //

    {
        uint64_t lv0FileAddress;
        uint64_t lv0FileSize;

        {
            uint8_t found = 0;

            if (found == 0)
            {
                puts("Searching for lv0 self...\n");

                uint32_t lv0SelfFileFlashOffset;
                uint64_t lv0SelfFileAddress;
                uint32_t lv0SelfFileSize;

                if (CoreOS2_FindFileEntry_Bank(os_bank_indicator, "lv0", &lv0SelfFileFlashOffset, &lv0SelfFileSize))
                {
                    found = 1;

                    lv0SelfFileAddress = 0xB000000;

                    puts("lv0SelfFileFlashOffset = ");
                    print_hex(lv0SelfFileFlashOffset);

                    puts(", lv0SelfFileAddress = ");
                    print_hex(lv0SelfFileAddress);

                    puts(", lv0SelfFileSize = ");
                    print_decimal(lv0SelfFileSize);

                    puts("\n");

                    FlashRead(lv0SelfFileFlashOffset, (void*)lv0SelfFileAddress, lv0SelfFileSize);

                    lv0FileAddress = 0xC000000;
                    lv0FileSize = (4 * 1024 * 1024);

                    memset((void*)lv0FileAddress, 0, lv0FileSize);

#if STAGE0_DECRYPTLV0SELF_SPU_ENABLED
                    {
                        static const uint64_t stagexSpuElf_MaxSize = (64 * 1024);
                        __attribute__((aligned(8))) uint8_t stagexSpuElf[stagexSpuElf_MaxSize];

                        SpuAux_CopyElfToMem(stagexSpuElf, stagexSpuElf_MaxSize);

                        uint64_t spu_id = myspu_id;
                        uint64_t spu_old_mfc_sr1 = SpuAux_Init(spu_id, stagexSpuElf);
                        SPU_DecryptLv0Self(spu_id, (void*)lv0FileAddress, (const void*)lv0SelfFileAddress);
                        SpuAux_Uninit(spu_id, spu_old_mfc_sr1);
                    }
#else
                    DecryptLv0Self((void*)lv0FileAddress, (const void*)lv0SelfFileAddress, 1);
#endif
                }
                else
                    puts("File not found!\n");
            }

            if (found == 0)
                dead_beep();
        }

        puts("lv0FileAddress = ");
        print_hex(lv0FileAddress);
        puts("\n");

        puts("lv0FileSize = ");
        print_decimal(lv0FileSize);
        puts("\n");

        {
#if HDDKEYDUMPER_ENABLED
            if ((hdd_key_dumper_flag == 0x1) && ((fwVersion >= 470) || isqCFW)) // dump request
            {
                sc_write_hdd_key_dumper_flag(0x2); // will dump soon

                uint8_t searchData[] = {0xE9, 0x22, 0x8C, 0x88, 0x78, 0x63, 0x9B, 0x24, 0x38, 0x00, 0x00, 0x03, 0xE9, 0x29, 0x00, 0x00, 0x3D, 0x29, 0x00, 0x04, 0x39, 0x29, 0x40, 0x1C, 0x7C, 0x03, 0x49, 0x2E, 0x4E, 0x80, 0x00, 0x20};
                uint8_t stage2jData[] = {0x48, 0x00, 0x00, 0x05, 0x7C, 0xA8, 0x02, 0xA6, 0x38, 0xA5, 0xFF, 0xFC, 0xE8, 0xA5, 0x00, 0x18, 0x7C, 0xA9, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x20, 0x00, 0x00, 0x02, 0x40, 0x1F, 0x03, 0x11, 0x00};

                *((uint64_t*)(&stage2jData[sizeof(stage2jData) - 8])) = (new_stagex_addr + 0x100);

                puts("Installing stage2j... (HDDKeyDumper)\n");

                if (!SearchAndReplace((void*)lv0FileAddress, lv0FileSize, searchData, sizeof(searchData), stage2jData, sizeof(stage2jData)))
                {
                    puts("failed!\n");
                    dead_beep();
                }
            }
#endif

            // ANTI BRICK!!!
            // isqCFW!!!
            if ((fwVersion >= 470) || isqCFW || isqCFW_jig)
            {
                uint8_t searchData[] = {0x38, 0x60, 0x01, 0x00, 0x7C, 0x69, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x20, 0x60, 0x00, 0x00, 0x00};
                uint8_t stage2jData[] = {0x48, 0x00, 0x00, 0x05, 0x7C, 0xA8, 0x02, 0xA6, 0x38, 0xA5, 0xFF, 0xFC, 0xE8, 0xA5, 0x00, 0x18, 0x7C, 0xA9, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x20, 0x00, 0x00, 0x02, 0x40, 0x1F, 0x03, 0x11, 0x00};

                *((uint64_t*)(&stage2jData[sizeof(stage2jData) - 8])) = (new_stagex_addr + 0x100);

                puts("Installing stage2j...\n");

                if (!SearchAndReplace((void*)lv0FileAddress, lv0FileSize, searchData, sizeof(searchData), stage2jData, sizeof(stage2jData)))
                {
                    puts("Install failed!\n");

                    if (isqCFW)
                        dead_beep();
                }
            }
            else
                puts("fw too low!\n");

            // ANTI BRICK!!!
            if (isqCFW || isqCFW_jig)
            {
                // lv1.self -> lv1.qelf

                uint8_t searchData[] = {0x6C, 0x76, 0x31, 0x2E, 0x73, 0x65, 0x6C, 0x66};
                uint8_t replaceData[] = {0x6C, 0x76, 0x31, 0x2E, 0x71, 0x65, 0x6C, 0x66};

                puts("lv1.self -> lv1.qelf\n");

                if (!SearchAndReplace((void*)lv0FileAddress, lv0FileSize, searchData, sizeof(searchData), replaceData, sizeof(replaceData)))
                {
                    puts("failed!\n");
                    dead_beep();
                }
            }
        }

        puts("Loading lv0...\n");
        LoadElf(lv0FileAddress, 0x0, 1);
    }

    // write lv0 .vector
    volatile uint64_t *ea0 = (volatile uint64_t *)0x0;
    *ea0 = 0x50001010000;

    puts("Booting lv0...\n");

    eieio();

    asm volatile("li 3, 0x100");
    asm volatile("mtctr 3");
    asm volatile("bctr");
}

__attribute__((section("main1"))) void stage1_main()
{
    sc_puts_init();

    Stage1();

    dead_beep();
}

__attribute__((noreturn, section("entry1"))) void stage1_entry()
{
    // set stage_entry_ra
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= 4;

    // set is_lv1 to 0
    is_lv1 = 0;

    // set stage_zero to 0
    stage_zero = 0;

    // call HW_Init
    asm volatile("bl HW_Init");

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x900; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // restore XDR now
    {
        // *XDR_CH0_SCMD = (XDR_SCMD(XDR_SCMD_SDW, 0, XDR_CFG) | XDR_SLE_DISABLED_X32);
        // *((volatile uint32_t*)0x2000050A108) = 0x4000205

        asm volatile("lis 3, 0x400");
        asm volatile("ori 3, 3, 0x205");

        asm volatile("lis 4, 0x200");
        asm volatile("ori 4, 4, 0x50");
        asm volatile("sldi 4, 4, 16");
        asm volatile("ori 4, 4, 0xA108");

        asm volatile("stw 3, 0(4)");
        eieio();

        // wait for it to finish
        asm volatile("lwz 3, 8(4)");
        eieio();
    }

    // set stage_sp to 0xDFFFF00
    stage_sp = 0xDFFFF00;

    // set r1 to stage_sp
    asm volatile("mr 1, %0" ::"r"(stage_sp) :);

#if ENTRY_WAIT_IN_MS > 0
    // Can't use ram yet until pico releases it, so we wait using register only
    asm volatile("li 3, %0" ::"i"(ENTRY_WAIT_IN_MS) :);
    asm volatile("bl WaitInMs2");
#endif

    // sync
    asm volatile("sync");

    // push stack
    asm volatile("addi 1, 1, -128");

    // jump to stage_main
    asm volatile("b stage1_main");

    __builtin_unreachable();
}