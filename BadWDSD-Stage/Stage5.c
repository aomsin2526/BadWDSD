// note: log should be disabled in normal use
// STAGE5_LOG_ENABLED

FUNC_DEF void Stage5(uint64_t type)
{
    //lv1_puts("BadWDSD Stage5 by Kafuu(aomsin2526)\n");

    // lv1_puts("(Build Date: ");
    // lv1_puts(__DATE__);
    // lv1_puts(" ");
    // lv1_puts(__TIME__);
    // lv1_puts(")\n");

    struct Stagex_Context_s *ctx = GetStagexContext();

    if (type == 0x1)
    {
        ctx->stage6_isAppldr = 1;
        //lv1_puts("appldr\n");

        {
            uint64_t *lv1_lv2AreaAddrPtr = (uint64_t *)0x370F20;
            uint64_t *lv1_lv2AreaSizePtr = (uint64_t *)0x370F28;

            *lv1_lv2AreaAddrPtr = 0x8000000000000000;
            *lv1_lv2AreaSizePtr = 16;

            uint64_t *lv1_lv2AreaHashPtr = (uint64_t *)0x370F30;

            lv1_lv2AreaHashPtr[0] = 0xfa60f9a679d561e2;
            lv1_lv2AreaHashPtr[1] = 0x4766aa39b90084b0;
            lv1_lv2AreaHashPtr[2] = 0x0b27d2ff00000000;
        }
    }
    else if (type == 0x2)
    {
        ctx->stage6_isLv2ldr = 1;
        //lv1_puts("lv2ldr\n");
    }
    else if (type == 0x3)
    {
        ctx->stage6_isLv2ldr_rvk = 1;
        //lv1_puts("lv2ldr_rvk\n");
    }

    //lv1_puts("Stage5 done.\n");
}

__attribute__((section("main5"))) void stage5_main(
    uint64_t in_r3, uint64_t in_r4, uint64_t in_r5, uint64_t in_r6, uint64_t in_r7, uint64_t in_r8, uint64_t in_r9, uint64_t in_r10
)
{
    is_emmc = FetchIsEmmc();
    sc_puts_init();

    Stage5(in_r10);
}

__attribute__((noreturn, section("entry5"))) void stage5_entry()
{
    register uint64_t r3 asm("r3");
    register uint64_t r10 asm("r10");

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
    stage_entry_ra -= (4 * 38);

    // set lv1_rtoc
    ASM("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9666 (stage5)
    is_lv1 = 0x9666;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x500; // .toc
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

    // jump to stage5_main
    ASM("bl stage5_main");

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
    ASM("ld 4, %0(1)" ::"i"(8 * 4) :);
    ASM("ld 5, %0(1)" ::"i"(8 * 5) :);
    ASM("ld 6, %0(1)" ::"i"(8 * 6) :);
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

    // push stack
    ASM("addi 1, 1, -16");

    // store original lr to stack
    ASM("mflr 0");
    ASM("std 0, 0(1)");

    // continue...

    if (r10 == 0x1)
    {
        ASM("ld 9, -0x30f8(2)");
        ASM("mr 0, 4");
        ASM("mr 6, 5");
        ASM("mr 4, 3");
        ASM("mr 5, 0");
        ASM("ld 3, 0(9)");

        // call 0x2B5088

        ASM("li 11, 0x2B50");
        ASM("sldi 11, 11, 8");
        ASM("addi 11, 11, 0x88");

        // push stack
        ASM("addi 1, 1, -128");

        //
        ASM("mtctr 11");
        ASM("bctrl");
        //

        // pop stack
        ASM("addi 1, 1, 128");
    }
    else if (r10 == 0x2)
    {
        ASM("ld 9, -0x30f8(2)");
        ASM("mr 0, 3");
        ASM("mr 11, 4");
        ASM("mr 10, 5");
        ASM("mr 10, 5");
        ASM("mr 7, 6");
        ASM("mr 4, 0");
        ASM("ld 3, 0(9)");
        ASM("mr 5, 11");
        ASM("mr 6, 10");

        // call 0x2B4EA8

        ASM("li 11, 0x2B40");
        ASM("sldi 11, 11, 8");
        ASM("addi 11, 11, 0xEA8");

        // push stack
        ASM("addi 1, 1, -128");

        //
        ASM("mtctr 11");
        ASM("bctrl");
        //

        // pop stack
        ASM("addi 1, 1, 128");
    }
    else if (r10 == 0x3)
    {
        ASM("ld 9, -0x30f8(2)");
        ASM("mr 5, 4");
        ASM("mr 4, 3");
        ASM("ld 3, 0(9)");

        // call 0x2B4DE8

        ASM("li 11, 0x2B40");
        ASM("sldi 11, 11, 8");
        ASM("addi 11, 11, 0xDE8");

        // push stack
        ASM("addi 1, 1, -128");

        //
        ASM("mtctr 11");
        ASM("bctrl");
        //

        // pop stack
        ASM("addi 1, 1, 128");
    }

    // continue end

    // restore original lr from stack
    ASM("ld 0, 0(1)");
    ASM("mtlr 0");

    // pop stack
    ASM("addi 1, 1, 16");

    //
    ASM("blr");

    __builtin_unreachable();
}