FUNC_DEF uint64_t lv1_peek(uint64_t addr)
{
    return *((uint64_t *)addr);
}

FUNC_DEF void lv1_poke(uint64_t addr, uint64_t val)
{
    *((uint64_t *)addr) = val;
}

FUNC_DEF void lv1_read(uint64_t addr, uint64_t size, void *out_Buf)
{
    if (size == 0)
        return;

    uint64_t curOffset = 0;
    uint64_t left = size;

    uint64_t chunkSize = sizeof(uint64_t);

    uint8_t *outBuf = (uint8_t *)out_Buf;

    uint64_t zz = (addr % chunkSize);

    if (zz != 0)
    {
        uint64_t readSize = (chunkSize - zz);

        if (readSize > left)
            readSize = left;

        uint64_t a = (addr - zz);

        uint64_t v = lv1_peek(a);
        uint8_t *vx = (uint8_t *)&v;

        memcpy(&outBuf[curOffset], &vx[zz], readSize);

        curOffset += readSize;
        left -= readSize;
    }

    while (1)
    {
        if (left == 0)
            break;

        uint64_t readSize = (left > chunkSize) ? chunkSize : left;

        uint64_t v = lv1_peek(addr + curOffset);

        memcpy(&outBuf[curOffset], &v, readSize);

        curOffset += readSize;
        left -= readSize;
    }
}

FUNC_DEF void lv1_write(uint64_t addr, uint64_t size, const void *in_Buf)
{
    if (size == 0)
        return;

    uint64_t curOffset = 0;
    uint64_t left = size;

    uint64_t chunkSize = sizeof(uint64_t);

    const uint8_t *inBuf = (const uint8_t *)in_Buf;

    uint64_t zz = (addr % chunkSize);

    if (zz != 0)
    {
        uint64_t writeSize = (chunkSize - zz);

        if (writeSize > left)
            writeSize = left;

        uint64_t a = (addr - zz);

        uint64_t v = lv1_peek(a);
        uint8_t *vx = (uint8_t *)&v;

        memcpy(&vx[zz], &inBuf[curOffset], writeSize);

        lv1_poke(a, v);

        curOffset += writeSize;
        left -= writeSize;
    }

    while (1)
    {
        if (left == 0)
            break;

        uint64_t writeSize = (left > chunkSize) ? chunkSize : left;

        uint64_t v = lv1_peek(addr + curOffset);
        memcpy(&v, &inBuf[curOffset], writeSize);

        lv1_poke(addr + curOffset, v);

        curOffset += writeSize;
        left -= writeSize;
    }
}

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

