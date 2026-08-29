#if 0

FUNC_DEF uint64_t FindHvcallTable()
{
    // if ((v[0] == 0x386000006463ffff) && (v[1] == 0x6063ffec4e800020))

    uint64_t invalid_handler_addr = 0;

    for (uint64_t addr = 0; addr < (16 * 1024 * 1024); addr += 4)
    {
        const uint32_t *v = (const uint32_t *)addr;

        if (v[0] == 0x38600000 && v[1] == 0x6463ffff && v[2] == 0x6063ffec && v[3] == 0x4e800020)
        {
            invalid_handler_addr = addr;
            break;
        }
    }

    if (invalid_handler_addr == 0)
        return 0;

    for (uint64_t addr = 0; addr < (16 * 1024 * 1024); addr += 8)
    {
        const uint64_t *v = (const uint64_t *)addr;

        if ((v[0] == invalid_handler_addr) &&
            (v[1] == invalid_handler_addr) &&
            (v[2] != invalid_handler_addr) &&
            (v[3] == invalid_handler_addr))
        {
            return (addr - (22 * 8));
        }
    }

    return 0;
}

#endif

FUNC_DEF void Stage3()
{
    lv1_puts("BadWDSD Stage3 by Kafuu(aomsin2526)\n");

    lv1_puts("(Build Date: ");
    lv1_puts(__DATE__);
    lv1_puts(" ");
    lv1_puts(__TIME__);
    lv1_puts(")\n");

    print_pc();

    {
        {
            lv1_puts("Patching lv1 182/183\n");

            uint64_t patches[4];

            patches[0] = 0xE8830018E8840000ULL;
            patches[1] = 0xF88300C84E800020ULL;
            patches[2] = 0x38000000E8A30020ULL;
            patches[3] = 0xE8830018F8A40000ULL;

            memcpy((void*)0x309E4C, patches, 32);
        }

#if 1

        // HTAB

        {
            lv1_puts("Patching HTAB write protection\n");

            uint64_t old;
            memcpy(&old, (const void*)0x0AC594, 8);
            old &= 0x00000000FFFFFFFFULL;

            uint64_t newval = 0x3860000000000000ULL | old;
            memcpy((void*)0x0AC594, &newval, 8);
        }

        // Repo nodes

        {
            lv1_puts("Patching Repo nodes modify\n");

            // poke_lv1(0x2E4E28 +  0, 0xE81E0020E95E0028ULL);
            // poke_lv1(0x2E4E28 +  8, 0xE91E0030E8FE0038ULL);
            // poke_lv1(0x2E4E28 + 12, 0xE8FE0038EBFE0018ULL);

            {
                uint32_t patches[5];

                patches[0] = 0xE81E0020;
                patches[1] = 0xE95E0028;

                patches[2] = 0xE91E0030;
                patches[3] = 0xE8FE0038;

                patches[4] = 0xEBFE0018;

                memcpy((void*)0x2E4E28, patches, 20);
            }

            // poke_lv1(0x2E50AC +  0, 0xE81E0020E93E0028ULL);
            // poke_lv1(0x2E50AC +  8, 0xE95E0030E91E0038ULL);
            // poke_lv1(0x2E50AC + 16, 0xE8FE0040E8DE0048ULL);
            // poke_lv1(0x2E50AC + 20, 0xE8DE0048EBFE0018ULL);

            {
                uint32_t patches[7];

                patches[0] = 0xE81E0020;
                patches[1] = 0xE93E0028;

                patches[2] = 0xE95E0030;
                patches[3] = 0xE91E0038;

                patches[4] = 0xE8FE0040;
                patches[5] = 0xE8DE0048;

                patches[6] = 0xEBFE0018;

                memcpy((void*)0x2E50AC, patches, 28);
            }

            // poke_lv1(0x2E5550 +  0, 0xE81E0020E93E0028ULL);
            // poke_lv1(0x2E5550 +  8, 0xE95E0030E91E0038ULL);
            // poke_lv1(0x2E5550 + 16, 0xE8FE0040E8DE0048ULL);
            // poke_lv1(0x2E5550 + 20, 0xE8DE0048EBFE0018ULL);

            {
                uint32_t patches[7];

                patches[0] = 0xE81E0020;
                patches[1] = 0xE93E0028;

                patches[2] = 0xE95E0030;
                patches[3] = 0xE91E0038;

                patches[4] = 0xE8FE0040;
                patches[5] = 0xE8DE0048;

                patches[6] = 0xEBFE0018;

                memcpy((void*)0x2E5550, patches, 28);
            }
        }

#endif

    }

    {
        uint64_t spu_id = calc_myspu_id();
        uint64_t spu_old_mfc_sr1 = SpuAux_Init_lv1(spu_id);
        spu_stage3(spu_id);
        SpuAux_Uninit(spu_id, spu_old_mfc_sr1);
    }

    // qcfw marker
    *((uint64_t*)0x240) = 0x11223344aabbccdd;

    lv1_puts("Stage3 done.\n");
}

