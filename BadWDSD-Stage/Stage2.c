#undef ENTRY_WAIT_IN_MS

FUNC_DEF void ApplyLv1Diff(uint64_t lv1DiffFileAddress, uint8_t verifyOrig)
{
    puts("ApplyLv1Diff()\n");

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

        uint32_t value = *((uint32_t *)curAddress);
        curAddress += 4;

        uint8_t origVal = (uint8_t)(value >> 8);
        uint8_t newVal = (uint8_t)(value & 0xFF);

#if 0
        puts("addr = ");
        print_hex(addr);

        puts(", origVal = ");
        print_hex(origVal);

        puts(", newVal = ");
        print_hex(newVal);

        puts("\n");
#endif

        if (verifyOrig)
        {
            uint8_t curVal = *((uint8_t *)(uint64_t)addr);

            if (curVal != origVal)
            {
                puts("verifyOrig failed at addr = ");
                print_hex(addr);

                puts(", curVal = ");
                print_hex(curVal);

                puts(", origVal = ");
                print_hex(origVal);
                puts("\n");

                dead_beep();
            }
        }

        *((uint8_t *)(uint64_t)addr) = newVal;
    }

    eieio();
    puts("ApplyLv1Diff() done.\n");
}

FUNC_DEF void Stage2()
{
    puts("BadWDSD Stage2 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

    uint8_t os_bank_indicator = get_os_bank_indicator();

    uint8_t isqCFW = CoreOS_Bank_IsqCFW(os_bank_indicator);
    uint8_t qcfw_lite_flag = get_qcfw_lite_flag();

    uint16_t fwVersion = CoreOS_Bank_GetFWVersion(os_bank_indicator);

    {
        uint64_t lv1FileAddress;
        uint64_t lv1FileSize;

        uint8_t foundlv1file = 0;

        {
            if (foundlv1file == 0)
            {
                puts("Searching for lv1.elf...\n");

                if (CoreOS_FindFileEntry_Bank(os_bank_indicator, "lv1.elf", &lv1FileAddress, &lv1FileSize))
                    foundlv1file = 1;
                else
                    puts("File not found!\n");
            }

            if (foundlv1file == 0)
            {
                puts("Searching for lv1.zelf...\n");

                uint64_t zelfFileAddress;
                uint64_t zelfFileSize;

                if (CoreOS_FindFileEntry_Bank(os_bank_indicator, "lv1.zelf", &zelfFileAddress, &zelfFileSize))
                {
                    foundlv1file = 1;

                    puts("zelfFileAddress = ");
                    print_hex(zelfFileAddress);

                    puts(", zelfFileSize = ");
                    print_decimal(zelfFileSize);

                    puts("\n");

                    lv1FileAddress = 0xC000000;
                    lv1FileSize = (8 * 1024 * 1024);

                    ZelfDecompress(zelfFileAddress, (void *)lv1FileAddress, &lv1FileSize, 1);
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
            uint64_t lv1DiffFileAddress = 0;
            uint64_t lv1DiffFileSize = 0;

            if (!isqCFW && (qcfw_lite_flag == 0x1))
            {
                if (fwVersion == 492)
                {
                    puts("Searching for qcfwlite492cex_lv1.diff...\n");
                    CoreOS_FindFileEntry_Aux("qcfwlite492cex_lv1.diff", &lv1DiffFileAddress, &lv1DiffFileSize);
                }
                else
                {
                    puts("Current firmware doesn't support qCFW lite!\n");
                    dead_beep();
                }

                if (lv1DiffFileAddress == 0)
                {
                    puts("File not found!\n");
                    dead_beep();
                }
            }
            else
            {
                puts("Searching for lv1.diff...\n");
                CoreOS_FindFileEntry_Bank(os_bank_indicator, "lv1.diff", &lv1DiffFileAddress, &lv1DiffFileSize);
            }

            if (lv1DiffFileAddress != 0)
            {
                puts("lv1DiffFileAddress = ");
                print_hex(lv1DiffFileAddress);

                puts(", lv1DiffFileSize = ");
                print_decimal(lv1DiffFileSize);

                puts("\n");

                ApplyLv1Diff(lv1DiffFileAddress, 1);
            }
            else
                puts("File not found!\n");
        }

        //static const uint64_t patchSearchSize = (8 * 1024 * 1024);

        {
            struct Stagex_spu_job_stage2_context_s job_context;
            job_context.patch_aim = 0;
            job_context.patch_inspect_package_tophalf = 0;

            if (isqCFW)
            {
                if (CoreOS_FindFileEntry_CurrentBank("lv2Rkernel.self", NULL, NULL))
                {
                    job_context.patch_aim = 1;
                    job_context.patch_inspect_package_tophalf = 1;
                }
            }

            uint64_t spu_id = calc_myspu_id();
            uint64_t spu_old_mfc_sr1 = SpuAux_Init(spu_id);
            spu_stage2(spu_id, &job_context);
            SpuAux_Uninit(spu_id, spu_old_mfc_sr1);
        }

#if 0

        // not working on 28nm
        {
            static const uint32_t vramClock = 800;

            puts("Patching RSX vram clock to ");
            print_decimal(vramClock);
            puts("Mhz\n");

            uint8_t searchData[] = {0x0a, 0x02, 0x00, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x04};
            uint8_t replaceData[] = {0x0a, 0x02, 0x00, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (vramClock / 25), 0x04};

            if (!SearchAndReplace((void *)0x0, patchSearchSize, searchData, sizeof(searchData), replaceData, sizeof(replaceData)))
                puts("Patch failed!\n");
        }

#endif

        {
            struct Stagex_Context_s* ctx = GetStagexContext();

            //

            ctx->cached_os_bank_indicator = os_bank_indicator;

            puts("cached_os_bank_indicator = ");
            print_hex(ctx->cached_os_bank_indicator);
            puts("\n");

            //

            ctx->cached_qcfw_lite_flag = qcfw_lite_flag;

            puts("cached_qcfw_lite_flag = ");
            print_hex(ctx->cached_qcfw_lite_flag);
            puts("\n");

            //

            ctx->stage3_alreadyDone = 0;

            ctx->stage6_isAppldr = 0;
            ctx->stage6_isLv2ldr = 0;

            ctx->stage6_spu_id = 0xff; // unknown

            //

            ctx->cached_myappldrElfAddress = 0;

            CoreOS_FindFileEntry_Bank(os_bank_indicator, "myappldr.elf", &ctx->cached_myappldrElfAddress, NULL);

            puts("cached_myappldrElfAddress = ");
            print_hex(ctx->cached_myappldrElfAddress);
            puts("\n");

            //

            ctx->cached_mylv2ldrElfAddress = 0;

            CoreOS_FindFileEntry_Bank(os_bank_indicator, "mylv2ldr.elf", &ctx->cached_mylv2ldrElfAddress, NULL);
 
            puts("cached_mylv2ldrElfAddress = ");
            print_hex(ctx->cached_mylv2ldrElfAddress);
            puts("\n");

            //

            ctx->cached_mymetldrElfAddress = 0;

            CoreOS_FindFileEntry_Aux("mymetldr.elf", &ctx->cached_mymetldrElfAddress, NULL);

            puts("cached_mymetldrElfAddress = ");
            print_hex(ctx->cached_mymetldrElfAddress);
            puts("\n");

            //
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
    stage_rtoc += 0x800; // .toc
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