FUNC_DEF void Stage3()
{
    puts("BadWDSD Stage3 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

    {
        uint64_t hvcallTable = FindHvcallTable();

        if (hvcallTable != 0)
        {
            puts("hvcallTable = ");
            print_hex(hvcallTable);
            puts("\n");

            {
                puts("Installing hvcall peek(34)\n");

                uint64_t code_addr = 0x130;
                uint64_t *code = (uint64_t *)code_addr;

                code[0] = 0xE86300004E800020;

                *((uint64_t *)(hvcallTable + (34 * 8))) = code_addr;
            }

            {
                puts("Installing hvcall poke(35)\n");

                uint64_t code_addr = 0x140;
                uint64_t *code = (uint64_t *)code_addr;

                code[0] = 0xF883000038600000;
                code[1] = 0x4E80002000000000;

                *((uint64_t *)(hvcallTable + (35 * 8))) = code_addr;
            }

            {
                puts("Installing hvcall exec(36)\n");

                uint64_t code_addr = 0x150;
                uint64_t *code = (uint64_t *)code_addr;

                code[0] = 0x3821FFF07C0802A6;
                code[1] = 0xF80100007D2903A6;

                code[2] = 0x4E800421E8010000;
                code[3] = 0x7C0803A638210010;

                code[4] = 0x4E80002000000000;

                *((uint64_t *)(hvcallTable + (36 * 8))) = code_addr;
            }
        }
        else
        {
            puts("hvcallTable not found!\n");
            dead_beep();
        }
    }

    {
        {
            puts("Patching hvcall 114...\n");

            {
                uint64_t newval = 0x6000000048000028;
                lv1_write(0x2DCF54, 8, &newval);
            }

            {
                uint64_t newval = 0x014BFFFBFD7C601B;
                lv1_write(0x2DD287, 8, &newval);
            }
        }

        {
            puts("Patching lv1 182/183\n");

            uint64_t patches[4];

            patches[0] = 0xE8830018E8840000ULL;
            patches[1] = 0xF88300C84E800020ULL;
            patches[2] = 0x38000000E8A30020ULL;
            patches[3] = 0xE8830018F8A40000ULL;

            lv1_write(0x309E4C, 32, patches);
        }

        {
            puts("Patching Shutdown on LV2 modification\n");

            uint64_t old;
            lv1_read(0x2b4434, 8, &old);
            old &= 0x00000000FFFFFFFFULL;

            uint64_t newval = 0x6000000000000000ULL | old;
            lv1_write(0x2b4434, 8, &newval);
        }

        // HTAB

        {
            puts("Patching HTAB write protection\n");

            uint64_t old;
            lv1_read(0x0AC594, 8, &old);
            old &= 0x00000000FFFFFFFFULL;

            uint64_t newval = 0x3860000000000000ULL | old;
            lv1_write(0x0AC594, 8, &newval);
        }

        {
            puts("Patching Update Manager EEPROM write access\n");

            uint64_t old;
            lv1_read(0x0FEBD4, 8, &old);
            old &= 0x00000000FFFFFFFFULL;

            uint64_t newval = 0x3800000000000000ULL | old;
            lv1_write(0x0FEBD4, 8, &newval);
        }

        // Repo nodes

        {
            puts("Patching Repo nodes modify\n");

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

                lv1_write(0x2E4E28, 20, patches);
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

                lv1_write(0x2E50AC, 28, patches);
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

                lv1_write(0x2E5550, 28, patches);
            }
        }

        {
            puts("Patching lv1_set_dabr\n");

            uint64_t old;
            lv1_read(0x2EB550, 8, &old);
            old &= 0x00000000FFFFFFFFULL;

            uint64_t newval = 0x3800000F00000000ULL | old;
            lv1_write(0x2EB550, 8, &newval);
        }

        {
            puts("Patching Dispatch Manager\n");

            {
                uint64_t old;
                lv1_read(0x16FA64, 8, &old);
                old &= 0x00000000FFFFFFFFULL;

                uint64_t newval = 0x6000000000000000ULL | old;
                lv1_write(0x16FA64, 8, &newval);
            }

            {
                uint64_t old;
                lv1_read(0x16FA88, 8, &old);
                old &= 0x00000000FFFFFFFFULL;

                uint64_t newval = 0x3860000100000000ULL | old;
                lv1_write(0x16FA88, 8, &newval);
            }

            {
                uint64_t old;
                lv1_read(0x16FB00, 8, &old);
                old &= 0x00000000FFFFFFFFULL;

                uint64_t newval = 0x3BE0000100000000ULL | old;
                lv1_write(0x16FB00, 8, &newval);
            }

            {
                uint64_t old;
                lv1_read(0x16FB08, 8, &old);
                old &= 0x00000000FFFFFFFFULL;

                uint64_t newval = 0x3860000000000000ULL | old;
                lv1_write(0x16FB08, 8, &newval);
            }
        }

        {
            puts("Patching MFC_SR1\n");

            uint64_t old;
            lv1_read(0x2F9EB8, 8, &old);
            old &= 0x00000000FFFFFFFFULL;

            uint64_t newval = 0x3920FFFF00000000ULL | old;
            lv1_write(0x2F9EB8, 8, &newval);
        }

        {
            puts("Patching ACL\n");

            uint64_t patches[2];

            patches[0] = 0x386000012F830000ULL;
            patches[1] = 0x419E001438000001ULL;

            lv1_write(0x25C504, 16, patches);
        }
    }

    eieio();

    puts("Stage3 done.\n");
}

