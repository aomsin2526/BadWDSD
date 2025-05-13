#define ENTRY_WAIT_IN_MS 1000
#define HW_INIT_ENABLED 1

FUNC_DEF void Stage1()
{
    puts("BadWDSD Stage1 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

    sc_triple_beep();

    // Memtest();
    // PatternTest_x16();
    // PatternTest_x32();

    // puts("hello superslim :)\n");

    {
        uint8_t os_bank_indicator = sc_read_os_bank_indicator();

        puts("os_bank_indicator = ");
        print_hex(os_bank_indicator);
        puts("\n");

        if (os_bank_indicator == 0xff)
            puts("Will use ros0\n");
        else
            puts("Will use ros1\n");

        uint64_t coreOSStartAddress = (os_bank_indicator == 0xff) ? 0x2401F0C0000 : 0x2401F7C0000;

        uint64_t lv0FileAddress;
        uint64_t lv0FileSize;

        {
            uint8_t found = 0;

            if (found == 0)
            {
                puts("Searching for lv0.elf...\n");

                if (CoreOS_FindFileEntry(coreOSStartAddress, "lv0.elf", &lv0FileAddress, &lv0FileSize))
                    found = 1;
                else
                    puts("File not found!\n");
            }

            if (found == 0)
            {
                puts("Searching for lv0.zelf...\n");

                uint64_t zelfFileAddress;
                uint64_t zelfFileSize;

                if (CoreOS_FindFileEntry(coreOSStartAddress, "lv0.zelf", &zelfFileAddress, &zelfFileSize))
                {
                    found = 1;

                    puts("zelfFileAddress = ");
                    print_hex(zelfFileAddress);

                    puts(", zelfFileSize = ");
                    print_decimal(zelfFileSize);

                    puts("\n");

                    lv0FileAddress = 0xC000000;
                    lv0FileSize = (4 * 1024 * 1024);

                    ZelfDecompress(zelfFileAddress, (void*)lv0FileAddress, &lv0FileSize);
                }
                else
                    puts("File not found!\n");
            }

            if (found == 0)
            {
                puts("Searching for lv0 self...\n");

                uint64_t lv0SelfFileAddress;
                uint64_t lv0SelfFileSize;

                if (CoreOS_FindFileEntry(coreOSStartAddress, "lv0", &lv0SelfFileAddress, &lv0SelfFileSize))
                {
                    found = 1;

                    puts("lv0SelfFileAddress = ");
                    print_hex(lv0SelfFileAddress);

                    puts(", lv0SelfFileSize = ");
                    print_decimal(lv0SelfFileSize);

                    puts("\n");

                    lv0FileAddress = 0xC000000;
                    lv0FileSize = (4 * 1024 * 1024);

                    memset((void*)lv0FileAddress, 0, lv0FileSize);

                    WaitInMs(1500);
                    sc_triple_beep();

                    DecryptLv0Self((void*)lv0FileAddress, (const void*)lv0SelfFileAddress);

                    {
                        uint8_t searchData[] = {0x38, 0x60, 0x01, 0x00, 0x7C, 0x69, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x20, 0x60, 0x00, 0x00, 0x00};
                        
                        uint8_t stage2jData[] = {0x48, 0x00, 0x00, 0x05, 0x7C, 0x68, 0x02, 0xA6, 0x38, 0x63, 0xFF, 0xFC, 0xE8, 0x83, 0x00, 0x18,
                             0x7C, 0x89, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x20, 0x00, 0x00, 0x02, 0x40, 0x1F, 0x03, 0x11, 0x00};

                        puts("Installing stage2j...\n");
                
                        if (!SearchAndReplace((void*)lv0FileAddress, lv0FileSize, searchData, 16, stage2jData, 32))
                            puts("Install failed!\n");
                    }
                }
                else
                    puts("File not found!\n");
            }

            if (found == 0)
            {
                dead_beep();
            }
        }

        puts("lv0FileAddress = ");
        print_hex(lv0FileAddress);
        puts("\n");

        puts("lv0FileSize = ");
        print_decimal(lv0FileSize);
        puts("\n");

        puts("Loading lv0...\n");
        LoadElf(lv0FileAddress, 0x0, 1);

        // write lv0 .vector
        volatile uint64_t *ea0 = (volatile uint64_t *)0x0;
        *ea0 = 0x50001010000;

        puts("Booting lv0...\n");

        eieio();

        asm volatile("li 3, 0x100");
        asm volatile("mtctr 3");
        asm volatile("bctr");
    }
}

__attribute__((section("main1"))) void stage1_main()
{
    // zeroing ram
    //memset((void*)0x0, 0, (256 * 1024 * 1024));
    //eieio();

    sc_puts_init();

    Stage1();

    dead();
}

__attribute__((noreturn, section("entry1"))) void stage1_entry()
{
    // set stage_entry_ra
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= 4;

    // disable interrupts
    asm volatile("li 0, 2");
    asm volatile("mtmsrd 0, 1");
    eieio();

    // set is_lv1 to 0
    is_lv1 = 0;

    // set stage_zero to 0
    stage_zero = 0;

#if HW_INIT_ENABLED
    // call HW_Init
    asm volatile("bl HW_Init");
#endif

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x800; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // restore XDR now
    {
        //register uint64_t r3 asm("r3");
        //r3 = (XDR_SCMD(SCMD_SDW, 0, XDR_CFG) | 0x5); // SLE Disabled, x32

        //register uint64_t r4 asm("r4");
        //r4 = (MMIO_BE_MIC | YREG_YDRAM_DTA_0);

        //asm volatile("stw %0, 0(%1)" ::"r"(r3),"r"(r4):);
        //eieio();

#if 0
        //entry1:000002401F031028                 lis       r3_0, 0x400
        //entry1:000002401F03102C                 ori       r3_0, r3_0, 0x205 # 0x4000205
        //entry1:000002401F031030                 lis       r4_0, 0x200
        //entry1:000002401F031034                 ori       r4_0, r4_0, 0x50 # 'P' # 0x2000050
        //entry1:000002401F031038                 sldi      r4_0, r4_0, 16
        //entry1:000002401F03103C                 ori       r4_0, r4_0, 0xA108
        //entry1:000002401F031040                 stw       r3_0, 0(r4_0)
#endif

        asm volatile("lis 3, 0x400");
        asm volatile("ori 3, 3, 0x205");

        asm volatile("lis 4, 0x200");
        asm volatile("ori 4, 4, 0x50");
        asm volatile("sldi 4, 4, 16");
        asm volatile("ori 4, 4, 0xA108");

        asm volatile("stw 3, 0(4)");
        eieio();
    }

    // set stage_sp to 0xE000000
    stage_sp = 0xE000000;

    // set r1 to stage_sp
    asm volatile("mr 1, %0" ::"r"(stage_sp) :);

#if ENTRY_WAIT_IN_MS > 0
    // Can't use ram yet until pico releases it, so we wait using register only
    asm volatile("li 3, %0" ::"i"(ENTRY_WAIT_IN_MS) :);
    asm volatile("bl WaitInMs2");
#endif

    // sync
    asm volatile("sync");

    // jump to stage_main
    asm volatile("b stage1_main");

    __builtin_unreachable();
}