FUNC_DEF uint8_t IsLv2(uint64_t addr)
{
    uint8_t searchData[] = {0x7C, 0x71, 0x43, 0xA6, 0x7C, 0x92, 0x43, 0xA6, 0x7C, 0xB3, 0x43, 0xA6, 0x7C, 0x7A, 0x02, 0xA6, 0x7C, 0x9B, 0x02,
                            0xA6, 0x7C, 0xA0, 0x00, 0xA6, 0x60, 0xA5, 0x00, 0x30, 0x7C, 0xBB, 0x03, 0xA6, 0x3C, 0xA0, 0x80, 0x00, 0x60, 0xA5,
                            0x00, 0x00, 0x78, 0xA5, 0x07, 0xC6, 0x64, 0xA5, 0x00, 0x00, 0x60, 0xA5, 0x08, 0x3C, 0x7C, 0xBA, 0x03, 0xA6, 0x4C, 0x00, 0x00, 0x24};

    if (!memcmp((const void *)(addr + 0x800), searchData, sizeof(searchData)))
        return 1;

    return 0;
}

FUNC_DEF uint8_t FindLv2(uint64_t *outFoundAddr)
{
    for (uint64_t addr = 0x1000000; addr < (250 * 1024 * 1024); addr += 0x1000000)
    {
        if (IsLv2(addr))
        {
            if (outFoundAddr != NULL)
                *outFoundAddr = addr;

            return 1;
        }
    }

    return 0;
}

FUNC_DEF void ApplyLv2Diff2(uint64_t lv2DiffFileAddress, uint64_t lv2AreaAddr, uint8_t useNewVal, uint8_t verifyOrig)
{
    lv1_puts("ApplyLv2Diff2()\n");

    lv1_puts("lv2AreaAddr = ");
    lv1_print_hex(lv2AreaAddr);
    lv1_puts("\n");

    {
        uint64_t curAddress = lv2DiffFileAddress;

        uint32_t diffCount = *((const uint32_t *)curAddress);
        curAddress += 4;

        lv1_puts("diffCount = ");
        lv1_print_decimal(diffCount);
        lv1_puts("\n");

        for (uint32_t i = 0; i < diffCount; ++i)
        {
            uint32_t addr = *((const uint32_t *)curAddress);
            curAddress += 4;

            uint32_t value = *((const uint32_t *)curAddress);
            curAddress += 4;

            uint8_t newVal = (uint8_t)(value >> 8);
            uint8_t origVal = (uint8_t)(value & 0xFF);

#if 0
            lv1_puts("addr = ");
            lv1_print_hex(addr);

            lv1_puts(", newVal = ");
            lv1_print_hex(newVal);

            lv1_puts(", origVal = ");
            lv1_print_hex(origVal);

            lv1_puts("\n");
#endif

            if (verifyOrig)
            {
                uint8_t curVal = *((const uint8_t *)((uint64_t)(addr + lv2AreaAddr)));

                if (curVal != origVal)
                {
                    lv1_puts("verifyOrig failed at addr = ");
                    lv1_print_hex(addr);

                    lv1_puts(", curVal = ");
                    lv1_print_hex(curVal);

                    lv1_puts(", origVal = ");
                    lv1_print_hex(origVal);
                    lv1_puts("\n");

                    dead_beep();
                }
            }

            *((uint8_t *)((uint64_t)(addr + lv2AreaAddr))) = useNewVal ? newVal : origVal;
        }
    }

    lv1_puts("ApplyLv2Diff2() done.\n");
}