FUNC_DEF void Stage3_AuthLv2(uint64_t laid)
{
    puts("Stage3_AuthLv2(), laid = ");
    print_hex(laid);
    puts("\n");

    // uint8_t* destPtr = (uint8_t*)0x8000000;

    // ps2 = 0x1020000003000001
    // ps3 = 0x1070000002000001

#if 1

    // apply lv2 diff....

    {
        uint8_t searchData[] = {0x7C, 0x71, 0x43, 0xA6, 0x7C, 0x92, 0x43, 0xA6, 0x7C, 0xB3, 0x43, 0xA6, 0x7C, 0x7A, 0x02, 0xA6, 0x7C, 0x9B, 0x02,
                                0xA6, 0x7C, 0xA0, 0x00, 0xA6, 0x60, 0xA5, 0x00, 0x30, 0x7C, 0xBB, 0x03, 0xA6, 0x3C, 0xA0, 0x80, 0x00, 0x60, 0xA5,
                                0x00, 0x00, 0x78, 0xA5, 0x07, 0xC6, 0x64, 0xA5, 0x00, 0x00, 0x60, 0xA5, 0x08, 0x3C, 0x7C, 0xBA, 0x03, 0xA6, 0x4C, 0x00, 0x00, 0x24};

        uint64_t foundAddr;

        if (SearchMemory((void *)0x1000000, (240 * 1024 * 1024), searchData, sizeof(searchData), &foundAddr))
        {
            foundAddr -= 0x800;

            puts("foundAddr = ");
            print_hex(foundAddr);
            puts("\n");

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

                puts("Searching for lv2_kernel.diff...\n");

                uint64_t lv2DiffFileAddress;
                uint64_t lv2DiffFileSize;

                if (CoreOS_FindFileEntry(coreOSStartAddress, "lv2_kernel.diff", &lv2DiffFileAddress, &lv2DiffFileSize))
                {
                    puts("lv2DiffFileAddress = ");
                    print_hex(lv2DiffFileAddress);

                    puts(", lv2DiffFileSize = ");
                    print_decimal(lv2DiffFileSize);

                    puts("\n");

                    {
                        uint64_t curAddress = lv2DiffFileAddress;

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

                            *((uint8_t *)(uint64_t)(addr + foundAddr)) = value;
                        }
                    }
                }
                else
                    puts("File not found!\n");
            }
        }
    }

#endif

    eieio();

    puts("Stage3_AuthLv2() done.\n");
}

#pragma GCC push_options
#pragma GCC optimize("O0")

