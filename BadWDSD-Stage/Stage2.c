#undef ENTRY_WAIT_IN_MS

FUNC_DEF void Stage2()
{
    puts("BadWDSD Stage2 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

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

        uint64_t lv1FileAddress;
        uint64_t lv1FileSize;

        uint8_t foundlv1file = 0;

        {
            if (foundlv1file == 0)
            {
                puts("Searching for lv1.elf...\n");

                if (CoreOS_FindFileEntry(coreOSStartAddress, "lv1.elf", &lv1FileAddress, &lv1FileSize))
                    foundlv1file = 1;
                else
                    puts("File not found!\n");
            }

            if (foundlv1file == 0)
            {
                puts("Searching for lv1.zelf...\n");

                uint64_t zelfFileAddress;
                uint64_t zelfFileSize;

                if (CoreOS_FindFileEntry(coreOSStartAddress, "lv1.zelf", &zelfFileAddress, &zelfFileSize))
                {
                    foundlv1file = 1;

                    puts("zelfFileAddress = ");
                    print_hex(zelfFileAddress);

                    puts(", zelfFileSize = ");
                    print_decimal(zelfFileSize);

                    puts("\n");

                    lv1FileAddress = 0xC000000;
                    lv1FileSize = (8 * 1024 * 1024);

                    ZelfDecompress(zelfFileAddress, (void *)lv1FileAddress, &lv1FileSize);
                }
                else
                    puts("File not found!\n");
            }
        }

        if (foundlv1file != 0)
        {
            puts("lv1FileAddress = ");
            print_hex(lv1FileAddress);
            puts("\n");

            puts("lv1FileSize = ");
            print_decimal(lv1FileSize);
            puts("\n");

            puts("Loading lv1...\n");
            LoadElf(lv1FileAddress, 0x0);
        }

        {
            puts("Searching for lv1.diff...\n");

            uint64_t lv1DiffFileAddress;
            uint64_t lv1DiffFileSize;

            if (CoreOS_FindFileEntry(coreOSStartAddress, "lv1.diff", &lv1DiffFileAddress, &lv1DiffFileSize))
            {
                puts("lv1DiffFileAddress = ");
                print_hex(lv1DiffFileAddress);

                puts(", lv1DiffFileSize = ");
                print_decimal(lv1DiffFileSize);

                puts("\n");

                {
                    uint64_t curAddress = lv1DiffFileAddress;

                    uint32_t diffCount = *((uint32_t *)curAddress);
                    curAddress += 4;

                    puts("diffCount = ");
                    print_decimal(diffCount);
                    puts("\n");

                    for (uint32_t i = 0; i < diffCount; ++i)
                    {
                        uint32_t addr = *((uint32_t *)curAddress);
                        curAddress += 4;

                        uint8_t value = (uint8_t)(*((uint32_t *)curAddress));
                        curAddress += 4;

#if 0
                        puts("addr = ");
                        print_hex(addr);

                        puts(", value = ");
                        print_hex(value);

                        puts("\n");
#endif

                        *((uint8_t*)(uint64_t)addr) = value;
                    }
                }
            }
            else
                puts("File not found!\n");
        }

        {
            puts("Patching CoreOS hash check...\n");

            uint8_t searchData[] = { 0x88, 0x18, 0x00, 0x36, 0x2F, 0x80, 0x00, 0xFF, 0x41, 0x9E, 0x00, 0x1C, 0x7F, 0x63, 0xDB, 0x78, 0xE8, 0xA2, 0x85, 0x78 };
            uint8_t replaceData[] = { 0x88, 0x18, 0x00, 0x36, 0x2F, 0x80, 0x00, 0xFF, 0x60, 0x00, 0x00, 0x00, 0x7F, 0x63, 0xDB, 0x78, 0xE8, 0xA2, 0x85, 0x78 };

            if (!SearchAndReplace((void*)0x0, (16 * 1024 * 1024), searchData, 20, replaceData, 20))
                puts("Patch failed!\n");
        }

        puts("Booting lv1...\n");

        eieio();

        asm volatile("li 3, 0x100");
        asm volatile("mtctr 3");
        asm volatile("bctr");
    }
}

__attribute__((section("main2"))) void stage2_main()
{
    sc_puts_init();

    Stage2();

    dead();
}

__attribute__((noreturn, section("entry2"))) void stage2_entry()
{
    // set stage_entry_ra
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= 4;

    // set is_lv1 to 0
    is_lv1 = 0;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x300; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // set stage_sp to 0xE000000
    stage_sp = 0xE000000;

    // set r1 to stage_sp
    asm volatile("mr 1, %0" ::"r"(stage_sp) :);

#if ENTRY_WAIT_IN_MS > 0
    // optional wait
    asm volatile("li 3, %0" ::"i"(ENTRY_WAIT_IN_MS) :);
    asm volatile("bl WaitInMs2");
#endif

    // sync
    asm volatile("sync");

    // jump to stage_main
    asm volatile("b stage2_main");

    __builtin_unreachable();
}