FUNC_DEF void ApplyLv2Diff(uint64_t lv2AreaAddr, uint8_t useNewVal, uint8_t verifyOrig)
{
    lv1_puts("ApplyLv2Diff()\n");

    uint8_t isqCFW = CoreOS2_CurrentBank_IsqCFW();
    uint8_t qcfw_lite_flag = get_qcfw_lite_flag();

    uint16_t fwVersion = CoreOS2_CurrentBank_GetFWVersion();

    {
        uint32_t lv2DiffFileFlashOffset = 0;
        uint64_t lv2DiffFileAddress = 0xC000000;
        uint32_t lv2DiffFileSize = 0;

        if (!isqCFW && (qcfw_lite_flag == 0x1))
        {
            if (fwVersion == 492)
            {
                lv1_puts("Searching for qcfwlite492cex_lv2_kernel.zdiff...\n");
                CoreOS2_FindFileEntry_Aux("qcfwlite492cex_lv2_kernel.zdiff", &lv2DiffFileFlashOffset, &lv2DiffFileSize);
            }
            else if (fwVersion == 493)
            {
                lv1_puts("Searching for qcfwlite493cex_lv2_kernel.zdiff...\n");
                CoreOS2_FindFileEntry_Aux("qcfwlite493cex_lv2_kernel.zdiff", &lv2DiffFileFlashOffset, &lv2DiffFileSize);
            }
            else
            {
                lv1_puts("Current firmware doesn't support qCFW lite!\n");
                dead_beep();
            }

            if (lv2DiffFileFlashOffset == 0)
            {
                lv1_puts("File not found!\n");
                dead_beep();
            }
        }
        else
        {
            lv1_puts("Searching for lv2_kernel.diff...\n");
            CoreOS2_FindFileEntry_CurrentBank("lv2_kernel.diff", &lv2DiffFileFlashOffset, &lv2DiffFileSize);

            if (lv2DiffFileFlashOffset == 0)
            {
                lv1_puts("Searching for lv2_kernel.zdiff...\n");
                CoreOS2_FindFileEntry_CurrentBank("lv2_kernel.zdiff", &lv2DiffFileFlashOffset, &lv2DiffFileSize);
            }
        }

        if (lv2DiffFileFlashOffset != 0)
        {
            lv1_puts("lv2DiffFileFlashOffset = ");
            lv1_print_hex(lv2DiffFileFlashOffset);

            lv1_puts(", lv2DiffFileAddress = ");
            lv1_print_hex(lv2DiffFileAddress);

            lv1_puts(", lv2DiffFileSize = ");
            lv1_print_decimal(lv2DiffFileSize);

            lv1_puts("\n");

            FlashRead(lv2DiffFileFlashOffset, (void*)lv2DiffFileAddress, lv2DiffFileSize);

            uint64_t zelf_magic = *((const uint64_t *)lv2DiffFileAddress);

            if ((zelf_magic == 0x5A454C465A454C46) || (zelf_magic == 0x5A454C465A454C32))
            {
                lv1_puts("ZELF/ZELF2 detected\n");

                uint64_t sz = (16 * 1024 * 1024);
                ZelfDecompress(lv2DiffFileAddress, (void *)0xB000000, &sz, 1, NULL);

                lv2DiffFileAddress = 0xB000000;
                lv2DiffFileSize = sz;

                lv1_puts("lv2DiffFileAddress = ");
                lv1_print_hex(lv2DiffFileAddress);

                lv1_puts(", lv2DiffFileSize = ");
                lv1_print_decimal(lv2DiffFileSize);

                lv1_puts("\n");
            }

            ApplyLv2Diff2(lv2DiffFileAddress, lv2AreaAddr, useNewVal, verifyOrig);
        }
        else
            lv1_puts("File not found!\n");
    }

    lv1_puts("ApplyLv2Diff() done.\n");
}

FUNC_DEF void EnableLoadCobraFromUSB(uint64_t lv2AreaAddr)
{
    lv1_puts("Enabling Load Cobra from USB...\n");

    uint8_t *enableLoadCobraFromUSB = (uint8_t *)(lv2AreaAddr + 0x30);
    *enableLoadCobraFromUSB = 1;

    sc_triple_beep();
}

FUNC_DEF void Stage3_AuthLv2(uint64_t laid)
{
    lv1_puts("Stage3_AuthLv2(), laid = ");
    lv1_print_hex(laid);
    lv1_puts("\n");

    // ps2 = 0x1020000003000001
    // ps3 = 0x1070000002000001

    if (laid == 0x1070000002000001)
    {
        uint64_t lv2AreaAddrRa = 0x0;

        if (!FindLv2(&lv2AreaAddrRa))
        {
            lv1_puts("lv2 area not found!\n");
            return;
        }

        lv1_puts("lv2AreaAddrRa = ");
        lv1_print_hex(lv2AreaAddrRa);
        lv1_puts("\n");

        ApplyLv2Diff(lv2AreaAddrRa, 1, 0);

        {
            uint8_t qcfw_lite_flag = get_qcfw_lite_flag();

            if (qcfw_lite_flag == 0x1)
                EnableLoadCobraFromUSB(lv2AreaAddrRa);
        }
    }

    lv1_puts("Stage3_AuthLv2() done.\n");
}

