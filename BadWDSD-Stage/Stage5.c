// note: log should be disabled in normal use
// STAGE5_LOG_ENABLED

#pragma GCC push_options
//#pragma GCC optimize("O0")

FUNC_DEF void Stage5()
{
    //puts("BadWDSD Stage5 by Kafuu(aomsin2526)\n");

    //puts("(Build Date: ");
    //puts(__DATE__);
    //puts(" ");
    //puts(__TIME__);
    //puts(")\n");

#if 0

    {
        //

        uint64_t *stage5_loopCount = (uint64_t *)0x220;
        ++(*stage5_loopCount);

        uint64_t *stage5_HashCache = (uint64_t *)0x218;

        //

        uint64_t *lv1_lv2AreaSizePtr = (uint64_t *)0x370F28;

        uint64_t lv2AreaAddrRa = 0x0;

        if (!FindLv2(&lv2AreaAddrRa))
        {
            puts("lv2 area not found!\n");
            return;
        }

        //

        uint64_t hash = 0;

        for (uint64_t addr = lv2AreaAddrRa; addr < (lv2AreaAddrRa + *lv1_lv2AreaSizePtr); addr += 8)
            hash += *((uint64_t*)addr);

        //

        if (*stage5_HashCache != hash)
        {
            *stage5_HashCache = hash;
            RegenLv2AreaHash(6);
        }
    }

#endif

    {
        uint64_t *lv1_lv2AreaAddrPtr = (uint64_t *)0x370F20;
        uint64_t *lv1_lv2AreaSizePtr = (uint64_t *)0x370F28;

        *lv1_lv2AreaAddrPtr = 0x8000000000000000; // 0x8000000000000000
        *lv1_lv2AreaSizePtr = 16;

        //Sc_Rx: after_lv1_lv2AreaHash[0] = 0xfa60f9a679d561e2
        //Sc_Rx: after_lv1_lv2AreaHash[1] = 0x4766aa39b90084b0
        //Sc_Rx: after_lv1_lv2AreaHash[2] = 0xb27d2ff00000000

        uint64_t *lv1_lv2AreaHashPtr = (uint64_t *)0x370F30;

        lv1_lv2AreaHashPtr[0] = 0xfa60f9a679d561e2;
        lv1_lv2AreaHashPtr[1] = 0x4766aa39b90084b0;
        lv1_lv2AreaHashPtr[2] = 0x0b27d2ff00000000;

        eieio();
    }

    //puts("Stage5 done.\n");
}

#pragma GCC pop_options

__attribute__((section("main5"))) void stage5_main()
{
    sc_puts_init();
    Stage5();
}

