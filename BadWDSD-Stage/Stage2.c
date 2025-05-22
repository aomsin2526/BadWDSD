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
        uint64_t lv1FileAddress;
        uint64_t lv1FileSize;

        uint8_t foundlv1file = 0;

        {
            if (foundlv1file == 0)
            {
                puts("Searching for lv1.elf...\n");

                if (CoreOS_FindFileEntry_CurrentBank("lv1.elf", &lv1FileAddress, &lv1FileSize))
                    foundlv1file = 1;
                else
                    puts("File not found!\n");
            }

            if (foundlv1file == 0)
            {
                puts("Searching for lv1.zelf...\n");

                uint64_t zelfFileAddress;
                uint64_t zelfFileSize;

                if (CoreOS_FindFileEntry_CurrentBank("lv1.zelf", &zelfFileAddress, &zelfFileSize))
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
            LoadElf(lv1FileAddress, 0x0, 1);
        }

        {
            puts("Searching for lv1.diff...\n");

            uint64_t lv1DiffFileAddress;
            uint64_t lv1DiffFileSize;

            if (CoreOS_FindFileEntry_CurrentBank("lv1.diff", &lv1DiffFileAddress, &lv1DiffFileSize))
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

                        *((uint8_t *)(uint64_t)addr) = value;
                    }
                }
            }
            else
                puts("File not found!\n");
        }

        {
            puts("Patching CoreOS hash check...\n");

            uint8_t searchData[] = {0x88, 0x18, 0x00, 0x36, 0x2F, 0x80, 0x00, 0xFF, 0x41, 0x9E, 0x00, 0x1C, 0x7F, 0x63, 0xDB, 0x78, 0xE8, 0xA2, 0x85, 0x78};
            uint8_t replaceData[] = {0x88, 0x18, 0x00, 0x36, 0x2F, 0x80, 0x00, 0xFF, 0x60, 0x00, 0x00, 0x00, 0x7F, 0x63, 0xDB, 0x78, 0xE8, 0xA2, 0x85, 0x78};

            if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, 20, replaceData, 20))
                puts("Patch failed!\n");
        }

        {
            puts("Patching disable_erase_hash_standby_bank_and_fsm (ANTI BRICK & EXIT FSM)...\n");

            uint8_t searchData[] = {0xF8, 0x21, 0xFE, 0xC1, 0x7C, 0x08, 0x02, 0xA6, 0xFB, 0x41, 0x01, 0x10, 0x3B, 0x41, 0x00, 0x70, 0xFB, 0xA1, 0x01, 0x28, 0x7C, 0x7D, 0x1B, 0x78, 0x7F, 0x43, 0xD3, 0x78};
            uint8_t replaceData[] = {0x7C, 0x08, 0x02, 0xA6, 0x38, 0x21, 0xFF, 0xC0, 0xF8, 0x01, 0x00, 0x00, 0x3D, 0x20, 0x80, 0x00, 0x61, 0x29, 0x41, 0x24, 0x79, 0x29, 0x00, 0x20, 0x38, 0x60, 0x00, 0xFF, 0x38, 0x80, 0x00, 0xFF, 0x7D, 0x29, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x21, 0xE8, 0x01, 0x00, 0x00, 0x38, 0x21, 0x00, 0x40, 0x38, 0x60, 0x00, 0x00, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20};

            if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, 28, replaceData, sizeof(replaceData)))
            {
                puts("Patch failed!\n");
                dead_beep();
            }
        }

        {
            puts("Patching get_version_and_hash (ANTI BRICK & Downgrading)...\n");

            uint8_t searchData[] = {0x4B, 0xFF, 0x45, 0xD1, 0xE8, 0x1F, 0x00, 0x80, 0xF8, 0x1C, 0x00, 0x00, 0x38, 0x9D, 0x00, 0x08};
            uint8_t replaceData[] = {0x38, 0x00, 0x30, 0x06, 0x78, 0x00, 0x26, 0xC6, 0xF8, 0x1C, 0x00, 0x00, 0x38, 0x9D, 0x00, 0x08};

            if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, 16, replaceData, 16))
            {
                puts("Patch failed!\n");
                dead_beep();
            }
        }

        {
            // Hvcall 114

            {
                uint8_t searchData[] = {0x2F, 0x80, 0x00, 0x00, 0x41, 0x9E, 0x00, 0x28, 0x38, 0x60, 0x00, 0x00, 0x38, 0x80, 0x00, 0x00};
                uint8_t replaceData[] = {0x60, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x28, 0x38, 0x60, 0x00, 0x00, 0x38, 0x80, 0x00, 0x00};

                puts("Patching hvcall 114 1...\n");

                if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, 16, replaceData, 16))
                    puts("patch failed!\n");
            }

            {
                uint8_t searchData[] = {0x00, 0x4B, 0xFF, 0xFB, 0xFD, 0x7C, 0x60, 0x1B};
                uint8_t replaceData[] = {0x01, 0x4B, 0xFF, 0xFB, 0xFD, 0x7C, 0x60, 0x1B};

                puts("Patching hvcall 114 2...\n");

                if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, 8, replaceData, 8))
                    puts("patch failed!\n");
            }
        }

        {
            puts("Patching FSM...\n");

            uint8_t searchData[] = {0x80, 0x01, 0x00, 0x74, 0x7F, 0xC3, 0xF3, 0x78, 0xE8, 0xA2, 0x84, 0xE8, 0x38, 0x80, 0x00, 0x01};
            uint8_t replaceData[] = {0x38, 0x00, 0x00, 0xFF, 0x7F, 0xC3, 0xF3, 0x78, 0xE8, 0xA2, 0x84, 0xE8, 0x38, 0x80, 0x00, 0x01};

            if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, 16, replaceData, 16))
                puts("Patch failed!\n");
        }

        {
            puts("Patching lv0/lv1 protection...\n");

            uint8_t searchData[] = {0x2F, 0x83, 0x00, 0x00, 0x38, 0x60, 0x00, 0x01, 0x41, 0x9E, 0x00, 0x20, 0xE8, 0x62, 0x8A, 0xB8, 0x48, 0x01, 0xE6, 0x49, 0x38, 0x60, 0x00, 0x04, 0x38, 0x80, 0x00, 0x00};
            uint8_t replaceData[] = {0x2F, 0x83, 0x00, 0x00, 0x38, 0x60, 0x00, 0x01, 0x48, 0x00};

            if (!SearchAndReplace((void *)0x0, (16 * 1024 * 1024), searchData, sizeof(searchData), replaceData, sizeof(replaceData)))
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

    // set interrupt_depth to 0
    interrupt_depth = 0;

    // set is_lv1 to 0
    is_lv1 = 0;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x700; // .toc
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

    // push stack
    asm volatile("addi 1, 1, -128");

    // jump to stage_main
    asm volatile("b stage2_main");

    __builtin_unreachable();
}