FUNC_DECL void Stage4();

__attribute__((section("main3"))) void stage3_main(
    uint64_t in_r3, uint64_t in_r4, uint64_t in_r5, uint64_t in_r6, uint64_t in_r7, uint64_t in_r8
)
{
    //lv1_puts("stage3_main()\n");

    is_emmc = FetchIsEmmc();

    // r5 = options
    // peek: r6 = addr, r4 = outValue
    // poke: r6 = addr, r7 = value

    // poke8
    if (in_r5 == 0x8)
    {
        uint8_t* p = (uint8_t*)in_r6;
        *p = (uint8_t)in_r7;
        return;
    }

    // poke64
    if (in_r5 == 0x2)
    {
        uint64_t* p = (uint64_t*)in_r6;
        *p = in_r7;
        return;
    }

    // auth lv2
    if (in_r5 == 0x30)
    {
        sc_puts_init();

        // r6 = laid
        Stage3_AuthLv2(in_r6);
        return;
    }

    // Call Stage4
    if (in_r5 == 0x21)
    {
        Stage4();
        return;
    }

    // only 0x31 (after spp load_profile)
    if (in_r5 != 0x31)
        return;

    struct Stagex_Context_s *ctx = GetStagexContext();

    if (ctx->stage3_alreadyDone == 0x69)
        return;

    sc_puts_init();
    Stage3();

    ctx->stage3_alreadyDone = 0x69;
}

// in:
// r4 = magic (0x6996)
// r5 = arg1
// r6 = arg2
// r7 = arg3
// r8 = arg4

// out:
// r3 = 0
// r4-r6 = return value