__attribute__((noreturn, section("entry5"))) void stage5_entry()
{
    // push stack
    asm volatile("addi 1, 1, -512");

    // store all registers to stack
    asm volatile("std 0, %0(1)" ::"i"(8 * 0) :);
    asm volatile("std 1, %0(1)" ::"i"(8 * 1) :);
    asm volatile("std 2, %0(1)" ::"i"(8 * 2) :);
    asm volatile("std 3, %0(1)" ::"i"(8 * 3) :);
    asm volatile("std 4, %0(1)" ::"i"(8 * 4) :);
    asm volatile("std 5, %0(1)" ::"i"(8 * 5) :);
    asm volatile("std 6, %0(1)" ::"i"(8 * 6) :);
    asm volatile("std 7, %0(1)" ::"i"(8 * 7) :);
    asm volatile("std 8, %0(1)" ::"i"(8 * 8) :);
    asm volatile("std 9, %0(1)" ::"i"(8 * 9) :);
    asm volatile("std 10, %0(1)" ::"i"(8 * 10) :);
    asm volatile("std 11, %0(1)" ::"i"(8 * 11) :);
    asm volatile("std 12, %0(1)" ::"i"(8 * 12) :);
    asm volatile("std 13, %0(1)" ::"i"(8 * 13) :);
    asm volatile("std 14, %0(1)" ::"i"(8 * 14) :);
    asm volatile("std 15, %0(1)" ::"i"(8 * 15) :);
    asm volatile("std 16, %0(1)" ::"i"(8 * 16) :);
    asm volatile("std 17, %0(1)" ::"i"(8 * 17) :);
    asm volatile("std 18, %0(1)" ::"i"(8 * 18) :);
    asm volatile("std 19, %0(1)" ::"i"(8 * 19) :);
    asm volatile("std 20, %0(1)" ::"i"(8 * 20) :);
    asm volatile("std 21, %0(1)" ::"i"(8 * 21) :);
    asm volatile("std 22, %0(1)" ::"i"(8 * 22) :);
    asm volatile("std 23, %0(1)" ::"i"(8 * 23) :);
    asm volatile("std 24, %0(1)" ::"i"(8 * 24) :);
    asm volatile("std 25, %0(1)" ::"i"(8 * 25) :);
    asm volatile("std 26, %0(1)" ::"i"(8 * 26) :);
    asm volatile("std 27, %0(1)" ::"i"(8 * 27) :);
    asm volatile("std 28, %0(1)" ::"i"(8 * 28) :);
    asm volatile("std 29, %0(1)" ::"i"(8 * 29) :);
    asm volatile("std 30, %0(1)" ::"i"(8 * 30) :);
    asm volatile("std 31, %0(1)" ::"i"(8 * 31) :);

#if 1

    register uint64_t r3 asm("r3");

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
    stage_entry_ra -= (4 * 38);

    // set lv1_rtoc
    asm volatile("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set interrupt_depth to 0
    interrupt_depth = 0;

    // set is_lv1 to 0x9666 (stage5)
    is_lv1 = 0x9666;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x400; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // set lv1_sp
    asm volatile("mr %0, 1" :"=r"(lv1_sp)::);

    // set stage_sp to 0xE000000
    //stage_sp = 0xE000000;

    // set r1 to stage_sp
    //asm volatile("mr 1, %0" ::"r"(stage_sp) :);

    // sync
    asm volatile("sync");

    // push stack
    asm volatile("addi 1, 1, -128");

    // jump to stage5_main
    asm volatile("bl stage5_main");

    // pop stack
    asm volatile("addi 1, 1, 128");

    // set r1 to lv1_sp
    asm volatile("mr 1, %0" ::"r"(lv1_sp) :);

    // restore original lr from stack
    asm volatile("ld %0, 8(1)" : "=r"(r3)::);
    asm volatile("mtlr %0" ::"r"(r3));

    // restore original rtoc from stack
    asm volatile("ld %0, 0(1)" : "=r"(r3)::);
    asm volatile("mr 2, %0" ::"r"(r3));

    // pop stack
    asm volatile("addi 1, 1, 64");

#endif

    // restore all registers from stack
    asm volatile("ld 0, %0(1)" ::"i"(8 * 0) :);
    asm volatile("ld 1, %0(1)" ::"i"(8 * 1) :);
    asm volatile("ld 2, %0(1)" ::"i"(8 * 2) :);
    asm volatile("ld 3, %0(1)" ::"i"(8 * 3) :);
    asm volatile("ld 4, %0(1)" ::"i"(8 * 4) :);
    asm volatile("ld 5, %0(1)" ::"i"(8 * 5) :);
    asm volatile("ld 6, %0(1)" ::"i"(8 * 6) :);
    asm volatile("ld 7, %0(1)" ::"i"(8 * 7) :);
    asm volatile("ld 8, %0(1)" ::"i"(8 * 8) :);
    asm volatile("ld 9, %0(1)" ::"i"(8 * 9) :);
    asm volatile("ld 10, %0(1)" ::"i"(8 * 10) :);
    asm volatile("ld 11, %0(1)" ::"i"(8 * 11) :);
    asm volatile("ld 12, %0(1)" ::"i"(8 * 12) :);
    asm volatile("ld 13, %0(1)" ::"i"(8 * 13) :);
    asm volatile("ld 14, %0(1)" ::"i"(8 * 14) :);
    asm volatile("ld 15, %0(1)" ::"i"(8 * 15) :);
    asm volatile("ld 16, %0(1)" ::"i"(8 * 16) :);
    asm volatile("ld 17, %0(1)" ::"i"(8 * 17) :);
    asm volatile("ld 18, %0(1)" ::"i"(8 * 18) :);
    asm volatile("ld 19, %0(1)" ::"i"(8 * 19) :);
    asm volatile("ld 20, %0(1)" ::"i"(8 * 20) :);
    asm volatile("ld 21, %0(1)" ::"i"(8 * 21) :);
    asm volatile("ld 22, %0(1)" ::"i"(8 * 22) :);
    asm volatile("ld 23, %0(1)" ::"i"(8 * 23) :);
    asm volatile("ld 24, %0(1)" ::"i"(8 * 24) :);
    asm volatile("ld 25, %0(1)" ::"i"(8 * 25) :);
    asm volatile("ld 26, %0(1)" ::"i"(8 * 26) :);
    asm volatile("ld 27, %0(1)" ::"i"(8 * 27) :);
    asm volatile("ld 28, %0(1)" ::"i"(8 * 28) :);
    asm volatile("ld 29, %0(1)" ::"i"(8 * 29) :);
    asm volatile("ld 30, %0(1)" ::"i"(8 * 30) :);
    asm volatile("ld 31, %0(1)" ::"i"(8 * 31) :);

    // pop stack
    asm volatile("addi 1, 1, 512");

    // sync
    asm volatile("sync");

    // push stack
    asm volatile("addi 1, 1, -16");

    // store original lr to stack
    asm volatile("mflr 0");
    asm volatile("std 0, 0(1)");

    // continue...

    asm volatile("ld 9, -0x30f8(2)");
    asm volatile("mr 0, 4");
    asm volatile("mr 6, 5");
    asm volatile("mr 4, 3");
    asm volatile("mr 5, 0");
    asm volatile("ld 3, 0(9)");

    // call 0x2B5088

    asm volatile("li 11, 0x2B50");
    asm volatile("sldi 11, 11, 8");
    asm volatile("addi 11, 11, 0x88");

    // push stack
    asm volatile("addi 1, 1, -128");

    //
    asm volatile("mtctr 11");
    asm volatile("bctrl");
    //

    // pop stack
    asm volatile("addi 1, 1, 128");

    // restore original lr from stack
    asm volatile("ld 0, 0(1)");
    asm volatile("mtlr 0");

    // pop stack
    asm volatile("addi 1, 1, 16");

#if 0

    // call stage6 (0x2401F031600)
    asm volatile("lis 11, 0x240");
    asm volatile("ori 11, 11, 0x1F03");
    asm volatile("sldi 11, 11, 16");
    asm volatile("ori 11, 11, 0x1600");

    asm volatile("mtctr 11");
    asm volatile("bctr");

#else

    asm volatile("blr");

#endif

    __builtin_unreachable();
}