__attribute__((section("main3"))) void stage3_main()
{
    register uint64_t r4 asm("r4");
    register uint64_t r5 asm("r5");
    register uint64_t r6 asm("r6");
    register uint64_t r7 asm("r7");

    // r5 = options
    // peek: r6 = addr, r4 = outValue
    // poke: r6 = addr, r7 = value

    uint64_t r5_2 = r5;
    uint64_t r6_2 = r6;
    uint64_t r7_2 = r7;

    // peekpoke64
    if (r5_2 == 0x1)
    {
        asm volatile("ld %0, 0(%1)" : "=r"(r4) : "r"(r6) :);
        return;
    }
    else if (r5_2 == 0x2)
    {
        asm volatile("std %0, 0(%1)" ::"r"(r7), "r"(r6) :);

        if ((r6_2 % 0x10000) == 0)
        {
            sc_puts_init();

            puts("poke64 addr = ");
            print_hex(r6_2);
            puts(", value = ");
            print_hex(r7_2);
            puts("\n");
        }

        return;
    }

    // peekpoke32
    if (r5_2 == 0x3)
    {
        asm volatile("lwz %0, 0(%1)" : "=r"(r4) : "r"(r6) :);
        return;
    }
    else if (r5_2 == 0x4)
    {
        asm volatile("stw %0, 0(%1)" ::"r"(r7), "r"(r6) :);

        if ((r6_2 % 0x10000) == 0)
        {
            sc_puts_init();

            puts("poke32 addr = ");
            print_hex(r6_2);
            puts(", value = ");
            print_hex(r7_2);
            puts("\n");
        }

        return;
    }

    // peekpoke8
    if (r5_2 == 0x7)
    {
        asm volatile("lbz %0, 0(%1)" : "=r"(r4) : "r"(r6) :);
        return;
    }
    else if (r5_2 == 0x8)
    {
        asm volatile("stb %0, 0(%1)" ::"r"(r7), "r"(r6) :);

        // if ((r6_2 % 0x10000) == 0)
        {
            sc_puts_init();

            puts("poke8 addr = ");
            print_hex(r6_2);
            puts(", value = ");
            print_hex(r7_2);
            puts("\n");
        }

        return;
    }

    // print hex value
    if (r5_2 == 0x20)
    {
        sc_puts_init();

        puts("print hex value = ");
        print_hex(r6_2);
        puts("\n");

        return;
    }

    // r4 = 0
    asm volatile("li 4, 0");

    // auth lv2
    if (r5_2 == 0x30)
    {
        sc_puts_init();

        // r6 = laid
        Stage3_AuthLv2(r6_2);
    }

    if ((r5_2 != 0x0) && (r5_2 != 0x30))
        return;

    uint64_t *alreadyDone = (uint64_t *)0x128;

    if (*alreadyDone == 0x69)
        return;

    sc_puts_init();
    Stage3();

    *alreadyDone = 0x69;
}

#pragma GCC pop_options

// in:
// r4 = magic (0x6996)
// r5 = arg1
// r6 = arg2
// r7 = arg3
// r8 = arg3

// out:
// r3 = 0
// r4 = return value

__attribute__((noreturn, section("entry3"))) void stage3_entry()
{
    register uint64_t r0 asm("r0");

    register uint64_t r3 asm("r3");
    register uint64_t r4 asm("r4");

    // start only if r4 = 0x6996
    asm volatile("li %0, 0x6996" : "=r"(r0)::);
    asm volatile("cmp 0, 0, %0, %1" ::"r"(r4), "r"(r0) :);
    asm volatile("beq stage3_start");
    asm volatile("li %0, -0x14" : "=r"(r3)::); // r3 = -0x14

    asm volatile("blr");

    // stage3_start:
    asm volatile("stage3_start:");

    // push stack
    asm volatile("addi 1, 1, -64");

    // store original rtoc to stack
    asm volatile("std 2, 0(1)");

    // store original lr to stack
    asm volatile("mflr %0" : "=r"(r3)::);
    asm volatile("std %0, 8(1)" ::"r"(r3) :);

    // set stage_entry_ra
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= (4 * 10);

    // set lv1_rtoc
    asm volatile("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9669
    is_lv1 = 0x9669;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x600; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // sync
    asm volatile("sync");

    // jump to stage3_main
    asm volatile("bl stage3_main");

    // restore original lr from stack
    asm volatile("ld %0, 8(1)" : "=r"(r3)::);
    asm volatile("mtlr %0" ::"r"(r3));

    // restore original rtoc from stack
    asm volatile("ld %0, 0(1)" : "=r"(r3)::);
    asm volatile("mr 2, %0" ::"r"(r3));

    // pop stack
    asm volatile("addi 1, 1, 64");

    // r3 = 0
    asm volatile("li %0, 0" : "=r"(r3)::);

    // sync
    asm volatile("sync");

    // blr
    asm volatile("blr");

    __builtin_unreachable();
}