__attribute__((noreturn, section("entry3"))) void stage3_entry()
{
    register uint64_t r0 asm("r0");

    register uint64_t r3 asm("r3");
    register uint64_t r4 asm("r4");

    // start only if r4 = 0x6996
    ASM("li %0, 0x6996" : "=r"(r0)::);
    ASM("cmp 0, 0, %0, %1" ::"r"(r4), "r"(r0) :);
    ASM("beq stage3_start");
    ASM("li %0, -0x14" : "=r"(r3)::); // r3 = -0x14

    ASM("blr");

    // stage3_start:
    ASM("stage3_start:");

    // push stack
    ASM("addi 1, 1, -512");

    // store all registers to stack
    ASM("std 0, %0(1)" ::"i"(8 * 0) :);
    ASM("std 1, %0(1)" ::"i"(8 * 1) :);
    ASM("std 2, %0(1)" ::"i"(8 * 2) :);
    ASM("std 3, %0(1)" ::"i"(8 * 3) :);
    ASM("std 4, %0(1)" ::"i"(8 * 4) :);
    ASM("std 5, %0(1)" ::"i"(8 * 5) :);
    ASM("std 6, %0(1)" ::"i"(8 * 6) :);
    ASM("std 7, %0(1)" ::"i"(8 * 7) :);
    ASM("std 8, %0(1)" ::"i"(8 * 8) :);
    ASM("std 9, %0(1)" ::"i"(8 * 9) :);
    ASM("std 10, %0(1)" ::"i"(8 * 10) :);
    ASM("std 11, %0(1)" ::"i"(8 * 11) :);
    ASM("std 12, %0(1)" ::"i"(8 * 12) :);
    ASM("std 13, %0(1)" ::"i"(8 * 13) :);
    ASM("std 14, %0(1)" ::"i"(8 * 14) :);
    ASM("std 15, %0(1)" ::"i"(8 * 15) :);
    ASM("std 16, %0(1)" ::"i"(8 * 16) :);
    ASM("std 17, %0(1)" ::"i"(8 * 17) :);
    ASM("std 18, %0(1)" ::"i"(8 * 18) :);
    ASM("std 19, %0(1)" ::"i"(8 * 19) :);
    ASM("std 20, %0(1)" ::"i"(8 * 20) :);
    ASM("std 21, %0(1)" ::"i"(8 * 21) :);
    ASM("std 22, %0(1)" ::"i"(8 * 22) :);
    ASM("std 23, %0(1)" ::"i"(8 * 23) :);
    ASM("std 24, %0(1)" ::"i"(8 * 24) :);
    ASM("std 25, %0(1)" ::"i"(8 * 25) :);
    ASM("std 26, %0(1)" ::"i"(8 * 26) :);
    ASM("std 27, %0(1)" ::"i"(8 * 27) :);
    ASM("std 28, %0(1)" ::"i"(8 * 28) :);
    ASM("std 29, %0(1)" ::"i"(8 * 29) :);
    ASM("std 30, %0(1)" ::"i"(8 * 30) :);
    ASM("std 31, %0(1)" ::"i"(8 * 31) :);

#if 1

    // push stack
    ASM("addi 1, 1, -64");

    // store original rtoc to stack
    ASM("std 2, 0(1)");

    // store original lr to stack
    ASM("mflr %0" : "=r"(r3)::);
    ASM("std %0, 8(1)" ::"r"(r3) :);

    // set stage_entry_ra
    ASM("bl 4");
    ASM("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= (4 * 43);

    // set lv1_rtoc
    ASM("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9669
    is_lv1 = 0x9669;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x700; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    ASM("mr 2, %0" ::"r"(stage_rtoc) :);

    // set lv1_sp
    ASM("mr %0, 1" : "=r"(lv1_sp)::);

    // set stage_sp to 0xE000000
    //stage_sp = 0xE000000;

    // set r1 to stage_sp
    //ASM("mr 1, %0" ::"r"(stage_sp) :);

    // sync
    ASM("sync");

    // push stack
    ASM("addi 1, 1, -128");

    // jump to stage3_main
    ASM("bl stage3_main");

    // pop stack
    ASM("addi 1, 1, 128");

    // set r1 to lv1_sp
    ASM("mr 1, %0" ::"r"(lv1_sp) :);

    // restore original lr from stack
    ASM("ld %0, 8(1)" : "=r"(r3)::);
    ASM("mtlr %0" ::"r"(r3));

    // restore original rtoc from stack
    ASM("ld %0, 0(1)" : "=r"(r3)::);
    ASM("mr 2, %0" ::"r"(r3));

    // pop stack
    ASM("addi 1, 1, 64");

#endif

    // restore all registers from stack
    ASM("ld 0, %0(1)" ::"i"(8 * 0) :);
    ASM("ld 1, %0(1)" ::"i"(8 * 1) :);
    ASM("ld 2, %0(1)" ::"i"(8 * 2) :);
    ASM("ld 3, %0(1)" ::"i"(8 * 3) :);
    //ASM("ld 4, %0(1)" ::"i"(8 * 4) :);//
    //ASM("ld 5, %0(1)" ::"i"(8 * 5) :);//
    //ASM("ld 6, %0(1)" ::"i"(8 * 6) :);//
    ASM("ld 7, %0(1)" ::"i"(8 * 7) :);
    ASM("ld 8, %0(1)" ::"i"(8 * 8) :);
    ASM("ld 9, %0(1)" ::"i"(8 * 9) :);
    ASM("ld 10, %0(1)" ::"i"(8 * 10) :);
    ASM("ld 11, %0(1)" ::"i"(8 * 11) :);
    ASM("ld 12, %0(1)" ::"i"(8 * 12) :);
    ASM("ld 13, %0(1)" ::"i"(8 * 13) :);
    ASM("ld 14, %0(1)" ::"i"(8 * 14) :);
    ASM("ld 15, %0(1)" ::"i"(8 * 15) :);
    ASM("ld 16, %0(1)" ::"i"(8 * 16) :);
    ASM("ld 17, %0(1)" ::"i"(8 * 17) :);
    ASM("ld 18, %0(1)" ::"i"(8 * 18) :);
    ASM("ld 19, %0(1)" ::"i"(8 * 19) :);
    ASM("ld 20, %0(1)" ::"i"(8 * 20) :);
    ASM("ld 21, %0(1)" ::"i"(8 * 21) :);
    ASM("ld 22, %0(1)" ::"i"(8 * 22) :);
    ASM("ld 23, %0(1)" ::"i"(8 * 23) :);
    ASM("ld 24, %0(1)" ::"i"(8 * 24) :);
    ASM("ld 25, %0(1)" ::"i"(8 * 25) :);
    ASM("ld 26, %0(1)" ::"i"(8 * 26) :);
    ASM("ld 27, %0(1)" ::"i"(8 * 27) :);
    ASM("ld 28, %0(1)" ::"i"(8 * 28) :);
    ASM("ld 29, %0(1)" ::"i"(8 * 29) :);
    ASM("ld 30, %0(1)" ::"i"(8 * 30) :);
    ASM("ld 31, %0(1)" ::"i"(8 * 31) :);

    // pop stack
    ASM("addi 1, 1, 512");

    // sync
    ASM("sync");

    // r3 = 0
    ASM("li %0, 0" : "=r"(r3)::);

    // blr
    ASM("blr");

    __builtin_unreachable();
}