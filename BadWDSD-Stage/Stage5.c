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
    sc_puts_init();
    is_emmc = FetchIsEmmc();
    check_pc(0, 1);

    Stage5(in_r10);
}

__attribute__((noreturn, section("entry5"))) void stage5_entry()
{
    register uint64_t r3 asm("r3");
    register uint64_t r10 asm("r10");

    // push stack
    ASM("addi 1, 1, -512");

    // store all registers to stack
    ASMX("std 0, %0(1)" ::"i"(8 * 0) :);
    ASMX("std 1, %0(1)" ::"i"(8 * 1) :);
    ASMX("std 2, %0(1)" ::"i"(8 * 2) :);
    ASMX("std 3, %0(1)" ::"i"(8 * 3) :);
    ASMX("std 4, %0(1)" ::"i"(8 * 4) :);
    ASMX("std 5, %0(1)" ::"i"(8 * 5) :);
    ASMX("std 6, %0(1)" ::"i"(8 * 6) :);
    ASMX("std 7, %0(1)" ::"i"(8 * 7) :);
    ASMX("std 8, %0(1)" ::"i"(8 * 8) :);
    ASMX("std 9, %0(1)" ::"i"(8 * 9) :);
    ASMX("std 10, %0(1)" ::"i"(8 * 10) :);
    ASMX("std 11, %0(1)" ::"i"(8 * 11) :);
    ASMX("std 12, %0(1)" ::"i"(8 * 12) :);
    ASMX("std 13, %0(1)" ::"i"(8 * 13) :);
    ASMX("std 14, %0(1)" ::"i"(8 * 14) :);
    ASMX("std 15, %0(1)" ::"i"(8 * 15) :);
    ASMX("std 16, %0(1)" ::"i"(8 * 16) :);
    ASMX("std 17, %0(1)" ::"i"(8 * 17) :);
    ASMX("std 18, %0(1)" ::"i"(8 * 18) :);
    ASMX("std 19, %0(1)" ::"i"(8 * 19) :);
    ASMX("std 20, %0(1)" ::"i"(8 * 20) :);
    ASMX("std 21, %0(1)" ::"i"(8 * 21) :);
    ASMX("std 22, %0(1)" ::"i"(8 * 22) :);
    ASMX("std 23, %0(1)" ::"i"(8 * 23) :);
    ASMX("std 24, %0(1)" ::"i"(8 * 24) :);
    ASMX("std 25, %0(1)" ::"i"(8 * 25) :);
    ASMX("std 26, %0(1)" ::"i"(8 * 26) :);
    ASMX("std 27, %0(1)" ::"i"(8 * 27) :);
    ASMX("std 28, %0(1)" ::"i"(8 * 28) :);
    ASMX("std 29, %0(1)" ::"i"(8 * 29) :);
    ASMX("std 30, %0(1)" ::"i"(8 * 30) :);
    ASMX("std 31, %0(1)" ::"i"(8 * 31) :);

#if 1

    // push stack
    ASM("addi 1, 1, -64");

    // store original rtoc to stack
    ASM("std 2, 0(1)");

    // store original lr to stack
    ASMX("mflr %0" : "=r"(r3)::);
    ASMX("std %0, 8(1)" ::"r"(r3) :);

    // set stage_entry_ra
    ASM("bl 4");
    ASMX("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= (4 * 38);

    // set lv1_rtoc
    ASMX("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9666 (stage5)
    is_lv1 = 0x9666;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x500; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    ASMX("mr 2, %0" ::"r"(stage_rtoc) :);

    // set lv1_sp
    ASMX("mr %0, 1" : "=r"(lv1_sp)::);

    // set stage_sp to 0xE000000
    //stage_sp = 0xE000000;

    // set r1 to stage_sp
    //ASMX("mr 1, %0" ::"r"(stage_sp) :);

    // push stack
    ASM("addi 1, 1, -128");

    // call stage5_main
    ASM("bl stage5_main");

    // pop stack
    ASM("addi 1, 1, 128");

    // set r1 to lv1_sp
    ASMX("mr 1, %0" ::"r"(lv1_sp) :);

    // restore original lr from stack
    ASMX("ld %0, 8(1)" : "=r"(r3)::);
    ASMX("mtlr %0" ::"r"(r3):);

    // restore original rtoc from stack
    ASMX("ld %0, 0(1)" : "=r"(r3)::);
    ASMX("mr 2, %0" ::"r"(r3):);

    // pop stack
    ASM("addi 1, 1, 64");

#endif

    // restore all registers from stack
    ASMX("ld 0, %0(1)" ::"i"(8 * 0) :);
    ASMX("ld 1, %0(1)" ::"i"(8 * 1) :);
    ASMX("ld 2, %0(1)" ::"i"(8 * 2) :);
    ASMX("ld 3, %0(1)" ::"i"(8 * 3) :);
    ASMX("ld 4, %0(1)" ::"i"(8 * 4) :);
    ASMX("ld 5, %0(1)" ::"i"(8 * 5) :);
    ASMX("ld 6, %0(1)" ::"i"(8 * 6) :);
    ASMX("ld 7, %0(1)" ::"i"(8 * 7) :);
    ASMX("ld 8, %0(1)" ::"i"(8 * 8) :);
    ASMX("ld 9, %0(1)" ::"i"(8 * 9) :);
    ASMX("ld 10, %0(1)" ::"i"(8 * 10) :);
    ASMX("ld 11, %0(1)" ::"i"(8 * 11) :);
    ASMX("ld 12, %0(1)" ::"i"(8 * 12) :);
    ASMX("ld 13, %0(1)" ::"i"(8 * 13) :);
    ASMX("ld 14, %0(1)" ::"i"(8 * 14) :);
    ASMX("ld 15, %0(1)" ::"i"(8 * 15) :);
    ASMX("ld 16, %0(1)" ::"i"(8 * 16) :);
    ASMX("ld 17, %0(1)" ::"i"(8 * 17) :);
    ASMX("ld 18, %0(1)" ::"i"(8 * 18) :);
    ASMX("ld 19, %0(1)" ::"i"(8 * 19) :);
    ASMX("ld 20, %0(1)" ::"i"(8 * 20) :);
    ASMX("ld 21, %0(1)" ::"i"(8 * 21) :);
    ASMX("ld 22, %0(1)" ::"i"(8 * 22) :);
    ASMX("ld 23, %0(1)" ::"i"(8 * 23) :);
    ASMX("ld 24, %0(1)" ::"i"(8 * 24) :);
    ASMX("ld 25, %0(1)" ::"i"(8 * 25) :);
    ASMX("ld 26, %0(1)" ::"i"(8 * 26) :);
    ASMX("ld 27, %0(1)" ::"i"(8 * 27) :);
    ASMX("ld 28, %0(1)" ::"i"(8 * 28) :);
    ASMX("ld 29, %0(1)" ::"i"(8 * 29) :);
    ASMX("ld 30, %0(1)" ::"i"(8 * 30) :);
    ASMX("ld 31, %0(1)" ::"i"(8 * 31) :);

    // pop stack
    ASM("addi 1, 1, 512");

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